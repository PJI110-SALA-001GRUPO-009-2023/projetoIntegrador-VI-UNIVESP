/**
 * @file calibrador.ino
 * @brief Ferramenta de calibração de sensor de umidade (AO/DO) com 2 modos.
 *
 * @details Uso de botão externo para navegação entre os dois modos
 */

// Configuração de Pinos
const int PINO_SENSOR_AO = 34; // Pino Analógico (ADC1)
const int PINO_SENSOR_DO = 25; // Pino Digital (Qualquer GPIO)
const int PINO_BOTAO = 26; // Pino Digital (Qualquer GPIO)

// Constantes de Lógica
const int QTD_AMOSTRAS = 10; //Amostras para o cálculo da MÉDIA no Modo 1
const long INTERVALO_LEITURA = 1*1000; // 1 segundo

/**
 * @enum Modo
 * @brief Define os estados de operação da máquina de estados
 */
enum Modo {
  MODO_CALIB_ANALOGICO, // Modo 1: Calcula média e máximo/mínimo do intervalo de medição
  MODO_CALIB_DIGITAL    // Modo 2: Visualizador vivo (AO/DO) para ajuste do potenciômetro
};

// Variáveis de Estado e Controle
Modo modoOperacao = MODO_CALIB_ANALOGICO; // Estado atual da máquina (default)
unsigned long timestampAnterior = 0;      // Para o timer não-bloqueante (millis())

// Variáveis de Debounce
int estadoBotaoAnterior = HIGH;     // Estado anterior do botão
unsigned long ultimoTempoBotao = 0; // Timestamp da última MUDANÇA de estado do botão
const long delayDebounce = 50;      // 50ms para filtrar ruído do botão (evitar disparos múltiplos)

// Variáveis Globais de Calibração (Modo 1)
int contadorLeituras = 0;     // Contador para o ciclo de média (0 a QTD_AMOSTRAS)
long totalLeituras = 0;       // Acumulador para o cálculo da média
int valorMaximo = 0;          // Maior valor (seco) possível - Ver datasheet do sensor ultilizado
int valorMinimo = 4095;       // Menor valor (úmido) possível - Ver datasheet do sensor ultilizado

/**
 * @brief Inicializa o microcontrolador, pinos e Serial.
 */
void setup() {
  Serial.begin(115200);

  // Configura os pinos do sensor
  pinMode(PINO_SENSOR_DO, INPUT);
  
  // Configura o botão externo com resistor de pull-up interno.
  pinMode(PINO_BOTAO, INPUT_PULLUP);

  Serial.println("FERRAMENTA DE CALIBRACAO DE SENSOR");
  Serial.println("MODO 1: CALIBRACAO PINO ANALOGICO");
  
  // Define o timestamp inicial para a primeira leitura
  timestampAnterior = millis();
}

/**
 * @brief Loop principal não-bloqueante
 * Verifica o botão e dispara a função do modo atual a cada segundo
 */
void loop() {
  verificarBotao(); // Verifica o estado do botão

  // Timer de 0 a INTERVALO_LEITURA
  if (millis() - timestampAnterior >= INTERVALO_LEITURA) {
    timestampAnterior = millis(); // Reseta o timer para a próxima contagem

    // 3. Executa a função do modo atual
    if (modoOperacao == MODO_CALIB_ANALOGICO) {
      executarModoAnalogico();
    } else {
      executarModoDigital();
    }
  }
}

/**
 * @brief Executa a lógica do Modo 1 a cada segundo.
 * Atualiza a calibração persistente (Min/Max) e calcula a média de 10s.
 */
void executarModoAnalogico() {
  int valorCruAO = analogRead(PINO_SENSOR_AO);
  
  // Lógica de Calibração valores máximo (seco) e mínimo (umido)
  if (valorCruAO > valorMaximo) valorMaximo = valorCruAO;
  if (valorCruAO < valorMinimo && valorCruAO > 0) valorMinimo = valorMinimo;

  // Lógica de Média (Ciclo de INTERVALO_LEITURA) ---
  contadorLeituras++;
  totalLeituras += valorCruAO;

  if (contadorLeituras >= QTD_AMOSTRAS) {
    Serial.println("[Modo 1] Resultado do Ciclo");

    int valorMedio = totalLeituras / QTD_AMOSTRAS;
    Serial.print("Média: "); 
    Serial.println(valorMedio);

    // Reporta a calibração PERSISTENTE
    Serial.println("Use o valor máximo para solo seco e valor mínimo para solo umido")
    Serial.print("Valor máximo: "); 
    Serial.println(valorMaximo);
    Serial.print("Valor mínimo: "); 
    Serial.println(valorMinimo);
    
    resetarCalibracaoAnalogica();
  }
}

/**
 * @brief Executa a lógica do Modo 2 a cada segundo.
 * Mostra uma leitura ao vivo (AO/DO) para ajuste do potenciômetro.
 */
void executarModoDigital() {
  int valorCruAO = analogRead(PINO_SENSOR_AO);
  int valorDigitalDO = digitalRead(PINO_SENSOR_DO);

  Serial.print("[Modo 2] Leitura AO: ");
  Serial.print(valorCruAO);
  Serial.print("\t| Estado DO: ");
  Serial.println( (valorDigitalDO == HIGH) ? "SECO (HIGH)" : "UMIDO (LOW)" );
}

/**
 * @brief Verifica o estado do botão
 */
void verificarBotao() {
  int estadoAtual = digitalRead(PINO_BOTAO);

  if (estadoAtual != estadoBotaoAnterior) {
    ultimoTempoBotao = millis();
  }

  if ((millis() - ultimoTempoBotao) > delayDebounce) {
    if (estadoAtual == LOW && estadoBotaoAnterior == HIGH) {
      
      
      if (modoOperacao == MODO_CALIB_ANALOGICO) {
        modoOperacao = MODO_CALIB_DIGITAL;
      } else {
        modoOperacao = MODO_CALIB_ANALOGICO;
      }
      
      // Imprime o novo modo no Serial
      Serial.println("\n==============================================");
      if (modoOperacao == MODO_CALIB_ANALOGICO) {
        Serial.println("MODO 1: CALIBRACAO PINO ANALOGICO");
        resetarCalibracaoAnalogica(); // Zera contadores ao entrar no modo 1
      } else {
        Serial.println("MODO 2: CALIBRACAO PINO DIGITAL");
        Serial.println("Gire o potenciometro. Observe AO e DO.");
      }
    }
  }
  estadoBotaoAnterior = estadoAtual;
}

/**
 * @brief Reseta os dados de calibração 
 */
void resetarCalibracaoAnalogica() {
  contadorLeituras = 0;
  totalLeituras = 0;
  valorMaximo = 0;
  valorMinimo = 4095;
}