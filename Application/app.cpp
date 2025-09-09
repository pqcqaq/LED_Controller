/**
 * @file app.cpp
 * @brief Application source file with C++23 support
 * @author User
 * @date 2025-09-06
 */

/* Includes ------------------------------------------------------------------*/
#include "app.h"
#include "animations/boot_animation.h"
#include "drivers/iwdg_a.h"
#include "global/controller.h"
#include "global/global_objects.h"
#include "hardware/devices.h"
#include "stm32_u8g2.h"
#include "tim.h"
#include "u8g2.h"
#include "usart.h"
#include "utils/custom_types.h"
#include <adc.h>
#include <cstdlib>
#include <i2c.h>
#include <stdio.h>

/* Forward declarations ------------------------------------------------------*/
extern void calculateChannelRatio(uint16_t colorTemp, uint16_t brightness,
                                  uint16_t *ch1PWM, uint16_t *ch2PWM);

/* Private variables ---------------------------------------------------------*/
static uint32_t loop_counter = 0;
static uint32_t last_tick = 0;

/* Private function prototypes -----------------------------------------------*/
static void button_event_handler(ButtonEvent_t event, ButtonState_t state);
static void button_click_handler();
static void button_long_press_handler(uint32_t duration_ms);
static void button_multi_click_handler(uint8_t click_count);
// static void encoder_event_handler(EncoderEvent_t event,
//                                   EncoderDirection_t direction, int32_t
//                                   steps);
static void encoder_rotation_handler(EncoderDirection_t direction,
                                     int32_t steps, EncoderSpeed_t speed);
// static void encoder_position_handler(int32_t position, int32_t delta);

/* Public functions
 * -----------------------------------------------------------*/

static const uint8_t u8g2_font_icon_5_t_bits[] = {
    0b10001, 0b01010, 0b00100, 0b01010, 0b10001,
};

// void I2CScan() {
//   uint8_t i = 0;
//   HAL_StatusTypeDef status;
//   serial_printf("I2C Scan Start:\r\n");
//   IWDG_Refresh();
//   for (i = 0; i < 127; i++) {
//     status = HAL_I2C_Master_Transmit(&hi2c2, i << 1, 0, 0, 200);
//     if (status == HAL_OK) {
//       serial_printf("i2c addr:0x%02X is ok\r\n", i << 1);
//     } else if (status == HAL_TIMEOUT) {
//       serial_printf("i2c addr:0x%02X is timeout\r\n", i << 1);
//     } else if (status == HAL_BUSY) {
//       serial_printf("i2c addr:0x%02X is busy\r\n", i << 1);
//     }
//     IWDG_Refresh();
//   }
//   serial_printf("I2C Scan End.\r\n");
// }

