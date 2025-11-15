// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT
// (Licença de software da Microsoft)

#ifndef AZIOTSASTOKEN_H
#define AZIOTSASTOKEN_H

#include <Arduino.h>
#include <az_iot_hub_client.h> // Inclui o cliente principal do Hub IoT
#include <az_span.h>           // Inclui a biblioteca 'az_span', um tipo de "string segura" do Azure

/**
 * @brief Classe utilitária para gerar e gerenciar a "senha" (Token SAS)
 * necessária para a autenticação no Hub IoT do Azure.
 */
class AzIoTSasToken
{
public:
  /**
   * @brief Construtor da classe.
   * @param client Um ponteiro para o objeto 'az_iot_hub_client' principal (do seu .ino).
   * @param deviceKey A "Chave do Dispositivo" secreta (do iot_configs.h).
   * @param signatureBuffer Um buffer de memória temporário para o processo de assinatura.
   * @param sasTokenBuffer Um buffer de memória para armazenar a "senha" (token) final.
   */
  AzIoTSasToken(
      az_iot_hub_client* client,
      az_span deviceKey,
      az_span signatureBuffer,
      az_span sasTokenBuffer);

  /**
   * @brief Gera um novo Token SAS que expira em 'expiryTimeInMinutes'.
   * Esta é a função principal que faz toda a criptografia.
   * @param expiryTimeInMinutes Por quantos minutos o token será válido (ex: 60).
   * @return 0 se for bem-sucedido, 1 se falhar.
   */
  int Generate(unsigned int expiryTimeInMinutes);

  /**
   * @brief Verifica se o token gerado anteriormente já expirou.
   * @return true se o token expirou (precisa gerar um novo), false caso contrário.
   */
  bool IsExpired();

  /**
   * @brief Obtém o token SAS gerado (a "senha" final).
   * @return Um 'az_span' contendo o token de texto.
   */
  az_span Get();

private:
  // --- Variáveis internas da classe ---
  az_iot_hub_client* client;  // Ponteiro para o cliente do Hub IoT
  az_span deviceKey;          // A chave secreta
  az_span signatureBuffer;    // Buffer de trabalho
  az_span sasTokenBuffer;     // Buffer final
  az_span sasToken;           // Um "ponteiro" para o token dentro do sasTokenBuffer
  uint32_t expirationUnixTime; // A data/hora (timestamp) de quando o token expira
};

#endif // AZIOTSASTOKEN_H