/**
 * @file commands.h
 * @brief Command system header file
 * @author User
 * @date 2025-09-09
 */

#ifndef __COMMANDS_H__
#define __COMMANDS_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdbool.h>
#include <stdint.h>

/* Defines -------------------------------------------------------------------*/
#define CMD_MAX_COMMANDS 128      // 最大命令队列长度
#define CMD_MAX_COMMAND_LENGTH 32 // 单个命令最大长度
#define CMD_MAX_PARAMS 8          // 命令最大参数数量
#define CMD_MAX_PARAM_LENGTH 4    // 参数最大长度
#define CMD_DELIMITER ';'         // 命令分隔符

/* Command execution status --------------------------------------------------*/
typedef enum {
  CMD_STATUS_SUCCESS = 0,
  CMD_STATUS_ERROR,
  CMD_STATUS_INVALID_PARAM,
  CMD_STATUS_UNKNOWN_COMMAND,
  CMD_STATUS_QUEUE_FULL,
  CMD_STATUS_QUEUE_EMPTY,
  CMD_STATUS_WAITING,
  CMD_STATUS_CONTINUE_SUBCOMMAND // 继续执行子命令
} CommandStatus_t;               /* Command structure
                                    ---------------------------------------------------------*/
typedef struct CommandStruct CommandStruct_t;

typedef CommandStatus_t (*CommandCallback_t)(const char *params[],
                                             uint8_t param_count);

struct CommandStruct {
  const char *command;                // 命令名称
  CommandCallback_t callback;         // 回调函数
  const CommandStruct_t *subcommands; // 子命令数组
  uint8_t subcommand_count;           // 子命令数量
  const char *description;            // 命令描述
};

/* Parsed command for execution queue ---------------------------------------*/
typedef struct {
  char command_line[CMD_MAX_COMMAND_LENGTH]; // 完整命令行
  char *params[CMD_MAX_PARAMS];              // 参数指针数组
  uint8_t param_count;                       // 参数数量
  uint32_t wait_cycles;                      // 等待周期（用于WAIT命令）
} ParsedCommand_t;

/* Command queue structure ---------------------------------------------------*/
typedef struct {
  ParsedCommand_t commands[CMD_MAX_COMMANDS]; // 命令队列
  volatile uint16_t head;                     // 队列头
  volatile uint16_t tail;                     // 队列尾
  volatile uint16_t count;                    // 队列中命令数量
  volatile uint32_t current_wait_cycles;      // 当前等待周期计数器
} CommandQueue_t;

/* Function prototypes -------------------------------------------------------*/

/**
 * @brief Initialize command system
 */
void Commands_Init(void);

/**
 * @brief Parse and enqueue commands from input string
 * @param input Input command string (commands separated by ';')
 * @return Number of commands successfully parsed and enqueued
 */
uint16_t Commands_Parse_And_Enqueue(const char *input);

/**
 * @brief Execute one command from the queue (called by executor loop)
 * @return Command execution status
 */
CommandStatus_t Commands_Execute_Next(void);

/**
 * @brief Get queue status
 * @return Number of commands in queue
 */
uint16_t Commands_Get_Queue_Count(void);

/**
 * @brief Clear all commands in queue
 */
void Commands_Clear_Queue(void);

/**
 * @brief Check if queue is empty
 * @return true if queue is empty, false otherwise
 */
bool Commands_Is_Queue_Empty(void);

/**
 * @brief Command executor loop function (call this from timer interrupt)
 */
void Commands_Executor_Loop(void);

/**
 * @brief Command result printf function
 * @param format Format string
 * @param ... Variable arguments
 * @return Number of characters written
 */
int Commands_Result_Printf(const char *format, ...);

/* Command callback function prototypes -------------------------------------*/
// 这些函数需要在其他模块中实现
CommandStatus_t Cmd_Power_Handler(const char *params[], uint8_t param_count);
CommandStatus_t Cmd_Power_Off_Handler(const char *params[],
                                      uint8_t param_count);
CommandStatus_t Cmd_Power_On_Handler(const char *params[], uint8_t param_count);
CommandStatus_t Cmd_Power_Ch1_Handler(const char *params[],
                                      uint8_t param_count);
CommandStatus_t Cmd_Power_Ch2_Handler(const char *params[],
                                      uint8_t param_count);
CommandStatus_t Cmd_Power_Ch1_Read_Handler(const char *params[],
                                           uint8_t param_count);
CommandStatus_t Cmd_Power_Ch1_Set_Handler(const char *params[],
                                          uint8_t param_count);
CommandStatus_t Cmd_Power_Ch2_Read_Handler(const char *params[],
                                           uint8_t param_count);
CommandStatus_t Cmd_Power_Ch2_Set_Handler(const char *params[],
                                          uint8_t param_count);
CommandStatus_t Cmd_Power_Fade_Handler(const char *params[],
                                       uint8_t param_count);

CommandStatus_t Cmd_Fan_Handler(const char *params[], uint8_t param_count);
CommandStatus_t Cmd_Fan_Auto_Handler(const char *params[], uint8_t param_count);
CommandStatus_t Cmd_Fan_Force_Handler(const char *params[],
                                      uint8_t param_count);

CommandStatus_t Cmd_Sleep_Handler(const char *params[], uint8_t param_count);
CommandStatus_t Cmd_Sleep_Deep_Handler(const char *params[],
                                       uint8_t param_count);

CommandStatus_t Cmd_Wait_Handler(const char *params[], uint8_t param_count);
CommandStatus_t Cmd_Reboot_Handler(const char *params[], uint8_t param_count);

CommandStatus_t Cmd_Eeprom_Handler(const char *params[], uint8_t param_count);
CommandStatus_t Cmd_Eeprom_Read_Handler(const char *params[],
                                        uint8_t param_count);
CommandStatus_t Cmd_Eeprom_Write_Handler(const char *params[],
                                         uint8_t param_count);

CommandStatus_t Cmd_Help_Handler(const char *params[], uint8_t param_count);

#ifdef __cplusplus
}
#endif

#endif /* __COMMANDS_H__ */
