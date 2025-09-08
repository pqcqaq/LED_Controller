import math

# NTC参数定义
R0_OHMS = 100000  # 25℃时NTC电阻值 (100K)
T0_KELVIN = 298.15  # 25℃对应的开尔文温度
B_VALUE = 3950  # B值
R_PULLUP = 100000  # 上拉电阻值 (100K)
ADC_MAX = 1<<12  # 12位ADC最大值

def adc_to_resistance(adc_value):
    """从ADC值计算NTC电阻值"""
    if adc_value == 0 or adc_value >= ADC_MAX:
        return 0
    
    denominator = ADC_MAX - adc_value
    if denominator == 0:
        return 0
    
    return R_PULLUP * adc_value / denominator

def resistance_to_temperature(r_ntc):
    """从电阻值计算温度"""
    if r_ntc <= 0:
        return -999
    
    # 使用B值公式: 1/T = 1/T0 + (1/B) * ln(R/R0)
    r_ratio = r_ntc / R0_OHMS
    ln_r_ratio = math.log(r_ratio)
    
    inv_t = (1/T0_KELVIN) + (ln_r_ratio / B_VALUE)
    temp_kelvin = 1 / inv_t
    temp_celsius = temp_kelvin - 273.15
    
    return temp_celsius

def adc_to_temperature(adc_value):
    """ADC值转换为温度值"""
    r_ntc = adc_to_resistance(adc_value)
    if r_ntc == 0:
        return -999
    return resistance_to_temperature(r_ntc)

# 分析ADC范围对应的温度范围
print("分析ADC与温度的对应关系...")
temps = []
for adc in range(1, ADC_MAX):
    temp = adc_to_temperature(adc)
    if temp != -999:
        temps.append((adc, temp))

if temps:
    min_temp = min(temps, key=lambda x: x[1])
    max_temp = max(temps, key=lambda x: x[1])
    print(f"实际温度范围: {min_temp[1]:.1f}°C (ADC={min_temp[0]}) 到 {max_temp[1]:.1f}°C (ADC={max_temp[0]})")

# 生成稀疏查找表 - 每16个ADC值一个条目
def generate_sparse_table(step=16):
    """生成稀疏查找表"""
    table_adc = []
    table_temp = []
    
    # 找到有效的ADC范围
    valid_range = []
    for adc in range(1, ADC_MAX, step):
        temp = adc_to_temperature(adc)
        if temp != -999 and -100 <= temp <= 200:  # 限制在合理范围内
            valid_range.append((adc, temp))
    
    if not valid_range:
        print("警告: 没有找到有效的温度范围!")
        return [], []
    
    # 扩展到目标温度范围 (-100 到 200度)
    # 计算需要的电阻值来覆盖目标温度范围
    target_temps = list(range(-100, 201, 10))  # -100到200度，每10度一个点
    
    for target_temp in target_temps:
        # 从温度反推电阻值
        temp_kelvin = target_temp + 273.15
        inv_t_target = 1 / temp_kelvin
        inv_t0 = 1 / T0_KELVIN
        
        ln_r_ratio = (inv_t_target - inv_t0) * B_VALUE
        r_ratio = math.exp(ln_r_ratio)
        r_ntc = r_ratio * R0_OHMS
        
        # 从电阻值反推ADC值
        # r_ntc = R_PULLUP * adc / (ADC_MAX - adc)
        # adc = r_ntc * ADC_MAX / (r_ntc + R_PULLUP)
        if r_ntc > 0:
            adc_calc = (r_ntc * ADC_MAX) / (r_ntc + R_PULLUP)
            adc_calc = int(round(adc_calc))
            
            if 1 <= adc_calc < ADC_MAX:
                # 验证计算的准确性
                verify_temp = adc_to_temperature(adc_calc)
                if abs(verify_temp - target_temp) < 5:  # 误差小于5度
                    table_adc.append(adc_calc)
                    table_temp.append(int(target_temp * 100))  # 温度*100
    
    # 移除重复项并排序
    combined = list(zip(table_adc, table_temp))
    combined = sorted(list(set(combined)))
    
    if combined:
        table_adc, table_temp = zip(*combined)
        return list(table_adc), list(table_temp)
    else:
        # 如果扩展失败，使用实际测量范围
        for adc, temp in valid_range:
            table_adc.append(adc)
            table_temp.append(int(temp * 100))
        
        return table_adc, table_temp

# 生成查找表
adc_values, temp_values = generate_sparse_table()

print(f"\n生成的查找表大小: {len(adc_values)} 个条目")
if adc_values:
    print(f"ADC范围: {min(adc_values)} - {max(adc_values)}")
    print(f"温度范围: {min(temp_values)/100:.1f}°C - {max(temp_values)/100:.1f}°C")

