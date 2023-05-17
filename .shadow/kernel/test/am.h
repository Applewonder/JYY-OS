#ifdef TEST
# ifndef MY_AM_
# define MY_AM_

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

# endif

#else

#include_next <./../abstract-machine/am/include/am.h>

#endif