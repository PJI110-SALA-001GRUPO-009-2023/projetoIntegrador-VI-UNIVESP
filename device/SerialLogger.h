/**
 * @file SerialLogger.h
 * @brief Declaração da classe SerialLogger para logging via Serial.
 */

#ifndef SERIALLOGGER_H // Necessário para evitar múltiplas inclusões
#define SERIALLOGGER_H // Início do arquivo de inclusão única

// Inclui a biblioteca principal do Arduino (necessário para ter acesso ao objeto 'Serial' e ao tipo 'String')
#include <Arduino.h>


#ifndef SERIAL_LOGGER_BAUD_RATE // Se a taxa de baud não estiver definida, define o padrão
#define SERIAL_LOGGER_BAUD_RATE 115200 // Define a velocidade de comunicação serial (baud rate) padrão
#endif

/**
 * @enum LogLevel
 * @brief Níveis de log para o SerialLogger
 */
enum LogLevel {
  LOG_LEVEL_NONE,  // 0: Não registrar nada
  LOG_LEVEL_EVENT, // 1: Registrar apenas eventos-chave (Ex: "Rega Iniciada")
  LOG_LEVEL_ERROR, // 2: Registrar Eventos + Erros
  LOG_LEVEL_INFO,  // 3: Registrar Eventos + Erros + Informações gerais
  LOG_LEVEL_DEBUG  // 4: Registrar TUDO (Ex: "lendo sensor...")
};

// Declaração da classe 'SerialLogger'.
class SerialLogger
{
public:
  SerialLogger(); // Construtor padrão
  
  /**
   * @brief Define o nível de filtro do logger
   * Mensagens com prioridade menor que o 'level' definido serão ignoradas
   * @param level O nível de log desejado (ex: LOG_LEVEL_INFO)
   */
  void setLevel(LogLevel level);

  /**
   * @brief Registra uma mensagem de "Evento" (alta prioridade)
   * Ex: "Rega Iniciada", "Botão Pressionado"
   */
  void Event(String message);

  /**
   * @brief Registra uma mensagem de "Erro" (prioridade média-alta)
   * Ex: "Falha ao ler DHT11", "Falha ao publicar"
   */
  void Error(String message);

  /**
   * @brief Registra uma mensagem de "Informação" (prioridade média)
   * Ex: "Wi-Fi conectado", "Sistema inicializado"
   */
  void Info(String message);
  
  /**
   * @brief Registra uma mensagem de "Debug" (baixa prioridade)
   * Ex: "Enviando telemetria...", "Sensores lidos..."
   */
  void Debug(String message);

private:
  LogLevel currentLevel; // Variável interna para armazenar o nível de filtro atual
};

extern SerialLogger Logger; // Declara a instância global do SerialLogger

#endif // Fim do arquivo de inclusão única