/**
 * @file app.cpp
 * @brief Application source file with C++23 support
 * @author User
 * @date 2025-09-06
 */

/* Includes ------------------------------------------------------------------*/
#include "app.h"
#include "stm32_u8g2.h"
#include "u8g2.h"
#include "usart.h"
#include "tim.h"
#include "utils/custom_types.h"
#include "global/global_objects.h"
#include <stdio.h>
#include <cstdlib>

/* Private variables ---------------------------------------------------------*/
static uint32_t loop_counter = 0;
static uint32_t last_tick = 0;

// 按键和波轮测试相关变量
static uint32_t button_press_count = 0;
static uint32_t button_click_count = 0;
static uint32_t button_long_press_count = 0;
static uint32_t button_multi_click_count = 0;
static uint8_t last_multi_click_count = 0;
static int32_t encoder_total_steps = 0;
static int32_t encoder_cw_steps = 0;
static int32_t encoder_ccw_steps = 0;
static bool button_is_pressed = false;
static char last_event_text[64] = "System Ready";
static EncoderSpeed_t current_encoder_speed = ENCODER_SPEED_SLOW;

// U8G2 显示对象
STM32_U8G2_Display u8g2;

/* Private function prototypes -----------------------------------------------*/
static void button_event_handler(ButtonEvent_t event, ButtonState_t state);
static void button_long_press_handler(uint32_t duration_ms);
static void button_multi_click_handler(uint8_t click_count);
static void encoder_event_handler(EncoderEvent_t event, EncoderDirection_t direction, int32_t steps);
static void encoder_rotation_handler(EncoderDirection_t direction, int32_t steps, EncoderSpeed_t speed);
static void encoder_button_handler(EncoderEvent_t event, uint32_t duration_ms);
static void encoder_position_handler(int32_t position, int32_t delta);
static void draw_button_encoder_test(void);

/* Public functions
 * -----------------------------------------------------------*/

/**
 * @brief Application initialization function
 * @note Add your initialization code here
 */
void App_Init(void) {
  // Reset variables
  loop_counter = 0;
  last_tick = HAL_GetTick();

  // 初始化按键和波轮测试变量
  button_press_count = 0;
  button_click_count = 0;
  button_long_press_count = 0;
  button_multi_click_count = 0;
  last_multi_click_count = 0;
  encoder_total_steps = 0;
  encoder_cw_steps = 0;
  encoder_ccw_steps = 0;
  button_is_pressed = false;
  current_encoder_speed = ENCODER_SPEED_SLOW;
  snprintf(last_event_text, sizeof(last_event_text), "System Ready");

  // Print startup message using C++23 features and custom types
  const string_view_t startup_msg =
      make_string_view("=== STM32 Button & Encoder Test Demo (C++23 OOP) ===\r\n");
  const string_view_t success_msg =
      make_string_view("Button & Encoder Test initialized successfully!\r\n\r\n");

  // Using our custom printf implementation
  print_string_view(&startup_msg);
  serial_printf("System Clock: %u Hz\r\n", (unsigned int)HAL_RCC_GetHCLKFreq());
  serial_printf("Tick Frequency: %u Hz\r\n", (unsigned int)HAL_GetTickFreq());
  serial_printf("Button & Encoder Test Mode: ENABLED\r\n");
  print_string_view(&success_msg);

  // 初始化显示器
  u8g2.init();

  // 初始化全局对象
  GlobalObjects_Init();

  // 配置按键回调（面向对象）
  encoder_button.setEventCallback(button_event_handler);
  encoder_button.handleLongPress(button_long_press_handler, 800);  // 800毫秒长按
  encoder_button.handleMultiClick(5, button_multi_click_handler, 400);  // 最多5击，间隔400ms
  
  // 启用按键中断模式
  encoder_button.setInterruptMode(true);
  
  // 配置编码器回调（面向对象）
  rotary_encoder.setEventCallback(encoder_event_handler);
  rotary_encoder.setRotationCallback(encoder_rotation_handler);
  rotary_encoder.setButtonCallback(encoder_button_handler);
  rotary_encoder.setPositionCallback(encoder_position_handler);
  
  // 启用编码器加速功能
  rotary_encoder.setAcceleration(true, 100, 3);  // 100ms阈值，3倍加速
  
  // 暂时禁用中断模式，使用轮询模式测试
  // rotary_encoder.setInterruptMode(true);
  
  // 可选：设置编码器方向反转
  // rotary_encoder.setReversed(true);

  serial_printf("Event callbacks configured successfully\r\n");
  serial_printf("Button Pin: PB14 (Encoder Push) - Polling Mode\r\n");
  serial_printf("Encoder Pin A: PB12 - Polling Mode\r\n");
  serial_printf("Encoder Pin B: PB13 - Polling Mode\r\n");
  serial_printf("Encoder features: Acceleration enabled, Button integrated, Polling Mode\r\n");
  
  // 启动TIM1的PWM输出
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);  // PA8 - TIM1_CH1
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);  // PA9 - TIM1_CH2
  
  // 初始化PWM占空比为0
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
  
  serial_printf("PWM initialized - PA8(TIM1_CH1) and PA9(TIM1_CH2)\r\n");
  serial_printf("PWM Frequency: ~1.1kHz, Resolution: 65536 levels (16-bit)\r\n");
  serial_printf("Encoder scaling: 1 step = 1000 PWM counts\r\n");
}

