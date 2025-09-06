# 编码器和按键调试指南

## 问题诊断

### 当前状态
- **已恢复轮询模式**: 暂时禁用中断模式，使用轮询模式测试基本功能
- **GPIO配置**: 您已配置为External Interrupt Mode，但代码暂时使用轮询模式

### 立即测试步骤

1. **编译并测试轮询模式**
   ```bash
   cmake --build build/Release
   ```

2. **检查串口输出**
   - 观察是否有"Button & Encoder Test initialized successfully"
   - 旋转编码器，观察是否有"Encoder Event: ROTATE_CW/CCW"
   - 按下按键，观察是否有"Button Event: PRESS/RELEASE"

### 如果轮询模式正常工作

则问题在中断配置。需要检查：

#### 1. NVIC中断使能
检查STM32CubeMX中是否启用了对应的NVIC中断：
- EXTI15_10_IRQn (对应PB12, PB13, PB14)
- EXTI9_5_IRQn (对应PA5, PA6)

#### 2. GPIO中断配置
确认STM32CubeMX配置：
```
PB12: External Interrupt Mode with Rising/Falling edge trigger
PB13: External Interrupt Mode with Rising/Falling edge trigger  
PB14: External Interrupt Mode with Rising/Falling edge trigger
PA5:  External Interrupt Mode with Rising/Falling edge trigger
PA6:  External Interrupt Mode with Rising/Falling edge trigger
```

#### 3. 启用中断模式
在`global_objects.cpp`中将以下行改为true：
```cpp
encoder_button.setInterruptMode(true);
btn_1.setInterruptMode(true);
btn_2.setInterruptMode(true);
rotary_encoder.setInterruptMode(true);
```

### 如果轮询模式也不工作

则问题在基础配置：

#### 1. 检查GPIO基础配置
```cpp
// 在main.cpp的User Code中添加测试代码
void Test_GPIO_Read(void) {
    HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12);  // 编码器A
    HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_13);  // 编码器B  
    HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_14);  // 编码器按键
}
```

#### 2. 检查引脚连接
- 编码器A相 → PB12
- 编码器B相 → PB13
- 编码器按键 → PB14
- 按键1 → PA5
- 按键2 → PA6

#### 3. 检查上拉电阻
确保GPIO配置为：
- Input mode
- Pull-up enabled
- 对于中断模式：External Interrupt Mode

### 调试代码

在`App_Loop()`中添加GPIO状态监控：

```cpp
void App_Loop(void) {
    static uint32_t debug_tick = 0;
    uint32_t current_tick = HAL_GetTick();
    
    // 每500ms输出一次GPIO状态
    if (current_tick - debug_tick >= 500) {
        debug_tick = current_tick;
        
        bool pb12 = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12);
        bool pb13 = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_13);  
        bool pb14 = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_14);
        
        serial_printf("GPIO Status: PB12=%d, PB13=%d, PB14=%d\r\n", 
                      pb12, pb13, pb14);
    }
    
    // 正常处理
    GlobalObjects_Process();
    draw_button_encoder_test();
}
```

### 中断模式调试

如果要测试中断模式，在中断处理函数中添加调试：

```cpp
extern "C" void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    // 添加调试输出
    serial_printf("EXTI IRQ: Pin %d\r\n", GPIO_Pin);
    
    // 原有处理代码...
}
```

### 分步恢复中断模式

1. **第一步**: 确认轮询模式完全正常
2. **第二步**: 只启用按键中断模式，编码器继续轮询
3. **第三步**: 只启用编码器中断模式，按键继续轮询  
4. **第四步**: 全部启用中断模式

### 常见问题

1. **中断函数名错误**: 确保`EXTI15_10_IRQHandler`和`EXTI9_5_IRQHandler`拼写正确
2. **NVIC未使能**: 检查STM32CubeMX的NVIC配置
3. **GPIO模式错误**: 必须是External Interrupt Mode，不是普通Input
4. **中断优先级冲突**: 检查中断优先级设置

## 当前代码状态

- ✅ 轮询模式已恢复
- ✅ 中断处理函数已准备
- ⚠️ 中断模式暂时禁用
- ⚠️ 需要验证GPIO和NVIC配置

请先测试轮询模式是否正常工作，然后我们可以逐步启用中断模式。
