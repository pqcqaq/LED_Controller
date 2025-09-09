/**
 * @file settings.h
 * @brief 重写的简化EEPROM设置系统
 * @author User
 * @date 2025-09-09
 */

#ifndef __SETTINGS_H__
#define __SETTINGS_H__

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/

/**
 * @brief 简化的设备配置结构体 - 固定16字节
 */
typedef struct {
    uint32_t magic;           // 魔术字: 0xA5A5C3C3
    uint16_t brightness;      // 亮度值 (0-512)
    uint16_t colorTemp;       // 色温值 (3000-5700K)
    uint8_t fanAuto;         // 风扇自动控制 (0/1)
    uint8_t reserved[3];     // 保留字节
    uint32_t checksum;       // 简单校验和
} __attribute__((packed)) SimpleSettings_t;

/* Exported constants --------------------------------------------------------*/

// EEPROM内存映射
#define EEPROM_ADDR_SETTINGS        0x0000  // 设备设置 (16字节)
#define EEPROM_ADDR_BACKUP          0x0010  // 备份设置 (16字节)

// 配置值
#define SETTINGS_MAGIC              0xA5A5C3C3

/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief 初始化EEPROM和设置系统
 */
bool Settings_Init(void);

/**
 * @brief 从EEPROM加载设置到SystemState
 */
bool Settings_Load(void* state);

/**
 * @brief 从SystemState保存设置到EEPROM
 */
bool Settings_Save(const void* state);

/**
 * @brief 恢复默认设置
 */
bool Settings_RestoreDefaults(void* state);

/**
 * @brief 清除EEPROM设置
 */
bool Settings_Erase(void);

#endif /* __SIMPLE_SETTINGS_H__ */