/**
 * @brief Application main loop function
 * @note Add your main application logic here
 */
void App_Loop(void) {
  const uint32_t current_tick = HAL_GetTick();

  // 处理全局对象（按键和波轮事件）
  GlobalObjects_Process();

  // 根据编码器位置控制PWM输出
  int32_t encoder_position = rotary_encoder.getPosition();
  uint32_t pwm_ch1_value = 0;  // PA8 - TIM1_CH1
  uint32_t pwm_ch2_value = 0;  // PA9 - TIM1_CH2
  
  // PWM缩放因子，将编码器值映射到16位PWM范围
  const uint32_t PWM_MAX_VALUE = 6100;    // 16位最大值 (1<<16 - 1)
  
  if (encoder_position > 0) {
    // 编码器值为正时，PA8输出PWM，PA9为0
    pwm_ch1_value = (uint32_t)encoder_position;
    pwm_ch2_value = 0;
  } else if (encoder_position < 0) {
    // 编码器值为负时，PA9输出PWM，PA8为0
    pwm_ch1_value = 0;
    pwm_ch2_value = (uint32_t)(-encoder_position);  // 取绝对值并缩放
  } else {
    // 编码器值为0时，两个通道都为0
    pwm_ch1_value = 0;
    pwm_ch2_value = 0;
  }
  
  // 限制PWM值不超过16位最大值
  if (pwm_ch1_value > PWM_MAX_VALUE) pwm_ch1_value = PWM_MAX_VALUE;
  if (pwm_ch2_value > PWM_MAX_VALUE) pwm_ch2_value = PWM_MAX_VALUE;
  
  // 设置PWM占空比
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, pwm_ch1_value);
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, pwm_ch2_value);

  // Print status every 1000ms (1 second)
  if (current_tick - last_tick >= 1000) {
    loop_counter++;
    last_tick = current_tick;

    serial_printf("Loop #%u - Tick: %u ms\r\n",
                  (unsigned int)loop_counter, (unsigned int)current_tick);
    serial_printf("  Button Stats - Press: %u, Click: %u, Long: %u\r\n",
                  (unsigned int)button_press_count, 
                  (unsigned int)button_click_count,
                  (unsigned int)button_long_press_count);
    serial_printf("  Encoder Stats - Total: %d, CW: %d, CCW: %d, Pos: %d\r\n",
                  (int)encoder_total_steps, 
                  (int)encoder_cw_steps, 
                  (int)encoder_ccw_steps,
                  (int)rotary_encoder.getPosition());
    
    // 显示PWM输出值和占空比
    uint32_t pwm_ch1 = __HAL_TIM_GET_COMPARE(&htim1, TIM_CHANNEL_1);
    uint32_t pwm_ch2 = __HAL_TIM_GET_COMPARE(&htim1, TIM_CHANNEL_2);
    float duty_ch1 = (float)pwm_ch1 / 65536.0f * 100.0f;
    float duty_ch2 = (float)pwm_ch2 / 65536.0f * 100.0f;
    serial_printf("  PWM Output - PA8(CH1): %u (%.1f%%), PA9(CH2): %u (%.1f%%)\r\n",
                  (unsigned int)pwm_ch1, duty_ch1, (unsigned int)pwm_ch2, duty_ch2);

    // Print additional debug info every 5 loops
    if (loop_counter % 5 == 0) {
      serial_printf("  -> System running for %u seconds\r\n",
                    (unsigned int)(current_tick / 1000));
      serial_printf("  -> Last event: %s\r\n", last_event_text);
      const char* speed_str = (current_encoder_speed == ENCODER_SPEED_FAST) ? "Fast" :
                            (current_encoder_speed == ENCODER_SPEED_MEDIUM) ? "Medium" : "Slow";
      serial_printf("  -> Encoder speed: %s\r\n", speed_str);
    }
  }

  // 渲染显示内容
  draw_button_encoder_test();
}

