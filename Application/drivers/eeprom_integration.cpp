/**
 * @file eeprom_integration.cpp
 * @brief 简化版EEPROM集成到主应用的实现文件
 * @author User
 * @date 2025-09-08
 */

/* Includes ------------------------------------------------------------------*/
#include "eeprom_integration.h"
#include "eeprom.h"
#include "i2c.h"
#include <cstdio>
#include <cstring>
#include "utils/custom_types.h"
#include "global/global_objects.h"

/* Private variables ---------------------------------------------------------*/
static EEPROM eeprom_instance;
static bool eeprom_available = false;
static bool settings_dirty = false;
static uint32_t last_auto_save = 0;

/* Private function prototypes -----------------------------------------------*/
static bool validateSettings(const DeviceSettings_t *settings);
static void initDefaultSettings(DeviceSettings_t *settings);
static void stateToSettings(const SystemState *state, DeviceSettings_t *settings);
static void settingsToState(const DeviceSettings_t *settings, SystemState *state);

/* Public functions ----------------------------------------------------------*/

/**
 * @brief 标记设置为脏状态，需要保存
 */
void Settings_MarkDirty(void) {
  settings_dirty = true;
}

/**
 * @brief 初始化EEPROM和设置系统
 */
bool Settings_Init(void) {
  // 初始化EEPROM
  if (!eeprom_instance.init(&hi2c2, EEPROM_TYPE_AT24C32, 0xA0)) {
    serial_printf("EEPROM初始化失败\r\n");
    eeprom_available = false;
    return false;
  }

  // 检查设备是否就绪
  if (!eeprom_instance.isReady()) {
    serial_printf("EEPROM设备未就绪\r\n");
    eeprom_available = false;
    return false;
  }

  eeprom_available = true;
  serial_printf("EEPROM初始化成功 (%u bytes)\r\n", eeprom_instance.getTotalSize());

  return true;
}

/**
 * @brief 从EEPROM加载设置到SystemState
 */
bool Settings_LoadToState(void* state_ptr) {
  if (!eeprom_available || !state_ptr) {
    return false;
  }

  SystemState *state = (SystemState*)state_ptr;
  DeviceSettings_t settings;

  // 读取设置
  if (!eeprom_instance.readStruct(EEPROM_ADDR_SETTINGS, settings)) {
    serial_printf("读取设置失败\r\n");
    return false;
  }

  // 验证设置
  if (!validateSettings(&settings)) {
    serial_printf("设置数据无效\r\n");
    return false;
  }

  // 将设置应用到state
  settingsToState(&settings, state);

  serial_printf("设置加载成功 (版本: 0x%04X)\r\n", settings.version);
  serial_printf("  亮度: %d, 色温: %dK, 风扇自动: %s\r\n", 
                state->brightness, state->colorTemp, 
                state->fanAuto ? "开启" : "关闭");

  return true;
}

/**
 * @brief 从SystemState保存设置到EEPROM
 */
bool Settings_SaveFromState(const void* state_ptr) {
  if (!eeprom_available || !state_ptr) {
    return false;
  }

  const SystemState *state = (const SystemState*)state_ptr;
  DeviceSettings_t settings;

  // 将state转换为设置
  stateToSettings(state, &settings);

  // 更新CRC
  settings.crc32 = EEPROM::calculateCRC32((const uint8_t *)&settings,
                                          sizeof(DeviceSettings_t) - sizeof(uint32_t));

  // 保存设置
  if (!eeprom_instance.writeStruct(EEPROM_ADDR_SETTINGS, settings)) {
    serial_printf("保存设置失败\r\n");
    return false;
  }

  settings_dirty = false;
  serial_printf("设置保存成功\r\n");
  serial_printf("  亮度: %d, 色温: %dK, 风扇自动: %s\r\n", 
                state->brightness, state->colorTemp, 
                state->fanAuto ? "开启" : "关闭");

  return true;
}

/**
 * @brief 恢复默认设置
 */
bool Settings_RestoreDefaults(void* state_ptr) {
  if (!state_ptr) {
    return false;
  }

  SystemState *state = (SystemState*)state_ptr;
  DeviceSettings_t settings;
  
  initDefaultSettings(&settings);
  settingsToState(&settings, state);
  settings_dirty = true;

  serial_printf("已恢复默认设置\r\n");
  return true;
}

/**
 * @brief 验证设置数据完整性
 */
bool Settings_ValidateIntegrity(void) {
  if (!eeprom_available) {
    return false;
  }

  DeviceSettings_t settings;
  
  if (!eeprom_instance.readStruct(EEPROM_ADDR_SETTINGS, settings)) {
    return false;
  }

  return validateSettings(&settings);
}

/**
 * @brief 自动保存任务
 */
void Settings_AutoSaveTask(const void* state_ptr) {
  const uint32_t AUTO_SAVE_INTERVAL = 30000; // 30秒自动保存

  // 检查是否需要保存
  if (!settings_dirty || !state_ptr) {
    return;
  }

  uint32_t current_time = HAL_GetTick();

  // 检查是否到了自动保存时间
  if (current_time - last_auto_save >= AUTO_SAVE_INTERVAL) {
    if (Settings_SaveFromState(state_ptr)) {
      last_auto_save = current_time;
    }
  }
}

/* Private functions ---------------------------------------------------------*/

/**
 * @brief 验证设置数据
 */
static bool validateSettings(const DeviceSettings_t *settings) {
  if (settings->magic != SETTINGS_MAGIC) {
    return false;
  }

  if (settings->brightness > 512) {  // LED_MAX_BRIGHTNESS = 512
    return false;
  }

  if (settings->colorTemp < 3000 || settings->colorTemp > 5700) {
    return false;
  }

  // 验证CRC
  uint32_t calculated_crc = EEPROM::calculateCRC32(
      (const uint8_t *)settings, sizeof(DeviceSettings_t) - sizeof(uint32_t));

  return (calculated_crc == settings->crc32);
}

/**
 * @brief 初始化默认设置
 */
static void initDefaultSettings(DeviceSettings_t *settings) {
  memset(settings, 0, sizeof(DeviceSettings_t));

  settings->magic = SETTINGS_MAGIC;
  settings->version = SETTINGS_VERSION;
  settings->fanAuto = true;           // 默认风扇自动控制
  settings->brightness = 100;         // 默认亮度
  settings->colorTemp = 4500;         // 默认色温

  // 计算CRC
  settings->crc32 = EEPROM::calculateCRC32(
      (const uint8_t *)settings, sizeof(DeviceSettings_t) - sizeof(uint32_t));
}

/**
 * @brief 将SystemState转换为设置结构体
 */
static void stateToSettings(const SystemState *state, DeviceSettings_t *settings) {
  memset(settings, 0, sizeof(DeviceSettings_t));
  
  settings->magic = SETTINGS_MAGIC;
  settings->version = SETTINGS_VERSION;
  settings->fanAuto = state->fanAuto;
  settings->brightness = state->brightness;
  settings->colorTemp = state->colorTemp;
}

/**
 * @brief 将设置结构体转换为SystemState
 */
static void settingsToState(const DeviceSettings_t *settings, SystemState *state) {
  state->fanAuto = settings->fanAuto;
  state->brightness = settings->brightness;
  state->colorTemp = settings->colorTemp;
  
  // 设置dirty标志，以便在下次loop中重新计算PWM
  settings_dirty = true;
}
