/**
 * @file commands.cpp
 * @brief Command system implementation
 * @author User
 * @date 2025-09-09
 */

/* Includes ------------------------------------------------------------------*/
#include "commands.h"
#include "global/controller.h"
#include "global_objects.h"
#include "usart.h"
#include <cstdio>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <tim.h>

/* Private variables ---------------------------------------------------------*/
static CommandQueue_t command_queue = {{0}};

/* Command structure definitions ---------------------------------------------*/

// POWER子命令定义
static const CommandStruct_t power_ch1_subcommands[] = {
    {"READ", Cmd_Power_Ch1_Read_Handler, NULL, 0, "Read CH1 target PWM"},
    {"SHOW", Cmd_Power_Ch1_Read_Handler, NULL, 0, "Show CH1 target PWM"},
    {"SET", Cmd_Power_Ch1_Set_Handler, NULL, 0, "Set CH1 target PWM value"}};

static const CommandStruct_t power_ch2_subcommands[] = {
    {"READ", Cmd_Power_Ch2_Read_Handler, NULL, 0, "Read CH2 target PWM"},
    {"SHOW", Cmd_Power_Ch2_Read_Handler, NULL, 0, "Show CH2 target PWM"},
    {"SET", Cmd_Power_Ch2_Set_Handler, NULL, 0, "Set CH2 target PWM value"}};

static const CommandStruct_t power_subcommands[] = {
    {"OFF", Cmd_Power_Off_Handler, NULL, 0, "Turn power off"},
    {"ON", Cmd_Power_On_Handler, NULL, 0, "Turn power on"},
    {"CH1", Cmd_Power_Ch1_Handler, power_ch1_subcommands,
     sizeof(power_ch1_subcommands) / sizeof(CommandStruct_t),
     "Channel 1 control"},
    {"CH2", Cmd_Power_Ch2_Handler, power_ch2_subcommands,
     sizeof(power_ch2_subcommands) / sizeof(CommandStruct_t),
     "Channel 2 control"},
    {"FADE", Cmd_Power_Fade_Handler, NULL, 0, "Set PWM fade step"}};

// FAN子命令定义
static const CommandStruct_t fan_subcommands[] = {
    {"AUTO", Cmd_Fan_Auto_Handler, NULL, 0, "Set fan to auto mode"},
    {"FORCE", Cmd_Fan_Force_Handler, NULL, 0, "Set fan to force mode"}};

// SLEEP子命令定义
static const CommandStruct_t sleep_subcommands[] = {
    {"DEEP", Cmd_Sleep_Deep_Handler, NULL, 0, "Enter deep sleep mode"}};

// EEPROM子命令定义
static const CommandStruct_t eeprom_subcommands[] = {
    {"READ", Cmd_Eeprom_Read_Handler, NULL, 0, "Read EEPROM data"},
    {"WRITE", Cmd_Eeprom_Write_Handler, NULL, 0, "Write EEPROM data"}};

// 主命令表
static const CommandStruct_t main_commands[] = {
    {"POWER", Cmd_Power_Handler, power_subcommands,
     sizeof(power_subcommands) / sizeof(CommandStruct_t), "Power control"},
    {"FAN", Cmd_Fan_Handler, fan_subcommands,
     sizeof(fan_subcommands) / sizeof(CommandStruct_t), "Fan control"},
    {"SLEEP", Cmd_Sleep_Handler, sleep_subcommands,
     sizeof(sleep_subcommands) / sizeof(CommandStruct_t), "Sleep control"},
    {"WAIT", Cmd_Wait_Handler, NULL, 0, "Wait for specified cycles"},
    {"REBOOT", Cmd_Reboot_Handler, NULL, 0, "Reboot system"},
    {"EEPROM", Cmd_Eeprom_Handler, eeprom_subcommands,
     sizeof(eeprom_subcommands) / sizeof(CommandStruct_t), "EEPROM operations"},
    {"HELP", Cmd_Help_Handler, NULL, 0, "Show available commands"}};

