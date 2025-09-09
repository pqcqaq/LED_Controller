#include "controller.h"
#include "custom_types.h"
#include "drivers/settings.h"
#include "gamma_table.h"
#include "global_objects.h"
#include "stm32f1xx_hal.h"
#include "temp_adc.h"
#include "tim.h"
#include "u8g2.h"
#include <adc.h>
#include <cstdint>
#include <cstdlib>
#include <stdio.h>

unsigned char btn_changed = 0;
unsigned char settings_changed = 0; // 设置已更改标志

// 计算色温对应的两个通道比例
// 修正后的色温通道比例计算函数
void calculateChannelRatio(uint16_t colorTemp, uint16_t brightness,
                           uint16_t *ch1PWM, uint16_t *ch2PWM) {
  if (brightness == 0) {
    *ch1PWM = 0;
    *ch2PWM = 0;
    return;
  }

  // 限制色温范围
  colorTemp = constrain(colorTemp, COLOR_TEMP_MIN, COLOR_TEMP_MAX);

  // 使用mired值进行线性插值 (避免浮点运算)
  int32_t mired_target = colorTempToMired(colorTemp);
  int32_t mired_min = colorTempToMired(COLOR_TEMP_MIN); // 3000K -> 3333
  int32_t mired_max = colorTempToMired(COLOR_TEMP_MAX); // 5700K -> 1754

  // 计算CCT比例 (0-LED_TEMP_WEIGHT_TOTAL，兼容WLED标准)
  int32_t cct_ratio = ((mired_target - mired_min) * LED_TEMP_WEIGHT_TOTAL) /
                      (mired_max - mired_min);
  cct_ratio = constrain(cct_ratio, 0, LED_TEMP_WEIGHT_TOTAL);

  // CCT叠加混合比例 (0-100)，可以根据需要调整
  // 0 = 纯线性混合，100 = 完全叠加混合

  // 计算各通道的基础权重 (0-LED_TEMP_WEIGHT_TOTAL)
  int32_t warm_weight = LED_TEMP_WEIGHT_TOTAL - cct_ratio; // ch1 (暖白) 权重
  int32_t cold_weight = cct_ratio;

  uint16_t ch1_brightness, ch2_brightness;

  // 计算线性混合的亮度
  int32_t ch1_linear = (brightness * warm_weight) / LED_TEMP_WEIGHT_TOTAL;
  int32_t ch2_linear = (brightness * cold_weight) / LED_TEMP_WEIGHT_TOTAL;

  // 计算叠加混合的亮度
  int32_t ch1_additive = (warm_weight > 0) ? brightness : 0;
  int32_t ch2_additive = (cold_weight > 0) ? brightness : 0;

  // 根据叠加混合比例插值
  ch1_brightness = ((ch1_linear * (LED_TEMP_SPRI_TOTAL - CCT_ADDITIVE_BLEND)) +
                    (ch1_additive * CCT_ADDITIVE_BLEND)) /
                   LED_TEMP_SPRI_TOTAL;
  ch2_brightness = ((ch2_linear * (LED_TEMP_SPRI_TOTAL - CCT_ADDITIVE_BLEND)) +
                    (ch2_additive * CCT_ADDITIVE_BLEND)) /
                   LED_TEMP_SPRI_TOTAL;

  // 限制最大值
  if (ch1_brightness > LED_MAX_BRIGHTNESS)
    ch1_brightness = LED_MAX_BRIGHTNESS;
  if (ch2_brightness > LED_MAX_BRIGHTNESS)
    ch2_brightness = LED_MAX_BRIGHTNESS;

  // 分别对每个通道进行伽马校正
  *ch1PWM = (ch1_brightness > 0) ? gammaTable[ch1_brightness] : 0;
  *ch2PWM = (ch2_brightness > 0) ? gammaTable[ch2_brightness] : 0;
}

// 处理按钮单击事件 - 在色温和亮度之间切换，始终编辑状态
void handleClick() {
  btn_changed = 1;

  // 只在色温(1)和亮度(2)之间切换
  if (state.item == 1) {
    state.item = 2; // 从色温切换到亮度
    serial_printf("Switched to Brightness\r\n");
  } else {
    state.item = 1; // 从亮度或开关切换到色温
    serial_printf("Switched to Color Temperature\r\n");
  }

  // 始终保持编辑状态
  state.edit = 0;
}

