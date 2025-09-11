/**
 * @file global_objects.cpp
 * @brief Global Objects Implementation
 * @author User
 * @date 2025-09-06
 */

/* Includes ------------------------------------------------------------------*/
#include "global_objects.h"
#include "global/controller.h"
#include "stm32_u8g2.h"
#include <sys/_types.h>

/* Global Objects ------------------------------------------------------------*/

SystemState state = {
    // 核心控制参数
    false, true, false, false, BRIGHTNESS_DEFAULT, COLOR_TEMP_DEFAULT,
    // PWM输出值
    0, 0, 0, 0,
    // 用户界面状态
    1, 0,
    // 动画状态
    0, 0, 0, 0,
    // OUT/OFF弹跳动画状态
    0, 0, 0, 0, 0,
    // 风扇模式切换动画状态
    0, 0, 0, 0, 0, 0,
    // 传感器数据
}; // 初始化所有动画状态为0，温度为无效值
SystemState lastState = {
    // 核心控制参数
    true, false, true, true, LED_MAX_BRIGHTNESS, 255,
    // PWM输出值
    32767, 32767, 0, 0,
    // 用户界面状态
    127, 127,
    // 动画状态
    127, 0, 0, 0,
    // OUT/OFF弹跳动画状态
    0, 0, 0, 0, 0,
    // 风扇模式切换动画状态
    0, 0, 0, 0, 0, 0,
    // 传感器数据
}; // 初始化为不同值，确保首次更新
//
//
uint16_t adc_value = 0;          // ADC采样值
unsigned char adc_done_flag = 0; // ADC转换完成标志

// U8G2 显示对象
STM32_U8G2_Display u8g2;

// Global button instance (encoder button)
Button encoder_button(GPIOB, GPIO_PIN_14, true);

// Global rotary encoder instance
// Pin configuration: PA (PB12), PB (PB13), Button (PB14)
RotaryEncoder rotary_encoder(GPIOB, GPIO_PIN_12, // Pin A
                             GPIOB, GPIO_PIN_13  // Pin B
);

// PA5
Button btn_1(GPIOA, GPIO_PIN_5, true);

// PA6
Button btn_2(GPIOA, GPIO_PIN_6, true);

/* Function implementations --------------------------------------------------*/

/**
 * @brief Initialize all global objects
 */
void GlobalObjects_Init(void) {
  // Initialize encoder
  rotary_encoder.init();

  // Initialize button
  encoder_button.init();

  // Initialize PA5 button
  btn_1.init();

  // Initialize PA6 button
  btn_2.init();

  // Temporarily disable interrupt mode and use polling mode
  // Enable interrupt mode for all buttons and encoder
  encoder_button.setInterruptMode(true); // Use polling mode for now
  btn_1.setInterruptMode(true);          // Use polling mode for now
  btn_2.setInterruptMode(true);          // Use polling mode for now
  rotary_encoder.setInterruptMode(true); // Use polling mode for now
}

/**
 * @brief Process all global objects
 */
void GlobalObjects_Process(void) {
  // Process encoder
  rotary_encoder.process();

  // Process button
  encoder_button.process();

  // Process PA5 button
  btn_1.process();

  // Process PA6 button
  btn_2.process();
}