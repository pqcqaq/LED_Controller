/**
 * @file app.h
 * @brief Application header file
 * @author User
 * @date 2025-09-06
 */

#ifndef __APP_H__
#define __APP_H__


/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "drivers/stm32_u8g2.h"
#include "drivers/encoder.h"
#include "drivers/button.h"
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
 * @brief Reset button and encoder statistics
 */
void App_ResetStats(void);

/**
 * @brief On TIM3 interrupt, increment the capture count
 */
void App_TIM3_IRQHandler(void);

#endif /* __APP_H__ */
