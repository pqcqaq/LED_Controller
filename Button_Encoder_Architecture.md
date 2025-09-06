# STM32 按键和编码器驱动架构

## 架构概述

现在的系统使用两个独立的面向对象模块：

### 1. Button 模块 (`button.h/cpp`)
- **面向对象设计**：使用C++类封装按键功能
- **支持功能**：
  - 基本按键检测（按下/释放）
  - 长按检测（可配置时间）
  - 多击检测（双击、三击等，可配置最大次数和间隔）
  - 硬件防抖
  - 灵活的回调机制

### 2. Encoder 模块 (`encoder.h/cpp`)
- **专用编码器驱动**：使用改进的John Main算法
- **防抖特性**：
  - 硬件防抖（2ms）
  - 四象限解码
  - 噪声过滤
  - 序列验证

## 使用方法

### 按键初始化和配置
```cpp
// 创建按键对象（PB14，低电平有效）
Button encoder_button(GPIOB, GPIO_PIN_14, true);

// 初始化
encoder_button.init();

// 设置基本事件回调
encoder_button.setEventCallback([](ButtonEvent_t event, ButtonState_t state) {
    // 处理按键事件
});

// 设置长按回调（1.5秒）
encoder_button.handleLongPress([](uint32_t duration_ms) {
    // 处理长按事件
}, 1500);

// 设置多击回调（最多5击，间隔400ms）
encoder_button.handleMultiClick(5, [](uint8_t click_count) {
    // 处理多击事件
}, 400);
```

### 编码器初始化和配置
```cpp
// 初始化编码器
Encoder_Init();

// 设置回调
Encoder_SetCallback([](EncoderEvent_t event, EncoderDirection_t direction, int32_t steps) {
    // 处理编码器事件
});
```

### 主循环处理
```cpp
void App_Loop(void) {
    // 处理按键事件
    encoder_button.process();
    
    // 处理编码器事件
    Encoder_Process();
}
```

### 中断处理
```cpp
extern "C" void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    // 编码器中断
    Encoder_GPIO_EXTI_Callback(GPIO_Pin);
    
    // 按键中断
    if (GPIO_Pin == GPIO_PIN_14) {
        encoder_button.onGpioInterrupt();
    }
}
```

## 硬件连接

- **按键**：PB14（编码器按下）
- **编码器CW**：PB12
- **编码器CCW**：PB13

## CubeMX 配置

需要在 CubeMX 中配置以下GPIO中断：
- PB12：GPIO_EXTI12（编码器CW）
- PB13：GPIO_EXTI13（编码器CCW）
- PB14：GPIO_EXTI14（按键）

中断模式：下降沿触发（如果是上拉输入）

## 优势

1. **模块化设计**：按键和编码器功能完全分离
2. **面向对象**：Button类提供清晰的接口和封装
3. **功能丰富**：支持长按、多击等高级功能
4. **可靠性高**：使用改进的防抖算法
5. **易于使用**：Lambda回调简化了事件处理
6. **可配置性**：时间阈值、最大点击次数等都可配置
