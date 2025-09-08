def constrain(value, min_val, max_val):
    """限制数值在指定范围内"""
    return max(min_val, min(max_val, value))

def color_temp_to_mired(color_temp):
    """色温到mired值转换 (避免浮点运算)"""
    return (1000000 * 10) // color_temp  # 返回mired*10

def calculate_channel_brightness(color_temp, brightness, 
                               COLOR_TEMP_MIN=3000, COLOR_TEMP_MAX=5700,
                               LED_TEMP_WEIGHT_TOTAL=1024, 
                               LED_TEMP_SPRI_TOTAL=100,
                               CCT_ADDITIVE_BLEND=50):
    """
    计算双通道LED的亮度分配
    
    参数:
    - color_temp: 色温值 (K)
    - brightness: 亮度 (0-100)
    - COLOR_TEMP_MIN: 最小色温 (默认3000K)
    - COLOR_TEMP_MAX: 最大色温 (默认5700K) 
    - LED_TEMP_WEIGHT_TOTAL: 权重总数 (默认1024)
    - LED_TEMP_SPRI_TOTAL: 混合总数 (默认100)
    - CCT_ADDITIVE_BLEND: 叠加混合比例 (默认50)
    
    返回:
    - (ch1_brightness, ch2_brightness): 两个通道的亮度值 (0-100)
    """
    
    if brightness == 0:
        return (0, 0)
    
    # 限制色温范围
    color_temp = constrain(color_temp, COLOR_TEMP_MIN, COLOR_TEMP_MAX)
    
    # 使用mired值进行线性插值
    mired_target = color_temp_to_mired(color_temp)
    mired_min = color_temp_to_mired(COLOR_TEMP_MIN)  # 3000K -> 3333
    mired_max = color_temp_to_mired(COLOR_TEMP_MAX)  # 5700K -> 1754
    
    # 计算CCT比例 (0-LED_TEMP_WEIGHT_TOTAL)
    cct_ratio = ((mired_target - mired_min) * LED_TEMP_WEIGHT_TOTAL) // (mired_max - mired_min)
    cct_ratio = constrain(cct_ratio, 0, LED_TEMP_WEIGHT_TOTAL)
    
    # 计算各通道的基础权重
    warm_weight = LED_TEMP_WEIGHT_TOTAL - cct_ratio  # ch1 (暖白) 权重
    cold_weight = cct_ratio                          # ch2 (冷白) 权重
    
    # 计算线性混合的亮度
    ch1_linear = (brightness * warm_weight) // LED_TEMP_WEIGHT_TOTAL
    ch2_linear = (brightness * cold_weight) // LED_TEMP_WEIGHT_TOTAL
    
    # 计算叠加混合的亮度
    ch1_additive = brightness if warm_weight > 0 else 0
    ch2_additive = brightness if cold_weight > 0 else 0
    
    # 根据叠加混合比例插值
    ch1_brightness = ((ch1_linear * (LED_TEMP_SPRI_TOTAL - CCT_ADDITIVE_BLEND)) +
                      (ch1_additive * CCT_ADDITIVE_BLEND)) // LED_TEMP_SPRI_TOTAL
    ch2_brightness = ((ch2_linear * (LED_TEMP_SPRI_TOTAL - CCT_ADDITIVE_BLEND)) +
                      (ch2_additive * CCT_ADDITIVE_BLEND)) // LED_TEMP_SPRI_TOTAL
    
    # 限制最大值
    ch1_brightness = min(ch1_brightness, 255)
    ch2_brightness = min(ch2_brightness, 255)
    
    return (ch1_brightness, ch2_brightness)

def calculate_with_gamma_mapping(color_temp, brightness, **kwargs):
    """
    计算双通道LED亮度并进行伽马表映射
    
    返回:
    - (ch1_gamma_index, ch2_gamma_index): 用于伽马表查找的索引值 (0-255)
    """
    ch1_brightness, ch2_brightness = calculate_channel_brightness(color_temp, brightness, **kwargs)
    
    # 将0-100的亮度值映射到0-255的伽马表索引范围
    gamma_ch1 = ch1_brightness
    gamma_ch2 = ch2_brightness
    
    return (gamma_ch1, gamma_ch2)

