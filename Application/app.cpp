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
#include "utils/custom_types.h"
#include <stdio.h>

/* Private variables ---------------------------------------------------------*/
static uint32_t loop_counter = 0;
static uint32_t last_tick = 0;

// FPS测试相关变量
static uint32_t frame_counter = 0;
static uint32_t fps_last_tick = 0;
static uint32_t current_fps_x1000 = 0;      // FPS值乘以1000存储为整数
static uint32_t fps_update_interval = 1000; // 每1000ms更新一次FPS显示
static bool fps_test_mode = true;

/* Private function prototypes -----------------------------------------------*/
static void show_startup_animation(void);
static void fps_test_draw(void);
static void update_fps_counter(void);
static void draw_fps_demo_content(void);

STM32_U8G2_Display u8g2;

/* Public functions
 * -----------------------------------------------------------*/

/**
 * @brief Application initialization function
 * @note Add your initialization code here
 */
extern "C" void App_Init(void) {
  // Reset variables
  loop_counter = 0;
  last_tick = HAL_GetTick();

  // 初始化FPS测试变量
  frame_counter = 0;
  fps_last_tick = HAL_GetTick();
  current_fps_x1000 = 0;
  fps_test_mode = true; // 启用FPS测试模式

  // Print startup message using C++23 features and custom types
  const string_view_t startup_msg =
      make_string_view("=== STM32 FPS Test Demo (C++23) ===\r\n");
  const string_view_t success_msg =
      make_string_view("FPS Test initialized successfully!\r\n\r\n");

  // Using our custom printf implementation
  print_string_view(&startup_msg);
  serial_printf("System Clock: %u Hz\r\n", (unsigned int)HAL_RCC_GetHCLKFreq());
  serial_printf("Tick Frequency: %u Hz\r\n", (unsigned int)HAL_GetTickFreq());
  serial_printf("FPS Test Mode: ENABLED\r\n");
  print_string_view(&success_msg);

  u8g2.init();

  // 开屏动画
  show_startup_animation();
}

/**
 * @brief Application main loop function
 * @note Add your main application logic here
 */
extern "C" void App_Loop(void) {
  const uint32_t current_tick = HAL_GetTick();

  // 更新FPS计数器
  update_fps_counter();

  // Print status every 1000ms (1 second)
  if (current_tick - last_tick >= 1000) {
    loop_counter++;
    last_tick = current_tick;

    serial_printf("Loop #%u - Tick: %u ms - FPS: %u.%u\r\n",
                  (unsigned int)loop_counter, (unsigned int)current_tick,
                  (unsigned int)(current_fps_x1000 / 1000),
                  (unsigned int)(current_fps_x1000 % 1000));

    // Print additional debug info every 5 loops
    if (loop_counter % 5 == 0) {
      serial_printf("  -> System running for %u seconds\r\n",
                    (unsigned int)(current_tick / 1000));
      serial_printf("  -> Frame counter: %u\r\n", (unsigned int)frame_counter);
    }
  }

  // 渲染显示内容
  if (fps_test_mode) {
    fps_test_draw();
  }
}

/**
 * @brief 显示开屏动画
 * @param u8g2 u8g2显示对象指针
 */
void show_startup_animation() {
  // 清屏
  u8g2.clearBuffer();

  // 第一帧：显示欢迎信息
  u8g2.setFont(u8g2_font_ncenB14_tr);
  u8g2.drawStr(10, 20, "STM32 System");
  u8g2.setFont(u8g2_font_ncenB10_tr);
  u8g2.drawStr(25, 35, "Starting...");
  u8g2.sendBuffer();
  HAL_Delay(1000);

  // 第二帧：绘制加载条动画
  for (int i = 0; i <= 100; i += 10) {
    u8g2.clearBuffer();

    // 标题
    u8g2.setFont(u8g2_font_ncenB12_tr);
    u8g2.drawStr(20, 15, "Initializing");

    // 加载条背景
    u8g2.drawFrame(10, 25, 108, 10);

    // 加载条进度
    if (i > 0) {
      u8g2.drawBox(12, 27, (i * 104) / 100, 6);
    }

    // 百分比
    char progress_text[8];
    snprintf(progress_text, sizeof(progress_text), "%d%%", i);
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(55, 50, progress_text);

    u8g2.sendBuffer();
    HAL_Delay(200);
  }

  // 第三帧：显示完成信息
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB14_tr);
  u8g2.drawStr(35, 25, "Ready!");

  // 绘制一个简单的图标
  u8g2.drawCircle(64, 45, 8, U8G2_DRAW_ALL);
  u8g2.drawStr(60, 50, "OK");

  u8g2.sendBuffer();
  HAL_Delay(1500);

  // 清屏，准备进入主循环
  u8g2.clearBuffer();
  u8g2.sendBuffer();
}

