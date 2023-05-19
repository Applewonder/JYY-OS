#ifdef TEST
# ifndef MY_AM_
# define MY_AM_

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>



#define SPIN_LOCK_INIT 0

#define CPU_NUM 1

typedef unsigned long uintptr_t;
typedef int spinlock_t;

int cpu_count();

int cpu_current();

int atomic_xchg(volatile int *addr, int newval);
void spin_lock(int *lk);
void spin_unlock(int *lk);

long align_to(long n, unsigned int align);

bool is_align_to(void *ptr, unsigned int align);

# endif

#else

#include_next <./../abstract-machine/am/include/am.h>

#endif