// 处理按钮双击事件 - 开关输出
void handleDoubleClick() {
  btn_changed = 1;
  // 双击切换主开关状态
  // state.master = !state.master;
  // state.edit = -1; // 退出编辑模式
  state.fanAuto = !state.fanAuto;
  settings_changed = 1;

  serial_printf("Fan Auto Mode: %s\r\n", state.fanAuto ? "ON" : "OFF");
}

// 处理按钮长按事件 - 开机/关机
void handleLongPress() {
  btn_changed = 1;
  // state.master = false;
  // state.colorTemp = COLOR_TEMP_DEFAULT;
  // state.brightness = BRIGHTNESS_DEFAULT;
  // state.item = 0;
  // state.edit = -1;
  // enc.pos = 0;
  if (!state.master) {
    turnOn();
  } else {
    turnOff();
  }
}

void turnOn() {
  if (!state.master) {
    state.master = true;
    // 开机的时候要重新设置目标PWM
    calculateChannelRatio(state.colorTemp, state.brightness,
                          &state.targetCh1PWM, &state.targetCh2PWM);
  }
  serial_printf("Master Power: ON\r\n");
}

void turnOff() {
  if (state.master) {
    state.master = false;
    state.targetCh1PWM = 0;
    state.targetCh2PWM = 0;
  }
  serial_printf("Master Power: OFF\r\n");
}

/**
 * @brief 使用查找表和线性插值计算温度
 * @param adc_value 12位ADC采样值 (0-4095)
 * @return 温度值×100 (例如: 2550表示25.5℃)
 *         返回-99900表示传感器开路或超出测量范围
 */
int32_t adc_to_temperature_fast(uint16_t adc_value) {
  if (adc_value == 0 || adc_value >= 4095) {
    return -99900L;
  }

  // 查找表二分搜索
  int left = 0;
  int right = TEMP_TABLE_SIZE - 1;

  // 边界检查
  uint16_t min_adc = tempTableADC[0];
  uint16_t max_adc = tempTableADC[TEMP_TABLE_SIZE - 1];

  if (adc_value <= min_adc) {
    return tempTableTemp[0];
  }
  if (adc_value >= max_adc) {
    return tempTableTemp[TEMP_TABLE_SIZE - 1];
  }

  // 二分查找
  while (left < right - 1) {
    int mid = (left + right) / 2;
    uint16_t mid_adc = tempTableADC[mid];

    if (adc_value <= mid_adc) {
      right = mid;
    } else {
      left = mid;
    }
  }

  // 线性插值
  uint16_t adc1 = tempTableADC[left];
  uint16_t adc2 = tempTableADC[right];
  int16_t temp1 = tempTableTemp[left];
  int16_t temp2 = tempTableTemp[right];

  if (adc2 == adc1) {
    return temp1;
  }

  // 线性插值: temp = temp1 + (temp2-temp1) * (adc-adc1) / (adc2-adc1)
  int32_t temp_diff = temp2 - temp1;
  int32_t adc_diff = adc2 - adc1;
  int32_t adc_offset = adc_value - adc1;

  int32_t result = temp1 + (temp_diff * adc_offset) / adc_diff;

  return result;
}

