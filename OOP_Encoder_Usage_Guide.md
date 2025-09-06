# STM32 面向对象编码器使用指南

## 概述

本指南介绍如何使用新的面向对象 RotaryEncoder 类，该类提供了稳定的编码器解码算法和丰富的事件系统。

## 特性

### 🎯 核心特性
- **稳定的解码算法**: 使用改进的表查找方法，有效消除开关抖动
- **多种事件回调**: 支持旋转、按键、位置变化等多种事件
- **速度检测**: 自动检测旋转速度（慢/中/快）
- **加速功能**: 可配置的加速倍率，提升用户体验
- **按键集成**: 内置编码器按键处理
- **方向反转**: 可配置编码器方向

### 🔧 算法改进
- 基于权威的表查找解码方法
- 抗抖动能力强，适用于低质量编码器
- 完整的detent-to-detent检测
- 无需外部硬件去抖动电路

## 快速开始

### 1. 创建编码器实例

```cpp
#include "drivers/encoder.h"

// 创建编码器实例（包含按键）
RotaryEncoder my_encoder(GPIOB, GPIO_PIN_12,  // Pin A
                        GPIOB, GPIO_PIN_13,   // Pin B  
                        GPIOB, GPIO_PIN_14,   // Button (可选)
                        true);                // Button active low
```

### 2. 初始化和配置

```cpp
void setup_encoder() {
    // 初始化编码器
    my_encoder.init();
    
    // 设置基本事件回调
    my_encoder.setEventCallback([](EncoderEvent_t event, EncoderDirection_t direction, int32_t steps) {
        switch (event) {
            case ENCODER_EVENT_ROTATE_CW:
                printf("顺时针旋转 %d 步\n", steps);
                break;
            case ENCODER_EVENT_ROTATE_CCW:
                printf("逆时针旋转 %d 步\n", steps);
                break;
            case ENCODER_EVENT_BUTTON_CLICK:
                printf("按键点击\n");
                break;
        }
    });
    
    // 启用加速功能
    my_encoder.setAcceleration(true, 100, 3);  // 100ms阈值，3倍加速
}
```

### 3. 主循环处理

```cpp
void main_loop() {
    while (1) {
        // 处理编码器事件
        my_encoder.process();
        
        // 其他代码...
        HAL_Delay(1);
    }
}
```

## 高级用法

### 🎨 多种回调类型

```cpp
// 1. 旋转专用回调（包含速度信息）
my_encoder.setRotationCallback([](EncoderDirection_t direction, int32_t steps, EncoderSpeed_t speed) {
    const char* speed_str = (speed == ENCODER_SPEED_FAST) ? "快速" :
                           (speed == ENCODER_SPEED_MEDIUM) ? "中速" : "慢速";
    const char* dir_str = (direction == ENCODER_DIR_CW) ? "顺时针" : "逆时针";
    printf("%s旋转 %d 步，速度：%s\n", dir_str, steps, speed_str);
});

// 2. 按键专用回调
my_encoder.setButtonCallback([](EncoderEvent_t event, uint32_t duration_ms) {
    switch (event) {
        case ENCODER_EVENT_BUTTON_PRESS:
            printf("按键按下\n");
            break;
        case ENCODER_EVENT_BUTTON_LONG_PRESS:
            printf("长按 %u ms\n", duration_ms);
            break;
    }
});

// 3. 位置变化回调
my_encoder.setPositionCallback([](int32_t position, int32_t delta) {
    printf("位置：%d，变化：%d\n", position, delta);
});
```

### ⚡ 加速配置

```cpp
// 启用加速功能
my_encoder.setAcceleration(true, 80, 4);   // 80ms阈值，4倍加速因子

// 根据速度自定义加速
my_encoder.setRotationCallback([](EncoderDirection_t direction, int32_t steps, EncoderSpeed_t speed) {
    int32_t actual_steps = steps;
    
    // 自定义加速逻辑
    switch (speed) {
        case ENCODER_SPEED_FAST:
            actual_steps *= 5;  // 快速旋转时5倍加速
            break;
        case ENCODER_SPEED_MEDIUM:
            actual_steps *= 2;  // 中速时2倍加速
            break;
        case ENCODER_SPEED_SLOW:
        default:
            // 慢速时无加速
            break;
    }
    
    update_parameter(direction, actual_steps);
});
```

### 🔄 方向控制

```cpp
// 反转编码器方向
my_encoder.setReversed(true);

// 动态切换方向
bool reversed = false;
if (some_condition) {
    reversed = !reversed;
    my_encoder.setReversed(reversed);
}
```

