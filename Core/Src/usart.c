/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.c
  * @brief   This file provides code for the configuration
  *          of the USART instances.
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
/* Includes ------------------------------------------------------------------*/
#include "usart.h"

/* USER CODE BEGIN 0 */
#include "dma.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

// DMA缓冲区和状态变量
uint8_t uart_rx_dma_buffer[UART_RX_BUFFER_SIZE];
volatile uint16_t uart_rx_write_pos = 0;
volatile uint16_t uart_rx_read_pos = 0;

// 消息处理相关变量
UartMessage_t uart_message = {0};

/* USER CODE END 0 */

UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_rx;

/* USART2 init function */

void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(uartHandle->Instance==USART2)
  {
  /* USER CODE BEGIN USART2_MspInit 0 */

  /* USER CODE END USART2_MspInit 0 */
    /* USART2 clock enable */
    __HAL_RCC_USART2_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**USART2 GPIO Configuration
    PA2     ------> USART2_TX
    PA3     ------> USART2_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USART2 DMA Init */
    /* USART2_RX Init */
    hdma_usart2_rx.Instance = DMA1_Channel6;
    hdma_usart2_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_usart2_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart2_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart2_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart2_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart2_rx.Init.Mode = DMA_NORMAL;
    hdma_usart2_rx.Init.Priority = DMA_PRIORITY_MEDIUM;
    if (HAL_DMA_Init(&hdma_usart2_rx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle,hdmarx,hdma_usart2_rx);

    /* USART2 interrupt Init */
    HAL_NVIC_SetPriority(USART2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
  /* USER CODE BEGIN USART2_MspInit 1 */

  /* USER CODE END USART2_MspInit 1 */
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{

  if(uartHandle->Instance==USART2)
  {
  /* USER CODE BEGIN USART2_MspDeInit 0 */

  /* USER CODE END USART2_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART2_CLK_DISABLE();

    /**USART2 GPIO Configuration
    PA2     ------> USART2_TX
    PA3     ------> USART2_RX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2|GPIO_PIN_3);

    /* USART2 DMA DeInit */
    HAL_DMA_DeInit(uartHandle->hdmarx);

    /* USART2 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART2_IRQn);
  /* USER CODE BEGIN USART2_MspDeInit 1 */

  /* USER CODE END USART2_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

/**
 * @brief 初始化UART DMA接收
 */
void UART_DMA_Init(void)
{
    // 清空缓冲区
    memset(uart_rx_dma_buffer, 0, UART_RX_BUFFER_SIZE);
    uart_rx_write_pos = 0;
    uart_rx_read_pos = 0;
    
    // 清空消息结构体
    memset(&uart_message, 0, sizeof(UartMessage_t));
    uart_message.state = UART_MSG_IDLE;
}

/**
 * @brief 启动UART DMA接收
 */
void UART_Start_DMA_Reception(void)
{
    // 启动DMA循环接收
    HAL_UART_Receive_DMA(&huart2, uart_rx_dma_buffer, UART_RX_BUFFER_SIZE);
    
    // 启用空闲线路中断
    __HAL_UART_ENABLE_IT(&huart2, UART_IT_IDLE);
}

/**
 * @brief 处理DMA接收的数据
 */
void UART_Process_DMA_Reception(void)
{
    // 获取当前DMA传输位置
    uint16_t current_pos = UART_RX_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(&hdma_usart2_rx);
    
    if (current_pos != uart_rx_write_pos) {
        uart_rx_write_pos = current_pos;
        UART_Parse_Buffer();
    }
}

/**
 * @brief 解析接收缓冲区中的数据
 */
void UART_Parse_Buffer(void)
{
    while (uart_rx_read_pos != uart_rx_write_pos) {
        uint8_t byte = uart_rx_dma_buffer[uart_rx_read_pos];
        uart_rx_read_pos = (uart_rx_read_pos + 1) % UART_RX_BUFFER_SIZE;
        
        // 如果当前没有正在接收的消息，开始新消息
        if (uart_message.state == UART_MSG_IDLE) {
            uart_message.length = 0;
            uart_message.state = UART_MSG_RECEIVING;
        }
        
        // 检查是否为命令结束符
        if (byte == UART_CMD_DELIMITER || byte == '\r') {
            if (uart_message.length > 0) {
                uart_message.data[uart_message.length] = '\0'; // 添加字符串结束符
                uart_message.state = UART_MSG_READY;
                return; // 消息完成，退出解析
            }
        }
        // 检查是否为可打印字符或有效字符
        else if (byte >= 0x20 && byte <= 0x7E) {
            if (uart_message.length < UART_CMD_MAX_LENGTH - 1) {
                uart_message.data[uart_message.length++] = byte;
            } else {
                // 缓冲区溢出，重置消息
                uart_message.length = 0;
                uart_message.state = UART_MSG_IDLE;
            }
        }
        // 忽略其他控制字符
    }
}

/**
 * @brief 处理空闲线路中断
 */
void UART_Handle_Idle_Interrupt(void)
{
    // 清除空闲标志
    __HAL_UART_CLEAR_IDLEFLAG(&huart2);
    
    // 处理接收到的数据
    UART_Process_DMA_Reception();
}

/**
 * @brief 检查是否有完整的消息
 * @return true 如果有完整消息，false 否则
 */
bool UART_Has_Message(void)
{
    return (uart_message.state == UART_MSG_READY);
}

/**
 * @brief 获取接收到的消息
 * @return 指向消息结构体的指针，如果没有消息则返回NULL
 */
UartMessage_t* UART_Get_Message(void)
{
    if (uart_message.state == UART_MSG_READY) {
        return &uart_message;
    }
    return NULL;
}

/**
 * @brief 清除当前消息，准备接收下一个消息
 */
void UART_Clear_Message(void)
{
    uart_message.length = 0;
    uart_message.state = UART_MSG_IDLE;
    memset(uart_message.data, 0, UART_CMD_MAX_LENGTH);
}

/**
 * @brief 发送字符串
 * @param str 要发送的字符串
 */
void UART_Send_String(const char* str)
{
    if (str != NULL) {
        HAL_UART_Transmit(&huart2, (uint8_t*)str, strlen(str), HAL_MAX_DELAY);
    }
}

/**
 * @brief 发送数据
 * @param data 要发送的数据
 * @param length 数据长度
 */
void UART_Send_Data(uint8_t* data, uint16_t length)
{
    if (data != NULL && length > 0) {
        HAL_UART_Transmit(&huart2, data, length, HAL_MAX_DELAY);
    }
}

/**
 * @brief printf风格的串口发送函数
 * @param format 格式化字符串
 * @param ... 可变参数
 */
void UART_Printf(const char* format, ...)
{
    char buffer[256];
    va_list args;
    va_start(args, format);
    int length = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    if (length > 0 && length < sizeof(buffer)) {
        UART_Send_String(buffer);
    }
}

/* USER CODE END 1 */
