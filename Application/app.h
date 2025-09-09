/**
 * @file app.h
 * @brief Application header file
 * @author User
 * @date 2025-09-06
 */

#ifndef __APP_H__
#define __APP_H__

/* Includes ------------------------------------------------------------------*/
#include "drivers/button.h"
#include "drivers/encoder.h"
#include "drivers/stm32_u8g2.h"
#include "main.h"
#include <stdbool.h>

/* Function prototypes -------------------------------------------------------*/

/**
 * @brief Application initialization function
 * @note This function is called once during system startup
 */
void App_Init(void);

/**
 * @brief Application main loop function
 * @note This function is called repeatedly in the main while loop
 */
void App_Loop(void);

/**
 * @brief On TIM3 interrupt, increment the capture count
 */
void App_TIM3_IRQHandler(void);

/**
 * @brief Process UART command
 * @param command Received command string
 * @param length Command length
 */
void App_Process_UART_Command(const char* command, uint16_t length);

#endif /* __APP_H__ */
