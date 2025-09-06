/**
 * @file custom_types.c
 * @brief Implementation of custom types and functions
 * @author User
 * @date 2025-09-06
 */

/* Includes ------------------------------------------------------------------*/
#include "custom_types.h"
#include "usart.h"
#include <stdarg.h>

/* Private function prototypes -----------------------------------------------*/
static int custom_strlen(const char* str);
static int custom_itoa(int value, char* buffer, int base);
static int custom_utoa(unsigned int value, char* buffer, int base);

/* Public functions ----------------------------------------------------------*/

string_view_t make_string_view(const char* str) {
    string_view_t sv;
    sv.data = str;
    sv.size = custom_strlen(str);
    return sv;
}

size_t string_view_size(const string_view_t* sv) {
    return sv->size;
}

const char* string_view_data(const string_view_t* sv) {
    return sv->data;
}

int serial_printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    char buffer[256];
    int count = 0;
    int i = 0;
    
    while (format[i] != '\0' && count < sizeof(buffer) - 1) {
        if (format[i] == '%' && format[i + 1] != '\0') {
            i++; // skip '%'
            
            switch (format[i]) {
                case 'd': {
                    int value = va_arg(args, int);
                    char num_buffer[12];
                    int len = custom_itoa(value, num_buffer, 10);
                    for (int j = 0; j < len && count < sizeof(buffer) - 1; j++) {
                        buffer[count++] = num_buffer[j];
                    }
                    break;
                }
                case 'u': {
                    unsigned int value = va_arg(args, unsigned int);
                    char num_buffer[12];
                    int len = custom_utoa(value, num_buffer, 10);
                    for (int j = 0; j < len && count < sizeof(buffer) - 1; j++) {
                        buffer[count++] = num_buffer[j];
                    }
                    break;
                }
                case 'c': {
                    char c = (char)va_arg(args, int);
                    buffer[count++] = c;
                    break;
                }
                case 's': {
                    const char* str = va_arg(args, const char*);
                    while (*str != '\0' && count < sizeof(buffer) - 1) {
                        buffer[count++] = *str++;
                    }
                    break;
                }
                case '%': {
                    buffer[count++] = '%';
                    break;
                }
                default: {
                    // Unknown format specifier, just copy it
                    buffer[count++] = '%';
                    if (count < sizeof(buffer) - 1) {
                        buffer[count++] = format[i];
                    }
                    break;
                }
            }
        } else {
            buffer[count++] = format[i];
        }
        i++;
    }
    
    buffer[count] = '\0';
    
    // Send via UART
    HAL_UART_Transmit(&huart2, (uint8_t*)buffer, count, HAL_MAX_DELAY);
    
    va_end(args);
    return count;
}

int print_string_view(const string_view_t* sv) {
    if (sv->data == NULL || sv->size == 0) {
        return 0;
    }
    
    // Send via UART
    HAL_UART_Transmit(&huart2, (uint8_t*)sv->data, sv->size, HAL_MAX_DELAY);
    return sv->size;
}

/* Private functions ---------------------------------------------------------*/

static int custom_strlen(const char* str) {
    int len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

static int custom_itoa(int value, char* buffer, int base) {
    if (base < 2 || base > 36) {
        buffer[0] = '\0';
        return 0;
    }
    
    char* ptr = buffer;
    char tmp_char;
    int tmp_value;
    int len = 0;
    
    // Handle negative numbers for decimal base
    if (value < 0 && base == 10) {
        *ptr++ = '-';
        value = -value;
        len++;
    }
    
    // Convert to string (in reverse order)
    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "0123456789abcdefghijklmnopqrstuvwxyz"[tmp_value - value * base];
        len++;
    } while (value);
    
    *ptr-- = '\0';
    
    // Reverse the string (skip the minus sign if present)
    char* start = (buffer[0] == '-') ? buffer + 1 : buffer;
    while (start < ptr) {
        tmp_char = *ptr;
        *ptr-- = *start;
        *start++ = tmp_char;
    }
    
    return len;
}

static int custom_utoa(unsigned int value, char* buffer, int base) {
    if (base < 2 || base > 36) {
        buffer[0] = '\0';
        return 0;
    }
    
    char* ptr = buffer;
    char tmp_char;
    unsigned int tmp_value;
    int len = 0;
    
    // Convert to string (in reverse order)
    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "0123456789abcdefghijklmnopqrstuvwxyz"[tmp_value - value * base];
        len++;
    } while (value);
    
    *ptr-- = '\0';
    
    // Reverse the string
    while (buffer < ptr) {
        tmp_char = *ptr;
        *ptr-- = *buffer;
        *buffer++ = tmp_char;
    }
    
    return len;
}
