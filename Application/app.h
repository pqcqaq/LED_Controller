/**
 * @file app.h
 * @brief Application header file
 * @author User
 * @date 2025-09-06
 */

#ifndef __APP_H__
#define __APP_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "drivers/stm32_u8g2.h"
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
 * @brief Enable or disable FPS test mode
 * @param enable true to enable FPS test mode, false to disable
 */
void App_SetFpsTestMode(bool enable);

/**
 * @brief Get current FPS value (multiplied by 1000)
 * @return Current FPS value as integer (actual FPS * 1000)
 */
uint32_t App_GetCurrentFps(void);

#ifdef __cplusplus
}
#endif

#endif /* __APP_H__ */
