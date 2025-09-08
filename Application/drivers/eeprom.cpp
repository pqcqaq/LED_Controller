/**
 * @file eeprom_cpp.cpp
 * @brief 简化版C++ EEPROM驱动类实现
 * @author User
 * @date 2025-09-08
 */

/* Includes ------------------------------------------------------------------*/
#include "eeprom.h"
#include "crc32_table.h"
#include <cstring>

/* EEPROM型号信息表 */
static const struct {
    uint32_t total_size;
    uint16_t page_size;
    uint16_t address_bytes;
} eeprom_specs[] = {
    { 128,   8,  1 },    // EEPROM_TYPE_AT24C01
    { 256,   8,  1 },    // EEPROM_TYPE_AT24C02
    { 512,  16,  1 },    // EEPROM_TYPE_AT24C04
    { 1024, 16,  1 },    // EEPROM_TYPE_AT24C08
    { 2048, 16,  1 },    // EEPROM_TYPE_AT24C16
    { 4096, 32,  2 },    // EEPROM_TYPE_AT24C32
    { 8192, 32,  2 },    // EEPROM_TYPE_AT24C64
    { 16384, 64, 2 },    // EEPROM_TYPE_AT24C128
    { 32768, 64, 2 },    // EEPROM_TYPE_AT24C256
    { 65536, 128, 2 }    // EEPROM_TYPE_AT24C512
};

/* Public functions ----------------------------------------------------------*/

/**
 * @brief 默认构造函数
 */
EEPROM::EEPROM() 
    : hi2c_(nullptr), type_(EEPROM_TYPE_AT24C256), device_address_(0xA0),
      last_error_(EEPROM_OK), last_hal_error_(HAL_OK), initialized_(false),
      write_timeout_(100), read_timeout_(50), write_delay_(5) {
    memset(&info_, 0, sizeof(info_));
}

/**
 * @brief 初始化EEPROM
 */
bool EEPROM::init(I2C_HandleTypeDef *hi2c, EEPROM_Type_t type, uint16_t device_addr) {
    if (hi2c == nullptr) {
        return updateError(EEPROM_ERROR_PARAM);
    }

    // 检查EEPROM类型是否有效
    if (type >= sizeof(eeprom_specs) / sizeof(eeprom_specs[0])) {
        return updateError(EEPROM_ERROR_PARAM);
    }

    hi2c_ = hi2c;
    type_ = type;
    device_address_ = device_addr;

    // 获取EEPROM信息
    if (!getInfoFromType(type, &info_)) {
        return updateError(EEPROM_ERROR_PARAM);
    }

    // 检测设备是否就绪
    initialized_ = isReady();
    return initialized_;
}

/**
 * @brief 检查设备是否就绪
 */
bool EEPROM::isReady() const {
    if (hi2c_ == nullptr) {
        return updateError(EEPROM_ERROR_PARAM);
    }

    HAL_StatusTypeDef hal_status = HAL_I2C_IsDeviceReady(hi2c_, device_address_, 3, read_timeout_);
    last_hal_error_ = hal_status;
    return updateError(HALToEEPROM(hal_status));
}

/**
 * @brief 读取多个字节
 */
bool EEPROM::read(uint16_t address, uint8_t *buffer, uint16_t size) const {
    if (!initialized_ || buffer == nullptr || size == 0) {
        return updateError(EEPROM_ERROR_PARAM);
    }

    if (address + size > info_.total_size) {
        return updateError(EEPROM_ERROR_ADDRESS);
    }

    HAL_StatusTypeDef hal_status;

    if (info_.address_bytes == 1) {
        // 8位地址
        uint8_t mem_addr = (uint8_t)address;
        hal_status = HAL_I2C_Mem_Read(hi2c_, device_address_, mem_addr,
                                     I2C_MEMADD_SIZE_8BIT, buffer, size, read_timeout_);
    } else {
        // 16位地址
        hal_status = HAL_I2C_Mem_Read(hi2c_, device_address_, address,
                                     I2C_MEMADD_SIZE_16BIT, buffer, size, read_timeout_);
    }

    last_hal_error_ = hal_status;
    return updateError(HALToEEPROM(hal_status));
}

/**
 * @brief 写入多个字节 (分页写入)
 */