# 生成C代码
print("\n// ===== 复制以下代码到Arduino项目中 =====")
print()
print("// 温度查找表定义")
print(f"#define TEMP_TABLE_SIZE {len(adc_values)}")
print()
print("// ADC值查找表")
print("const uint16_t PROGMEM tempTableADC[] = {")
for i in range(0, len(adc_values), 8):
    line_vals = adc_values[i:i+8]
    line_str = ", ".join(f"{val:4d}" for val in line_vals)
    if i + 8 < len(adc_values):
        print(f"  {line_str},")
    else:
        print(f"  {line_str}")
print("};")
print()
print("// 对应的温度值查找表 (温度*100)")
print("const int16_t PROGMEM tempTableTemp[] = {")
for i in range(0, len(temp_values), 8):
    line_vals = temp_values[i:i+8]
    line_str = ", ".join(f"{val:6d}" for val in line_vals)
    if i + 8 < len(temp_values):
        print(f"  {line_str},")
    else:
        print(f"  {line_str}")
print("};")

print("""
/**
 * @brief 使用查找表和线性插值计算温度
 * @param adc_value 12位ADC采样值 (0-4095)
 * @return 温度值×100 (例如: 2550表示25.5℃)
 *         返回-99900表示传感器开路或超出测量范围
 */
int32_t adc_to_temperature_fast(uint16_t adc_value) {
    if (adc_value == 0 || adc_value >= 4095) {
        return -99900L;
    }
    
    // 查找表二分搜索
    int left = 0;
    int right = TEMP_TABLE_SIZE - 1;
    
    // 边界检查
    uint16_t min_adc = pgm_read_word(&tempTableADC[0]);
    uint16_t max_adc = pgm_read_word(&tempTableADC[TEMP_TABLE_SIZE - 1]);
    
    if (adc_value <= min_adc) {
        return pgm_read_word(&tempTableTemp[0]);
    }
    if (adc_value >= max_adc) {
        return pgm_read_word(&tempTableTemp[TEMP_TABLE_SIZE - 1]);
    }
    
    // 二分查找
    while (left < right - 1) {
        int mid = (left + right) / 2;
        uint16_t mid_adc = pgm_read_word(&tempTableADC[mid]);
        
        if (adc_value <= mid_adc) {
            right = mid;
        } else {
            left = mid;
        }
    }
    
    // 线性插值
    uint16_t adc1 = pgm_read_word(&tempTableADC[left]);
    uint16_t adc2 = pgm_read_word(&tempTableADC[right]);
    int16_t temp1 = pgm_read_word(&tempTableTemp[left]);
    int16_t temp2 = pgm_read_word(&tempTableTemp[right]);
    
    if (adc2 == adc1) {
        return temp1;
    }
    
    // 线性插值: temp = temp1 + (temp2-temp1) * (adc-adc1) / (adc2-adc1)
    int32_t temp_diff = temp2 - temp1;
    int32_t adc_diff = adc2 - adc1;
    int32_t adc_offset = adc_value - adc1;
    
    int32_t result = temp1 + (temp_diff * adc_offset) / adc_diff;
    
    return result;
}

/**
 * @brief 获取温度的整数部分
 * @param temp_x100 温度值×100
 * @return 温度整数部分
 */
int32_t get_temperature_int(int32_t temp_x100) {
    return temp_x100 / 100;
}

/**
 * @brief 获取温度的小数部分
 * @param temp_x100 温度值×100
 * @return 温度小数部分×100
 */
uint32_t get_temperature_frac(int32_t temp_x100) {
    int32_t abs_temp = (temp_x100 < 0) ? -temp_x100 : temp_x100;
    return abs_temp % 100;
}""")

# 生成测试代码
print("""
// ===== 测试代码 =====
void test_temperature_table() {
    Serial.begin(115200);
    Serial.println("温度查找表测试:");
    
    // 测试几个ADC值
    uint16_t test_adcs[] = {100, 500, 1000, 1500, 2000, 2500, 3000, 3500, 4000};
    
    for (int i = 0; i < 9; i++) {
        uint16_t adc = test_adcs[i];
        int32_t temp = adc_to_temperature_fast(adc);
        
        if (temp != -99900) {
            Serial.print("ADC=");
            Serial.print(adc);
            Serial.print(" -> ");
            Serial.print(get_temperature_int(temp));
            Serial.print(".");
            Serial.print(get_temperature_frac(temp));
            Serial.println("°C");
        } else {
            Serial.print("ADC=");
            Serial.print(adc);
            Serial.println(" -> 超出范围");
        }
    }
}""")

print("\n// ===== 代码复制结束 =====")