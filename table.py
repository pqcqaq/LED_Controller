# This script generates a gamma correction lookup table for C++ code.
# The output can be copied and pasted directly into a C++ program to
# replace the floating-point calculations at runtime, saving Flash space
# and CPU cycles during initialization.
#
# 这个脚本用于为C++代码生成伽马校正查找表。
# 生成的输出可以直接复制粘贴到C++程序中，取代运行时的浮点计算，
# 从而节省初始化时的Flash空间和CPU周期。

import math

# PWM parameters, matching the C++ code
# PWM参数，与C++代码保持一致
MAX_PWM = 6100
GAMMA_CORRECTION_VALUE = 2.20

# Define the actual range you want to map to
# 定义你想要映射到的实际范围
MIN_PWM_VALUE = 1  # 最小PWM值 (可以根据需要调整)
MAX_PWM_VALUE = MAX_PWM - 2  # 最大PWM值 (可以根据需要调整)

# Calculate the range
# 计算范围
PWM_RANGE = MAX_PWM_VALUE - MIN_PWM_VALUE

# Generate the lookup table values
# 生成查找表的值
gamma_table = []
for i in range(513):
    # Normalize the input value (0-255) to a float between 0.0 and 1.0
    # 将输入值 (0-255) 归一化到 0.0 到 1.0 之间的浮点数
    normalized_input = i / 512.0

    # Apply gamma correction formula: output = pow(input, gamma)
    # 应用伽马校正公式: output = pow(input, gamma)
    gamma_corrected_value = math.pow(normalized_input, GAMMA_CORRECTION_VALUE)
    
    # Map the gamma corrected value to the desired PWM range
    # 将伽马校正后的值映射到所需的PWM范围
    pwm_value = MIN_PWM_VALUE + int(round(gamma_corrected_value * PWM_RANGE))
    
    # Ensure the value stays within bounds
    # 确保值保持在边界内
    pwm_value = max(MIN_PWM_VALUE, min(pwm_value, MAX_PWM_VALUE))
    
    gamma_table.append(pwm_value)

# Print the table in a format suitable for C++ array initialization
# 以适合C++数组初始化的格式打印表格
print("// Pre-generated gamma correction lookup table for PWM values 0-100%")
print(f"// Maps input range [0, 513] to PWM range [{MIN_PWM_VALUE}, {MAX_PWM_VALUE}]")
print(f"// Gamma value: {GAMMA_CORRECTION_VALUE}")
print(f"const uint16_t gammaTable[513] = {{")

# Print values in rows of 10 for better readability
# 每行打印10个值以提高可读性
for i in range(0, 513, 10):
    row_end = min(i + 10, 513)
    row_values = gamma_table[i:row_end]
    if i == 0:
        print("  " + ", ".join(f"{val:5d}" for val in row_values) + ("," if row_end < 513 else ""))
    else:
        print("  " + ", ".join(f"{val:5d}" for val in row_values) + ("," if row_end < 513 else ""))

print("};")

# Print some useful information
# 打印一些有用信息
print(f"\n// Table info:")
print(f"// Input range: 0-100 (percentage)")
print(f"// Output range: {MIN_PWM_VALUE}-{MAX_PWM_VALUE} (PWM values)")
print(f"// Gamma correction: {GAMMA_CORRECTION_VALUE}")
print(f"// Example: input 0% -> PWM {gamma_table[0]}, input 100% -> PWM {gamma_table[100]}")