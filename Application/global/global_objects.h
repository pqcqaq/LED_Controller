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

#define TITLE_TEXT "LED Controller" ///< Boot text to display
#define AUTHOR_TEXT "by QCQCQC"     ///< Author text to display

// 系统状态结构体 (System state struct)
typedef struct {
  // === 核心控制参数 ===
  bool master;         // 主开关状态
  bool fanAuto;        // 风扇自动控制
  bool isSleeping;     // 屏幕是否息屏
  bool deepSleep;      // 进入深度睡眠
  uint16_t brightness; // 亮度值 (0-MAX%)
  uint16_t colorTemp;  // 色温值 (3000-5700K)

  // === PWM输出值 ===
  uint16_t currentCh1PWM; // 通道1的当前PWM值 (0-6100)
  uint16_t currentCh2PWM; // 通道2的当前PWM值 (0-6100)

  uint16_t targetCh1PWM; // 通道1的目标PWM值 (0-6100)
  uint16_t targetCh2PWM; // 通道2的目标PWM值 (0-6100)

  // === 用户界面状态 ===
  uint8_t item; // 当前选中的项目 (0=主开关, 1=色温, 2=亮度)
  int8_t edit;  // 编辑模式 (-1=导航模式, 0=编辑值)

  // === 动画状态 ===
  uint8_t animStarted;    // 动画是否已开始
  uint8_t animProgress;   // 动画进度 (0-100)
  uint8_t animDirection;  // 动画方向 (0=左到右, 1=右到左)
  uint32_t animStartTime; // 动画开始时间

  // === OUT/OFF弹跳动画状态 ===
  uint8_t bounceAnimActive;  // 弹跳动画是否激活
  uint32_t bounceStartTime;  // 弹跳动画开始时间
  int16_t bounceY;           // 当前Y偏移量 (定点数 * 256)
  int16_t bounceVelocityY;   // Y方向速度 (定点数 * 256)
  uint8_t bounceCount;       // 反弹次数计数

  // === 风扇模式切换动画状态 ===
  uint8_t fanAnimActive;     // 风扇模式切换动画是否激活
  uint32_t fanAnimStartTime; // 风扇模式切换动画开始时间
  uint8_t fanAnimCursorPos;  // 光标当前位置
  uint8_t fanAnimCharIndex;  // 当前切换的字符索引

  // === 传感器数据 ===
  int32_t temp; // ADC读取到的温度
} SystemState;

extern SystemState state;
extern SystemState lastState;


/* Global Objects ------------------------------------------------------------*/

extern uint16_t adc_value;          // ADC采样值
extern unsigned char adc_done_flag; // ADC转换完成标志

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