
// bool
#include <stdbool.h>
// uint8_t, uint16_t, etc.
#include "drivers/encoder.h"
#include <stdint.h>

// 时间参数 (Time parameters)
#define DISPLAY_UPDATE_MS 15     // 显示屏更新间隔，单位毫秒
#define ANIMATION_FRAME_MS 30    // 动画帧间隔，单位毫秒
#define ITEM_SWITCH_ANIM_MS 300  // 选项切换动画持续时间，单位毫秒
#define SLEEP_TIME_MS 15000      // 屏幕息屏时间
#define DEEP_SLEEP_TIME_MS 60000 // 进入深度睡眠时间

// === 弹跳动画配置 ===
#define BOUNCE_ANIM_DURATION_MS 600  // 弹跳动画总持续时间
#define BOUNCE_GRAVITY 120           // 重力加速度 (定点数 * 256)
#define BOUNCE_INITIAL_VELOCITY -900 // 初始向上速度 (定点数 * 256)
#define BOUNCE_DAMPING 90            // 阻尼系数 (百分比)
#define BOUNCE_MIN_VELOCITY 1        // 最小速度阈值 (定点数 * 256)
#define BOUNCE_MAX_BOUNCES 5         // 最大反弹次数

// === 风扇模式切换动画配置 ===
#define FAN_MODE_ANIM_DURATION_MS 600   // 风扇模式切换动画总持续时间
#define FAN_MODE_CURSOR_SPEED 8         // 光标移动速度 (像素/帧) - 已废弃
#define FAN_MODE_CHAR_DELAY_MS 80       // 每个字符切换的延迟时间

// ADC相关
#define ADC_POW 12             // ADC读取精度
#define ADC_READ_INTERVAL 250  // ADC读取间隔
#define FAN_START_TEMP 4500    // 风扇开始旋转的温度 x100
#define FAN_FULL_TEMP 8000     // 风扇满速的温度    x100
#define FAN_UPDATE_INTERVAL 50 // 风扇状态更新间隔
#define TOTAL_CYCLES 20        // 多少次循环为一个周期

// NTC参数定义
#define R0_OHMS 100000L   // 25℃时NTC电阻值 (100K)
#define T0_KELVIN 298150L // 25℃对应的开尔文温度×1000 (298.15K×1000)
#define B_VALUE 3950L     // B值
#define R_PULLUP 100000L  // 上拉电阻值 (100K)
#define ADC_MAX 4095L     // 12位ADC最大值
#define VCC_MV 3300L      // 电源电压 (mV)

// PWM 缓变
#define MAX_PWM 6100      // 最大PWM值
#define PWM_FADE_STEP 256 // PWM 每次缓变最大值
// #define PWM_FADE_INTERVAL_MS 32 // 每隔32ms更新一次PWM值
#define CALC_PWM_INTERVAL_MS 1000 // 每隔50ms计算一次目标PWM值

// 色温参数 (Color temperature parameters)
#define COLOR_TEMP_MIN 3000         // 最低色温 (通道1)
#define COLOR_TEMP_MAX 5700         // 最高色温 (通道2)
#define COLOR_TEMP_DEFAULT 4500     // 默认色温
#define BRIGHTNESS_DEFAULT 100      // 默认亮度
#define LED_TEMP_STEP 10            // 色温调节步进
#define LED_TEMP_WEIGHT_TOTAL 1024L // 调节总权重
#define LED_TEMP_SPRI_TOTAL 550     // 离散度
#define CCT_ADDITIVE_BLEND 255      // 推荐100%的叠加混合
#define LED_MAX_BRIGHTNESS 512      // LED最大亮度值

// 温度查找表定义
#define TEMP_TABLE_SIZE 30

// 硬件引脚定义 (Hardware pin definitions)
#define TEMP_ADC_CHANNEL                                                       \
  ADC_CHANNEL_0 // 温度传感器ADC通道
                // PA0 - ADC1_IN0
#define TEMP_ADC_PORT GPIOA
#define TEMP_ADC_PIN GPIO_PIN_0
#define FAN_EN_PORT GPIOA     // 风扇启用端口
#define FAN_EN_PIN GPIO_PIN_4 // 风扇启用引脚

// 配置保存
#define SAVE_INTERVAL_MS 1000 // 设置保存间隔，单位毫秒

#define colorTempToMired(colorTemp)                                            \
  (1000000L * 10 / colorTemp) // 色温到mired值转换 (避免浮点运算)

#define get_temperature_int(temp_x100) (temp_x100 / 100)
#define get_temperature_frac(temp_x100)                                        \
  ((temp_x100 < 0) ? -temp_x100 : temp_x100 % 100)

#define set_pwm1(value) __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, value)
#define set_pwm2(value) __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, value)

#define open_fan() HAL_GPIO_WritePin(FAN_EN_PORT, FAN_EN_PIN, GPIO_PIN_SET)
#define close_fan() HAL_GPIO_WritePin(FAN_EN_PORT, FAN_EN_PIN, GPIO_PIN_RESET)

// constrain宏定义
#ifndef constrain
#define constrain(amt, low, high)                                              \
  ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))
#endif

static const char *activeStates[] = {
    "    ACTIVE    ", "   .ACTIVE.   ", "  ..ACTIVE..  ", " ...ACTIVE... ",
    "... ACTIVE ...", "..  ACTIVE  ..", ".   ACTIVE   .", "    ACTIVE    "};

void turnOn();
void turnOff();

// 弹跳动画相关函数
void startBounceAnimation();
void updateBounceAnimation();

// 风扇模式切换动画相关函数
void startFanModeAnimation();
void updateFanModeAnimation();
void drawFanModeAnimation();

// 处理按钮单击事件
void handleClick();

// 处理按钮双击事件
void handleDoubleClick();

// 处理按钮长按事件
void handleLongPress();

// 波轮事件
void handleEnc(EncoderDirection_t direction, int32_t steps,
               EncoderSpeed_t speed);

// 绘制选项切换动画
void drawItemSwitchAnimation();

void loop();

void updatePWM();
