#ifdef TEST
# ifndef MY_AM_
# define MY_AM_

#include <pthread.h>

#define CPU_NUM 3

int cpu_count();

int cpu_current();

# endif

#else

#include_next <./../abstract-machine/am/include/am.h>

#endif