### 🎛️ 去抖动调节

```cpp
// 设置去抖动时间（默认5ms）
my_encoder.setDebounceTime(3);  // 3ms，适用于高质量编码器
my_encoder.setDebounceTime(10); // 10ms，适用于低质量编码器
```

## 实际应用示例

### 📊 菜单导航

```cpp
class MenuSystem {
private:
    int current_item = 0;
    const int max_items = 10;
    
public:
    void setupEncoder() {
        encoder.setRotationCallback([this](EncoderDirection_t direction, int32_t steps, EncoderSpeed_t speed) {
            // 根据速度调整导航步长
            int step_size = (speed == ENCODER_SPEED_FAST) ? 3 : 1;
            
            if (direction == ENCODER_DIR_CW) {
                current_item = min(current_item + step_size, max_items - 1);
            } else {
                current_item = max(current_item - step_size, 0);
            }
            
            updateDisplay();
        });
        
        encoder.setButtonCallback([this](EncoderEvent_t event, uint32_t duration) {
            if (event == ENCODER_EVENT_BUTTON_CLICK) {
                selectCurrentItem();
            } else if (event == ENCODER_EVENT_BUTTON_LONG_PRESS) {
                exitMenu();
            }
        });
    }
};
```

### 🎚️ 音量控制

```cpp
class VolumeControl {
private:
    int volume = 50;  // 0-100
    
public:
    void setupEncoder() {
        encoder.setRotationCallback([this](EncoderDirection_t direction, int32_t steps, EncoderSpeed_t speed) {
            // 快速旋转时大步调节
            int volume_step = (speed == ENCODER_SPEED_FAST) ? 5 : 1;
            
            if (direction == ENCODER_DIR_CW) {
                volume = min(volume + volume_step, 100);
            } else {
                volume = max(volume - volume_step, 0);
            }
            
            updateVolumeDisplay();
            setSystemVolume(volume);
        });
        
        encoder.setButtonCallback([this](EncoderEvent_t event, uint32_t duration) {
            if (event == ENCODER_EVENT_BUTTON_CLICK) {
                toggleMute();
            }
        });
    }
};
```

## 中断处理

确保在中断服务程序中调用编码器的中断处理函数：

```cpp
extern "C" void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    // 将中断传递给编码器处理
    my_encoder.onGpioInterrupt(GPIO_Pin);
}
```

## 性能优化建议

### 📈 最佳实践
1. **调用频率**: 在主循环中定期调用 `process()` 方法，建议频率 > 100Hz
2. **中断响应**: 中断处理程序应尽量简短，主要处理在主循环中进行
3. **回调效率**: 回调函数应避免耗时操作，如需要可使用标志位延后处理
4. **内存使用**: 类的实例化开销很小，可以创建多个编码器实例

### ⚠️ 注意事项
- 确保GPIO已正确配置为输入并启用上拉电阻
- 如果编码器质量较差，可以适当增加去抖动时间
- 加速功能可能导致参数跳跃过大，需要根据应用场景调节
- 长时间旋转可能导致位置溢出，可定期重置位置

## 故障排除

### 🔍 常见问题
1. **编码器跳步**: 检查去抖动时间设置，增加到10-20ms
2. **方向错误**: 使用 `setReversed(true)` 或交换Pin A和Pin B
3. **回调未触发**: 确认中断配置正确，GPIO配置无误
4. **位置漂移**: 可能是编码器质量问题，尝试增加去抖动时间

### 🛠️ 调试技巧
```cpp
// 启用详细日志
my_encoder.setEventCallback([](EncoderEvent_t event, EncoderDirection_t direction, int32_t steps) {
    printf("Event: %d, Direction: %d, Steps: %d, Position: %d\n", 
           event, direction, steps, my_encoder.getPosition());
});
```

## 迁移指南

从旧的C风格编码器代码迁移：

```cpp
// 旧代码
Encoder_Init();
Encoder_SetCallback(my_callback);
Encoder_Process();
int32_t pos = Encoder_GetPosition();

// 新代码
RotaryEncoder encoder(GPIOB, GPIO_PIN_12, GPIOB, GPIO_PIN_13);
encoder.init();
encoder.setEventCallback(my_callback);
encoder.process();
int32_t pos = encoder.getPosition();
```

---

该面向对象编码器类提供了更强大、更稳定、更灵活的编码器处理能力，适用于各种嵌入式应用场景。
