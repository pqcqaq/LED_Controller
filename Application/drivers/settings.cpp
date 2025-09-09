/**
 * @file settings.cpp
 * @brief 重写的简化EEPROM设置系统实现
 * @author User
 * @date 2025-09-09
 */

/* Includes ------------------------------------------------------------------*/
#include "settings.h"
#include "eeprom.h"
#include "i2c.h"
#include <cstdio>
#include <cstring>
#include "utils/custom_types.h"
#include "global/global_objects.h"

/* Private variables ---------------------------------------------------------*/
static EEPROM eeprom_instance;
static bool eeprom_available = false;

/* Private function prototypes -----------------------------------------------*/
static uint32_t calculateChecksum(const SimpleSettings_t *settings);
static bool validateSettings(const SimpleSettings_t *settings);
static void initDefaultSettings(SimpleSettings_t *settings);
static void stateToSettings(const SystemState *state, SimpleSettings_t *settings);
static void settingsToState(const SimpleSettings_t *settings, SystemState *state);

/* Public functions ----------------------------------------------------------*/

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
bool Settings_Load(void* state_ptr) {
  if (!eeprom_available || !state_ptr) {
    return false;
  }

  SystemState *state = (SystemState*)state_ptr;
  SimpleSettings_t settings;

  // 读取主设置
  if (!eeprom_instance.readStruct(EEPROM_ADDR_SETTINGS, settings)) {
    serial_printf("读取主设置失败\r\n");
    
    // 尝试读取备份设置
    if (!eeprom_instance.readStruct(EEPROM_ADDR_BACKUP, settings)) {
      serial_printf("读取备份设置也失败\r\n");
      return false;
    }
    serial_printf("使用备份设置\r\n");
  }

  // 检查是否为首次启动（EEPROM为空）
  if (settings.magic == 0xFFFFFFFF) {
    serial_printf("首次启动，EEPROM为空\r\n");
    return false;
  }

  // 验证设置
  if (!validateSettings(&settings)) {
    serial_printf("设置数据无效 (magic: 0x%08X)\r\n", settings.magic);
    return false;
  }

  // 将设置应用到state
  settingsToState(&settings, state);

  serial_printf("设置加载成功\r\n");
  serial_printf("  亮度: %d, 色温: %dK, 风扇自动: %s\r\n", 
                state->brightness, state->colorTemp, 
                state->fanAuto ? "开启" : "关闭");

  return true;
}

/**
 * @brief 从SystemState保存设置到EEPROM
 */
bool Settings_Save(const void* state_ptr) {
  if (!eeprom_available || !state_ptr) {
    return false;
  }

  const SystemState *state = (const SystemState*)state_ptr;
  SimpleSettings_t settings;

  // 将state转换为设置
  stateToSettings(state, &settings);

  // 计算校验和
  settings.checksum = calculateChecksum(&settings);

  // 保存到主位置
  bool main_ok = eeprom_instance.writeStruct(EEPROM_ADDR_SETTINGS, settings);
  
  // 保存到备份位置
  bool backup_ok = eeprom_instance.writeStruct(EEPROM_ADDR_BACKUP, settings);

  if (!main_ok && !backup_ok) {
    serial_printf("保存设置失败\r\n");
    return false;
  }

  if (!main_ok) {
    serial_printf("主设置保存失败，但备份成功\r\n");
  } else if (!backup_ok) {
    serial_printf("备份设置保存失败，但主设置成功\r\n");
  } else {
    serial_printf("设置保存成功\r\n");
  }

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
  SimpleSettings_t settings;
  
  initDefaultSettings(&settings);
  settingsToState(&settings, state);

  serial_printf("已恢复默认设置\r\n");
  return true;
}

/**
 * @brief 清除EEPROM设置
 */
bool Settings_Erase(void) {
  if (!eeprom_available) {
    return false;
  }

  // 用0xFF填充设置区域
  uint8_t eraseBuffer[sizeof(SimpleSettings_t)];
  memset(eraseBuffer, 0xFF, sizeof(eraseBuffer));

  bool main_ok = eeprom_instance.write(EEPROM_ADDR_SETTINGS, eraseBuffer, sizeof(eraseBuffer));
  bool backup_ok = eeprom_instance.write(EEPROM_ADDR_BACKUP, eraseBuffer, sizeof(eraseBuffer));

  if (main_ok || backup_ok) {
    serial_printf("EEPROM设置已清除\r\n");
    return true;
  } else {
    serial_printf("清除EEPROM设置失败\r\n");
    return false;
  }
}

/* Private functions ---------------------------------------------------------*/

/**
 * @brief 计算校验和（简单的32位加法校验）
 */
static uint32_t calculateChecksum(const SimpleSettings_t *settings) {
  uint32_t sum = 0;
  const uint32_t *data = (const uint32_t *)settings;
  
  // 计算除了最后一个checksum字段的所有字段
  for (int i = 0; i < (sizeof(SimpleSettings_t) - sizeof(uint32_t)) / sizeof(uint32_t); i++) {
    sum += data[i];
  }
  
  return sum;
}

/**
 * @brief 验证设置数据
 */
static bool validateSettings(const SimpleSettings_t *settings) {
  // 检查魔术字
  if (settings->magic != SETTINGS_MAGIC) {
    return false;
  }

  // 检查亮度范围
  if (settings->brightness > 512) {
    return false;
  }

  // 检查色温范围
  if (settings->colorTemp < 3000 || settings->colorTemp > 5700) {
    return false;
  }

  // 检查风扇设置
  if (settings->fanAuto > 1) {
    return false;
  }

  // 验证校验和
  uint32_t calculated_checksum = calculateChecksum(settings);
  if (calculated_checksum != settings->checksum) {
    serial_printf("校验和错误 (期望: 0x%08X, 实际: 0x%08X)\r\n", 
                  calculated_checksum, settings->checksum);
    return false;
  }

  return true;
}

/**
 * @brief 初始化默认设置
 */
static void initDefaultSettings(SimpleSettings_t *settings) {
  memset(settings, 0, sizeof(SimpleSettings_t));

  settings->magic = SETTINGS_MAGIC;
  settings->brightness = 100;         // 默认亮度
  settings->colorTemp = 4500;         // 默认色温
  settings->fanAuto = 1;              // 默认风扇自动控制

  // 计算校验和
  settings->checksum = calculateChecksum(settings);
}

/**
 * @brief 将SystemState转换为设置结构体
 */
static void stateToSettings(const SystemState *state, SimpleSettings_t *settings) {
  memset(settings, 0, sizeof(SimpleSettings_t));
  
  settings->magic = SETTINGS_MAGIC;
  settings->brightness = state->brightness;
  settings->colorTemp = state->colorTemp;
  settings->fanAuto = state->fanAuto ? 1 : 0;
  // checksum会在外部计算
}

/**
 * @brief 将设置结构体转换为SystemState
 */
static void settingsToState(const SimpleSettings_t *settings, SystemState *state) {
  state->brightness = settings->brightness;
  state->colorTemp = settings->colorTemp;
  state->fanAuto = (settings->fanAuto != 0);
}
