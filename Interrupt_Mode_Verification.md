# STM32 中断模式验证指南

## 中断配置验证

### 🔧 GPIO中断配置
根据您的配置，以下引脚应该在STM32CubeMX中配置为External Interrupt Mode with Rising/Falling edge trigger detection：

- **PB12** (编码器A相) - EXTI12，Rising/Falling edge
- **PB13** (编码器B相) - EXTI13，Rising/Falling edge  
- **PB14** (编码器按键) - EXTI14，Rising/Falling edge
- **PA5** (按键1) - EXTI5，Rising/Falling edge
- **PA6** (按键2) - EXTI6，Rising/Falling edge

### 📋 中断向量配置
确保在STM32CubeMX的NVIC设置中启用了以下中断：
- **EXTI line[9:5] interrupts** - 用于PA5, PA6
- **EXTI line[15:10] interrupts** - 用于PB12, PB13, PB14

## 代码实现验证

### ✅ 中断服务例程 (ISR)
文件：`Core/Src/stm32f1xx_it.c`

```c
// EXTI5-9中断处理 (PA5, PA6)
void EXTI9_5_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_5);   // Button 1
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_6);   // Button 2
}

// EXTI10-15中断处理 (PB12, PB13, PB14)
void EXTI15_10_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_12);  // Encoder A
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_13);  // Encoder B
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_14);  // Encoder Button
}
```

### ✅ 中断回调函数
文件：`Core/Src/main.cpp`

```cpp
extern "C" void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
  switch (GPIO_Pin) {
    case GPIO_PIN_12:  // 编码器A相
    case GPIO_PIN_13:  // 编码器B相
      rotary_encoder.onGpioInterrupt(GPIO_Pin);
      break;
      
    case GPIO_PIN_14:  // 编码器按键
      encoder_button.onGpioInterrupt();
      break;
      
    case GPIO_PIN_5:   // 按键1
      btn_1.onGpioInterrupt();
      break;
      
    case GPIO_PIN_6:   // 按键2
      btn_2.onGpioInterrupt();
      break;
  }
}
```

## 中断模式特性

### 🚀 性能优势
1. **实时响应**：中断触发立即处理，无轮询延迟
2. **低功耗**：CPU可以进入睡眠模式，中断唤醒
3. **高精度**：精确捕获边沿变化时刻
4. **减少CPU占用**：无需持续轮询GPIO状态

### ⚡ 编码器中断处理
- **双边沿触发**：上升沿和下降沿都触发中断
- **硬件去抖**：利用中断延迟自然实现去抖
- **实时解码**：每次状态变化立即处理
- **稳定算法**：使用表查找算法处理噪声

### 🔘 按键中断处理
- **边沿检测**：按下和释放都触发中断
- **软件去抖**：中断中实现时间去抖动
- **多击检测**：中断+定时器实现多击判断
- **长按检测**：定时器检测长按状态

## 测试验证步骤

### 1️⃣ 基本功能测试
```cpp
// 在串口输出中应该看到：
// "RotaryEncoder: Interrupt mode enabled"
// "Button: Interrupt mode enabled"
```

### 2️⃣ 编码器测试
- **慢速旋转**：每个detent产生一个事件
- **快速旋转**：应触发加速，步数增加
- **方向测试**：顺时针/逆时针方向正确
- **抖动测试**：低质量编码器不应产生额外脉冲

### 3️⃣ 按键测试
- **单击测试**：快速按下释放，应触发CLICK事件
- **长按测试**：按住1.5秒，应触发LONG_PRESS事件
- **多击测试**：快速连续点击，应正确计数
- **去抖测试**：机械按键不应产生多余触发

### 4️⃣ 性能测试
- **响应延迟**：中断模式应比轮询模式响应更快
- **CPU占用**：主循环可以降低调用频率
- **稳定性**：长时间运行不应丢失事件

## 调试技巧

### 🔍 常见问题排查

#### 编码器无响应
1. 检查GPIO配置是否为EXTI模式
2. 确认上拉电阻已启用
3. 验证中断向量是否正确映射
4. 检查去抖时间是否过长

#### 按键抖动
1. 增加去抖时间 (`setDebounceTime(10)`)
2. 检查硬件连接和上拉电阻
3. 验证中断触发配置

#### 多击误触发
1. 调整多击间隔时间
2. 检查去抖逻辑
3. 验证单击与多击逻辑分离

#### 中断冲突
1. 确保每个EXTI线只有一个GPIO
2. 检查中断优先级设置
3. 验证ISR中代码执行时间

### 📊 性能监控
```cpp
// 添加调试代码监控中断频率
static uint32_t interrupt_count = 0;
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    interrupt_count++;
    // ... 处理逻辑
}
```

### 🛠️ 优化建议
1. **ISR优化**：中断中避免复杂计算
2. **去抖调整**：根据硬件质量调整去抖时间
3. **优先级设置**：编码器中断优先级高于按键
4. **错误处理**：添加异常状态检测

## 迁移对比

### 轮询模式 vs 中断模式
| 特性 | 轮询模式 | 中断模式 |
|------|----------|----------|
| 响应延迟 | 取决于轮询频率 | 硬件级实时响应 |
| CPU占用 | 持续占用 | 仅在事件时占用 |
| 功耗 | 较高 | 较低 |
| 实现复杂度 | 简单 | 中等 |
| 抗噪声能力 | 一般 | 较强 |
| 精度 | 受轮询频率限制 | 硬件级精度 |

## 注意事项

⚠️ **重要提醒**：
1. 确保STM32CubeMX配置与代码实现一致
2. 中断模式下仍需调用`process()`处理超时逻辑
3. 去抖时间设置要合理，太短会抖动，太长会漏检
4. 硬件质量影响中断频率，低质量编码器需要更长去抖时间
5. 中断优先级要合理设置，避免高频中断阻塞系统

---

*通过以上验证步骤，确保中断模式正确配置和工作。*