void Scan_I2C_Devices(void) {
  // I2CScan();
  HAL_StatusTypeDef status;
  bool oled_ok, adc_ok, eeprom_ok;
  const uint8_t OLED_ADDR = 0x78;
  const uint8_t ADC_ADDR = 0x43 << 1;
  const uint8_t EEPROM_ADDR = 0xA0;

  serial_printf("Scanning I2C devices...\r\n");

  // 阶段1: 动态扫描动画
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 10, "Scanning I2C...");

  // 绘制3个圆形代表3个设备，开始时为空心
  u8g2.drawCircle(20, 30, 5, U8G2_DRAW_ALL);
  u8g2.drawCircle(60, 30, 5, U8G2_DRAW_ALL);
  u8g2.drawCircle(100, 30, 5, U8G2_DRAW_ALL);

  u8g2.sendBuffer();

  // 扫描屏幕
  u8g2.drawStr(0, 50, "Checking OLED...");
  u8g2.sendBuffer();
  status = HAL_I2C_IsDeviceReady(&hi2c1, OLED_ADDR, 2, 50);
  oled_ok = (status == HAL_OK);
  if (oled_ok) {
    u8g2.drawDisc(20, 30, 5, U8G2_DRAW_ALL);
    serial_printf("OLED 0x%02X: OK\r\n", OLED_ADDR);
    devices.oled = true;
  } else {
    u8g2.drawXBMP(18, 28, 5, 5, u8g2_font_icon_5_t_bits); // 使用自定义叉号图标
    serial_printf("OLED 0x%02X: NO\r\n", OLED_ADDR);
  }
  u8g2.sendBuffer();
  HAL_Delay(100);
  IWDG_Refresh();

  // 扫描电压电流采样
  u8g2.drawStr(0, 50, "Checking ADC...");
  u8g2.sendBuffer();
  status = HAL_I2C_IsDeviceReady(&hi2c1, ADC_ADDR, 2, 50);
  adc_ok = (status == HAL_OK);
  if (adc_ok) {
    u8g2.drawDisc(60, 30, 5, U8G2_DRAW_ALL);
    serial_printf("ADC 0x%02X: OK\r\n", ADC_ADDR);
    devices.extern_adc = true;
  } else {
    u8g2.drawXBMP(58, 28, 5, 5, u8g2_font_icon_5_t_bits);
    serial_printf("ADC 0x%02X: NO\r\n", ADC_ADDR);
  }
  u8g2.sendBuffer();
  HAL_Delay(100);
  IWDG_Refresh();

  // 扫描EEPROM
  u8g2.drawStr(0, 50, "Checking EEPROM...");
  u8g2.sendBuffer();
  status = HAL_I2C_IsDeviceReady(&hi2c2, EEPROM_ADDR, 2, 50);
  eeprom_ok = (status == HAL_OK);
  if (eeprom_ok) {
    u8g2.drawDisc(100, 30, 5, U8G2_DRAW_ALL);
    serial_printf("EEPROM 0x%02X: OK\r\n", EEPROM_ADDR);
    devices.eeprom = true;
  } else {
    u8g2.drawXBMP(98, 28, 5, 5, u8g2_font_icon_5_t_bits);
    serial_printf("EEPROM 0x%02X: NO\r\n", EEPROM_ADDR);

    // 显示一个settings will not be saved的提示
    u8g2.drawStr(0, 58, "Settings will");
    u8g2.drawStr(0, 64, "not be saved!");
  }
  u8g2.sendBuffer();
  HAL_Delay(800);
  IWDG_Refresh();
}

/**
 * @brief Application initialization function
 * @note Add your initialization code here
 */
void App_Init(void) {
  // Reset variables
  loop_counter = 0;
  last_tick = HAL_GetTick();

  // 初始化显示器
  u8g2.init();

  // 初始化全局对象
  GlobalObjects_Init();

  // 启动ADC校准
  HAL_ADCEx_Calibration_Start(&hadc1);

  // 初始化开机动画系统
  if (BootAnimation_Init(&u8g2)) {

    // 启动开机动画
    if (BootAnimation_Start()) {

      // 播放开机动画直到完成
      while (!BootAnimation_IsComplete()) {
        // 更新动画状态
        BootAnimation_Update();

        // 渲染当前帧
        BootAnimation_Render();

        // 小延迟以控制帧率 (~60 FPS)
        HAL_Delay(16);

        // 可选：添加看门狗喂狗或其他必要的系统维护
        IWDG_Refresh(); // 如果使用了看门狗
      }

      serial_printf("Boot animation completed\r\n");

    } else {
      serial_printf("Failed to start boot animation, skipping...\r\n");
    }

  } else {
    serial_printf("Failed to initialize boot animation, skipping...\r\n");
  }

  // 动画完成后清空显示，准备进入正常模式
  u8g2.clearBuffer();

  // 扫描外设，在串口打印一下
  Scan_I2C_Devices();

  Init_Devices();

  // 配置按键回调（面向对象）
  encoder_button.setEventCallback(button_event_handler);
  encoder_button.handleClick(button_click_handler); // 单击
  encoder_button.handleLongPress(button_long_press_handler,
                                 800); // 800毫秒长按
  encoder_button.handleMultiClick(5, button_multi_click_handler,
                                  400); // 最多5击，间隔400ms

  // 启用按键中断模式
  encoder_button.setInterruptMode(true);

  // 配置编码器回调（面向对象）
  // rotary_encoder.setEventCallback(encoder_event_handler);
  rotary_encoder.setRotationCallback(encoder_rotation_handler);
  // rotary_encoder.setPositionCallback(encoder_position_handler);

  // 启用编码器加速功能
  rotary_encoder.setAcceleration(true, 50, 3); // 200ms阈值，3倍加速

  // 暂时禁用中断模式，使用轮询模式测试
  // rotary_encoder.setInterruptMode(true);

  // 可选：设置编码器方向反转
  // rotary_encoder.setReversed(true);

  // 启动TIM1的PWM输出
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1); // PA8 - TIM1_CH1
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2); // PA9 - TIM1_CH2

  // 初始化PWM占空比为0
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
}

