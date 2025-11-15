/**
 * @file iot_configs.h
 * @brief Configurações sensíveis do dispositivo IoT (segredos).
 */

#ifndef IOT_CONFIGS_H // Necessário para evitar múltiplas inclusões
#define IOT_CONFIGS_H // Início do arquivo de inclusão única

// Configurações de Rede Wi-Fi
#define IOT_CONFIG_WIFI_SSID       "SEU_SSID_AQUI"        // SSID da rede Wi-Fi
#define IOT_CONFIG_WIFI_PASSWORD   "SUA_SENHA_AQUI"       // Senha da rede Wi-Fi

// Configuração para usar certificado X.509
#define IOT_CONFIG_USE_X509_CERT
#ifdef IOT_CONFIG_USE_X509_CERT
#define IOT_CONFIG_DEVICE_CERT
#define IOT_CONFIG_DEVICE_CERT_PRIVATE_KEY
#endif

// Configurações do Azure IoT Hub
#define IOT_CONFIG_IOTHUB_FQDN        "SEU_HUB.azure-devices.net" // FQDN do seu Hub IoT
#define IOT_CONFIG_DEVICE_ID          "SEU_DEVICE_ID_AQUI"        // ID do dispositivo registrado no Hub IoT
#define IOT_CONFIG_DEVICE_KEY         "SUA_CHAVE_SECRETA_AQUI"   // Chave primária do dispositivo (Device Key)

#define TELEMETRY_FREQUENCY_MILLISECS 3*60*1000UL // 3 minutos (Frequência para enviar telemetria para a nuvem)

#endif // Fim do arquivo de inclusão única