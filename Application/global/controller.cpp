#include "controller.h"
#include "custom_types.h"
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
#include <sys/_types.h>

SystemState state = {
    false, true, BRIGHTNESS_DEFAULT, COLOR_TEMP_DEFAULT, 0, 0, 1, 0, 0};
SystemState lastState = {true, false, LED_MAX_BRIGHTNESS,
                         255,  32767, 32767,
                         127,  127,   127}; // 初始化为不同值，确保首次更新

unsigned char btn_changed = 0;

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
  state.master = !state.master;
  serial_printf("Master Power: %s\r\n", state.master ? "ON" : "OFF");
}

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
  int32_t cold_weight = cct_ratio;                         // ch2 (冷白) 权重

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
      // uint16_t step = (td > SPEED_SLOW_THRESHOLD) ? LED_TEMP_STEP :
      //                (td > SPEED_FAST_THRESHOLD) ? LED_TEMP_STEP * 2 :
      //                (td > SPEED_VERY_FAST_THRESHOLD) ? LED_TEMP_STEP * 3
      //                :  LED_TEMP_STEP * 4;
      // state.colorTemp = constrain(state.colorTemp + dir * step,
      // COLOR_TEMP_MIN, COLOR_TEMP_MAX);
      state.colorTemp =
          constrain((direction == ENCODER_DIR_CW
                         ? state.colorTemp + steps * LED_TEMP_STEP
                         : state.colorTemp - steps * LED_TEMP_STEP),
                    COLOR_TEMP_MIN, COLOR_TEMP_MAX);

      serial_printf("Color Temp: %dK, last Step: %d\r\n", state.colorTemp,
                    (direction == ENCODER_DIR_CW ? steps : -steps));

    } else if (state.item == 2) { // 亮度调节
      // byte step = (td > SPEED_SLOW_THRESHOLD) ? 1 :
      //            (td > SPEED_FAST_THRESHOLD) ? 5 :
      //            (td > SPEED_VERY_FAST_THRESHOLD) ? 10 : 20;
      // state.brightness = constrain(state.brightness + dir * step, 0,
      // 100);
      state.brightness =
          constrain(direction == ENCODER_DIR_CW ? state.brightness + steps
                                                : state.brightness - steps,
                    0, LED_MAX_BRIGHTNESS);

      serial_printf("Brightness: %d%%, last Step: %d\r\n", state.brightness,
                    direction == ENCODER_DIR_CW ? steps : -steps);
    }
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
// 绘制装饰线条和边框
void drawDecorations() {
  // === 绘制亮度进度条 ===
  // 亮度进度条参数
  const int brightnessBarX = 10;
  const int brightnessBarY = 32;
  const int brightnessBarWidth = 126 - 8 * 2; // 为W标识预留空间
  const int brightnessBarHeight = 8;

  // 绘制"S"标识
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(brightnessBarX - 8, brightnessBarY + 8, "S");

  // 绘制亮度进度条背景
  u8g2.drawFrame(brightnessBarX, brightnessBarY, brightnessBarWidth,
                 brightnessBarHeight);

  // 计算亮度填充宽度
  int brightness = constrain(state.brightness, 0, LED_MAX_BRIGHTNESS);
  int fillWidth = (brightness * (brightnessBarWidth - 2)) /
                  LED_MAX_BRIGHTNESS; // 减2是内边距

  // 绘制填充部分
  if (fillWidth > 0) {
    u8g2.drawBox(brightnessBarX + 1, brightnessBarY + 1, fillWidth,
                 brightnessBarHeight - 2);
  }

  // 绘制"L"标识
  u8g2.drawStr(brightnessBarX + brightnessBarWidth + 2, brightnessBarY + 8,
               "L");

  // === 绘制色温进度条 ===
  // 色温进度条参数
  const int tempBarX = 10;
  const int tempBarY = 48;
  const int tempBarWidth = 126 - 8 * 2; // 为W和C标识预留空间
  const int tempBarHeight = 8;

  // 绘制"W"标识
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(tempBarX - 8, tempBarY + 8, "W");

  // 绘制色温进度条背景
  u8g2.drawFrame(tempBarX, tempBarY, tempBarWidth, tempBarHeight);

  // 计算色温进度
  int tempRange = COLOR_TEMP_MAX - COLOR_TEMP_MIN;
  int currentOffset = constrain(state.colorTemp - COLOR_TEMP_MIN, 0, tempRange);
  int starPos = (currentOffset * (tempBarWidth - 4)) / tempRange; // 星号位置

  // 绘制进度条线条
  for (int i = 2; i < tempBarWidth - 2; i += 3) {
    u8g2.drawPixel(tempBarX + i, tempBarY + tempBarHeight / 2);
  }

  // 绘制当前位置星号（用小方块代替）
  if (starPos >= 0) {
    u8g2.drawBox(tempBarX + starPos + 1, tempBarY + 2, 3, tempBarHeight - 4);
  }

  // 绘制"C"标识
  u8g2.drawStr(tempBarX + tempBarWidth + 2, tempBarY + 8, "C");
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
    u8g2.drawStr(0, 8, tempStr);

    // 风扇状态
    const char *fan_status = state.fanAuto ? " AUTO" : "FORCE";
    u8g2.drawStr(96, 8, fan_status);

    if (animUpdate) {
      animFrame = (animFrame + 1) % 8;
      lastAnim = now;
    }
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
    const char *activeStates[] = {"    ACTIVE    ", "   .ACTIVE.   ",
                                  "  ..ACTIVE..  ", " ...ACTIVE... ",
                                  " .. ACTIVE .. ", " .  ACTIVE  . "};
    u8g2.drawStr(32, 64, activeStates[animFrame % 6]);
  }

  // 更新lastState
  if (stateChanged) {
    lastState = state;
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

// 更新PWM输出
void updatePWM() {
  // static uint32_t lastFadeTime = 0;
  static uint16_t lastColorTemp = 0;
  static uint16_t lastBrightness = 0;
  static uint16_t cachedTarget1 = 0;
  static uint16_t cachedTarget2 = 0;

  // uint32_t now = HAL_GetTick();

  // if (now - lastFadeTime < PWM_FADE_INTERVAL_MS) {
  //   return;
  // }
  // lastFadeTime = now;

  // serial_printf("PWM Update\r\n");

  // 计算目标PWM值 - 只有在参数变化时才重新计算
  uint16_t target1, target2;

  if (state.master) {
    // 检查colorTemp和brightness是否发生变化
    if (state.colorTemp != lastColorTemp ||
        state.brightness != lastBrightness) {
      calculateChannelRatio(state.colorTemp, state.brightness, &cachedTarget1,
                            &cachedTarget2);
      lastColorTemp = state.colorTemp;
      lastBrightness = state.brightness;
    }
    target1 = cachedTarget1;
    target2 = cachedTarget2;
  } else {
    target1 = target2 = 0;
    // 当master为false时，清空缓存避免状态混乱
    lastColorTemp = 0;
    lastBrightness = 0;
    cachedTarget1 = 0;
    cachedTarget2 = 0;
  }

  // 平滑过渡
  state.currentCh1PWM = lerp(state.currentCh1PWM, target1, PWM_FADE_STEP);
  state.currentCh2PWM = lerp(state.currentCh2PWM, target2, PWM_FADE_STEP);

  // 输出到硬件
  uint16_t out1 = constrain(state.currentCh1PWM, 0, MAX_PWM);
  // analogWrite(LED1_PIN, out1);
  set_pwm1(out1);

  uint16_t out2 = constrain(state.currentCh2PWM, 0, MAX_PWM);
  // analogWrite(LED2_PIN, out2);
  set_pwm2(out2);
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
    u8g2.setContrast(255);
    btn_changed = 0;
  }

  static uint32_t lastDisplayUpdate = 0;

  if (now - lastDisplayUpdate > DISPLAY_UPDATE_MS) {
    // serial_printf("Loop: %lu\r\n", now);
    updateDisp();
    lastDisplayUpdate = now;
  }
  // updatePWM();
  updateADC();

  // 息屏处理
  if (now - lastChanged > SLEEP_TIME_MS) {
    // if(state.master){
    u8g2.setContrast(1);
    // } else {
    // }
  }
}