static const uint8_t main_command_count =
    sizeof(main_commands) / sizeof(CommandStruct_t);

/* Private function prototypes -----------------------------------------------*/
static char *trim_whitespace(char *str);
static CommandStatus_t execute_command(const CommandStruct_t *cmd_table,
                                       uint8_t cmd_count, const char *params[],
                                       uint8_t param_count);
static bool enqueue_command(const ParsedCommand_t *cmd);
static bool dequeue_command(ParsedCommand_t *cmd);

/* Public functions ----------------------------------------------------------*/

/**
 * @brief Initialize command system
 */
void Commands_Init(void) {
  memset(&command_queue, 0, sizeof(CommandQueue_t));
  command_queue.head = 0;
  command_queue.tail = 0;
  command_queue.count = 0;
  command_queue.current_wait_cycles = 0;

  UART_Printf("Command system initialized\r\n");
}

/**
 * @brief Parse and enqueue commands from input string
 * @param input Input command string (commands separated by ';')
 * @return Number of commands successfully parsed and enqueued
 */
uint16_t Commands_Parse_And_Enqueue(const char *input) {
  if (input == NULL) {
    return 0;
  }

  char input_copy[1024];
  strncpy(input_copy, input, sizeof(input_copy) - 1);
  input_copy[sizeof(input_copy) - 1] = '\0';

  uint16_t parsed_count = 0;
  char *token = strtok(input_copy, ";");

  while (token != NULL && parsed_count < CMD_MAX_COMMANDS) {
    token = trim_whitespace(token);

    if (strlen(token) > 0) {
      ParsedCommand_t cmd;
      memset(&cmd, 0, sizeof(ParsedCommand_t));

      // 复制命令行 (保持原样，不在这里解析)
      strncpy(cmd.command_line, token, CMD_MAX_COMMAND_LENGTH - 1);
      cmd.command_line[CMD_MAX_COMMAND_LENGTH - 1] = '\0';

      // 参数解析将在 enqueue_command 中进行
      cmd.param_count = 0;

      // 入队命令
      if (enqueue_command(&cmd)) {
        parsed_count++;
      } else {
        UART_Printf("Error: Command queue full\r\n");
        break;
      }
    }

    token = strtok(NULL, ";");
  }

  UART_Printf("Parsed and enqueued %d commands\r\n", parsed_count);
  return parsed_count;
}

/**
 * @brief Execute one command from the queue (called by executor loop)
 * @return Command execution status
 */
CommandStatus_t Commands_Execute_Next(void) {
  if (command_queue.count == 0) {
    return CMD_STATUS_QUEUE_EMPTY;
  }

  // 检查是否在等待状态
  if (command_queue.current_wait_cycles > 0) {
    command_queue.current_wait_cycles--;
    return CMD_STATUS_WAITING;
  }

  ParsedCommand_t cmd;
  if (!dequeue_command(&cmd)) {
    return CMD_STATUS_QUEUE_EMPTY;
  }

  // 特殊处理WAIT命令
  if (cmd.param_count > 0 && strcmp(cmd.params[0], "WAIT") == 0) {
    if (cmd.param_count >= 2) {
      command_queue.current_wait_cycles = atoi(cmd.params[1]);
      return CMD_STATUS_SUCCESS;
    } else {
      UART_Printf("Error: WAIT command requires cycle count\r\n");
      return CMD_STATUS_INVALID_PARAM;
    }
  }

  // 执行普通命令
  CommandStatus_t status =
      execute_command(main_commands, main_command_count,
                      (const char **)cmd.params, cmd.param_count);

  if (status == CMD_STATUS_UNKNOWN_COMMAND) {
    UART_Printf("Error: Unknown command '%s'\r\n", cmd.params[0]);
  }

  return status;
}

/**
 * @brief Get queue status
 * @return Number of commands in queue
 */
uint16_t Commands_Get_Queue_Count(void) { return command_queue.count; }

