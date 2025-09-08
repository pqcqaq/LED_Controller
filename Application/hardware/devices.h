// devices.h
#ifndef __DEVICES_H__
#define __DEVICES_H__

struct SystemDeviceAvailable {
  bool oled = false;       // OLED显示屏可用
  bool eeprom = false;     // EEPROM可用
  bool extern_adc = false; // 外部ADC可用
};

extern SystemDeviceAvailable devices;

void Init_Devices(void);

#endif // __DEVICES_H__
