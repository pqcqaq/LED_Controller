/**
 * @file custom_types.h
 * @brief Custom type definitions to replace std library types
 * @author User
 * @date 2025-09-06
 */

#ifndef __CUSTOM_TYPES_H__
#define __CUSTOM_TYPES_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stddef.h>

/* Type definitions ----------------------------------------------------------*/

// Custom string view implementation (simple version)
typedef struct {
    const char* data;
    size_t size;
} string_view_t;

/* Function prototypes -------------------------------------------------------*/

/**
 * @brief Create a string view from a C string
 * @param str C string
 * @return string_view_t structure
 */
string_view_t make_string_view(const char* str);

/**
 * @brief Get the size of a string view
 * @param sv string view
 * @return size of the string
 */
size_t string_view_size(const string_view_t* sv);

/**
 * @brief Get the data pointer of a string view
 * @param sv string view
 * @return pointer to string data
 */
const char* string_view_data(const string_view_t* sv);

/**
 * @brief Custom printf-like function without std library dependencies
 * @param format Format string
 * @param ... Variable arguments
 * @return Number of characters printed
 */
int serial_printf(const char* format, ...);

/**
 * @brief Print a string view
 * @param sv string view to print
 * @return Number of characters printed
 */
int print_string_view(const string_view_t* sv);

#ifdef __cplusplus
}
#endif

#endif /* __CUSTOM_TYPES_H__ */