/**
 * @brief 按键事件回调函数
 */
static void button_event_handler(ButtonEvent_t event, ButtonState_t state) {
  switch (event) {
    case BUTTON_EVENT_PRESS:
      button_press_count++;
      button_is_pressed = true;
      snprintf(last_event_text, sizeof(last_event_text), "Button Pressed");
      serial_printf("Button Event: PRESS (Count: %u)\r\n", (unsigned int)button_press_count);
      break;
      
    case BUTTON_EVENT_RELEASE:
      button_is_pressed = false;
      snprintf(last_event_text, sizeof(last_event_text), "Button Released");
      serial_printf("Button Event: RELEASE\r\n");
      break;
      
    case BUTTON_EVENT_CLICK:
      button_click_count++;
      snprintf(last_event_text, sizeof(last_event_text), "Button Clicked");
      serial_printf("Button Event: CLICK (Count: %u)\r\n", (unsigned int)button_click_count);
      break;
      
    case BUTTON_EVENT_LONG_PRESS:
      button_long_press_count++;
      snprintf(last_event_text, sizeof(last_event_text), "Button Long Press");
      serial_printf("Button Event: LONG_PRESS (Count: %u)\r\n", (unsigned int)button_long_press_count);
      break;
      
    case BUTTON_EVENT_MULTI_CLICK:
      button_multi_click_count++;
      snprintf(last_event_text, sizeof(last_event_text), "Multi-Click (%u)", (unsigned int)last_multi_click_count);
      serial_printf("Button Event: MULTI_CLICK (%u clicks, total: %u)\r\n", 
                    (unsigned int)last_multi_click_count, (unsigned int)button_multi_click_count);
      break;
  }
}

/**
 * @brief 按键长按回调函数
 */
static void button_long_press_handler(uint32_t duration_ms) {
  snprintf(last_event_text, sizeof(last_event_text), "Long Press %ums", (unsigned int)duration_ms);
  serial_printf("Button Long Press Duration: %u ms\r\n", (unsigned int)duration_ms);
}

/**
 * @brief 按键多击回调函数
 */
static void button_multi_click_handler(uint8_t click_count) {
  last_multi_click_count = click_count;
  snprintf(last_event_text, sizeof(last_event_text), "%u-Click Detected", click_count);
  serial_printf("Button Multi-Click: %u clicks detected\r\n", click_count);
}

/**
 * @brief 编码器事件回调函数
 */