def debug_calculation(color_temp, brightness, **kwargs):
    """
    详细调试函数，显示计算过程中的所有中间值
    """
    print(f"\n=== 调试计算: 色温={color_temp}K, 亮度={brightness}% ===")
    
    if brightness == 0:
        print("亮度为0，直接返回 (0, 0)")
        return (0, 0)
    
    # 获取参数
    COLOR_TEMP_MIN = kwargs.get('COLOR_TEMP_MIN', 3000)
    COLOR_TEMP_MAX = kwargs.get('COLOR_TEMP_MAX', 5700)
    LED_TEMP_WEIGHT_TOTAL = kwargs.get('LED_TEMP_WEIGHT_TOTAL', 1024)
    LED_TEMP_SPRI_TOTAL = kwargs.get('LED_TEMP_SPRI_TOTAL', 100)
    CCT_ADDITIVE_BLEND = kwargs.get('CCT_ADDITIVE_BLEND', 50)
    
    # 限制色温范围
    color_temp_constrained = constrain(color_temp, COLOR_TEMP_MIN, COLOR_TEMP_MAX)
    print(f"色温限制后: {color_temp_constrained}K")
    
    # mired计算
    mired_target = color_temp_to_mired(color_temp_constrained)
    mired_min = color_temp_to_mired(COLOR_TEMP_MIN)
    mired_max = color_temp_to_mired(COLOR_TEMP_MAX)
    print(f"Mired值: target={mired_target}, min={mired_min}, max={mired_max}")
    
    # CCT比例计算
    cct_ratio = ((mired_target - mired_min) * LED_TEMP_WEIGHT_TOTAL) // (mired_max - mired_min)
    cct_ratio = constrain(cct_ratio, 0, LED_TEMP_WEIGHT_TOTAL)
    print(f"CCT Ratio: {cct_ratio}")
    
    # 权重计算
    warm_weight = LED_TEMP_WEIGHT_TOTAL - cct_ratio
    cold_weight = cct_ratio
    print(f"权重 - 暖白: {warm_weight}, 冷白: {cold_weight}")
    
    # 线性混合
    ch1_linear = (brightness * warm_weight) // LED_TEMP_WEIGHT_TOTAL
    ch2_linear = (brightness * cold_weight) // LED_TEMP_WEIGHT_TOTAL
    print(f"线性混合 - CH1: {ch1_linear}, CH2: {ch2_linear}")
    
    # 叠加混合
    ch1_additive = brightness if warm_weight > 0 else 0
    ch2_additive = brightness if cold_weight > 0 else 0
    print(f"叠加混合 - CH1: {ch1_additive}, CH2: {ch2_additive}")
    
    # 最终插值
    ch1_brightness = ((ch1_linear * (LED_TEMP_SPRI_TOTAL - CCT_ADDITIVE_BLEND)) +
                      (ch1_additive * CCT_ADDITIVE_BLEND)) // LED_TEMP_SPRI_TOTAL
    ch2_brightness = ((ch2_linear * (LED_TEMP_SPRI_TOTAL - CCT_ADDITIVE_BLEND)) +
                      (ch2_additive * CCT_ADDITIVE_BLEND)) // LED_TEMP_SPRI_TOTAL
    
    # 限制最大值
    ch1_brightness = min(ch1_brightness, 255)
    ch2_brightness = min(ch2_brightness, 255)
    print(f"最终亮度 - CH1: {ch1_brightness}, CH2: {ch2_brightness}")
    
    # 伽马表映射
    gamma_ch1 = min((ch1_brightness * 255) // 100, 255)
    gamma_ch2 = min((ch2_brightness * 255) // 100, 255)
    print(f"伽马表索引 - CH1: {gamma_ch1}, CH2: {gamma_ch2}")
    
    return (ch1_brightness, ch2_brightness)

# 测试示例
if __name__ == "__main__":
    # 测试你的日志中的数据
    print("=== 复现你的日志数据 ===")
    result = debug_calculation(5700, 100)
    
    print("\n=== 其他测试用例 ===")
    test_cases = [
        (3000, 100),  # 最暖色温，最大亮度
        (5700, 100),  # 最冷色温，最大亮度
        (4350, 100),  # 中等色温，最大亮度
        (4350, 50),   # 中等色温，中等亮度
        (3000, 0),    # 任意色温，零亮度
    ]
    
    for temp, bright in test_cases:
        ch1, ch2 = calculate_channel_brightness(temp, bright)
        gamma1, gamma2 = calculate_with_gamma_mapping(temp, bright)
        print(f"色温:{temp}K, 亮度:{bright}% -> CH1:{ch1}, CH2:{ch2} (伽马索引: {gamma1}, {gamma2})")