/**
 * @brief Clear all commands in queue
 */
void Commands_Clear_Queue(void) {
  command_queue.head = 0;
  command_queue.tail = 0;
  command_queue.count = 0;
  command_queue.current_wait_cycles = 0;
}

/**
 * @brief Check if queue is empty
 * @return true if queue is empty, false otherwise
 */
bool Commands_Is_Queue_Empty(void) { return (command_queue.count == 0); }

/**
 * @brief Command executor loop function (call this from timer interrupt)
 */
void Commands_Executor_Loop(void) {
  CommandStatus_t status = Commands_Execute_Next();

  // 为了调试，输出执行状态
  if (status == CMD_STATUS_SUCCESS) {
    // 成功执行，不输出（避免过多输出）
  } else if (status == CMD_STATUS_WAITING) {
    // 等待状态，不输出
  } else if (status == CMD_STATUS_QUEUE_EMPTY) {
    // 队列空，不输出
  } else if (status == CMD_STATUS_CONTINUE_SUBCOMMAND) {
    // 这个状态不应该出现在这里，如果出现说明逻辑有问题
    UART_Printf("Warning: Unexpected CONTINUE_SUBCOMMAND status\r\n");
  } else {
    // 只在有错误时输出状态
    UART_Printf("Command execution error: %d\r\n", status);
  }
}

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Trim leading and trailing whitespace
 * @param str String to trim
 * @return Trimmed string
 */
static char *trim_whitespace(char *str) {
  char *end;

  // Trim leading space
  while (isspace((unsigned char)*str))
    str++;

  if (*str == 0)
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while (end > str && isspace((unsigned char)*end))
    end--;

  end[1] = '\0';
  return str;
}

/**
 * @brief Execute command from command table
 * @param cmd_table Command table to search
 * @param cmd_count Number of commands in table
 * @param params Command parameters
 * @param param_count Number of parameters
 * @return Command execution status
 */
static CommandStatus_t execute_command(const CommandStruct_t *cmd_table,
                                       uint8_t cmd_count, const char *params[],
                                       uint8_t param_count) {
  if (param_count == 0) {
    return CMD_STATUS_INVALID_PARAM;
  }

  // 查找命令
  for (uint8_t i = 0; i < cmd_count; i++) {
    if (strcmp(params[0], cmd_table[i].command) == 0) {
      CommandStatus_t callback_status = CMD_STATUS_SUCCESS;

      // 如果有回调函数，先执行回调
      if (cmd_table[i].callback != NULL) {
        callback_status = cmd_table[i].callback(params, param_count);
      }

      // 如果回调返回CONTINUE_SUBCOMMAND，且有子命令和足够参数，继续执行子命令
      if (callback_status == CMD_STATUS_CONTINUE_SUBCOMMAND &&
          cmd_table[i].subcommands != NULL && param_count > 1) {
        return execute_command(cmd_table[i].subcommands,
                               cmd_table[i].subcommand_count, &params[1],
                               param_count - 1);
      }
      // 如果回调返回SUCCESS，且有子命令和足够参数，也继续执行子命令
      else if (callback_status == CMD_STATUS_SUCCESS &&
               cmd_table[i].subcommands != NULL && param_count > 1) {
        return execute_command(cmd_table[i].subcommands,
                               cmd_table[i].subcommand_count, &params[1],
                               param_count - 1);
      }

      // 返回回调函数的状态（如果是错误状态，父命令已经输出了错误信息）
      return callback_status;
    }
  }

  return CMD_STATUS_UNKNOWN_COMMAND;
}

/**
 * @brief Add command to queue
 * @param cmd Command to enqueue
 * @return true if successful, false if queue is full
 */
