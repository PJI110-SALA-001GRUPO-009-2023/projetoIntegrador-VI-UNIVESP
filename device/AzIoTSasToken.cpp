// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "AzIoTSasToken.h" // Inclui o "contrato" (o .h)
#include "SerialLogger.h"  // Inclui nosso logger para registrar erros
#include <az_result.h>     // Para códigos de resultado do Azure SDK

// --- Bibliotecas de Criptografia (mbedtls) ---
// Usadas para o processo de assinatura e codificação.
#include <mbedtls/base64.h> // Para codificar/decodificar a chave
#include <mbedtls/md.h>      // Funções de "Message Digest"
#include <mbedtls/sha256.h> // Para o algoritmo de assinatura SHA256

#include <stdlib.h>
#include <time.h> // Para obter a hora atual e calcular a expiração

// Define um tempo "indefinido", usado para checagem de erro
#define INDEFINITE_TIME ((time_t)-1)

// Macro auxiliar para verificar se um 'az_span' está vazio
#define az_span_is_content_equal(x, AZ_SPAN_EMPTY) \
  (az_span_size(x) == az_span_size(AZ_SPAN_EMPTY) && az_span_ptr(x) == az_span_ptr(AZ_SPAN_EMPTY))

/**
 * @brief Função auxiliar para LER a data de expiração de um token SAS.
 * O token final tem um formato como "...&sig=...&se=1678886400&...".
 * Esta função procura o "&se=" (Signature Expiration) e extrai
 * o número (o timestamp Unix) que vem depois dele.
 */
static uint32_t getSasTokenExpiration(const char* sasToken)
{
  const char SE[] = { '&', 's', 'e', '=' }; // O que estamos procurando
  uint32_t se_as_unix_time = 0;

  int i, j;
  // Loop para encontrar o padrão "&se=" dentro da string
  for (i = 0, j = 0; sasToken[i] != '\0'; i++)
  {
    if (sasToken[i] == SE[j])
    {
      j++;
      if (j == sizeof(SE)) { i++; break; } // Achamos!
    }
    else { j = 0; } // Reinicia a busca
  }

  if (j != sizeof(SE))
  {
    Logger.Error("Falha ao encontrar o campo `se` no token SAS");
  }
  else
  {
    // Encontra o final do número (próximo '&' ou fim da string)
    int k = i;
    while (sasToken[k] != '\0' && sasToken[k] != '&') { k++; }

    // Converte o texto (ex: "1678886400") para um número (uint32_t)
    if (az_result_failed(
            az_span_atou32(az_span_create((uint8_t*)sasToken + i, k - i), &se_as_unix_time)))
    {
      Logger.Error("Falha ao analisar o timestamp de expiração do SAS");
    }
  }
  return se_as_unix_time; // Retorna o timestamp de expiração
}

// Funções Auxiliares de Criptografia

/**
 * @brief Wrapper para a função de criptografia HMAC-SHA256 da mbedtls.
 * "Assina" um 'payload' usando uma 'key'.
 */
static void mbedtls_hmac_sha256(az_span key, az_span payload, az_span signed_payload)
{
  mbedtls_md_context_t ctx;
  mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1);
  mbedtls_md_hmac_starts(&ctx, (const unsigned char*)az_span_ptr(key), az_span_size(key));
  mbedtls_md_hmac_update(&ctx, (const unsigned char*)az_span_ptr(payload), az_span_size(payload));
  mbedtls_md_hmac_finish(&ctx, (byte*)az_span_ptr(signed_payload));
  mbedtls_md_free(&ctx);
}

/**
 * @brief Apenas chama a função de assinatura.
 */
static void hmac_sha256_sign_signature(
    az_span decoded_key,
    az_span signature,
    az_span signed_signature,
    az_span* out_signed_signature)
{
  mbedtls_hmac_sha256(decoded_key, signature, signed_signature);
  *out_signed_signature = az_span_slice(signed_signature, 0, 32); // Pega os 32 bytes do resultado
}

/**
 * @brief Codifica dados binários para texto Base64.
 * O resultado da assinatura HMAC é binário, mas o token SAS é texto.
 * Esta função converte o resultado da assinatura em texto.
 */
