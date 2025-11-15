/**
 * @file device.ino
 * @brief Código principal do dispositivo IoT para monitoramento e controle de irrigação.
 * 
 * Este código implementa a lógica de leitura de sensores, controle de relé,
 * comunicação com o Azure IoT Hub via MQTT, e logging via Serial.
 */

// Bibliotecas necessárias
#include <cstdlib>        // Padrão C
#include <string.h>       // Padrão C
#include <time.h>         // Para sincronização de relógio (NTP)
#include <WiFi.h>         // Para conexão Wi-Fi
#include <mqtt_client.h>  // Para o cliente MQTT nativo do ESP-IDF
#include <az_core.h>      // Bibliotecas principais do Azure SDK
#include <az_iot.h>       // Biblioteca do Azure IoT Hub
#include <azure_ca.h>     // Certificado de segurança do Azure
#include "DHT.h"          // Para o sensor de temperatura/umidade

// Arquivos Locais
#include "AzIoTSasToken.h" // Utilitário para gerar o token SAS
#include "SerialLogger.h"  // Biblioteca de log customizada
#include "iot_configs.h"   // SEGREDOS (SSID, Senhas, Chaves)
#include "config.h"        // CONFIGURAÇÕES (Pinos, Timers, Limites)

// Instanciação de Objetos Globais
DHT dht(DHTPIN, DHTTYPE); // Cria o objeto 'dht' usando os pinos definidos no config.h


// Variáveis Globais de ESTADO e CONTROLE
// Variáveis de Conexão (Azure/MQTT)
static esp_mqtt_client_handle_t mqtt_client; // O "handle" da conexão MQTT
static az_iot_hub_client client;             // O "handle" do cliente Azure

// Buffers (reservas de memória) para os dados de conexão
static char mqtt_client_id[128];                      // Client ID gerado pelo SDK
static char mqtt_username[128];                       // Username gerado pelo SDK
static char mqtt_password[200];                       // Senha (Token SAS) gerado pelo SDK
static uint8_t sas_signature_buffer[256];             // Buffer temporário para gerar o Token SAS
static char telemetry_topic[128] = "DBPet2";          // (Será atualizado pelo SDK)
static String telemetry_payload = "{}";               // O JSON a ser enviado
#define INCOMING_DATA_BUFFER_SIZE 128                 // Tamanho do buffer para mensagens recebidas da nuvem
static char incoming_data[INCOMING_DATA_BUFFER_SIZE]; // Buffer para mensagens recebidas

// Variáveis de Temporizadores (Timers)
static unsigned long next_sensor_read_time_ms = 0; // Timer da Lógica Local
static unsigned long next_telemetry_send_time_ms = 0; // Timer da Lógica de Nuvem

// Variáveis de Estado de Hardware
static bool releAtivo = false;            // Estado lógico do relé (false=DESLIGADO)
static float umidadeSoloPercentual = 0.0; // Última leitura válida
static float umidadeAr = 0.0;             // Última leitura válida
static float temperaturaAr = 0.0;         // Última leitura válida

// Variáveis de Estado de Controle (Botão e Failsafe)
static unsigned long lastDebounceTime = 0; // Timer do debounce do botão
static unsigned long debounceDelay = 50;   // Duração do debounce (50ms)
static int lastButtonState = HIGH;         // Último estado do botão (HIGH = solto)
static unsigned long tempoReleLigado = 0;  // Timer do Failsafe
static uint32_t telemetry_send_count = 0;  // Contador de mensagens enviadas

// Instância do Gerador de Token SAS (Apenas se não usar certificado X.509)
#ifndef IOT_CONFIG_USE_X509_CERT
static AzIoTSasToken sasToken(
    &client,
    AZ_SPAN_FROM_STR(IOT_CONFIG_DEVICE_KEY),
    AZ_SPAN_FROM_BUFFER(sas_signature_buffer),
    AZ_SPAN_FROM_BUFFER(mqtt_password));
#endif

/**
 * @brief Conecta-se à rede Wi-Fi.
 */
static void connectToWiFi()
{
  Logger::Info("Conectando ao Wi-Fi SSID: " + String(ssid));
  WiFi.mode(WIFI_STA); // Define modo "Station" (cliente)
  WiFi.disconnect();
  delay(100);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Logger::Event("Wi-Fi conectado, IP: " + WiFi.localIP().toString());
}