static bool enqueue_command(const ParsedCommand_t *cmd) {
  if (command_queue.count >= CMD_MAX_COMMANDS) {
    return false;
  }

  // 复制命令数据
  memcpy(&command_queue.commands[command_queue.tail], cmd,
         sizeof(ParsedCommand_t));

  // 更新参数指针，指向新复制的命令行数据
  char *cmd_line = command_queue.commands[command_queue.tail].command_line;
  char *token = strtok(cmd_line, " \t");
  uint8_t param_idx = 0;

  while (token != NULL && param_idx < CMD_MAX_PARAMS) {
    command_queue.commands[command_queue.tail].params[param_idx] = token;
    param_idx++;
    token = strtok(NULL, " \t");
  }
  command_queue.commands[command_queue.tail].param_count = param_idx;

  command_queue.tail = (command_queue.tail + 1) % CMD_MAX_COMMANDS;
  command_queue.count++;

  return true;
}

/**
 * @brief Remove command from queue
 * @param cmd Pointer to store dequeued command
 * @return true if successful, false if queue is empty
 */
static bool dequeue_command(ParsedCommand_t *cmd) {
  if (command_queue.count == 0) {
    return false;
  }

  memcpy(cmd, &command_queue.commands[command_queue.head],
         sizeof(ParsedCommand_t));

  command_queue.head = (command_queue.head + 1) % CMD_MAX_COMMANDS;
  command_queue.count--;

  return true;
}

/* Default command handlers (weak implementations) --------------------------*/
// 这些函数提供默认实现，可以在其他文件中重新定义

__weak CommandStatus_t Cmd_Power_Handler(const char *params[],
                                         uint8_t param_count) {
  // 如果只有一个参数（就是POWER本身），提示需要子命令
  if (param_count <= 1) {
    UART_Printf(
        "Error: POWER command requires subcommand (ON/OFF/CH1/CH2/FADE)\r\n");
    return CMD_STATUS_INVALID_PARAM;
  }

  return CMD_STATUS_CONTINUE_SUBCOMMAND;
}

__weak CommandStatus_t Cmd_Power_Off_Handler(const char *params[],
                                             uint8_t param_count) {
  UART_Printf("Power OFF command executed\r\n");
  turnOff(); // 模拟长按关机
  Commands_Result_Printf("System powered off\r\n");
  return CMD_STATUS_SUCCESS;
}

__weak CommandStatus_t Cmd_Power_On_Handler(const char *params[],
                                            uint8_t param_count) {
  UART_Printf("Power ON command executed\r\n");
  turnOn(); // 模拟长按开机
  Commands_Result_Printf("System powered on\r\n");
  return CMD_STATUS_SUCCESS;
}

__weak CommandStatus_t Cmd_Power_Ch1_Handler(const char *params[],
                                             uint8_t param_count) {
  // CH1命令至少需要2个参数：CH1 SUBCOMMAND
  if (param_count < 2) {
    UART_Printf("Error: CH1 command requires subcommand (READ/SHOW/SET)\r\n");
    return CMD_STATUS_INVALID_PARAM;
  }

  // 继续执行子命令
  return CMD_STATUS_CONTINUE_SUBCOMMAND;
}

__weak CommandStatus_t Cmd_Power_Ch2_Handler(const char *params[],
                                             uint8_t param_count) {
  // CH2命令至少需要2个参数：CH2 SUBCOMMAND
  if (param_count < 2) {
    UART_Printf("Error: CH2 command requires subcommand (READ/SHOW/SET)\r\n");
    return CMD_STATUS_INVALID_PARAM;
  }

  // 继续执行子命令
  return CMD_STATUS_CONTINUE_SUBCOMMAND;
}

__weak CommandStatus_t Cmd_Power_Ch1_Read_Handler(const char *params[],
                                                  uint8_t param_count) {
  Commands_Result_Printf("CH1 PWM: %d\r\n", state.targetCh1PWM);
  return CMD_STATUS_SUCCESS;
}