static void encoder_event_handler(EncoderEvent_t event, EncoderDirection_t direction, int32_t steps) {
  switch (event) {
    case ENCODER_EVENT_ROTATE_CW:
      encoder_cw_steps += steps;
      encoder_total_steps += steps;
      snprintf(last_event_text, sizeof(last_event_text), "Encoder CW (%d)", (int)steps);
      serial_printf("Encoder Event: ROTATE_CW (Steps: %d, Total: %d, Pos: %d)\r\n", 
                    (int)steps, (int)encoder_total_steps, (int)rotary_encoder.getPosition());
      break;
      
    case ENCODER_EVENT_ROTATE_CCW:
      encoder_ccw_steps += steps;
      encoder_total_steps -= steps;
      snprintf(last_event_text, sizeof(last_event_text), "Encoder CCW (%d)", (int)steps);
      serial_printf("Encoder Event: ROTATE_CCW (Steps: %d, Total: %d, Pos: %d)\r\n", 
                    (int)steps, (int)encoder_total_steps, (int)rotary_encoder.getPosition());
      break;
      
    case ENCODER_EVENT_BUTTON_PRESS:
      snprintf(last_event_text, sizeof(last_event_text), "Encoder Button Press");
      serial_printf("Encoder Button Event: PRESS\r\n");
      break;
      
    case ENCODER_EVENT_BUTTON_RELEASE:
      snprintf(last_event_text, sizeof(last_event_text), "Encoder Button Release");
      serial_printf("Encoder Button Event: RELEASE\r\n");
      break;
      
    case ENCODER_EVENT_BUTTON_CLICK:
      snprintf(last_event_text, sizeof(last_event_text), "Encoder Button Click");
      serial_printf("Encoder Button Event: CLICK\r\n");
      break;
      
    case ENCODER_EVENT_BUTTON_LONG_PRESS:
      snprintf(last_event_text, sizeof(last_event_text), "Encoder Button Long");
      serial_printf("Encoder Button Event: LONG_PRESS\r\n");
      break;
  }
}

/**
 * @brief 编码器旋转专用回调函数
 */
static void encoder_rotation_handler(EncoderDirection_t direction, int32_t steps, EncoderSpeed_t speed) {
  current_encoder_speed = speed;
  const char* speed_str = (speed == ENCODER_SPEED_FAST) ? "Fast" :
                        (speed == ENCODER_SPEED_MEDIUM) ? "Medium" : "Slow";
  const char* dir_str = (direction == ENCODER_DIR_CW) ? "CW" : "CCW";
  
  serial_printf("Encoder Rotation: %s, Steps: %d, Speed: %s\r\n", 
                dir_str, (int)steps, speed_str);
}

/**
 * @brief 编码器按键专用回调函数
 */
static void encoder_button_handler(EncoderEvent_t event, uint32_t duration_ms) {
  switch (event) {
    case ENCODER_EVENT_BUTTON_PRESS:
      serial_printf("Encoder Button: Pressed\r\n");
      break;
    case ENCODER_EVENT_BUTTON_RELEASE:
      serial_printf("Encoder Button: Released (Duration: %u ms)\r\n", (unsigned int)duration_ms);
      break;
    case ENCODER_EVENT_BUTTON_CLICK:
      serial_printf("Encoder Button: Click (Duration: %u ms)\r\n", (unsigned int)duration_ms);
      break;
    case ENCODER_EVENT_BUTTON_LONG_PRESS:
      serial_printf("Encoder Button: Long Press (Duration: %u ms)\r\n", (unsigned int)duration_ms);
      break;
    default:
      break;
  }
}

/**
 * @brief 编码器位置变化回调函数
 */
static void encoder_position_handler(int32_t position, int32_t delta) {
  // 这个回调可用于实时位置监控
  // 为了避免输出过多，这里仅在调试时使用
  // serial_printf("Encoder Position: %d (Delta: %d)\r\n", (int)position, (int)delta);
}

/**
 * @brief 绘制按键和波轮测试界面
 */