static void base64_encode_bytes(
    az_span decoded_bytes,
    az_span base64_encoded_bytes,
    az_span* out_base64_encoded_bytes)
{
  size_t len;
  if (mbedtls_base64_encode(
          /* ... buffers ... */
          az_span_ptr(base64_encoded_bytes), (size_t)az_span_size(base64_encoded_bytes), &len,
          az_span_ptr(decoded_bytes), (size_t)az_span_size(decoded_bytes))
      != 0)
  {
    Logger.Error("mbedtls_base64_encode fail");
  }
  *out_base64_encoded_bytes = az_span_create(az_span_ptr(base64_encoded_bytes), (int32_t)len);
}

/**
 * @brief Decodifica texto Base64 para dados binários.
 * A "Chave do Dispositivo" (deviceKey) vem como Base64, mas a criptografia
 * HMAC precisa da chave em binário. Esta função faz essa conversão.
 */
static int decode_base64_bytes(
    az_span base64_encoded_bytes,
    az_span decoded_bytes,
    az_span* out_decoded_bytes)
{
  memset(az_span_ptr(decoded_bytes), 0, (size_t)az_span_size(decoded_bytes));
  size_t len;
  if (mbedtls_base64_decode(
          az_span_ptr(decoded_bytes), (size_t)az_span_size(decoded_bytes), &len,
          az_span_ptr(base64_encoded_bytes), (size_t)az_span_size(base64_encoded_bytes))
      != 0)
  {
    Logger.Error("mbedtls_base64_decode fail");
    return 1;
  }
  else
  {
    *out_decoded_bytes = az_span_create(az_span_ptr(decoded_bytes), (int32_t)len);
    return 0;
  }
}

/**
 * @brief Função de alto nível que junta os passos da criptografia.
 */
static int iot_sample_generate_sas_base64_encoded_signed_signature(
    az_span sas_base64_encoded_key,              // A chave secreta (em Base64)
    az_span sas_signature,                       // A string para assinar
    az_span sas_base64_encoded_signed_signature, // Buffer de saída
    az_span* out_sas_base64_encoded_signed_signature)
{
  // Decodificar a chave (Base64 -> Binário)
  char sas_decoded_key_buffer[32];
  az_span sas_decoded_key = AZ_SPAN_FROM_BUFFER(sas_decoded_key_buffer);
  if (decode_base64_bytes(sas_base64_encoded_key, sas_decoded_key, &sas_decoded_key) != 0)
  {
    Logger.Error("Falha ao gerar assinatura codificada");
    return 1;
  }

  // Assinar a string (HMAC-SHA256) com a chave binária
  char sas_hmac256_signed_signature_buffer[32];
  az_span sas_hmac256_signed_signature = AZ_SPAN_FROM_BUFFER(sas_hmac256_signed_signature_buffer);
  hmac_sha256_sign_signature(
      sas_decoded_key, sas_signature, sas_hmac256_signed_signature, &sas_hmac256_signed_signature);

  // Codificar o resultado da assinatura (Binário -> Base64)
  base64_encode_bytes(
      sas_hmac256_signed_signature,
      sas_base64_encoded_signed_signature,
      out_sas_base64_encoded_signed_signature);

  return 0;
}

/**
 * @brief Função auxiliar de tempo.
 * Pega a hora ATUAL (time(NULL)) e adiciona (minutos * 60) segundos.
 */
int64_t iot_sample_get_epoch_expiration_time_from_minutes(uint32_t minutes)
{
  time_t now = time(NULL); // Pega a hora atual do relógio (NTP/RTC)
  return (int64_t)(now + minutes * 60); // Retorna a hora futura
}

/**
 * @brief A FUNÇÃO PRINCIPAL que gera o Token SAS.
 * Esta função orquestra todo o processo.
 */