// 更新ADC读取数据
void updateADC() {
  static uint32_t lastUpdateADCTime = 0;
  uint32_t now = HAL_GetTick();
  if (now - lastUpdateADCTime > ADC_READ_INTERVAL) {
    lastUpdateADCTime = now;
    // state.temp = adc_to_temperature_fast(analogRead(TEMP_ADC));
    // 这里可能是第二次循环才进来
    if (adc_done_flag) {
      state.temp = adc_to_temperature_fast(adc_value);
      adc_done_flag = 0;
    } else {
      HAL_ADC_Start_IT(&hadc1); // 开启采样
    }
  }

  static uint32_t lastUpdateFanTime = 0;
  if (now - lastUpdateFanTime > FAN_UPDATE_INTERVAL) {

    // serial_printf("Fan Update\r\n");

    lastUpdateFanTime = now;
    // 以下是模拟PWM与风扇控制
    if (!state.fanAuto) {
      // digitalWrite(FAN_EN, HIGH);
      open_fan();
      return;
    }
    int16_t tempNow = state.temp;
    if (tempNow < FAN_START_TEMP) {
      // digitalWrite(FAN_EN, LOW);
      close_fan();
      return;
    }
    if (tempNow > FAN_FULL_TEMP) {
      // digitalWrite(FAN_EN, HIGH);
      open_fan();
      return;
    }

    // PWM控制：固定总周期数，根据温度分配开启/关闭次数
    static uint16_t cycleCounter = 0;

    // 根据温度计算占空比 (0-100)
    uint16_t tempRange = FAN_FULL_TEMP - FAN_START_TEMP;
    uint16_t tempOffset = tempNow - FAN_START_TEMP;
    uint16_t dutyCycle = (tempOffset * 100) / tempRange; // 0-100的占空比

    // 计算开启周期数
    uint16_t onCycles = (dutyCycle * TOTAL_CYCLES) / 100;

    // PWM输出控制
    cycleCounter++;
    if (cycleCounter > TOTAL_CYCLES) {
      cycleCounter = 1; // 重新开始周期
    }

    if (cycleCounter <= onCycles) {
      // digitalWrite(FAN_EN, HIGH);  // 开启阶段
      open_fan();
    } else {
      // digitalWrite(FAN_EN, LOW);   // 关闭阶段
      close_fan();
    }
  }
}

// 处理编码器旋转
void handleEnc(EncoderDirection_t direction, int32_t steps,
               EncoderSpeed_t speed) {
  // static int lastPos = 0;
  // static int steps = 0;
  // static uint32_t lastTime = 0;

  // int delta = enc.pos - lastPos;
  // if (abs(delta) > ENCODER_NOISE_THRESHOLD) {
  //   enc.pos = lastPos;
  //   return;
  // }

  // if (delta) {
  //   steps += (delta > 0) ? 1 : -1;

  //   if (abs(steps) >= ENCODER_STEPS_PER_CLICK) {
  //     int dir = (steps > 0) ? 1 : -1;

  btn_changed = 1;

  serial_printf("Encoder Event: Dir=%d, Steps=%d, Speed=%d\r\n", (int)direction,
                (int)steps, (int)speed);

  if (state.edit == 0) {
    // // 编辑模式：根据旋转速度调整步进大小
    // uint32_t td = millis() - lastTime;
    // lastTime = millis();

    if (state.item == 1) { // 色温调节
      state.colorTemp =
          constrain((direction == ENCODER_DIR_CW
                         ? state.colorTemp + steps * LED_TEMP_STEP
                         : state.colorTemp - steps * LED_TEMP_STEP),
                    COLOR_TEMP_MIN, COLOR_TEMP_MAX);

      serial_printf("Color Temp: %dK, last Step: %d\r\n", state.colorTemp,
                    (direction == ENCODER_DIR_CW ? steps : -steps));

    } else if (state.item == 2) { // 亮度调节
      state.brightness =
          constrain(direction == ENCODER_DIR_CW ? state.brightness + steps
                                                : state.brightness - steps,
                    0, LED_MAX_BRIGHTNESS);

      serial_printf("Brightness: %d%%, last Step: %d\r\n", state.brightness,
                    direction == ENCODER_DIR_CW ? steps : -steps);
    }
    settings_changed = 1;
  }
  //     } else {
  //       // 导航模式：选择项目
  //       state.item = (state.item + dir + 3) % 3;
  //     }
  //     steps %= ENCODER_STEPS_PER_CLICK;
  //   }
  //   lastPos = enc.pos;
  // }
}
// 简单的正弦波近似函数 (避免使用浮点数学库)
int16_t fastSin(int16_t angle) {
  // 使用查找表方式实现简单正弦波
  static const int8_t sinTable[16] = {0,   24,  49,  71,  90, 106, 117, 125,
                                      127, 125, 117, 106, 90, 71,  49,  24};
  return sinTable[angle & 15] / 32; // 缩放到合适范围
}