static void draw_button_encoder_test(void) {
  static uint32_t last_update = 0;
  uint32_t current_tick = HAL_GetTick();
  
  // 限制更新频率，每50ms更新一次（20 FPS）
  if (current_tick - last_update < 50) {
    return;
  }
  last_update = current_tick;
  
  u8g2.firstPage();
  do {
    // 清空缓冲区
    u8g2.clearBuffer();

    // 标题
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(0, 10, "Button & Encoder Test");

    // 按键状态显示
    u8g2.setFont(u8g2_font_6x10_tr);
    char button_status[32];
    if (button_is_pressed || rotary_encoder.isButtonPressed()) {
      snprintf(button_status, sizeof(button_status), "Button: PRESSED");
    } else {
      snprintf(button_status, sizeof(button_status), "Button: Released");
    }
    u8g2.drawStr(0, 22, button_status);

    // 按键统计
    char button_stats[32];
    snprintf(button_stats, sizeof(button_stats), "P:%u C:%u L:%u M:%u", 
             (unsigned int)button_press_count, 
             (unsigned int)button_click_count,
             (unsigned int)button_long_press_count,
             (unsigned int)button_multi_click_count);
    u8g2.drawStr(0, 32, button_stats);

    // 波轮位置和速度
    char encoder_pos[32];
    const char* speed_str = (current_encoder_speed == ENCODER_SPEED_FAST) ? "F" :
                          (current_encoder_speed == ENCODER_SPEED_MEDIUM) ? "M" : "S";
    snprintf(encoder_pos, sizeof(encoder_pos), "Encoder: %d (%s)", 
             (int)rotary_encoder.getPosition(), speed_str);
    u8g2.drawStr(0, 42, encoder_pos);

    // 波轮统计
    char encoder_stats[32];
    snprintf(encoder_stats, sizeof(encoder_stats), "CW:%d CCW:%d", 
             (int)encoder_cw_steps, (int)encoder_ccw_steps);
    u8g2.drawStr(0, 52, encoder_stats);

    // PWM输出值显示
    char pwm_info[32];
    uint32_t pwm_ch1 = __HAL_TIM_GET_COMPARE(&htim1, TIM_CHANNEL_1);
    uint32_t pwm_ch2 = __HAL_TIM_GET_COMPARE(&htim1, TIM_CHANNEL_2);
    snprintf(pwm_info, sizeof(pwm_info), "PWM A8:%u A9:%u", 
             (unsigned int)pwm_ch1, (unsigned int)pwm_ch2);
    u8g2.drawStr(0, 62, pwm_info);

    // 最后事件（缩短显示）
    u8g2.setFont(u8g2_font_5x7_tr);
    char short_event[20];
    snprintf(short_event, sizeof(short_event), "%.18s", last_event_text);
    u8g2.drawStr(0, 72, short_event);

    // 绘制一个简单的视觉指示器
    // 按键状态指示器
    if (button_is_pressed || rotary_encoder.isButtonPressed()) {
      u8g2.drawBox(110, 20, 8, 8);  // 实心方块表示按下
    } else {
      u8g2.drawFrame(110, 20, 8, 8); // 空心方块表示释放
    }

    // 波轮位置指示器（简单的条形图）
    int32_t pos = rotary_encoder.getPosition();
    int bar_length = abs((int)pos) % 50;
    if (pos >= 0) {
      u8g2.drawBox(110, 40, bar_length / 2, 4);
    } else {
      u8g2.drawBox(110 - bar_length / 2, 40, bar_length / 2, 4);
    }
    
    // 速度指示器
    uint8_t speed_bars = (uint8_t)current_encoder_speed;
    for (uint8_t i = 0; i < speed_bars; i++) {
      u8g2.drawBox(110 + i * 3, 50, 2, 6);
    }
    
  } while (u8g2.nextPage());
}

/**
 * @brief 重置按键和波轮统计
 */
void App_ResetStats(void) {
  button_press_count = 0;
  button_click_count = 0;
  button_long_press_count = 0;
  button_multi_click_count = 0;
  last_multi_click_count = 0;
  encoder_total_steps = 0;
  encoder_cw_steps = 0;
  encoder_ccw_steps = 0;
  rotary_encoder.resetPosition();
  snprintf(last_event_text, sizeof(last_event_text), "Stats Reset");
  serial_printf("Button & Encoder statistics reset\r\n");
}
