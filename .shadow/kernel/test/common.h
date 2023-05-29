#ifdef TEST
#ifndef TEST_COMMON
#define TEST_COMMON

#include <kernel.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include "am.h"

typedef unsigned long intptr_t;

#endif

#else

#include_next <include/common.h>

#endif