// 弹球动画参数结构体 (使用定点数运算避免浮点)
typedef struct {
  int16_t x, y;   // 弹球位置 (实际位置 * 256)
  int16_t vx, vy; // 弹球速度 (速度 * 256)
  uint8_t radius; // 弹球半径
  struct {
    uint8_t x, y;       // 拖尾位置历史
  } trail[6];           // 拖尾效果，保存6个历史位置
  uint8_t trailIdx;     // 拖尾索引
  uint8_t bounceEffect; // 反弹特效计数器
} BounceBall;

// 静态弹球实例 (使用定点数，实际值*256)
static BounceBall ball = {.x = 64 * 256, // 64.0f -> 64*256
                          .y = 32 * 256, // 32.0f -> 32*256
                          .vx = 307,     // 1.2f -> 1.2*256 ≈ 307
                          .vy = 205,     // 0.8f -> 0.8*256 ≈ 205
                          .radius = 3,
                          .trail = {{0}},
                          .trailIdx = 0,
                          .bounceEffect = 0};

// 更新弹球物理 (使用整数运算)
void updateBallPhysics() {
  // 更新位置 (定点数运算)
  ball.x += ball.vx;
  ball.y += ball.vy;

  // 转换为像素坐标进行边界检测
  int16_t pixelX = ball.x / 256;
  int16_t pixelY = ball.y / 256;

  // 屏幕边界检测和反弹
  bool bounced = false;

  if (pixelX <= ball.radius || pixelX >= 128 - ball.radius) {
    ball.vx = -ball.vx;
    bounced = true;
    // 限制位置在边界内
    if (pixelX <= ball.radius) {
      ball.x = ball.radius * 256;
    } else {
      ball.x = (128 - ball.radius) * 256;
    }
  }

  if (pixelY <= ball.radius + 10 ||
      pixelY >= 64 - ball.radius) { // 顶部留空给状态栏
    ball.vy = -ball.vy;
    bounced = true;
    // 限制位置在边界内
    if (pixelY <= ball.radius + 10) {
      ball.y = (ball.radius + 11) * 256; // +11 而不是 +10，避免边界重叠
    } else {
      ball.y = (64 - ball.radius) * 256;
    }
  }

  // 只在实际发生反弹时触发特效
  if (bounced && ball.bounceEffect == 0) {
    ball.bounceEffect = 8;
  }

  // 减少反弹特效计数器
  if (ball.bounceEffect > 0) {
    ball.bounceEffect--;
  }

  // 每隔几帧更新拖尾 - 让轨迹更加自然
  static uint8_t updateCounter = 0;
  updateCounter++;
  if (updateCounter >= 2) { // 每2帧更新一次拖尾
    updateCounter = 0;
    ball.trail[ball.trailIdx].x = (uint8_t)(ball.x / 256);
    ball.trail[ball.trailIdx].y = (uint8_t)(ball.y / 256);
    ball.trailIdx = (ball.trailIdx + 1) % 6;
  }
}

// 绘制弹球和拖尾
void drawBounceBall() {
  // 当前位置
  int16_t currentX = ball.x / 256;
  int16_t currentY = ball.y / 256;

  // 绘制拖尾效果 - 根据历史位置绘制轨迹
  int16_t lastX = currentX, lastY = currentY;

  for (int i = 1; i < 6; i++) {
    int trailIndex = (ball.trailIdx - i + 6) % 6;
    uint8_t trailX = ball.trail[trailIndex].x;
    uint8_t trailY = ball.trail[trailIndex].y;

    if (trailX > 0 && trailY > 0) {
      // 绘制连接线显示轨迹
      if (i == 1) {
        u8g2.drawLine(lastX, lastY, trailX, trailY);
      }

      // 根据距离当前位置的远近调整拖尾大小
      if (i == 1) {
        u8g2.drawDisc(trailX, trailY, 2); // 最近的拖尾点较大
      } else if (i == 2) {
        u8g2.drawDisc(trailX, trailY, 1); // 中等大小
      } else {
        u8g2.drawPixel(trailX, trailY); // 远处的拖尾点只是像素
      }

      lastX = trailX;
      lastY = trailY;
    }
  }

  // 绘制主弹球 - 使用实心圆
  uint8_t drawRadius = ball.radius;
  if (ball.bounceEffect > 0) {
    // 反弹时弹球变大
    drawRadius = ball.radius + (ball.bounceEffect / 2);
    // 绘制反弹波纹
    u8g2.drawCircle(currentX, currentY, ball.radius + ball.bounceEffect);
  }
  u8g2.drawDisc(currentX, currentY, drawRadius);

  // 绘制弹球内部高光点
  u8g2.setDrawColor(0);
  u8g2.drawPixel(currentX - 1, currentY - 1);
  u8g2.setDrawColor(1);
}

