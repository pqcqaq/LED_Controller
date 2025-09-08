#include "devices.h"
#include "drivers/eeprom_integration.h"
#include "global/global_objects.h"
#include "utils/custom_types.h"
#include "global/controller.h"

SystemDeviceAvailable devices;

void Init_Devices() {
  if (devices.eeprom) {
    // // 初始化EEPROM设置系统
    serial_printf("Initializing EEPROM settings system...\r\n");
    if (Settings_Init()) {
      // 尝试从EEPROM加载设置到state
      if (Settings_LoadToState(&state)) {
        serial_printf("Settings loaded from EEPROM successfully\r\n");
      } else {
        serial_printf("Failed to load settings, using defaults\r\n");
        Settings_RestoreDefaults(&state);
      }
    }
  }
}