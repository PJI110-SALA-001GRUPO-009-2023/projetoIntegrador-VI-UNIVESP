/**
 * @file config.h
 * @brief Configurações gerais do dispositivo
 */

#ifndef CONFIG_H // Necessário para evitar múltiplas inclusões
#define CONFIG_H // Início do arquivo de inclusão única

#include "DHT.h" // Necessário para a definição de DHTTYPE

// Configurações de Hardware (Pinos)
#define RELE_PIN 5           // Pino GPIO do Relé
#define BUTTON_PIN 4         // Pino GPIO do Botão
#define SENSOR_SOLO_PIN 34   // Pino analógico do sensor de umidade do solo
#define SENSOR_SOLO_PIN_D 25 // Pino digital do sensor de umidade do solo
#define DHTPIN 21            // Pino digital do sensor DHT
#define DHTTYPE DHT11        // Tipo de sensor (DHT11)

// Configurações de Calibração (Sensores)
#define VALOR_SENSOR_SECO 4095   // Valor analógico (0-4095) para 100% seco
#define VALOR_SENSOR_UMIDO 1250  // Valor analógico (0-4095) para 100% úmido

// Configurações de Automação (Histerese)
#define UMIDADE_LIMITE_LIGAR 20.0     // O relé LIGA quando a umidade cai ABAIXO deste valor
#define UMIDADE_LIMITE_DESLIGAR 30.0  // O relé DESLIGA quando a umidade sobe ACIMA deste valor

// Configurações de Temporizadores (ms)
#define SENSOR_READ_FREQUENCY_MILLISECS 3*60*1000  // 3 minutos (Frequência para LER os sensores e rodar a automação local)
#define TEMPO_MAXIMO_DE_REGAR 10*60*1000UL         // UL = Unsigned Long (10 minutos - Tempo máximo que o relé pode ficar ligado)

// Configurações de Conexão e Fuso Horário
#define NTP_SERVERS "pool.ntp.org", "time.nist.gov" // Servidores de Tempo para sincronização do relógio
#define TIME_ZONE_OFFSET_HOURS -8                   // Defina seu fuso horário em horas (-8 para PST)
#define TIME_ZONE_DAYLIGHT_SAVINGS_DIFF 1           // Diferença (em horas) para o horário de verão

// Configurações Técnicas (Azure/MQTT)
#define AZURE_SDK_CLIENT_USER_AGENT "c%2F" AZ_SDK_VERSION_STRING "(ard;esp32)" // Identificador do agente do dispositivo
#define MQTT_QOS1 1                       // Qualidade de Serviço MQTT nível 1 (pelo menos uma vez)
#define DO_NOT_RETAIN_MSG 0               // Não reter mensagens MQTT no broker
#define SAS_TOKEN_DURATION_IN_MINUTES 60  // Duração do token de segurança
#define INCOMING_DATA_BUFFER_SIZE 128     // Tamanho do buffer para mensagens recebidas da nuvem
#define UNIX_TIME_NOV_13_2017 1510592825  // "Data teste" usado para verificar se o relógio foi sincronizado

#endif // Fim do arquivo de inclusão única