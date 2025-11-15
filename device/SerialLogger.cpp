/**
 * @file SerialLogger.cpp
 * @brief Implementação da classe SerialLogger para logging via Serial.
 */

#include "SerialLogger.h" // Inclui o "contrato" (o arquivo .h) para que este arquivo .cpp saiba o que implementar.
#include <time.h>         // Inclui a biblioteca de tempo para formatar o timestamp (data e hora).

#define UNIX_EPOCH_START_YEAR 1900 // Define a constante necessária para corrigir o ano

/**
 * @brief Função auxiliar INTERNA para formatar e imprimir o timestamp.
 * * 'static' significa que esta função só pode ser "vista" e usada
 * DENTRO deste arquivo .cpp. Ela é invisível para o resto do projeto.
 * * Esta função SÓ FUNCIONA CORRETAMENTE se o relógio do ESP32
 * (interno ou via RTC) estiver sincronizado.
 */
static void writeTime()
{
  struct tm* ptm; // Ponteiro para a estrutura de tempo "quebrada"
  
  // time(NULL) pega o tempo atual do sistema como um "timestamp Unix"
  // (segundos desde 01/01/1970).
  time_t now = time(NULL);

  // gmtime() converte o timestamp para o fuso horário UTC/GMT.
  ptm = gmtime(&now);

  // Imprime a data formatada: YYYY/MM/DD
  Serial.print(ptm->tm_year + UNIX_EPOCH_START_YEAR); // Corrige o ano
  Serial.print("/");
  Serial.print(ptm->tm_mon + 1); // Corrige o mês (que é 0-11)
  Serial.print("/");
  Serial.print(ptm->tm_mday);
  Serial.print(" ");

  // Imprime a hora formatada: HH:MM:SS
  // Adiciona um zero à esquerda para horas/min/seg menores que 10.
  if (ptm->tm_hour < 10) { Serial.print(0); }
  Serial.print(ptm->tm_hour);
  Serial.print(":");

  if (ptm->tm_min < 10) { Serial.print(0); }
  Serial.print(ptm->tm_min);
  Serial.print(":");

  if (ptm->tm_sec < 10) { Serial.print(0); }
  Serial.print(ptm->tm_sec);
}

/**
 * @brief Implementação do Construtor da classe.
 * * Este código é executado automaticamente UMA VEZ, no momento em
 * que a variável global 'Logger' é criada (antes mesmo do setup()).
 */
SerialLogger::SerialLogger() 
{
  // Inicia a comunicação serial na velocidade definida.
  // É por isso que você não precisa mais de 'Serial.begin()' no seu setup().
  Serial.begin(SERIAL_LOGGER_BAUD_RATE); 
  
  // Define o nível de filtro padrão. Por padrão, veremos
  // Eventos, Erros e Infos, mas não veremos Debug.
  currentLevel = LOG_LEVEL_INFO; 
}

/**
 * @brief Implementação da função setLevel.
 * Atualiza a variável interna 'currentLevel'.
 */
void SerialLogger::setLevel(LogLevel level) {
  currentLevel = level;
  Serial.print(F("\n[LOGGER] Nivel de log definido para: "));
  Serial.println(level);
}

/**
 * @brief Implementação do log de Evento.
 */
void SerialLogger::Event(String message)
{
  // Só imprime se o nível atual for >= EVENT (1)
  if (currentLevel >= LOG_LEVEL_EVENT) {
    writeTime(); // Imprime o timestamp
    Serial.print(" [EVENT] "); // Imprime o Nível
    Serial.println(message); // Imprime a Mensagem
  }
}

/**
 * @brief Implementação do log de Erro.
 */
void SerialLogger::Error(String message)
{
  // Só imprime se o nível atual for >= ERROR (2)
  if (currentLevel >= LOG_LEVEL_ERROR) {
    writeTime();
    Serial.print(" [ERROR] ");
    Serial.println(message);
  }
}

/**
 * @brief Implementação do log de Informação.
 */
void SerialLogger::Info(String message)
{
  // Só imprime se o nível atual for >= INFO (3)
  if (currentLevel >= LOG_LEVEL_INFO) {
    writeTime();
    Serial.print(" [INFO] ");
    Serial.println(message);
  }
}

/**
 * @brief Implementação do log de Debug.
 */
void SerialLogger::Debug(String message)
{
  // Só imprime se o nível atual for >= DEBUG (4)
  if (currentLevel >= LOG_LEVEL_DEBUG) {
    writeTime();
    Serial.print(" [DEBUG] ");
    Serial.println(message);
  }
}

SerialLogger Logger; // Declara a instância global do SerialLogger