__weak CommandStatus_t Cmd_Power_Ch1_Set_Handler(const char *params[],
                                                 uint8_t param_count) {
  if (param_count < 2) {
    UART_Printf("Error: CH1 SET requires value parameter\r\n");
    return CMD_STATUS_INVALID_PARAM;
  }

  // 范围必须在0-LED_MAX_PWM
  uint16_t pwm_value = atoi(params[1]);
  if (pwm_value < 0 || pwm_value > MAX_PWM) {
    UART_Printf("Error: CH1 SET value must be between 0 and %d\r\n", MAX_PWM);
    return CMD_STATUS_INVALID_PARAM;
  }

  state.targetCh1PWM = pwm_value;

  Commands_Result_Printf("CH1 PWM set to %d\r\n", state.targetCh1PWM);

  return CMD_STATUS_SUCCESS;
}

__weak CommandStatus_t Cmd_Power_Ch2_Read_Handler(const char *params[],
                                                  uint8_t param_count) {
  Commands_Result_Printf("CH2 PWM: %d\r\n", state.targetCh2PWM);
  return CMD_STATUS_SUCCESS;
}

__weak CommandStatus_t Cmd_Power_Ch2_Set_Handler(const char *params[],
                                                 uint8_t param_count) {
  if (param_count < 2) {
    UART_Printf("Error: CH2 SET requires value parameter\r\n");
    return CMD_STATUS_INVALID_PARAM;
  }

  // 范围必须在0-LED_MAX_PWM
  uint16_t pwm_value = atoi(params[1]);
  if (pwm_value < 0 || pwm_value > MAX_PWM) {
    UART_Printf("Error: CH2 SET value must be between 0 and %d\r\n", MAX_PWM);
    return CMD_STATUS_INVALID_PARAM;
  }

  state.targetCh2PWM = pwm_value;

  Commands_Result_Printf("CH2 PWM set to %d\r\n", state.targetCh2PWM);

  return CMD_STATUS_SUCCESS;
}

__weak CommandStatus_t Cmd_Power_Fade_Handler(const char *params[],
                                              uint8_t param_count) {
  if (param_count >= 2) {
    Commands_Result_Printf("Power FADE %s command executed\r\n", params[1]);
  } else {
    UART_Printf("Error: FADE requires value parameter\r\n");
    return CMD_STATUS_INVALID_PARAM;
  }
  return CMD_STATUS_SUCCESS;
}

__weak CommandStatus_t Cmd_Fan_Handler(const char *params[],
                                       uint8_t param_count) {
  // FAN命令至少需要2个参数：FAN SUBCOMMAND
  if (param_count < 2) {
    UART_Printf("Error: FAN command requires subcommand (AUTO/FORCE)\r\n");
    return CMD_STATUS_INVALID_PARAM;
  }

  // 继续执行子命令
  return CMD_STATUS_CONTINUE_SUBCOMMAND;
}

__weak CommandStatus_t Cmd_Fan_Auto_Handler(const char *params[],
                                            uint8_t param_count) {
  fan_auto();
  Commands_Result_Printf("Fan set to AUTO mode\r\n");
  return CMD_STATUS_SUCCESS;
}

__weak CommandStatus_t Cmd_Fan_Force_Handler(const char *params[],
                                             uint8_t param_count) {
  fan_force();
  Commands_Result_Printf("Fan set to FORCE mode\r\n");
  return CMD_STATUS_SUCCESS;
}

__weak CommandStatus_t Cmd_Sleep_Handler(const char *params[],
                                         uint8_t param_count) {
  // 如果只有SLEEP，执行普通睡眠
  if (param_count == 1) {
    state.isSleeping = true;
    state.deepSleep = false;
    Commands_Result_Printf("Sleep command executed\r\n");
    return CMD_STATUS_SUCCESS;
  }

  // 如果有子命令，继续执行
  return CMD_STATUS_CONTINUE_SUBCOMMAND;
}

__weak CommandStatus_t Cmd_Sleep_Deep_Handler(const char *params[],
                                              uint8_t param_count) {
  state.isSleeping = true;
  state.deepSleep = true;
  Commands_Result_Printf("Deep sleep command executed\r\n");
  return CMD_STATUS_SUCCESS;
}

