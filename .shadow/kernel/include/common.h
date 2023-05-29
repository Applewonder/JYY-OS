#ifndef TEST
#ifndef COMMON_H
#define COMMON_H

#include <kernel.h>
#include <klib.h>
#include <klib-macros.h>

typedef int spinlock_t;

#define SPIN_LOCK_INIT 0

void spin_lock(int *lk);

void spin_unlock(int *lk);

bool try_lock(int *lk);

long align_to(long n, unsigned int align);

bool is_align_to(void *ptr, unsigned int align);

#endif

#else

#include_next <test/common.h>

#endif

