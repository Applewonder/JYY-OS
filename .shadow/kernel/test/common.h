#ifdef TEST
#ifndef TEST_COMMON
#define TEST_COMMON

#include <kernel.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include "am.h"

// typedef unsigned long uintptr_t;
#ifdef __DEBUG
    #define ENTER_FUNC(format, ...) \
        printf("\033[32mTHREAD: #%d, ENTER FUNCTION: %s, LINE %d, FILE: %s\n\033[0m"format, cpu_current(), __func__, __LINE__, __FILE__, ##__VA_ARGS__)
    #define SPIN_LOCK(format, ...) \
        printf("\033[33mTHREAD: #%d, LOCK IN FUNCTION: %s, LINE %d, FILE: %s\n\033[0m"format, cpu_current(), __func__, __LINE__, __FILE__, ##__VA_ARGS__)
    #define LEAVE_FUNC(format, ...) \
        printf("\033[32mTHREAD: #%d, LEAVE FUNCTION: %s, LINE %d, FILE: %s\n\033[0m"format, cpu_current(), __func__, __LINE__, __FILE__, ##__VA_ARGS__)
    #define SPIN_UNLOCK(format, ...) \
        printf("\033[33mTHREAD: #%d, UNLOCK IN FUNCTION: %s, LINE %d, FILE: %s\n\033[0m"format, cpu_current(), __func__, __LINE__, __FILE__, ##__VA_ARGS__)
#else
    #define ENTER_FUNC(format, ...)
    #define SPIN_LOCK(format, ...)
    #define LEAVE_FUNC(format, ...)
    #define SPIN_UNLOCK(format, ...)
#endif

#endif

#else

#include_next <include/common.h>

#endif