__weak CommandStatus_t Cmd_Wait_Handler(const char *params[],
                                        uint8_t param_count) {
  // WAIT命令在Commands_Execute_Next中特殊处理
  return CMD_STATUS_SUCCESS;
}

__weak CommandStatus_t Cmd_Reboot_Handler(const char *params[],
                                          uint8_t param_count) {
  return CMD_STATUS_SUCCESS;
}

__weak CommandStatus_t Cmd_Eeprom_Handler(const char *params[],
                                          uint8_t param_count) {
  // EEPROM命令至少需要2个参数：EEPROM SUBCOMMAND
  if (param_count < 2) {
    UART_Printf("Error: EEPROM command requires subcommand (READ/WRITE)\r\n");
    return CMD_STATUS_INVALID_PARAM;
  }

  // 继续执行子命令
  return CMD_STATUS_CONTINUE_SUBCOMMAND;
}

__weak CommandStatus_t Cmd_Eeprom_Read_Handler(const char *params[],
                                               uint8_t param_count) {
  if (param_count >= 3) {
    UART_Printf("EEPROM READ addr:%s length:%s command executed\r\n", params[1],
                params[2]);
  } else {
    UART_Printf("Error: EEPROM READ requires address and length\r\n");
    return CMD_STATUS_INVALID_PARAM;
  }
  return CMD_STATUS_SUCCESS;
}

__weak CommandStatus_t Cmd_Eeprom_Write_Handler(const char *params[],
                                                uint8_t param_count) {
  if (param_count >= 3) {
    UART_Printf("EEPROM WRITE addr:%s data:%s command executed\r\n", params[1],
                params[2]);
  } else {
    UART_Printf("Error: EEPROM WRITE requires address and data\r\n");
    return CMD_STATUS_INVALID_PARAM;
  }
  return CMD_STATUS_SUCCESS;
}

__weak CommandStatus_t Cmd_Help_Handler(const char *params[],
                                        uint8_t param_count) {
  UART_Printf("Available commands:\r\n");
  UART_Printf("POWER ON/OFF - Power control\r\n");
  UART_Printf("POWER CH1 READ/SHOW - Read CH1 PWM\r\n");
  UART_Printf("POWER CH1 SET <value> - Set CH1 PWM\r\n");
  UART_Printf("POWER CH2 READ/SHOW - Read CH2 PWM\r\n");
  UART_Printf("POWER CH2 SET <value> - Set CH2 PWM\r\n");
  UART_Printf("POWER FADE <step> - Set PWM fade step\r\n");
  UART_Printf("FAN AUTO/FORCE - Fan control\r\n");
  UART_Printf("SLEEP [DEEP] - Sleep mode\r\n");
  UART_Printf("WAIT <cycles> - Wait cycles\r\n");
  UART_Printf("REBOOT - Restart system\r\n");
  UART_Printf("EEPROM READ <addr> <length> - Read EEPROM\r\n");
  UART_Printf("EEPROM WRITE <addr> <data> - Write EEPROM\r\n");
  UART_Printf("HELP - Show this help\r\n");
  return CMD_STATUS_SUCCESS;
}

/**
 * @brief Command result printf function
 * @param format Format string
 * @param ... Variable arguments
 * @return Number of characters written
 */
int Commands_Result_Printf(const char *format, ...) {
  char buffer[128];
  va_list args;
  va_start(args, format);

  // 添加前缀
  int prefix_len = snprintf(buffer, sizeof(buffer), "#$%%>");

  // 格式化用户内容
  int content_len =
      vsnprintf(buffer + prefix_len, sizeof(buffer) - prefix_len, format, args);
  va_end(args);

  int total_len = prefix_len + content_len;
  if (total_len > 0) {
    // Send via UART
    HAL_UART_Transmit(&huart2, (uint8_t *)buffer,
                      (total_len > sizeof(buffer) ? sizeof(buffer) : total_len),
                      HAL_MAX_DELAY);
  }

  return total_len;
}