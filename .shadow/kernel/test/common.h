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
typedef unsigned long uintptr_t;

#define SPIN_LOCK_INIT 0
#define CPU_NUM 3

int thread_id[CPU_NUM];

void spin_lock(int *lk);

void spin_unlock(int *lk);

long align_to(long n, unsigned int align);

bool is_align_to(void *ptr, unsigned int align);


int cpu_count() {
    return CPU_NUM;
}

int cpu_current() {
    for (size_t i = 0; i < CPU_NUM; i++)
    {
        if (thread_id[i] == pthread_self()) {
            return i;
        }
    }
}

#endif

#else

#include_next <include/common.h>

#endif