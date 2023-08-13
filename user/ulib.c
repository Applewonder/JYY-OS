#include "ulib.h"

size_t strlen(const char *s) {
    size_t len = 0;
    const char *p = s;
    for (; *p; p++, len++) {}
    return len;
}
