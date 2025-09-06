#include "stm32_u8g2.h"
#include "i2c.h"
#include "utils/delay.h"

#include <U8g2lib.h>
#include <string.h>

uint8_t u8x8_byte_hw_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int,
                         void *arg_ptr) {
  static uint8_t buffer[32]; // 优化：缩小到32字节
  static uint8_t buf_idx;
  uint8_t *data;

  switch (msg) {
  case U8X8_MSG_BYTE_INIT:
    // I2C初始化（如果需要的话）
    break;

  case U8X8_MSG_BYTE_START_TRANSFER:
    buf_idx = 0;
    break;

  case U8X8_MSG_BYTE_SEND:
    data = (uint8_t *)arg_ptr;

    // 边界检查，防止缓冲区溢出
    if (buf_idx + arg_int > sizeof(buffer)) {
      return 0;
    }

    // 优化：使用memcpy替代循环
    memcpy(&buffer[buf_idx], data, arg_int);
    buf_idx += arg_int;
    break;

  case U8X8_MSG_BYTE_END_TRANSFER:
    // 优化：减少超时时间到100ms
    if (HAL_I2C_Master_Transmit(&hi2c1, OLED_ADDRESS, buffer, buf_idx, 100) !=
        HAL_OK) {
      return 0;
    }
    break;

  case U8X8_MSG_BYTE_SET_DC:
    break;

  default:
    return 0;
  }

  return 1;
}

uint8_t u8x8_gpio_and_delay(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int,
                            void *arg_ptr) {
  switch (msg) {
  case U8X8_MSG_DELAY_100NANO:
    // 100纳秒延时
    __NOP();
    break;

  case U8X8_MSG_DELAY_10MICRO:
    // 10微秒延时，使用arg_int参数
    Tims_delay_us(10);
    break;

  case U8X8_MSG_DELAY_MILLI:
    // 毫秒延时，使用arg_int参数
    HAL_Delay(arg_int);
    break;

  case U8X8_MSG_DELAY_I2C:
    // I2C延时，根据速度调整
    Tims_delay_us(1); // 更高速度 -> 1us
    break;

  case U8X8_MSG_GPIO_I2C_CLOCK:
  case U8X8_MSG_GPIO_I2C_DATA:
    // I2C硬件模式不需要GPIO控制
    break;

  case U8X8_MSG_GPIO_MENU_SELECT:
  case U8X8_MSG_GPIO_MENU_NEXT:
  case U8X8_MSG_GPIO_MENU_PREV:
  case U8X8_MSG_GPIO_MENU_HOME:
    u8x8_SetGPIOResult(u8x8, 0);
    break;

  default:
    u8x8_SetGPIOResult(u8x8, 1);
    break;
  }
  return 1;
}

// U8g2的初始化，需要调用下面这个u8g2_Setup_ssd1306_128x64_noname_f函数，该函数的4个参数含义：
// u8g2：传入的U8g2结构体
// U8G2_R0：默认使用U8G2_R0即可（用于配置屏幕是否要旋转）
// u8x8_byte_sw_i2c：使用软件IIC驱动，该函数由U8g2源码提供
// u8x8_gpio_and_delay：就是上面我们写的配置函数

void u8g2Init(u8g2_t *u8g2) {
  u8g2_Setup_ssd1306_i2c_128x64_noname_f(
      u8g2, U8G2_R0, u8x8_byte_hw_i2c,
      u8x8_gpio_and_delay);   // 初始化u8g2 结构体
  u8g2_InitDisplay(u8g2);     //
  u8g2_SetPowerSave(u8g2, 0); //
  u8g2_ClearBuffer(u8g2);
}