// 绘制装饰性星星
void drawStars(uint8_t animFrame) {
  // 固定位置的小星星，避开弹球活动区域
  uint8_t starPositions[][2] = {{10, 20},  {118, 25}, {20, 55},
                                {108, 58}, {35, 45},  {90, 40}};

  for (int i = 0; i < 7; i++) {
    // 每个星星有不同的闪烁周期和速度
    uint8_t phase = (animFrame + i * 5) % 48;
    if (phase < 32) { // 星星显示周期更长
      int x = starPositions[i][0];
      int y = starPositions[i][1];

      // 根据相位绘制不同大小的星星
      if (phase < 4) {
        // 最亮阶段 - 绘制大星星
        u8g2.drawPixel(x, y);
        u8g2.drawPixel(x - 1, y);
        u8g2.drawPixel(x + 1, y);
        u8g2.drawPixel(x, y - 1);
        u8g2.drawPixel(x, y + 1);
        u8g2.drawPixel(x - 1, y - 1);
        u8g2.drawPixel(x + 1, y - 1);
        u8g2.drawPixel(x - 1, y + 1);
        u8g2.drawPixel(x + 1, y + 1);
        // 外围光晕
        u8g2.drawPixel(x - 2, y);
        u8g2.drawPixel(x + 2, y);
        u8g2.drawPixel(x, y - 2);
        u8g2.drawPixel(x, y + 2);
      } else if (phase < 8) {
        // 中等亮度 - 绘制中星星
        u8g2.drawPixel(x, y);
        u8g2.drawPixel(x - 1, y);
        u8g2.drawPixel(x + 1, y);
        u8g2.drawPixel(x, y - 1);
        u8g2.drawPixel(x, y + 1);
        u8g2.drawPixel(x - 1, y - 1);
        u8g2.drawPixel(x + 1, y - 1);
        u8g2.drawPixel(x - 1, y + 1);
        u8g2.drawPixel(x + 1, y + 1);
      } else if (phase < 12) {
        // 较暗阶段 - 绘制小星星
        u8g2.drawPixel(x, y);
        u8g2.drawPixel(x - 1, y);
        u8g2.drawPixel(x + 1, y);
        u8g2.drawPixel(x, y - 1);
        u8g2.drawPixel(x, y + 1);
      } else {
        // 最暗阶段 - 只有中心点
        u8g2.drawPixel(x, y);
      }
    }
  }
}

// 绘制波浪形边框
void drawWaveBorder() {
  uint32_t time = HAL_GetTick();

  // 顶部波浪边框
  for (int x = 0; x < 128; x += 4) {
    int wave = fastSin((x + time / 200) / 4) + 9;
    u8g2.drawPixel(x, wave);
    u8g2.drawPixel(x + 1, wave);
  }

  // 底部波浪边框
  for (int x = 0; x < 128; x += 4) {
    int wave = fastSin((x + time / 150) / 3) + 62;
    u8g2.drawPixel(x, wave);
    u8g2.drawPixel(x + 1, wave);
  }
}