/**
 * @brief Application main loop function
 * @note Add your main application logic here
 */
void App_Loop(void) {

  GlobalObjects_Process();

  loop();

  // const uint32_t current_tick = HAL_GetTick();

  // // 处理全局对象（按键和波轮事件）

  // 根据编码器位置控制PWM输出
  // int32_t encoder_position = rotary_encoder.getPosition();
  // uint32_t pwm_ch1_value = 0; // PA8 - TIM1_CH1
  // uint32_t pwm_ch2_value = 0; // PA9 - TIM1_CH2

  // // PWM缩放因子，将编码器值映射到16位PWM范围
  // const uint32_t PWM_MAX_VALUE = 6100; // 16位最大值 (1<<16 - 1)

  // if (encoder_position > 0) {
  //   // 编码器值为正时，PA8输出PWM，PA9为0
  //   pwm_ch1_value = (uint32_t)encoder_position;
  //   pwm_ch2_value = 0;
  // } else if (encoder_position < 0) {
  //   // 编码器值为负时，PA9输出PWM，PA8为0
  //   pwm_ch1_value = 0;
  //   pwm_ch2_value = (uint32_t)(-encoder_position); // 取绝对值并缩放
  // } else {
  //   // 编码器值为0时，两个通道都为0
  //   pwm_ch1_value = 0;
  //   pwm_ch2_value = 0;
  // }

  // // 限制PWM值不超过16位最大值
  // if (pwm_ch1_value > PWM_MAX_VALUE)
  //   pwm_ch1_value = PWM_MAX_VALUE;
  // if (pwm_ch2_value > PWM_MAX_VALUE)
  //   pwm_ch2_value = PWM_MAX_VALUE;

  // // 设置PWM占空比
  // __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, pwm_ch1_value);
  // __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, pwm_ch2_value);

  // // Print status every 1000ms (1 second)
  // if (current_tick - last_tick >= 1000) {
  //   loop_counter++;
  //   last_tick = current_tick;

  //   serial_printf("Loop #%u - Tick: %u ms\r\n", (unsigned int)loop_counter,
  //                 (unsigned int)current_tick);
  //   serial_printf("  Button Stats - Press: %u, Click: %u, Long: %u\r\n",
  //                 (unsigned int)button_press_count,
  //                 (unsigned int)button_click_count,
  //                 (unsigned int)button_long_press_count);
  //   serial_printf("  Encoder Stats - Total: %d, CW: %d, CCW: %d, Pos:
  //   %d\r\n",
  //                 (int)encoder_total_steps, (int)encoder_cw_steps,
  //                 (int)encoder_ccw_steps,
  //                 (int)rotary_encoder.getPosition());

  //   // 显示PWM输出值和占空比
  //   uint32_t pwm_ch1 = __HAL_TIM_GET_COMPARE(&htim1, TIM_CHANNEL_1);
  //   uint32_t pwm_ch2 = __HAL_TIM_GET_COMPARE(&htim1, TIM_CHANNEL_2);
  //   float duty_ch1 = (float)pwm_ch1 / 65536.0f * 100.0f;
  //   float duty_ch2 = (float)pwm_ch2 / 65536.0f * 100.0f;
  //   serial_printf(
  //       "  PWM Output - PA8(CH1): %u (%.1f%%), PA9(CH2): %u (%.1f%%)\r\n",
  //       (unsigned int)pwm_ch1, duty_ch1, (unsigned int)pwm_ch2, duty_ch2);

  //   // Print additional debug info every 5 loops
  //   if (loop_counter % 5 == 0) {
  //     serial_printf("  -> System running for %u seconds\r\n",
  //                   (unsigned int)(current_tick / 1000));
  //     serial_printf("  -> Last event: %s\r\n", last_event_text);
  //     const char *speed_str =
  //         (current_encoder_speed == ENCODER_SPEED_FAST)     ? "Fast"
  //         : (current_encoder_speed == ENCODER_SPEED_MEDIUM) ? "Medium"
  //                                                           : "Slow";
  //     serial_printf("  -> Encoder speed: %s\r\n", speed_str);
  //   }
  // }

  // // 渲染显示内容
  // draw_button_encoder_test();

}