az_span generate_sas_token(
    az_iot_hub_client* hub_client,
    az_span device_key,
    az_span sas_signature,
    unsigned int expiryTimeInMinutes,
    az_span sas_token)
{
  az_result rc;
  
  // Calcula o timestamp de expiração (ex: agora + 60 minutos)
  uint64_t sas_duration = iot_sample_get_epoch_expiration_time_from_minutes(expiryTimeInMinutes);

  // Pede ao SDK do Azure para criar a "string de assinatura"
  rc = az_iot_hub_client_sas_get_signature(hub_client, sas_duration, sas_signature, &sas_signature);
  if (az_result_failed(rc))
  {
    Logger.Error("Nao foi possivel obter a assinatura para a chave SAS");
    return AZ_SPAN_EMPTY;
  }

  // Executa a criptografia (Decodifica a chave, Assina a string, Codifica o resultado)
  char b64enc_hmacsha256_signature[64];
  az_span sas_base64_encoded_signed_signature = AZ_SPAN_FROM_BUFFER(b64enc_hmacsha256_signature);
  if (iot_sample_generate_sas_base64_encoded_signed_signature(
          device_key, sas_signature,
          sas_base64_encoded_signed_signature, &sas_base64_encoded_signed_signature)
      != 0)
  {
    Logger.Error("Falha ao gerar a assinatura do token SAS");
    return AZ_SPAN_EMPTY;
  }

  // Pede ao SDK do Azure para montar a "senha" (token) final (usando a assinatura criptografada que acabamos de gerar)
  size_t mqtt_password_length;
  rc = az_iot_hub_client_sas_get_password(
      hub_client, sas_duration, sas_base64_encoded_signed_signature, AZ_SPAN_EMPTY,
      (char*)az_span_ptr(sas_token), az_span_size(sas_token), &mqtt_password_length);

  if (az_result_failed(rc))
  {
    Logger.Error("Nao foi possivel obter a senha");
    return AZ_SPAN_EMPTY;
  }
  else
  {
    // Retorna a "senha" final
    return az_span_slice(sas_token, 0, mqtt_password_length);
  }
}

/**
 * @brief Implementação do Construtor (do .h)
 * Apenas armazena as referências (ponteiros, buffers) nas variáveis da classe.
 */
AzIoTSasToken::AzIoTSasToken(
    az_iot_hub_client* client,
    az_span deviceKey,
    az_span signatureBuffer,
    az_span sasTokenBuffer)
{
  this->client = client;
  this->deviceKey = deviceKey;
  this->signatureBuffer = signatureBuffer;
  this->sasTokenBuffer = sasTokenBuffer;
  this->expirationUnixTime = 0; // Zera o tempo de expiração
  this->sasToken = AZ_SPAN_EMPTY; // Zera o token
}

/**
 * @brief Implementação da função Generate (do .h)
 */
int AzIoTSasToken::Generate(unsigned int expiryTimeInMinutes)
{
  // Chama a função principal de geração de token
  this->sasToken = generate_sas_token(
      this->client, this->deviceKey, this->signatureBuffer,
      expiryTimeInMinutes, this->sasTokenBuffer);

  // Verifica se a geração falhou
  if (az_span_is_content_equal(this->sasToken, AZ_SPAN_EMPTY))
  {
    Logger::Error("Falha ao gerar o token SAS");
    return 1; // Retorna erro
  }
  else
  {
    // Se funcionou, "lê" o token que acabou de ser gerado para extrair e armazenar sua data de expiração
    this->expirationUnixTime = getSasTokenExpiration((const char*)az_span_ptr(this->sasToken));

    if (this->expirationUnixTime == 0)
    {
      Logger::Error("Falha ao obter o tempo de expiração do token");
      this->sasToken = AZ_SPAN_EMPTY; // Anula o token
      return 1; // Retorna erro
    }
    else
    {
      return 0; // Sucesso!
    }
  }
}

/**
 * @brief Implementação da função IsExpired (do .h)
 */
bool AzIoTSasToken::IsExpired()
{
  time_t now = time(NULL); // Pega a hora atual do sistema

  if (now == INDEFINITE_TIME)
  {
    Logger::Error("Falha ao obter a hora atual do sistema");
    return true; // Se não sabemos a hora, é mais seguro dizer que expirou.
  }
  else
  {
    // Retorna 'true' se a hora atual for maior ou igual à hora de expiração.
    return (now >= this->expirationUnixTime);
  }
}

/**
 * @brief Implementação da função Get (do .h)
 * Apenas retorna o token (a "senha") que está armazenado.
 */
az_span AzIoTSasToken::Get() { return this->sasToken; }