bool EEPROM::write(uint16_t address, const uint8_t *buffer, uint16_t size) {
    if (!initialized_ || buffer == nullptr || size == 0) {
        return updateError(EEPROM_ERROR_PARAM);
    }

    if (address + size > info_.total_size) {
        return updateError(EEPROM_ERROR_ADDRESS);
    }

    uint16_t bytes_written = 0;

    while (bytes_written < size) {
        uint16_t current_address = address + bytes_written;
        uint16_t page_address = getPageAddress(current_address);
        uint8_t page_offset = getPageOffset(current_address);
        
        // 计算当前页可写入的字节数
        uint16_t bytes_to_page_end = info_.page_size - page_offset;
        uint16_t bytes_to_write = (size - bytes_written > bytes_to_page_end) ? 
                                  bytes_to_page_end : (size - bytes_written);

        // 执行页写入
        if (!writePage(page_address, page_offset, &buffer[bytes_written], (uint8_t)bytes_to_write)) {
            return false;
        }

        bytes_written += bytes_to_write;
        
        // 写入延迟
        if (bytes_written < size) {
            HAL_Delay(write_delay_);
        }
    }

    return true;
}

/**
 * @brief 页写入
 */
bool EEPROM::writePage(uint16_t page_addr, uint8_t offset, const uint8_t *buffer, uint8_t size) {
    if (!initialized_ || buffer == nullptr || size == 0) {
        return updateError(EEPROM_ERROR_PARAM);
    }

    if (offset + size > info_.page_size) {
        return updateError(EEPROM_ERROR_SIZE);
    }

    uint16_t address = page_addr + offset;
    if (address >= info_.total_size) {
        return updateError(EEPROM_ERROR_ADDRESS);
    }

    HAL_StatusTypeDef hal_status;

    if (info_.address_bytes == 1) {
        // 8位地址
        uint8_t mem_addr = (uint8_t)address;
        hal_status = HAL_I2C_Mem_Write(hi2c_, device_address_, mem_addr,
                                      I2C_MEMADD_SIZE_8BIT, (uint8_t*)buffer, size, write_timeout_);
    } else {
        // 16位地址
        hal_status = HAL_I2C_Mem_Write(hi2c_, device_address_, address,
                                      I2C_MEMADD_SIZE_16BIT, (uint8_t*)buffer, size, write_timeout_);
    }

    last_hal_error_ = hal_status;
    
    if (hal_status == HAL_OK) {
        // 等待写入完成
        EEPROM_Status_t wait_status = waitForReady(write_timeout_);
        if (wait_status != EEPROM_OK) {
            return updateError(wait_status);
        }
    }

    return updateError(HALToEEPROM(hal_status));
}

/**
 * @brief 计算CRC32校验码
 */
uint32_t EEPROM::calculateCRC32(const uint8_t* data, uint32_t length) {
    uint32_t crc = 0xFFFFFFFF;
    
    for (uint32_t i = 0; i < length; i++) {
        uint8_t byte = data[i];
        crc = crc32_table[(crc ^ byte) & 0xFF] ^ (crc >> 8);
    }
    
    return crc ^ 0xFFFFFFFF;
}

/* Private functions ---------------------------------------------------------*/

/**
 * @brief 更新错误状态
 */
bool EEPROM::updateError(EEPROM_Status_t status) const {
    last_error_ = status;
    return (status == EEPROM_OK);
}

/**
 * @brief 将HAL状态转换为EEPROM状态
 */
EEPROM_Status_t EEPROM::HALToEEPROM(HAL_StatusTypeDef hal_status) const {
    switch (hal_status) {
        case HAL_OK:
            return EEPROM_OK;
        case HAL_TIMEOUT:
            return EEPROM_ERROR_TIMEOUT;
        case HAL_BUSY:
            return EEPROM_ERROR_BUSY;
        default:
            return EEPROM_ERROR_HAL;
    }
}

/**
 * @brief 从EEPROM类型获取信息
 */
bool EEPROM::getInfoFromType(EEPROM_Type_t type, EEPROM_Info_t *info) {
    if (info == nullptr || type >= sizeof(eeprom_specs) / sizeof(eeprom_specs[0])) {
        return false;
    }

    info->total_size = eeprom_specs[type].total_size;
    info->page_size = eeprom_specs[type].page_size;
    info->page_count = info->total_size / info->page_size;
    info->address_bytes = eeprom_specs[type].address_bytes;

    return true;
}

/**
 * @brief 等待EEPROM就绪
 */
EEPROM_Status_t EEPROM::waitForReady(uint32_t timeout_ms) {
    uint32_t start_tick = HAL_GetTick();
    
    while ((HAL_GetTick() - start_tick) < timeout_ms) {
        HAL_StatusTypeDef hal_status = HAL_I2C_IsDeviceReady(hi2c_, device_address_, 1, 1);
        
        if (hal_status == HAL_OK) {
            return EEPROM_OK;
        }
        
        HAL_Delay(1);
    }
    
    return EEPROM_ERROR_TIMEOUT;
}

/**
 * @brief 获取页地址
 */
uint16_t EEPROM::getPageAddress(uint16_t address) const {
    return (address / info_.page_size) * info_.page_size;
}

/**
 * @brief 获取页内偏移
 */
uint8_t EEPROM::getPageOffset(uint16_t address) const {
    return (uint8_t)(address % info_.page_size);
}