/**
 * @brief 按键事件回调函数
 */
static void button_event_handler(ButtonEvent_t event, ButtonState_t state) {
  switch (event) {
  case BUTTON_EVENT_PRESS:
    // 按键按下事件，无需特殊处理
    break;

  case BUTTON_EVENT_RELEASE:
    // 按键释放事件，无需特殊处理
    break;

  case BUTTON_EVENT_CLICK:
    // 按键单击事件，无需特殊处理
    break;

  case BUTTON_EVENT_LONG_PRESS:
    // 按键长按事件，无需特殊处理
    break;

  case BUTTON_EVENT_MULTI_CLICK:
    // 按键多击事件，无需特殊处理
    break;
  }
}

/**
 * @brief 按键单击回调函数
 */
static void button_click_handler() { handleClick(); }

/**
 * @brief 按键长按回调函数
 */
static void button_long_press_handler(uint32_t duration_ms) {
  // snprintf(last_event_text, sizeof(last_event_text), "Long Press %ums",
  //          (unsigned int)duration_ms);
  serial_printf("Button Long Press Duration: %u ms\r\n",
                (unsigned int)duration_ms);
  handleLongPress();
}

/**
 * @brief 按键多击回调函数
 */
static void button_multi_click_handler(uint8_t click_count) {
  // last_multi_click_count = click_count;
  // snprintf(last_event_text, sizeof(last_event_text), "%u-Click Detected",
  //          click_count);
  // serial_printf("Button Multi-Click: %u clicks detected\r\n", click_count);
  if (click_count == 2) {
    handleDoubleClick();
  }
}

// /**
//  * @brief 编码器事件回调函数
//  */
// static void encoder_event_handler(EncoderEvent_t event,
//                                   EncoderDirection_t direction, int32_t
//                                   steps) {
//   switch (event) {
//   case ENCODER_EVENT_ROTATE_CW:
//     // encoder_cw_steps += steps;
//     // encoder_total_steps += steps;
//     // snprintf(last_event_text, sizeof(last_event_text), "Encoder CW
//     (%d)",
//     //          (int)steps);
//     // serial_printf(
//     //     "Encoder Event: ROTATE_CW (Steps: %d, Total: %d, Pos: %d)\r\n",
//     //     (int)steps, (int)encoder_total_steps,
//     //     (int)rotary_encoder.getPosition());
//     break;

//   case ENCODER_EVENT_ROTATE_CCW:
//     // encoder_ccw_steps += steps;
//     // encoder_total_steps -= steps;
//     // snprintf(last_event_text, sizeof(last_event_text), "Encoder CCW
//     (%d)",
//     //          (int)steps);
//     // serial_printf(
//     //     "Encoder Event: ROTATE_CCW (Steps: %d, Total: %d, Pos: %d)\r\n",
//     //     (int)steps, (int)encoder_total_steps,
//     //     (int)rotary_encoder.getPosition());
//     break;
//   }
// }

/**
 * @brief 编码器旋转专用回调函数
 */
static void encoder_rotation_handler(EncoderDirection_t direction,
                                     int32_t steps, EncoderSpeed_t speed) {
  // 调用控制器处理编码器事件
  handleEnc(direction, steps, speed);

  // 不在每次旋转时保存设置，而是在编辑结束时保存
}

// /**
//  * @brief 编码器按键专用回调函数
//  */
// /**
//  * @brief 编码器位置变化回调函数
//  */
// static void encoder_position_handler(int32_t position, int32_t delta) {
//   // 这个回调可用于实时位置监控
//   // 为了避免输出过多，这里仅在调试时使用
//   // serial_printf("Encoder Position: %d (Delta: %d)\r\n", (int)position,
//   // (int)delta);
// }

void App_TIM3_IRQHandler(void) {
  // 每次中断发生时，计数器加1
  updatePWM();
}