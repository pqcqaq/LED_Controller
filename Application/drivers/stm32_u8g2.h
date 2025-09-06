#ifndef __STM32_U8G2_H
#define __STM32_U8G2_H

#ifdef __cplusplus
extern "C" {
#endif

/* USER CODE BEGIN Includes */
#include "main.h"
#include "u8g2.h"
#include <U8g2lib.h>
/* USER CODE END Includes */

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */
#define u8 unsigned char  // ?unsigned char ????
#define MAX_LEN 128       //
#define OLED_ADDRESS 0x78 // oled
#define OLED_CMD 0x00     //
#define OLED_DATA 0x40    //

/* USER CODE BEGIN Prototypes */
uint8_t u8x8_byte_hw_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int,
                         void *arg_ptr);
uint8_t u8x8_gpio_and_delay(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int,
                            void *arg_ptr);
void u8g2Init(u8g2_t *u8g2);
void draw(u8g2_t *u8g2);
void testDrawPixelToFillScreen(u8g2_t *u8g2);
void Tims_delay_us(uint32_t us);
#ifdef __cplusplus
}
#endif

class STM32_U8G2_Display : public U8G2 {
public:
  STM32_U8G2_Display() : U8G2() {
    // 构造函数调用 u8g2_Setup 函数来初始化显示
    u8g2_Setup_ssd1306_i2c_128x64_noname_f(&u8g2, U8G2_R0, u8x8_byte_hw_i2c, u8x8_gpio_and_delay);
  }
  
  void init() {
    u8g2_InitDisplay(&u8g2);     // 初始化显示
    u8g2_SetPowerSave(&u8g2, 0); // 关闭节能模式
    u8g2_ClearBuffer(&u8g2);     // 清空缓冲区
  }
};


#endif /* __STM32_U8G2_H */