// 绘制装饰线条和边框
void drawDecorations() {

  // === 绘制亮度进度条 ===
  const int brightnessBarX = 10;
  const int brightnessBarY = 32;
  const int brightnessBarWidth = 126 - 8 * 2;
  const int brightnessBarHeight = 8;

  // 绘制"S"标识
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(brightnessBarX - 8, brightnessBarY + 8, "S");

  // 绘制亮度进度条背景
  u8g2.drawFrame(brightnessBarX, brightnessBarY, brightnessBarWidth,
                 brightnessBarHeight);

  // 计算亮度填充宽度
  int brightness = constrain(state.brightness, 0, LED_MAX_BRIGHTNESS);
  int fillWidth = (brightness * (brightnessBarWidth - 2)) / LED_MAX_BRIGHTNESS;

  // 绘制填充部分 - 添加渐变效果
  if (fillWidth > 0) {
    u8g2.drawBox(brightnessBarX + 1, brightnessBarY + 1, fillWidth,
                 brightnessBarHeight - 2);

    // 添加动态指示器
    if (state.item == 2 && state.edit == 0) {
      // int indicatorX = brightnessBarX + fillWidth - 1;
      // if ((animFrame / 4) % 2) { // 闪烁效果
      //   u8g2.setDrawColor(0);
      //   u8g2.drawVLine(indicatorX, brightnessBarY + 1, brightnessBarHeight -
      //   2); u8g2.setDrawColor(1);
      // }
    }
  }

  u8g2.drawStr(brightnessBarX + brightnessBarWidth + 2, brightnessBarY + 8,
               "L");

  // === 绘制色温进度条 ===
  const int tempBarX = 10;
  const int tempBarY = 48;
  const int tempBarWidth = 126 - 8 * 2;
  const int tempBarHeight = 8;

  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(tempBarX - 8, tempBarY + 8, "W");

  // 绘制色温进度条背景
  u8g2.drawFrame(tempBarX, tempBarY, tempBarWidth, tempBarHeight);

  // 计算色温进度
  int tempRange = COLOR_TEMP_MAX - COLOR_TEMP_MIN;
  int currentOffset = constrain(state.colorTemp - COLOR_TEMP_MIN, 0, tempRange);
  int starPos = (currentOffset * (tempBarWidth - 4)) / tempRange;

  // 绘制进度条刻度线
  for (int i = 5; i < tempBarWidth - 5; i += 8) {
    u8g2.drawPixel(tempBarX + i, tempBarY + tempBarHeight / 2);
  }

  // 绘制当前位置指示器 - 增强视觉效果
  if (starPos >= 0) {
    // if (state.item == 1 && state.edit == 0 && (animFrame / 4) % 2) {
    // 选中时闪烁的大指示器
    // u8g2.drawBox(tempBarX + starPos, tempBarY + 1, 4, tempBarHeight - 2);
    // } else {
    // 正常指示器
    u8g2.drawBox(tempBarX + starPos + 1, tempBarY + 2, 3, tempBarHeight - 4);
    // }
  }

  u8g2.drawStr(tempBarX + tempBarWidth + 2, tempBarY + 8, "C");

  // 添加装饰性边角
  u8g2.drawPixel(0, 10);
  u8g2.drawPixel(1, 11);
  u8g2.drawPixel(127, 10);
  u8g2.drawPixel(126, 11);
  u8g2.drawPixel(0, 63);
  u8g2.drawPixel(1, 62);
  u8g2.drawPixel(127, 63);
  u8g2.drawPixel(126, 62);
}

