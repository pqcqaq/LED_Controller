
/**
 * @file delay.c
 * @brief Delay utilities implementation
 * @author User
 * @date 2025-09-06
 */

/* Includes ------------------------------------------------------------------*/
#include "delay.h"

/* Public functions ----------------------------------------------------------*/

/**
 * @brief 微秒级延时函数
 * @param us 延时微秒数
 * @note 基于TIM2定时器实现，TIM2运行在1MHz (预分频器=71)
 */
void Tims_delay_us(uint32_t us) {
  // TIM2现在运行在1MHz，即每微秒1个计数
  // 预分频器=71，所以 72MHz/72 = 1MHz

  uint32_t start = __HAL_TIM_GET_COUNTER(&htim2);

  // 对于小于65535us的延时（避免16位定时器溢出）
  if (us < 65535) {
    uint32_t target = start + us;

    // 处理计数器溢出
    if (target > 65535) {
      // 等待溢出
      while (__HAL_TIM_GET_COUNTER(&htim2) >= start)
        ;
      target = target - 65536;
    }

    while (__HAL_TIM_GET_COUNTER(&htim2) < target)
      ;
  } else {
    // 对于较长的延时，使用HAL_Delay
    HAL_Delay(us / 1000);
    if (us % 1000 != 0) {
      Tims_delay_us(us % 1000);
    }
  }
}
