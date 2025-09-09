# STM32 LED Controller

![Project Status](https://img.shields.io/badge/status-stable-brightgreen)
![Platform](https://img.shields.io/badge/platform-STM32F103C8T6-blue)
![License](https://img.shields.io/badge/license-MIT-green)

一个基于STM32F103C8T6的智能LED控制器项目，支持双通道PWM调光、色温调节、温度监控和智能风扇控制。

## 🚀 项目特点

### 核心功能

- **双通道LED控制**: 支持暖白和冷白LED独立调光
- **色温调节**: 3000K-5700K范围内平滑调节
- **亮度控制**: 0-100%精确亮度调节，支持伽马校正
- **温度监控**: 实时监测LED温度，防止过热
- **智能风扇**: 自动/手动温控风扇，PWM调速
- **OLED显示**: 128x64 SSD1306显示屏，实时状态显示
- **人机交互**: 旋转编码器+按键操作
- **设置保存**: EEPROM持久化存储用户设置

### 高级特性

- **超频运行**: STM32F103优化时钟配置，提升性能
- **平滑过渡**: 亮度和色温变化支持软件渐变
- **息屏动画**: 创意弹球动画和星空效果
- **温度保护**: 过热自动降功率或关闭输出
- **看门狗**: 硬件看门狗确保系统稳定性
- **USB通信**: 支持USB HID设备功能

## 🛠️ 硬件规格

### 主控芯片

- **MCU**: STM32F103C8T6 (ARM Cortex-M3, 72MHz)
- **Flash**: 64KB
- **RAM**: 20KB
- **封装**: LQFP48

### 外设接口

| 功能 | 引脚 | 说明 |
|------|------|------|
| LED PWM通道1 | TIM1_CH1 | 暖白LED控制 |
| LED PWM通道2 | TIM1_CH2 | 冷白LED控制 |
| 温度传感器 | ADC1_IN8 | NTC温度检测 |
| 旋转编码器 | GPIO | A/B相+按键 |
| OLED显示 | I2C1 | SSD1306控制器 |
| 风扇控制 | GPIO | PWM软件调速 |
| 扩展EEPROM | I2C2 | 设置存储 |

### 温度传感器

- **类型**: NTC 100KΩ (B3950)
- **精度**: ±0.5°C (25°C)
- **范围**: -20°C ~ +85°C
- **响应**: 快速温度响应和过热保护

## 📋 开发环境

### 必需工具

- **IDE**: Visual Studio Code
- **编译器**: ARM GCC (arm-none-eabi-gcc)
- **构建系统**: CMake 3.22+
- **调试器**: ST-Link V2/V3
- **代码生成**: STM32CubeMX

### 开发环境配置

#### 1. 安装必需软件

```bash
# Windows (使用Chocolatey)
choco install cmake
choco install gcc-arm-embedded
choco install stm32cubeprog

# 或者手动下载安装：
# - STM32CubeIDE 或 ARM GCC Toolchain
# - STM32CubeProgrammer
# - CMake
```

#### 2. VS Code扩展

```json
{
  "recommendations": [
    "ms-vscode.cpptools",
    "ms-vscode.cmake-tools",
    "marus25.cortex-debug",
    "dan-c-underwood.arm",
    "twxs.cmake"
  ]
}
```

#### 3. 克隆项目

```bash
git clone <repository-url>
cd LED_Controller
```

## 🔧 编译和烧录

### CMake编译

```bash
# 配置构建环境 (Debug模式)
cmake --preset Debug

# 编译项目
cmake --build build/Debug

# 或使用Release模式优化
cmake --preset Release
cmake --build build/Release
```

### VS Code集成

1. 打开项目文件夹
2. 使用 `Ctrl+Shift+P` 调用命令面板
3. 选择 `CMake: Configure`
4. 选择 `CMake: Build`

### 烧录固件

```bash
# 使用STM32CubeProgrammer
STM32_Programmer_CLI -c port=SWD -w build/Debug/mx.elf -v -rst

# 或使用OpenOCD
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg -c "program build/Debug/mx.elf verify reset exit"
```

## 📁 项目结构

```text
LED_Controller/
├── Application/                 # 应用程序代码
│   ├── animations/             # 动画效果
│   │   ├── boot_animation.cpp  # 启动动画
│   │   └── boot_animation.h
│   ├── drivers/                # 硬件驱动
│   │   ├── button.cpp         # 按键驱动
│   │   ├── encoder.cpp        # 编码器驱动
│   │   ├── eeprom.cpp         # EEPROM驱动
│   │   ├── settings.cpp       # 设置管理
│   │   ├── stm32_u8g2.cpp     # OLED显示驱动
│   │   └── iwdg_a.cpp         # 看门狗驱动
│   ├── global/                # 全局对象和控制器
│   │   ├── controller.cpp     # 主控制逻辑
│   │   ├── global_objects.cpp # 全局对象定义
│   │   ├── gamma_table.h      # 伽马校正表
│   │   └── temp_adc.h         # 温度转换表
│   ├── hardware/              # 硬件抽象
│   │   └── devices.cpp        # 设备检测和初始化
│   ├── utils/                 # 工具函数
│   │   ├── custom_types.c     # 自定义类型
│   │   └── delay.c            # 延时函数
│   ├── app.cpp                # 应用程序入口
│   └── app.h
├── Core/                       # STM32CubeMX生成代码
│   ├── Inc/                   # 头文件
│   ├── Src/                   # 源文件
│   └── Startup/               # 启动文件
├── Drivers/                    # STM32 HAL驱动
├── Libs/                      # 第三方库
│   └── u8g2/                  # U8g2图形库
├── cmake/                     # CMake配置文件
├── build/                     # 编译输出目录
├── CMakeLists.txt             # CMake主配置
├── CMakePresets.json          # CMake预设
└── README.md                  # 项目说明文档
```

## 🎮 使用说明

### 基本操作

- **旋转编码器**: 调节当前选中参数（色温/亮度）
- **单击按键**: 切换调节模式（色温↔亮度）
- **双击按键**: 切换风扇模式（自动/强制）
- **长按按键**: 主电源开关

### 显示界面

- **顶部**: LED温度和风扇状态
- **中部**: 主开关状态（ON/OFF）
- **左侧**: 色温显示（3000K-5700K）
- **右侧**: 亮度显示（0-100%）
- **底部**: 系统运行状态

### 参数范围

| 参数 | 范围 | 步进 | 说明 |
|------|------|------|------|
| 色温 | 3000K-5700K | 10K | 暖白到冷白 |
| 亮度 | 0-100% | 1% | 支持伽马校正 |
| 温度 | 自动检测 | - | NTC传感器 |

## 🔬 技术特性

### 色温控制算法

- **Mired值计算**: 避免浮点运算的高效色温转换
- **线性插值**: 平滑的色温过渡
- **叠加混合**: 支持WLED兼容的CCT混合模式
- **伽马校正**: 视觉线性的亮度调节

### 温度保护机制

```cpp
// 温度保护策略
#define FAN_START_TEMP  4500   // 45.0°C 启动风扇
#define FAN_FULL_TEMP   6500   // 65.0°C 满速运行
#define THERMAL_LIMIT   8000   // 80.0°C 过热保护
```

### PWM控制

- **频率**: 1kHz（避免频闪）
- **分辨率**: 12位（4096级）
- **平滑过渡**: 软件渐变算法
- **双通道**: 独立控制暖白/冷白

## 📊 性能参数

| 指标 | 数值 | 说明 |
|------|------|------|
| PWM频率 | 1kHz | 无频闪 |
| 色温精度 | ±10K | 视觉无感知 |
| 亮度精度 | ±0.5% | 12位分辨率 |
| 温度精度 | ±0.5°C | NTC传感器 |
| 响应速度 | <100ms | 按键响应 |
| 显示刷新 | 30fps | 流畅动画 |

## 🔍 调试功能

### 串口调试

```cpp
// 启用串口输出（USART2, 115200bps）
#define DEBUG_ENABLED 1

// 调试信息示例
serial_printf("PWM: CH1=%d, CH2=%d\r\n", ch1_pwm, ch2_pwm);
serial_printf("Temp: %.2f°C\r\n", temperature/100.0f);
```

### LED指示

- **启动动画**: 系统初始化状态
- **运行指示**: 实时状态显示
- **错误指示**: 故障状态提示

## 🚨 注意事项

### 硬件连接

1. **电源**: 确保3.3V稳定供电
2. **LED负载**: 检查电流限制电阻
3. **温度传感器**: 正确安装在散热器上
4. **编码器**: 确认A/B相和按键连接

### 软件配置

1. **时钟配置**: 已优化为72MHz超频运行
2. **中断优先级**: 按重要性合理配置
3. **堆栈大小**: 根据功能需求调整
4. **看门狗**: 生产环境建议启用

### 安全提醒

- ⚠️ **过热保护**: 确保温度传感器正常工作
- ⚠️ **电流限制**: LED驱动电路需要限流保护
- ⚠️ **电压匹配**: 确认各模块电压兼容性

## 🤝 贡献指南

欢迎提交Issue和Pull Request！

### 开发流程

1. Fork本项目
2. 创建特性分支 (`git checkout -b feature/amazing-feature`)
3. 提交更改 (`git commit -m 'Add amazing feature'`)
4. 推送分支 (`git push origin feature/amazing-feature`)
5. 创建Pull Request

### 代码规范

- 使用4空格缩进
- 遵循C++23标准
- 添加适当的注释
- 保持函数单一职责

## 📄 许可证

本项目采用MIT许可证 - 详见 [LICENSE](LICENSE) 文件

## 👨‍💻 作者

**QCQCQC** - *初始版本* - [GitHub](https://github.com/pqcqaq)

## 🙏 致谢

- [STMicroelectronics](https://www.st.com/) - STM32 HAL库
- [U8g2](https://github.com/olikraus/u8g2) - 图形显示库
- [CMake](https://cmake.org/) - 构建系统
- [ARM](https://www.arm.com/) - Cortex-M3处理器

## 📈 项目状态

- ✅ 基础功能完成
- ✅ 硬件驱动完成
- ✅ UI界面完成
- ✅ 温度保护完成
- 🚧 USB通信功能开发中
- 📋 PC端控制软件计划中

---

如有问题或建议，欢迎通过Issue联系我们！
