#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>


#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

#define VBUF_MAX_SIZE 128
#define PBUF_MAX_SIZE 1024

union arg {
  int intarg;
  char chararg;
  char *pchararg;
} uarg;

char vbuf[VBUF_MAX_SIZE];
char pbuf[PBUF_MAX_SIZE];

char *_itos(uint64_t val, char *end, int base, int length) {
    static const char table[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
    *end = 0;
    while (val != 0 || length) {
        if (val == 0 && length < 0) break;
        --end;
        *end = table[val % base];
        val /= base;
        --length;
    }
    if (base == 16) {
        --end;
        *end = 'x';
        --end;
        *end = '0';
    }
    return end;
}

int lock_print = 0;

int printf(const char *fmt, ...) {
    // putstr(fmt);
    //putch('w'); putch('\n');
    char buffer[2048];
    va_list args;
    va_start(args, fmt);
    vsprintf(buffer, fmt, args);
    va_end(args);
    int val = 1;
    int itr = ienabled();
    iset(false);
    while (1) {
        val = atomic_xchg(&lock_print, val);
        if (val == 0) break;
        // while (lock_print)
        //     yield();
        // putch('c');
    }
    putstr(buffer);
    atomic_xchg(&lock_print, val);
    iset(itr);
    return 0;
}

int vsprintf(char *out, const char *fmt, va_list arg) {
    unsigned int i;
    uintptr_t iptr;
    int next = 0;
    char s[50], *ps;
    s[49] = 0;
    // putch('e'); putch('\n');
    for (const char *c = fmt; *c != '\0'; ++c) {
        if (*c != '%') {
            out[next++] = *c;
            continue;
        }
        ++c;
        switch (*c) {
        case 'c':
            i = va_arg(arg, int);
            out[next++] = i;
            break;
        
        case 'l':
            ++c;
        case 'd':
        case 'u':
        
            i = va_arg(arg, int);
            if (i == 0) {
                out[next++] = '0';
                break;
            }
            if (i < 0) {
                i = -i;
                out[next++] = '-';
            }
            ps = &s[49];
            ps = _itos(i, ps, 10, -1);
            for (const char *p = ps; *p; ++p) out[next++] = *p;
            // putstr(ps);
            break;
        
        case 's':
            ps = va_arg(arg, char *);
            for (const char *p = ps; *p; ++p) out[next++] = *p;
            // putstr(ps);
            break;

        case 'p':
            iptr = va_arg(arg, uintptr_t);
            ps = &s[49];
            ps = _itos(iptr, ps, 16, 8);
            for (const char *p = ps; *p; ++p) out[next++] = *p;
            // putstr(ps);
            break;

        default:
            out[next++] = '?';
            // putch('?'); // Fuck you!
        }
    }
    // putch('r'); putch('\n');
    out[next++] = 0;
    return next;
}

int sprintf(char *out, const char *fmt, ...) {
    int ret = 0;
    va_list ap;

    va_start(ap, fmt);
    ret = vsprintf(out, fmt, ap);
    va_end(ap);
    return ret;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
    int ret = 0;
    va_list ap;

    va_start(ap, fmt);
    ret = vsprintf(pbuf, fmt, ap);
    va_end(ap);

    // move n bytes from pbuf to out
    // if ret < n, move ret instead
    if (ret > n) ret = n;
    assert(ret < PBUF_MAX_SIZE);
    strncpy(out, pbuf, ret);
    out[ret] = '\0'; 

    return ret;
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}



#endif
