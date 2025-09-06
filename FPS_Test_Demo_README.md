# STM32 FPS测试Demo使用说明

## 概述

这是一个基于STM32F103C8T6和U8G2图形库的FPS（每秒帧数）测试演示程序。该程序能够实时显示和监控STM32系统的图形渲染性能。

## 功能特性

### 1. 实时FPS显示
- 在OLED屏幕上实时显示当前FPS值
- 通过串口输出详细的性能数据
- 支持动态FPS计算和更新

### 2. 动画效果演示
- **移动圆点**: 水平移动的圆形，用于测试基本图形绘制性能
- **移动矩形**: 水平移动的矩形，测试填充图形的绘制性能
- **旋转线条**: 8方向旋转的线条，测试线条绘制和动画流畅度

### 3. 系统信息显示
- 当前帧计数
- 系统运行时间
- 实时FPS数值

### 4. 串口监控
```
=== STM32 FPS Test Demo (C++23) ===
System Clock: 72000000 Hz
Tick Frequency: 1000 Hz
FPS Test Mode: ENABLED
FPS Test initialized successfully!

Loop #1 - Tick: 1003 ms - FPS: 12.5
Loop #2 - Tick: 2004 ms - FPS: 15.8
Loop #3 - Tick: 3005 ms - FPS: 18.2
```

## API接口

### 初始化函数
```cpp
void App_Init(void);
```
- 初始化FPS测试系统
- 设置默认参数
- 显示启动动画

### 主循环函数
```cpp
void App_Loop(void);
```
- 更新FPS计数器
- 渲染测试内容
- 输出性能数据

### 控制函数
```cpp
void App_SetFpsTestMode(bool enable);
```
- 启用/禁用FPS测试模式
- `enable = true`: 启用FPS测试显示
- `enable = false`: 切换到普通显示模式

```cpp
float App_GetCurrentFps(void);
```
- 获取当前FPS数值
- 返回值：当前的FPS值（浮点数）

## 配置参数

### FPS更新间隔
```cpp
static uint32_t fps_update_interval = 1000; // 每1000ms更新一次FPS显示
```

### 动画速度参数
- 圆点移动周期：50ms
- 矩形移动周期：30ms  
- 线条旋转周期：100ms

## 性能基准

### 典型性能指标
- **STM32F103C8T6 @ 72MHz**: 预期FPS 10-20
- **I2C OLED (128x64)**: 限制因素主要是I2C通信速度
- **U8G2库优化**: 使用页面缓冲提高效率

### 影响FPS的因素
1. **I2C时钟频率**: 更高的I2C频率可以提升FPS
2. **显示内容复杂度**: 更多图形元素会降低FPS
3. **系统时钟频率**: 更高的CPU频率可以提升FPS
4. **内存使用**: 充足的RAM有助于图形缓冲

## 使用方法

### 1. 编译和烧录
```bash
# 构建项目
cmake --build build --config Debug

# 或使用Release模式获得更好性能
cmake --build build --config Release
```

### 2. 串口监控
- 波特率：115200
- 数据位：8
- 停止位：1
- 校验位：无

### 3. 运行时控制
```cpp
// 在代码中动态切换模式
App_SetFpsTestMode(true);   // 启用FPS测试
App_SetFpsTestMode(false);  // 禁用FPS测试

// 获取当前FPS
float current_fps = App_GetCurrentFps();
```

## 优化建议

### 1. 硬件优化
- 使用更高速的SPI接口替代I2C
- 提高I2C时钟频率（在稳定性允许的范围内）
- 使用具有更多RAM的MCU型号

### 2. 软件优化
- 减少不必要的图形绘制操作
- 使用局部刷新而非全屏刷新
- 优化动画算法，减少计算复杂度

### 3. 系统优化
- 提高系统时钟频率
- 优化中断处理，减少绘制中断
- 合理配置DMA以减少CPU负载

## 故障排除

### 常见问题

1. **FPS显示为0或异常低**
   - 检查I2C连接
   - 验证OLED屏幕工作状态
   - 确认U8G2库初始化正常

2. **串口无输出**
   - 检查串口配置和连接
   - 验证波特率设置
   - 确认串口初始化代码

3. **动画不流畅**
   - 检查系统负载
   - 优化绘制代码
   - 考虑降低动画复杂度

## 扩展功能

### 可添加的测试项目
- 文本渲染性能测试
- 图像/位图显示测试
- 3D线框渲染测试
- 粒子系统性能测试

### 高级功能
- 性能分析和统计
- 自动化性能测试
- 不同分辨率适配
- 多种显示设备支持

## 技术栈

- **MCU**: STM32F103C8T6
- **显示库**: U8G2
- **编程语言**: C++23
- **构建系统**: CMake
- **开发环境**: VS Code + STM32 Extension

## 许可证

本项目遵循MIT许可证，可自由使用和修改。
