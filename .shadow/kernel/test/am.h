#ifdef TEST
# ifndef MY_AM_
# define MY_AM_

#define CPU_NUM 3
int cpu_count() {
    return CPU_NUM;
}

int cpu_current() {
    return cur_cpu;
}


# endif

#else

#include_next <./../abstract-machine/am/include/am.h>

#endif