// 更新显示屏
void updateDisp() {
  static uint32_t lastAnim = 0;
  static uint8_t animFrame = 0;
  uint32_t now = HAL_GetTick();

  bool animUpdate = (now - lastAnim > ANIMATION_FRAME_MS);

  bool stateChanged =
      (state.master != lastState.master ||
       state.colorTemp != lastState.colorTemp ||
       state.brightness != lastState.brightness ||
       state.item != lastState.item || state.edit != lastState.edit);

  if (!stateChanged && !animUpdate) {
    return;
  }

  // 清除缓冲区
  u8g2.clearBuffer();

  // 如果在睡眠模式，绘制特殊动画效果
  if (state.isSleeping) {

    if (animUpdate) {
      animFrame = (animFrame + 1) % 64; // 循环动画帧
      lastAnim = now;
    }

    u8g2.setContrast(1);

    drawWaveBorder();
    drawStars(animFrame);

    if (state.deepSleep) {
      // 最上方绘制项目名称和作者
      u8g2.setFont(u8g2_font_3x5im_tr);
      u8g2.drawStr(0, 6, TITLE_TEXT);
      u8g2.drawStr(90, 6, AUTHOR_TEXT);

      // 如果息屏，显示弹球动画和装饰效果
      u8g2.setFont(u8g2_font_10x20_tr);
      u8g2.drawStr(22, 32, "SLEEPING");

      // 小一点的字显示提示
      u8g2.setFont(u8g2_font_6x10_tf);
      u8g2.drawStr(24, 50, "Press any key");
      u8g2.drawStr(32, 60, "to wake up");

      // 绘制弹球动画和装饰
      if (animUpdate) {
        updateBallPhysics();
      }

      // 先绘制背景装饰
      drawStars(animFrame);
      drawWaveBorder();

      // 再绘制弹球，这样弹球会在星星上方
      drawBounceBall();

      u8g2.sendBuffer();
      return;
    }

  } else {
    u8g2.setContrast(255); // 恢复正常对比度
  }

  // === 顶部温度信息（动画更新） ===
  if (animUpdate || stateChanged) {
    u8g2.setFont(u8g2_font_6x10_tf);

    // 绘制温度标签和值
    int temp_int = get_temperature_int(state.temp);
    int temp_frac = get_temperature_frac(state.temp);
    char tempStr[20];

    if (state.temp == -99900L) {
      sprintf(tempStr, "LED:--.-C");
    } else {
      sprintf(tempStr, "LED:%d.%02dC", temp_int, temp_frac);
    }
    u8g2.drawStr(0, 7, tempStr);

    // 风扇状态
    const char *fan_status = state.fanAuto ? " AUTO" : "FORCE";
    u8g2.drawStr(96, 7, fan_status);
  }

  // === 主电源状态显示 ===
  // 电源状态 - 居中大字体显示
  u8g2.setFont(u8g2_font_8x13B_tr);
  if (state.master) {
    u8g2.drawStr(54, 24, "OUT");
  } else {
    u8g2.drawStr(54, 24, "OFF");
  }

  // === 色温和亮度数值显示 ===
  u8g2.setFont(u8g2_font_8x13B_tr);

  // 色温显示（左侧）
  char tempStr[8];
  sprintf(tempStr, "%4dK", state.colorTemp);

  // 如果色温被选中，绘制反色背景
  if (state.item == 1) {
    u8g2.setDrawColor(1);
    u8g2.drawBox(0, 14, 44, 14);
    u8g2.setDrawColor(0);
    u8g2.drawStr(2, 26, tempStr);
    u8g2.setDrawColor(1);
  } else {
    u8g2.drawStr(2, 24, tempStr);
  }

  // 亮度显示（右侧）
  char brightStr[8];
  if (state.brightness == LED_MAX_BRIGHTNESS) {
    sprintf(brightStr, "100%%");
  } else {
    int brightPercent = (state.brightness * 1000 + LED_MAX_BRIGHTNESS / 2) /
                        LED_MAX_BRIGHTNESS; // +127实现四舍五入
    sprintf(brightStr, "%2d.%d%%", brightPercent / 10, brightPercent % 10);
  }

  // 如果亮度被选中，绘制反色背景
  if (state.item == 2) {
    u8g2.setDrawColor(1);
    u8g2.drawBox(82, 14, 44, 14);
    u8g2.setDrawColor(0);
    u8g2.drawStr(84, 26, brightStr);
    u8g2.setDrawColor(1);
  } else {
    u8g2.drawStr(84, 24, brightStr);
  }

  // === 标识符 ===
  uint16_t labelY = 47;
  u8g2.setFont(u8g2_font_5x8_tf);
  u8g2.drawStr(10, labelY, "TEMPR");
  u8g2.drawStr(95, labelY, "LIGHT");

  // 当前选择项目的额外指示
  u8g2.setFont(u8g2_font_6x10_tf);
  if (state.item == 1 && state.edit == 0) { // 色温选中
    u8g2.drawStr(40, labelY, "<<*");
  } else if (state.item == 2 && state.edit == 0) { // 亮度选中
    u8g2.drawStr(70, labelY, "*>>");
  }

  // 绘制装饰元素（进度条等）
  drawDecorations();

  // === 底部状态行动画 ===
  u8g2.setFont(u8g2_font_5x8_tf);
  if (!state.master) {
    u8g2.drawStr(38, 64, "[ STANDBY ]");
  } else if (state.brightness == 0) {
    u8g2.drawStr(38, 64, "[  READY  ]");
  } else {
    // 运行中动画
    u8g2.drawStr(32, 64, activeStates[(animFrame / 8) % 8]);
  }

  // 更新lastState
  if (stateChanged) {
    lastState.master = state.master;
    lastState.colorTemp = state.colorTemp;
    lastState.brightness = state.brightness;
    lastState.item = state.item;
    lastState.edit = state.edit;
  }

  // 发送缓冲区到显示屏
  u8g2.sendBuffer();
}

