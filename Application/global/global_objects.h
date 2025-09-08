/**
 * @file global_objects.h
 * @brief Global Objects Header
 * @author User
 * @date 2025-09-06
 */

#ifndef __GLOBAL_OBJECTS_H__
#define __GLOBAL_OBJECTS_H__

/* Includes ------------------------------------------------------------------*/
#include "drivers/button.h"
#include "drivers/encoder.h"
#include "stm32_u8g2.h"

/* Global Objects ------------------------------------------------------------*/

// Global encoder instance
extern RotaryEncoder rotary_encoder;

// Global U8G2 display instance
extern STM32_U8G2_Display u8g2;

// Global button instance
extern Button encoder_button;

// PA5
extern Button btn_1;

// PA6
extern Button btn_2;

/* Function declarations -----------------------------------------------------*/

/**
 * @brief Initialize all global objects
 */
void GlobalObjects_Init(void);

/**
 * @brief Process all global objects (call in main loop)
 */
void GlobalObjects_Process(void);

#endif /* __GLOBAL_OBJECTS_H__ */