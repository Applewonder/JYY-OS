#ifdef TEST
#ifndef TEST_COMMON
#define TEST_COMMON

#include <kernel.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include "am.h"
typedef int spinlock_t;

#define SPIN_LOCK_INIT 0

void spin_lock(int *lk);

void spin_unlock(int *lk);

long align_to(long n, unsigned int align);

bool is_align_to(void *ptr, unsigned int align);

#endif

#else

#include_next <include/common.h>

#endif