#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
双色温LED查找表生成器
解决混合色温时亮度不一致的问题
"""

import math

# 配置参数
COLOR_TEMP_MIN = 3000  # 最低色温 (K)
COLOR_TEMP_MAX = 5700  # 最高色温 (K)
COLOR_TEMP_STEP = 50  # 色温步进 (K)
BRIGHTNESS_MAX = 100   # 最大亮度值
PWM_MAX = 32766        # PWM最大值

# 伽马校正表 (从您的代码复制)
GAMMA_TABLE = [
    256, 256, 257, 258, 260, 263, 268, 275, 284, 295, 308, 324, 343, 364, 389, 418, 
    450, 485, 525, 569, 618, 671, 728, 791, 859, 932, 1010, 1094, 1184, 1280, 1382, 
    1490, 1605, 1726, 1854, 1989, 2131, 2281, 2438, 2602, 2775, 2955, 3144, 3340, 3545, 
    3759, 3981, 4212, 4453, 4702, 4961, 5229, 5507, 5795, 6092, 6400, 6718, 7046, 7385, 
    7735, 8095, 8466, 8849, 9242, 9648, 10064, 10493, 10933, 11385, 11849, 12326, 12815, 
    13317, 13831, 14358, 14898, 15451, 16018, 16598, 17191, 17798, 18419, 19054, 19703, 
    20366, 21044, 21736, 22443, 23164, 23900, 24652, 25418, 26200, 26998, 27811, 28639, 
    29484, 30344, 31221, 32114, 32766
]

def color_temp_to_mired(color_temp):
    """色温转换为mired值"""
    return 1000000 / color_temp

def calculate_mixing_ratio(target_temp, temp_min, temp_max):
    """
    计算混合比例
    使用mired值进行线性插值以获得更好的视觉效果
    """
    mired_target = color_temp_to_mired(target_temp)
    mired_min = color_temp_to_mired(temp_min)
    mired_max = color_temp_to_mired(temp_max)
    
    # 注意：mired_max < mired_min (因为色温越高，mired越小)
    ratio = (mired_target - mired_min) / (mired_max - mired_min)
    return max(0, min(1, ratio))

def calculate_brightness_compensation(ratio):
    """
    计算亮度补偿系数
    当混合两个通道时，需要增加总功率以保持相同的视觉亮度
    这个函数可以根据实际LED特性进行调整
    """
    if ratio == 0 or ratio == 1:
        # 单通道工作，不需要补偿
        return 1.0
    else:
        # 混合工作时需要补偿
        # 这里使用一个经验公式，您可以根据实际测试调整
        compensation = 1.0 / math.sqrt(ratio * ratio + (1 - ratio) * (1 - ratio))
        return min(compensation, 1.5)  # 限制最大补偿倍数

def generate_lookup_table():
    """生成查找表"""
    
    # 计算色温数组
    color_temps = []
    temp = COLOR_TEMP_MIN
    while temp <= COLOR_TEMP_MAX:
        color_temps.append(temp)
        temp += COLOR_TEMP_STEP
    
    print(f"// 双色温LED查找表")
    print(f"// 色温范围: {COLOR_TEMP_MIN}K - {COLOR_TEMP_MAX}K, 步进: {COLOR_TEMP_STEP}K")
    print(f"// 亮度范围: 0 - {BRIGHTNESS_MAX}")
    print(f"// 数组大小: {len(color_temps)} x {BRIGHTNESS_MAX + 1}")
    print()
    
    # 生成结构体定义
    print("typedef struct {")
    print("  uint16_t ch1PWM;  // 3000K通道PWM值")
    print("  uint16_t ch2PWM;  // 5700K通道PWM值") 
    print("} TableItem;")
    print()
    
    # 生成色温索引映射
    print("// 色温值到索引的映射")
    print(f"#define TEMP_TABLE_SIZE {len(color_temps)}")
    print(f"#define TEMP_MIN_INDEX 0")
    print(f"#define TEMP_MAX_INDEX {len(color_temps) - 1}")
    print()
    
    # 生成查找表
    print("const TableItem colorTempTable[TEMP_TABLE_SIZE][BRIGHTNESS_MAX + 1] = {")
    
    for temp_idx, color_temp in enumerate(color_temps):
        print(f"  // {color_temp}K")
        print("  {", end="")
        
        for brightness in range(BRIGHTNESS_MAX + 1):
            if brightness == 0:
                ch1_pwm = 0
                ch2_pwm = 0
            else:
                # 计算混合比例
                ratio = calculate_mixing_ratio(color_temp, COLOR_TEMP_MIN, COLOR_TEMP_MAX)
                
                # 计算亮度补偿
                compensation = calculate_brightness_compensation(ratio)
                
                # 计算每个通道的亮度百分比
                if ratio == 0:
                    # 完全是3000K
                    ch1_brightness = min(brightness * compensation, BRIGHTNESS_MAX)
                    ch2_brightness = 0
                elif ratio == 1:
                    # 完全是5700K
                    ch1_brightness = 0
                    ch2_brightness = min(brightness * compensation, BRIGHTNESS_MAX)
                else:
                    # 混合色温
                    total_brightness = brightness * compensation
                    ch1_brightness = min(total_brightness * (1 - ratio), BRIGHTNESS_MAX)
                    ch2_brightness = min(total_brightness * ratio, BRIGHTNESS_MAX)
                
                # 转换为整数并应用伽马校正
                ch1_brightness = int(ch1_brightness)
                ch2_brightness = int(ch2_brightness)
                
                ch1_pwm = GAMMA_TABLE[ch1_brightness] if ch1_brightness > 0 else 0
                ch2_pwm = GAMMA_TABLE[ch2_brightness] if ch2_brightness > 0 else 0
            
            print(f"{{{ch1_pwm:5d}, {ch2_pwm:5d}}}", end="")
            if brightness < BRIGHTNESS_MAX:
                print(", ", end="")
            if (brightness + 1) % 5 == 0 and brightness < BRIGHTNESS_MAX:
                print()
                print("   ", end="")
        
        print("}")
        if temp_idx < len(color_temps) - 1:
            print("  ,")
        else:
            print()
    
    print("};")
    print()
    
    # 生成色温到索引的转换函数
    print("// 色温值转换为表格索引")
    print("int colorTempToIndex(uint16_t colorTemp) {")
    print(f"  if (colorTemp <= {COLOR_TEMP_MIN}) return 0;")
    print(f"  if (colorTemp >= {COLOR_TEMP_MAX}) return TEMP_MAX_INDEX;")
    print(f"  return (colorTemp - {COLOR_TEMP_MIN}) / {COLOR_TEMP_STEP};")
    print("}")

if __name__ == "__main__":
    generate_lookup_table()