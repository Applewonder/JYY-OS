#ifndef TEST
#ifndef COMMON_H
#define COMMON_H

#include <kernel.h>
#include <klib.h>
#include <klib-macros.h>

typedef int my_spinlock_t;

#define SPIN_LOCK_INIT 0
#define K_LOCK_NAME 128
#define K_SEM_NAME 128
#define K_TASK_NAME 128
#define K_MAX_TASK 1024
#define MAX_TASK 1024
#define STACK_SIZE 8000

void spin_lock(int *lk);

void spin_unlock(int *lk);

bool try_lock(int *lk);

unsigned long align_to(unsigned long n, unsigned long align);

bool is_align_to(void *ptr, unsigned int align);

#endif

#else

#include_next <test/common.h>

#endif

