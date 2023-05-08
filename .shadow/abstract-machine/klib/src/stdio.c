#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>


#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int print_int(int n) {
  int count = 0;

  if (n < 0) {
      putch('-');
      n = -n;
      count++;
  }

  if (n / 10) {
      count += print_int(n / 10);
  }

  putch('0' + n % 10);
  count++;

  return count;
}


int print_pointer(void *ptr) {
    uintptr_t value = (uintptr_t)ptr;
    int count = 0;

    putch('0');
    putch('x');
    count += 2;

    bool leading_zeros = true;

    for (int shift = (sizeof(uintptr_t) << 3) - 4; shift >= 0; shift -= 4) {
        char hex_digit = (value >> shift) & 0xF;

        if (hex_digit || !leading_zeros || shift == 0) {
            putch(hex_digit < 10 ? '0' + hex_digit : 'a' + hex_digit - 10);
            count++;
            leading_zeros = false;
        }
    }

    return count;
}


int printf(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  const char* format = fmt;
  int count = 0;

  while (*format != '\0') {
      if (*format == '%') {
          format++; // 跳过 '%'

          // 根据格式说明符处理参数
          switch (*format) {
              case 'd': {
                  int int_arg = va_arg(args, int);
                  count += print_int(int_arg);
                  break;
              }
              case 'c': {
                  int char_arg = va_arg(args, int);
                  putch(char_arg);
                  count++;
                  break;
              }
              case 's': {
                  char *str_arg = va_arg(args, char*);
                  while (*str_arg) {
                      putch(*str_arg++);
                      count++;
                  }
                  break;
              }
              case 'p': {
                    void *ptr_arg = va_arg(args, void*);
                    count += print_pointer(ptr_arg);
                    break;
              }
              default:
                  putch('%');
                  count++;
                  break;
          }
      } else {
          putch(*format);
          count++;
      }

      format++;
  }

  va_end(args);

  return count;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  panic("Not implemented");
}

int sprintf(char *out, const char *fmt, ...) {
  panic("Not implemented");
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#endif
