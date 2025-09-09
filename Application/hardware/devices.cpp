#include "devices.h"
#include "drivers/iwdg_a.h"
#include "drivers/settings.h"
#include "global/controller.h"
#include "global/global_objects.h"
#include "utils/custom_types.h"

SystemDeviceAvailable devices;

void Init_Devices() {
  if (devices.eeprom) {
    serial_printf("Initializing EEPROM settings system...\r\n");
    if (Settings_Init()) {
      // 尝试从EEPROM加载设置到state
      if (Settings_Load(&state)) {
        serial_printf("Settings loaded from EEPROM successfully\r\n");
      } else {
        serial_printf("Loading defaults (first boot or corrupted data)\r\n");
        Settings_RestoreDefaults(&state);
        // 立即保存默认设置
        Settings_Save(&state);
      }
    }
  }
}