/**
 * @brief 更新FPS计数器
 */
static void update_fps_counter(void) {
  frame_counter++;

  uint32_t current_tick = HAL_GetTick();
  uint32_t elapsed = current_tick - fps_last_tick;

  // 每隔指定时间间隔更新FPS计算
  if (elapsed >= fps_update_interval) {
    // 使用整数运算：FPS = frame_counter * 1000000 / elapsed (结果乘以1000)
    current_fps_x1000 = (frame_counter * 1000000) / elapsed;
    frame_counter = 0;
    fps_last_tick = current_tick;
  }
}

/**
 * @brief FPS测试专用绘制函数
 * @param u8g2 u8g2显示对象指针
 */
static void fps_test_draw() {
  u8g2.firstPage();
  do {
    draw_fps_demo_content();
  } while (u8g2.nextPage());
}

/**
 * @brief 绘制FPS测试演示内容
 * @param u8g2 u8g2显示对象指针
 */
static void draw_fps_demo_content() {
  static uint32_t animation_counter = 0;
  uint32_t current_tick = HAL_GetTick();

  // 清空缓冲区
  u8g2.clearBuffer();

  // 标题
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(0, 10, "STM32 FPS Test Demo");

  // 显示FPS
  char fps_text[32];
  snprintf(fps_text, sizeof(fps_text), "FPS: %u.%u",
           (unsigned int)(current_fps_x1000 / 1000),
           (unsigned int)((current_fps_x1000 % 1000) / 100)); // 显示一位小数
  u8g2.setFont(u8g2_font_7x13_tr);
  u8g2.drawStr(0, 25, fps_text);

  // 显示帧计数
  char frame_text[32];
  snprintf(frame_text, sizeof(frame_text), "Frames: %u",
           (unsigned int)frame_counter);
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(0, 35, frame_text);

  // 显示运行时间
  char time_text[32];
  uint32_t seconds = current_tick / 1000;
  snprintf(time_text, sizeof(time_text), "Time: %us", (unsigned int)seconds);
  u8g2.drawStr(0, 45, time_text);

  // 绘制一些动画效果来测试FPS
  animation_counter = (current_tick / 50) % 128; // 50ms为一个周期，范围0-127

  // 移动的圆点
  u8g2.drawCircle(animation_counter, 55, 3, U8G2_DRAW_ALL);

  // 移动的矩形
  uint8_t rect_x = (current_tick / 30) % 108; // 30ms为一个周期
  u8g2.drawBox(rect_x, 58, 20, 4);

  // 旋转的线条（简化版）
  int angle = (current_tick / 100) % 8; // 8个方向
  int center_x = 100, center_y = 55;
  int line_length = 15;

  switch (angle) {
  case 0:
    u8g2.drawLine(center_x, center_y, center_x + line_length, center_y);
    break;
  case 1:
    u8g2.drawLine(center_x, center_y, center_x + line_length,
                  center_y - line_length);
    break;
  case 2:
    u8g2.drawLine(center_x, center_y, center_x, center_y - line_length);
    break;
  case 3:
    u8g2.drawLine(center_x, center_y, center_x - line_length,
                  center_y - line_length);
    break;
  case 4:
    u8g2.drawLine(center_x, center_y, center_x - line_length, center_y);
    break;
  case 5:
    u8g2.drawLine(center_x, center_y, center_x - line_length,
                  center_y + line_length);
    break;
  case 6:
    u8g2.drawLine(center_x, center_y, center_x, center_y + line_length);
    break;
  case 7:
    u8g2.drawLine(center_x, center_y, center_x + line_length,
                  center_y + line_length);
    break;
  }
}

/**
 * @brief 设置FPS测试模式
 * @param enable true启用FPS测试模式，false禁用
 */
void App_SetFpsTestMode(bool enable) {
  fps_test_mode = enable;
  if (enable) {
    // 重置FPS计数器
    frame_counter = 0;
    fps_last_tick = HAL_GetTick();
    current_fps_x1000 = 0;
    serial_printf("FPS Test Mode: ENABLED\r\n");
  } else {
    serial_printf("FPS Test Mode: DISABLED\r\n");
  }
}

/**
 * @brief 获取当前FPS值（扩大1000倍的整数）
 * @return 当前FPS值乘以1000的整数
 */
uint32_t App_GetCurrentFps(void) { return current_fps_x1000; }