/**
 * @brief Sincroniza o relógio interno do ESP32 com servidores NTP.
 * CRÍTICO: Sem hora certa, a autenticação SAS do Azure falha.
 */
static void initializeTime()
{
  Logger::Info("Configurando hora via SNTP...");
  // Calcula os offsets em segundos, usando as constantes do config.h
  long gmtOffsetSecs = TIME_ZONE_OFFSET_HOURS * 3600;
  long daylightOffsetSecs = (TIME_ZONE_OFFSET_HOURS + TIME_ZONE_DAYLIGHT_SAVINGS_DIFF) * 3600;

  configTime(gmtOffsetSecs, daylightOffsetSecs, NTP_SERVERS);
  
  // Espera até que a hora seja válida (posterior a 2017)
  time_t now = time(NULL);
  while (now < UNIX_TIME_NOV_13_2017)
  {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  Logger::Info("Hora inicializada!");
}

/**
 * @brief Prepara os pinos de hardware.
 */
static void initializeHardware() {
  dht.begin(); // Inicializa o sensor DHT
  pinMode(BUTTON_PIN, INPUT_PULLUP); // Configura o pino do botão como ENTRADA (INPUT)
  pinMode(RELE_PIN, OUTPUT); // Configura o pino do relé como SAÍDA (OUTPUT)
  digitalWrite(RELE_PIN, LOW); // Garante que o relé comece DESLIGADO (LOW) - (Inverta para HIGH se seu módulo relé for "ativo em baixo")

  Logger::Info("Hardware inicializado.");
}

/**
 * @brief Prepara o cliente Azure SDK (gera ClientID e Username).
 */
static void initializeIoTHubClient()
{
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  options.user_agent = AZ_SPAN_FROM_STR(AZURE_SDK_CLIENT_USER_AGENT);

  if (az_result_failed(az_iot_hub_client_init(
          &client,
          az_span_create((uint8_t*)host, strlen(host)),
          az_span_create((uint8_t*)device_id, strlen(device_id)),
          &options)))
  { Logger::Error("Falha ao inicializar o cliente Azure IoT Hub"); return; }

  size_t client_id_length;
  if (az_result_failed(az_iot_hub_client_get_client_id(
          &client, mqtt_client_id, sizeof(mqtt_client_id) - 1, &client_id_length)))
  { Logger::Error("Falha ao obter o client id"); return; }

  if (az_result_failed(az_iot_hub_client_get_user_name(
          &client, mqtt_username, sizeofarray(mqtt_username), NULL)))
  { Logger::Error("Falha ao obter o MQTT username"); return; }

  // Estas são mensagens de "ruído", rebaixadas para Debug
  Logger::Debug("Client ID: " + String(mqtt_client_id));
  Logger::Debug("Username: " + String(mqtt_username));
}

/**
 * @brief Callback principal do MQTT. Chamado para TODOS os eventos.
 */
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
#else
static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
#endif
{
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  (void)handler_args; (void)base; (void)event_id;
  esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
#endif

  switch (event->event_id)
  {
    int i, r;
    case MQTT_EVENT_ERROR:
      Logger::Error("MQTT_EVENT_ERROR");
      break;
    case MQTT_EVENT_CONNECTED: // Evento: Conectado com sucesso
      Logger::Event("MQTT Conectado ao Azure IoT Hub");
      // Imediatamente se inscreve no tópico de C2D (Cloud-to-Device)
      r = esp_mqtt_client_subscribe(mqtt_client, AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC, 1);
      if (r == -1) { Logger::Error("Nao foi possivel se inscrever para mensagens C2D."); }
      else { Logger::Info("Inscrito para mensagens C2D."); }
      break;
    case MQTT_EVENT_DISCONNECTED: // Evento: Desconectado
      Logger::Event("MQTT Desconectado");
      break;
    case MQTT_EVENT_SUBSCRIBED: // Confirmação de inscrição
      Logger::Info("MQTT Inscrito (SUBSCRIBED)");
      break;
    case MQTT_EVENT_PUBLISHED: // Confirmação de envio
      Logger::Debug("MQTT Mensagem Publicada (PUBLISHED)");
      break;
    case MQTT_EVENT_DATA: // Evento: MENSAGEM RECEBIDA (C2D)
      Logger::Event("MQTT_EVENT_DATA (Comando C2D Recebido)");
      // Copia o tópico e os dados para os buffers
      for (i = 0; i < (INCOMING_DATA_BUFFER_SIZE - 1) && i < event->topic_len; i++)
      { incoming_data[i] = event->topic[i]; }
      incoming_data[i] = '\0';
      Logger::Info("Topico: " + String(incoming_data));
      for (i = 0; i < (INCOMING_DATA_BUFFER_SIZE - 1) && i < event->data_len; i++)
      { incoming_data[i] = event->data[i]; }
      incoming_data[i] = '\0';
      Logger::Info("Dados: " + String(incoming_data));
      break;
    case MQTT_EVENT_BEFORE_CONNECT:
      Logger::Debug("MQTT evento BEFORE_CONNECT");
      break;
    default:
      Logger::Error("MQTT evento DESCONHECIDO");
      break;
  }
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
#else
  return ESP_OK;
#endif
}

/**
 * @brief Inicializa e configura o cliente MQTT do ESP-IDF.
 */
static int initializeMqttClient()
{
#ifndef IOT_CONFIG_USE_X509_CERT
  // Gera um novo Token SAS (senha) válido por 60 minutos
  if (sasToken.Generate(SAS_TOKEN_DURATION_IN_MINUTES) != 0)
  { Logger::Error("Falha ao gerar o token SAS"); return 1; }
#endif

  esp_mqtt_client_config_t mqtt_config;
  memset(&mqtt_config, 0, sizeof(mqtt_config)); // Limpa a configuração

// Bloco de compatibilidade para diferentes versões do framework Arduino-ESP32
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    mqtt_config.broker.address.uri = mqtt_broker_uri;
    mqtt_config.broker.address.port = mqtt_port;
    mqtt_config.credentials.client_id = mqtt_client_id;
    mqtt_config.credentials.username = mqtt_username;
  #ifdef IOT_CONFIG_USE_X509_CERT
    // (Lógica para certificados X.509)
  #else
    mqtt_config.credentials.authentication.password = (const char*)az_span_ptr(sasToken.Get());
  #endif
    mqtt_config.session.keepalive = 30;
    mqtt_config.network.disable_auto_reconnect = false;
    mqtt_config.broker.verification.certificate = (const char*)ca_pem;
#else
  mqtt_config.uri = mqtt_broker_uri;
  mqtt_config.port = mqtt_port;
  mqtt_config.client_id = mqtt_client_id;
  mqtt_config.username = mqtt_username;
  #ifdef IOT_CONFIG_USE_X509_CERT
    // (Lógica para certificados X.509)
  #else
    mqtt_config.password = (const char*)az_span_ptr(sasToken.Get());
  #endif
  mqtt_config.keepalive = 30;
  mqtt_config.disable_auto_reconnect = false;
  mqtt_config.event_handle = mqtt_event_handler; // Registra o callback AQUI
  mqtt_config.cert_pem = (const char*)ca_pem;
#endif

  mqtt_client = esp_mqtt_client_init(&mqtt_config); // Cria o cliente
  if (mqtt_client == NULL) { Logger::Error("Falha ao criar o cliente mqtt"); return 1; }

// Registra o callback para a versão 3+ AQUI
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    esp_mqtt_client_register_event(mqtt_client, MQTT_EVENT_ANY, mqtt_event_handler, NULL);
#endif

  esp_err_t start_result = esp_mqtt_client_start(mqtt_client); // Inicia a conexão
  if (start_result != ESP_OK)
  { Logger::Error("Nao foi possivel iniciar o cliente mqtt; codigo erro:" + start_result); return 1; }
  else
  { Logger::Info("Cliente MQTT iniciado"); return 0; }
}

/**
 * @brief Função "mestra" de configuração.
 */
static void establishConnection()
{
  connectToWiFi();               // 1. Wi-Fi
  initializeTime();              // 2. Relógio
  initializeIoTHubClient();      // 3. Cliente Azure (prepara credenciais)
  (void)initializeMqttClient();  // 4. Cliente MQTT (conecta)
}

/**
 * @brief PREPARA o payload JSON para envio.
 * Apenas formata os dados das variáveis globais.
 */
static void generateTelemetryPayload()
{
  telemetry_payload = "{ \"msgCount\": " + String(telemetry_send_count++) + ", " +
                      "\"soilMoisture\": " + String(umidadeSoloPercentual) + ", " +
                      "\"airTempC\": " + String(temperaturaAr) + ", " +
                      "\"airHumidity\": " + String(umidadeAr) + " }";
}

/**
 * @brief ENVIA a telemetria periódica para o Azure.
 */
static void sendTelemetry()
{
  Logger::Debug("Enviando telemetria..."); // Mensagem de "ruído"
  if (az_result_failed(az_iot_hub_client_telemetry_get_publish_topic(
          &client, NULL, telemetry_topic, sizeof(telemetry_topic), NULL)))
  { Logger::Error("Falha ao obter o topico de telemetria"); return; }

  generateTelemetryPayload(); // Prepara o JSON

  // Publica a mensagem na nuvem
  if (esp_mqtt_client_publish(
          mqtt_client, telemetry_topic, (const char*)telemetry_payload.c_str(),
          telemetry_payload.length(), MQTT_QOS1, DO_NOT_RETAIN_MSG) == 0)
  { Logger::Error("Falha ao publicar telemetria"); }
  else
  { Logger::Debug("Telemetria publicada com sucesso"); }
}

/**
 * @brief ENVIA um evento de mudança de estado do relé para o Azure.
 * Chamada instantaneamente quando o relé muda (manual ou auto).
 */
static void sendRelayState(bool isAtivo)
{
  Logger::Info("Enviando estado do rele: " + String(isAtivo ? "LIGADO" : "DESLIGADO"));
  
  if (az_result_failed(az_iot_hub_client_telemetry_get_publish_topic(
          &client, NULL, telemetry_topic, sizeof(telemetry_topic), NULL)))
  { Logger::Error("Falha ao obter o topico de telemetria para o rele"); return; }

  String relayPayload = "{ \"releEstado\": " + String(isAtivo ? "true" : "false") + " }";

  if (esp_mqtt_client_publish(
          mqtt_client, telemetry_topic, (const char*)relayPayload.c_str(),
          relayPayload.length(), MQTT_QOS1, DO_NOT_RETAIN_MSG) == 0)
  { Logger::Error("Falha ao publicar estado do rele"); }
  else
  { Logger::Info("Estado do rele publicado com sucesso!"); }
}

/**
 * @brief Lê os sensores e ATUALIZA as variáveis globais.
 */
static void monitorarSensores() {
  int valorAnalogico = analogRead(SENSOR_SOLO_PIN);
  umidadeSoloPercentual = map(valorAnalogico, VALOR_SENSOR_SECO, VALOR_SENSOR_UMIDO, 0, 100);  
  umidadeSoloPercentual = constrain(umidadeSoloPercentual, 0, 100);                            

  // 2. Leitura do Sensor DHT
  float newUmidadeAr = dht.readHumidity();
  float newTemperaturaAr = dht.readTemperature();

  // 3. Validação (só atualiza globais se a leitura for válida)
  if (!isnan(newUmidadeAr) && !isnan(newTemperaturaAr)) {
    umidadeAr = newUmidadeAr;
    temperaturaAr = newTemperaturaAr;
  } else {
    Logger::Error("Falha ao ler o sensor DHT11!");
  }
  
  Logger::Debug("Sensores lidos");
}

/**
 * @brief AGE com base nos valores dos sensores (lógica de HISTERESE).
 */
static void handleAutomation() {
  
  // CONDIÇÃO DE LIGAR: Solo está muito seco?
  if (umidadeSoloPercentual < UMIDADE_LIMITE_LIGAR) {
    if (!releAtivo) { // Só age se já não estiver ligado
      Logger::Event("Histerese: Solo SECO (<" + String(UMIDADE_LIMITE_LIGAR) + "%%). Ligando rele.");
      releAtivo = true;
      digitalWrite(RELE_PIN, HIGH);
      tempoReleLigado = millis(); // Inicia o timer do Failsafe
      sendRelayState(releAtivo); // Informa a nuvem
    }
  } 
  
  // CONDIÇÃO DE DESLIGAR: Solo atingiu o limite superior?
  else if (umidadeSoloPercentual > UMIDADE_LIMITE_DESLIGAR) {
    if (releAtivo) { // Só age se já não estiver desligado
      Logger::Event("Histerese: Solo UMIDO (>" + String(UMIDADE_LIMITE_DESLIGAR) + "%%). Desligando rele.");
      releAtivo = false;
      digitalWrite(RELE_PIN, LOW);
      sendRelayState(releAtivo); // Informa a nuvem
    }
  }
  // Se estiver na "zona morta" (entre LIGAR e DESLIGAR), não faz nada.
}

/**
 * @brief Lógica de override manual (BOTÃO).
 * Chamada em alta velocidade (todo ciclo do loop).
 */
static void handleButtonAndRelay()
{
  int reading = digitalRead(BUTTON_PIN); // Lê o pino

  // Verifica se MUDOU de HIGH (solto) para LOW (pressionado)
  if (reading == LOW && lastButtonState == HIGH) 
  {
    // Espera o "debounce" (50ms) para filtrar ruído elétrico
    if ((millis() - lastDebounceTime) > debounceDelay)
    {
      Logger::Event("Botao pressionado, invertendo o rele!");
      
      releAtivo = !releAtivo; // Inverte o estado lógico
      digitalWrite(RELE_PIN, releAtivo ? HIGH : LOW); // Aplica no pino

      if (releAtivo) {
        tempoReleLigado = millis(); // Inicia o timer do Failsafe
      }

      sendRelayState(releAtivo); // Informa a nuvem
      lastDebounceTime = millis(); // Reseta o timer do debounce
    }
  }
  lastButtonState = reading; // Salva o estado atual para a próxima checagem
}

/**
 * @brief Proteção da bomba.
 */
static void handleFailsafe()
{
  // Se o relé não está ativo, não há nada a fazer.
  if (!releAtivo) {
    return;
  }

  // Se o relé está ativo, calcula há quanto tempo.
  unsigned long tempoLigado = millis() - tempoReleLigado;

  // Verifica se excedeu o tempo máximo permitido (do config.h)
  if (tempoLigado > TEMPO_MAXIMO_DE_REGAR)
  {
    Logger::Event("--- !!! PROTEÇÃO ACIONADA !!! ---");
    Logger::Error("Rele ficou ligado por mais de " + String(TEMPO_MAXIMO_DE_REGAR / 60000) + " min. Desligando forcado.");
    
    // Força o desligamento
    releAtivo = false;
    digitalWrite(RELE_PIN, LOW);
    sendRelayState(releAtivo); // Informa a nuvem
  }
}

/**
 * @brief Executado uma vez na inicialização.
 */
void setup() 
{
  Logger::setLevel(LOG_LEVEL_INFO); 

  establishConnection(); // Conecta Wi-Fi, NTP e Azure
  initializeHardware();  // Prepara os pinos e sensores
  
  Logger::Event("Sistema inicializado. Iniciando loop principal.");
}

/**
 * @brief Executado repetidamente, o mais rápido possível.
 * Este é o "maestro" que gerencia as 5 tarefas principais.
 */
void loop()
{
  bool isCloudConnected = false; // Flag para tarefas de nuvem

  // Gerenciamento de Conexão (Sempre)
  if (WiFi.status() != WL_CONNECTED)
  {
    connectToWiFi(); // Tenta reconectar (bloqueante)
  }
#ifndef IOT_CONFIG_USE_X509_CERT
  else if (sasToken.IsExpired())
  {
    Logger::Event("Token SAS expirado; reconectando com um novo.");
    (void)esp_mqtt_client_destroy(mqtt_client);
    initializeMqttClient();
  }
#endif
  else 
  {
    isCloudConnected = true; // Se tudo estiver OK, libera a nuvem
  }

  // Lógica Local (Timer de Sensores)
  if (millis() > next_sensor_read_time_ms)
  {
    monitorarSensores();  // Lê os dados
    handleAutomation();   // Age com base nos dados
    next_sensor_read_time_ms = millis() + SENSOR_READ_FREQUENCY_MILLISECS; // Agenda a próxima execução
  }

  // Lógica de Nuvem (Timer de Telemetria)
  if (millis() > next_telemetry_send_time_ms)
  {
    if (isCloudConnected) // Só envia se estiver conectado
    { 
      sendTelemetry(); 
    }
    else 
    { 
      Logger::Debug("Offline. Pulando envio de telemetria."); 
    }
    next_telemetry_send_time_ms = millis() + TELEMETRY_FREQUENCY_MILLISECS; // Agenda a próxima tentativa de envio
  }

  // Funções em tempo real (alta prioridade)
  handleButtonAndRelay(); // Lógica Manual
  handleFailsafe(); // Lógica de Segurança
}