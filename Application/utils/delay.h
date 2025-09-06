/**
 * @file delay.h
 * @brief Delay utilities header file
 * @author User
 * @date 2025-09-06
 */

#ifndef __DELAY_H__
#define __DELAY_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "tim.h"

/* Function prototypes -------------------------------------------------------*/

/**
 * @brief 微秒级延时函数
 * @param us 延时微秒数
 * @note 基于TIM1定时器实现，TIM1运行在1MHz (预分频器=71)
 */
void Tims_delay_us(uint32_t us);

#ifdef __cplusplus
}
#endif

#endif /* __DELAY_H__ */
