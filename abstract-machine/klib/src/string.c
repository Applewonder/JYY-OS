#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

void *memset(void *s, int c, size_t n) {
    assert(s != NULL);
    char *p = (char *) s;
    for (size_t i = 0; i < n; *p = c, i++, p++) {
    }
    return s;
}

void *memcpy(void *out, const void *in, size_t n) {
    assert(out != NULL && in != NULL);
    char *p_dst = (char *) out;
    const char *p_src = (const char *) in;
    for (size_t i = 0; i < n; i++, p_dst++, p_src++) {
        *p_dst = *p_src;
    }
    return out;
}

void *memmove(void *dst, const void *src, size_t n) {
    assert(dst != NULL && src != NULL);
    char *p_dst = (char *) dst;
    const char *p_src = (const char *) src;
    if (dst > src) {
        for (int i = n - 1; i >= 0; ++i) {
            p_dst[i] = p_src[i];
        }
    } else {
        for (int i = 0; i < n; ++i) {
            p_dst[i] = p_src[i];
        }
    }
    return dst;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    assert(s1 != NULL && s2 != NULL);
    const char *p_s1 = (const char *) s1;
    const char *p_s2 = (const char *) s2;
    for (size_t i = 0; i < n; i++, p_s1++, p_s2++) {
        if (*p_s1 != *p_s2) {
            return *p_s1 - *p_s2;
        }
    }
    return 0;
}

size_t strlen(const char *s) {
    assert(s != NULL);
    size_t len = 0;
    const char *p = s;
    for (; *p; p++, len++) {}
    return len;
}

char *strcat(char *dst, const char *src) {
    assert(dst != NULL && src != NULL);
    char *p_dst = dst + strlen(dst);
    strcpy(p_dst, src);
    return dst;
}

char *strcpy(char *dst, const char *src) {
    assert(dst != NULL && src != NULL);
    return strncpy(dst, src, strlen(src));
}

char *strncpy(char *dst, const char *src, size_t n) {
    assert(dst != NULL && src != NULL);
    char *p_dst = (char *) dst;
    const char *p_src = (const char *) src;
    for (size_t i = 0; i < n; i++, p_dst++, p_src++) {
        *p_dst = *p_src;
    }
    *p_dst = '\0';
    return dst;
}

int strcmp(const char *s1, const char *s2) {
    assert(s1 != NULL && s2 != NULL);
    size_t len1 = strlen(s1);
    size_t len2 = strlen(s2);
    size_t n = len1 < len2 ? len1 : len2;
    return strncmp(s1, s2, n + 1);
}

int strncmp(const char *s1, const char *s2, size_t n) {
    assert(s1 != NULL && s2 != NULL);
    const char *p_s1 = s1;
    const char *p_s2 = s2;
    for (size_t i = 0; i < n; i++, p_s1++, p_s2++) {
        if (*p_s1 != *p_s2) {
            return *p_s1 - *p_s2;
        }
    }
    return 0;
}

char *strtok(char *s, const char *delim) {
    panic("Not implemented");
}

char *strstr(const char *str1, const char *str2) {
    panic("Not implemented");
}

char *strchr(const char *s, int c) {
    panic("Not implemented");
}

char *strrchr(const char *s, int c) {
    panic("Not implemented");
}

#endif
