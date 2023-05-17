#ifdef TEST
#ifndef TEST_COMMON
#define TEST_COMMON

#include <kernel.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
// #include <klib-macros.h>

#endif

#else

#include_next <include/common.h>

#endif