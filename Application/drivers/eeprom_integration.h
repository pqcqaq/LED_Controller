/**
 * @file eeprom_integration.h
 * @brief 简化版EEPROM集成到主应用的头文件
 * @author User
 * @date 2025-09-08
 */

#ifndef __EEPROM_INTEGRATION_H__
#define __EEPROM_INTEGRATION_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/

/**
 * @brief 设备配置结构体 (存储在EEPROM中) - 基于SystemState
 */
typedef struct {
    uint32_t magic;           // 魔术字: 0xDEADBEEF
    uint16_t version;         // 配置版本
    
    // === 核心控制参数 (从SystemState复制) ===
    bool fanAuto;            // 风扇自动控制
    uint16_t brightness;     // 亮度值 (0-LED_MAX_BRIGHTNESS)
    uint16_t colorTemp;      // 色温值 (COLOR_TEMP_MIN - COLOR_TEMP_MAX)
    
    uint32_t crc32;          // CRC32校验码
    uint8_t padding[48];     // 填充到64字节边界
} __attribute__((packed)) DeviceSettings_t;

/* Exported constants --------------------------------------------------------*/

// EEPROM内存映射
#define EEPROM_ADDR_SETTINGS        0x0000  // 设备设置 (64字节)
#define EEPROM_ADDR_USER_DATA       0x0040  // 用户数据区开始

// 默认配置值
#define SETTINGS_MAGIC              0xDEADBEEF
#define SETTINGS_VERSION            0x0100

/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief 初始化EEPROM和设置系统
 * @retval bool 成功返回true
 */
bool Settings_Init(void);

/**
 * @brief 从EEPROM加载设置到SystemState
 * @param state 指向SystemState的指针
 * @retval bool 成功返回true
 */
bool Settings_LoadToState(void* state);

/**
 * @brief 从SystemState保存设置到EEPROM
 * @param state 指向SystemState的指针
 * @retval bool 成功返回true
 */
bool Settings_SaveFromState(const void* state);

/**
 * @brief 恢复默认设置
 * @param state 指向SystemState的指针
 * @retval bool 成功返回true
 */
bool Settings_RestoreDefaults(void* state);

/**
 * @brief 标记设置为脏状态，需要保存
 */
void Settings_MarkDirty(void);

/**
 * @brief 验证设置数据完整性
 * @retval bool 完整返回true
 */
bool Settings_ValidateIntegrity(void);

/**
 * @brief 自动保存任务 (在主循环中调用)
 * @param state 指向SystemState的指针
 */
void Settings_AutoSaveTask(const void* state);

#ifdef __cplusplus
}
#endif

#endif /* __EEPROM_INTEGRATION_H__ */
