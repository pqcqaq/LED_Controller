#!/usr/bin/env python3
"""
CRC32 表生成器
生成用于EEPROM驱动的CRC32查找表
"""

def generate_crc32_table():
    """生成CRC32查找表"""
    polynomial = 0xEDB88320  # CRC32标准多项式
    table = []
    
    for i in range(256):
        crc = i
        for j in range(8):
            if crc & 1:
                crc = (crc >> 1) ^ polynomial
            else:
                crc >>= 1
        table.append(crc)
    
    return table

def generate_c_header():
    """生成C语言头文件格式的CRC表"""
    table = generate_crc32_table()
    
    header = """/**
 * @file crc32_table.h
 * @brief CRC32查找表 - 由Python脚本自动生成
 * @note 请勿手动修改此文件
 */

#ifndef __CRC32_TABLE_H__
#define __CRC32_TABLE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// CRC32查找表 (256个条目)
static const uint32_t crc32_table[256] = {
"""
    
    # 生成表数据，每行8个值
    for i in range(0, 256, 8):
        line = "    "
        for j in range(8):
            if i + j < 256:
                line += f"0x{table[i + j]:08X}"
                if i + j < 255:
                    line += ", "
        header += line + "\n"
    
    header += """};

#ifdef __cplusplus
}
#endif

#endif /* __CRC32_TABLE_H__ */
"""
    
    return header

def main():
    """主函数"""
    print("生成CRC32查找表...")
    
    # 生成C头文件
    c_header = generate_c_header()
    
    # 写入文件
    with open("Application/drivers/crc32_table.h", "w", encoding="utf-8") as f:
        f.write(c_header)
    
    print("CRC32表已生成: Application/drivers/crc32_table.h")
    
    # 生成验证信息
    table = generate_crc32_table()
    print(f"表大小: {len(table)} 条目")
    print(f"第一个值: 0x{table[0]:08X}")
    print(f"最后一个值: 0x{table[255]:08X}")
    
    # 验证CRC计算
    test_data = b"123456789"
    crc = 0xFFFFFFFF
    for byte in test_data:
        crc = table[(crc ^ byte) & 0xFF] ^ (crc >> 8)
    crc ^= 0xFFFFFFFF
    print(f"测试数据 '123456789' 的CRC32: 0x{crc:08X} (应该是 0xCBF43926)")

if __name__ == "__main__":
    main()
