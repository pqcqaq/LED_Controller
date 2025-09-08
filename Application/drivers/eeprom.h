/**
 * @file eeprom_cpp.h
 * @brief 简化版C++ EEPROM驱动类
 * @author User
 * @date 2025-09-08
 */

#ifndef __EEPROM_CPP_H__
#define __EEPROM_CPP_H__

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"
#include <stdint.h>

/**
 * @brief EEPROM 型号枚举
 */
typedef enum {
    EEPROM_TYPE_AT24C01 = 0,    // 128 bytes,  页大小: 8字节
    EEPROM_TYPE_AT24C02,        // 256 bytes,  页大小: 8字节
    EEPROM_TYPE_AT24C04,        // 512 bytes,  页大小: 16字节
    EEPROM_TYPE_AT24C08,        // 1KB,        页大小: 16字节
    EEPROM_TYPE_AT24C16,        // 2KB,        页大小: 16字节
    EEPROM_TYPE_AT24C32,        // 4KB,        页大小: 32字节
    EEPROM_TYPE_AT24C64,        // 8KB,        页大小: 32字节
    EEPROM_TYPE_AT24C128,       // 16KB,       页大小: 64字节
    EEPROM_TYPE_AT24C256,       // 32KB,       页大小: 64字节
    EEPROM_TYPE_AT24C512        // 64KB,       页大小: 128字节
} EEPROM_Type_t;

/**
 * @brief EEPROM 错误代码
 */
typedef enum {
    EEPROM_OK = 0,
    EEPROM_ERROR_HAL,           // HAL库错误
    EEPROM_ERROR_TIMEOUT,       // 超时错误
    EEPROM_ERROR_BUSY,          // 设备忙
    EEPROM_ERROR_PARAM,         // 参数错误
    EEPROM_ERROR_ADDRESS,       // 地址超出范围
    EEPROM_ERROR_SIZE           // 大小错误
} EEPROM_Status_t;

/**
 * @brief EEPROM 信息结构体
 */
typedef struct {
    uint32_t total_size;        // 总容量 (字节)
    uint16_t page_size;         // 页大小 (字节)
    uint16_t page_count;        // 页数量
    uint16_t address_bytes;     // 地址字节数 (1或2)
} EEPROM_Info_t;

/**
 * @brief 简化版C++ EEPROM驱动类
 */
class EEPROM {
public:
    // 构造函数和析构函数
    EEPROM();
    ~EEPROM() = default;

    // 禁用拷贝构造和赋值
    EEPROM(const EEPROM&) = delete;
    EEPROM& operator=(const EEPROM&) = delete;

    // 初始化和配置
    bool init(I2C_HandleTypeDef *hi2c, EEPROM_Type_t type, uint16_t device_addr = 0xA0);
    bool isReady() const;
    bool isInitialized() const { return initialized_; }

    // 基本读写操作
    bool read(uint16_t address, uint8_t *buffer, uint16_t size) const;
    bool write(uint16_t address, const uint8_t *buffer, uint16_t size);

    // 结构体操作 (模板)
    template<typename T>
    bool readStruct(uint16_t address, T &data) const {
        return read(address, reinterpret_cast<uint8_t*>(&data), sizeof(T));
    }

    template<typename T>
    bool writeStruct(uint16_t address, const T &data) {
        return write(address, reinterpret_cast<const uint8_t*>(&data), sizeof(T));
    }

    // 页操作
    bool writePage(uint16_t page_addr, uint8_t offset, const uint8_t *buffer, uint8_t size);

    // 设备信息
    EEPROM_Info_t getInfo() const { return info_; }
    uint32_t getTotalSize() const { return info_.total_size; }
    uint16_t getPageSize() const { return info_.page_size; }

    // 错误处理
    EEPROM_Status_t getLastError() const { return last_error_; }
    HAL_StatusTypeDef getLastHALError() const { return last_hal_error_; }

    // CRC32计算
    static uint32_t calculateCRC32(const uint8_t* data, uint32_t length);

private:
    I2C_HandleTypeDef *hi2c_;
    EEPROM_Type_t type_;
    uint16_t device_address_;
    EEPROM_Info_t info_;
    mutable EEPROM_Status_t last_error_;
    mutable HAL_StatusTypeDef last_hal_error_;
    bool initialized_;
    uint32_t write_timeout_;
    uint32_t read_timeout_;
    uint32_t write_delay_;

    // 内部辅助函数
    bool updateError(EEPROM_Status_t status) const;
    EEPROM_Status_t HALToEEPROM(HAL_StatusTypeDef hal_status) const;
    bool getInfoFromType(EEPROM_Type_t type, EEPROM_Info_t *info);
    EEPROM_Status_t waitForReady(uint32_t timeout_ms);
    uint16_t getPageAddress(uint16_t address) const;
    uint8_t getPageOffset(uint16_t address) const;
};

#endif /* __EEPROM_CPP_H__ */
