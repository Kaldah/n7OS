#ifndef __PRINTK_H__
#define __PRINTK_H__

#include <stdarg.h>

/**
 * @brief Kernel version of printf that outputs directly to console
 * 
 * @param fmt Format string
 * @param ... Variable arguments
 * @return int 0 on success
 */
int printfk(const char *fmt, ...);

/**
 * @brief Kernel version of vprintf
 * 
 * @param fmt Format string
 * @param args Variable argument list
 * @return int 0 on success
 */
int vprintfk(const char *fmt, va_list args);

/**
 * @brief Kernel version of putchar
 * 
 * @param c Character to print
 * @return int The character printed
 */
int putchark(int c);

/**
 * @brief Kernel version of puts
 * 
 * @param s String to print
 * @return int 0 on success
 */
int putsk(const char *s);

#endif /* __PRINTK_H__ */