// 线性插值函数
int16_t lerp(int16_t current, int16_t target, int16_t step) {
  if (current == target) {
    return target;
  }

  int16_t diff = target - current;

  if (abs(diff) <= step) {
    return target;
  }

  if (diff > 0) {
    return current + step;
  } else {
    return current - step;
  }
}

void calcPWM() {
  // 只有在参数变化时才重新计算
  if (state.master) {
    if ((state.colorTemp != lastState.colorTemp ||
         state.brightness != lastState.brightness)) {
      lastState.colorTemp = state.colorTemp;
      lastState.brightness = state.brightness;
      serial_printf("Calculating PWM: ColorTemp=%dK, Brightness=%d%%\r\n",
                    state.colorTemp, state.brightness);
      calculateChannelRatio(state.colorTemp, state.brightness,
                            &state.targetCh1PWM, &state.targetCh2PWM);
      lastState.targetCh1PWM = state.targetCh1PWM;
      lastState.targetCh2PWM = state.targetCh2PWM;
    }
  } else {
    state.targetCh1PWM = 0;
    state.targetCh2PWM = 0;
  }
}

// 更新PWM输出
void updatePWM() {
  // 平滑过渡
  state.currentCh1PWM =
      lerp(state.currentCh1PWM, state.targetCh1PWM, PWM_FADE_STEP);
  state.currentCh2PWM =
      lerp(state.currentCh2PWM, state.targetCh2PWM, PWM_FADE_STEP);

  // 输出到硬件
  set_pwm1(state.currentCh1PWM);
  set_pwm2(state.currentCh2PWM);
}

// 程序主循环
void loop() {
  // 喂狗，重置看门狗计时器
  // IWDG_Feed();

  uint32_t now = HAL_GetTick();
  static uint32_t lastChanged = 0;

  // handleButton();

  if (btn_changed) {
    lastChanged = now;
    btn_changed = 0;
    state.isSleeping = false;
    state.deepSleep = false;
  }

  static uint32_t lastDisplayUpdate = 0;

  if (now - lastDisplayUpdate > DISPLAY_UPDATE_MS) {
    // serial_printf("Loop: %lu\r\n", now);
    updateDisp();
    lastDisplayUpdate = now;
  }
  // updatePWM();
  calcPWM();
  updateADC();

  // 息屏处理
  if (now - lastChanged > SLEEP_TIME_MS) {
    state.isSleeping = true;
  }

  // 关机状态下一分钟进入深度休眠，显示动画
  if (now - lastChanged > DEEP_SLEEP_TIME_MS) {
    if (state.master) {
      return;
    }
    state.deepSleep = true;
  }

  // 延迟保存设置 (避免频繁EEPROM写入)
  static uint32_t last_save_time = 0;
  uint32_t current_time = HAL_GetTick();
  if (current_time - last_save_time >= SAVE_INTERVAL_MS && settings_changed) {
    Settings_Save(&state);
    last_save_time = current_time;
    settings_changed = 0;
  }
}
