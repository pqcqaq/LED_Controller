/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.h
  * @brief   This file contains all the function prototypes for
  *          the usart.c file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USART_H__
#define __USART_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */
#include <stdint.h>
#include <stdbool.h>
/* USER CODE END Includes */

extern UART_HandleTypeDef huart2;

/* USER CODE BEGIN Private defines */
// 串口接收缓冲区大小
#define UART_RX_BUFFER_SIZE     512
#define UART_CMD_MAX_LENGTH     256
#define UART_CMD_DELIMITER      '\n'

// 串口消息状态
typedef enum {
    UART_MSG_IDLE = 0,
    UART_MSG_RECEIVING,
    UART_MSG_READY
} UartMsgState_t;

// 串口消息结构体
typedef struct {
    uint8_t data[UART_CMD_MAX_LENGTH];
    uint16_t length;
    UartMsgState_t state;
} UartMessage_t;

// 外部变量声明
extern DMA_HandleTypeDef hdma_usart2_rx;
extern uint8_t uart_rx_dma_buffer[UART_RX_BUFFER_SIZE];
extern volatile uint16_t uart_rx_write_pos;
extern volatile uint16_t uart_rx_read_pos;
extern UartMessage_t uart_message;

/* USER CODE END Private defines */

void MX_USART2_UART_Init(void);

/* USER CODE BEGIN Prototypes */
// 串口初始化相关函数
void UART_DMA_Init(void);
void UART_Start_DMA_Reception(void);

// 串口消息处理函数
void UART_Process_DMA_Reception(void);
bool UART_Has_Message(void);
UartMessage_t* UART_Get_Message(void);
void UART_Clear_Message(void);

// 串口发送函数
void UART_Send_String(const char* str);
void UART_Send_Data(uint8_t* data, uint16_t length);
void UART_Printf(const char* format, ...);

// 内部处理函数
void UART_Parse_Buffer(void);
void UART_Handle_Idle_Interrupt(void);
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __USART_H__ */

