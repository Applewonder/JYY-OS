# include "am.h"

int thread_id[CPU_NUM];

int cpu_count() {
    return CPU_NUM;
}

int cpu_current() {
    for (int i = 0; i < CPU_NUM; i++)
    {
        if (thread_id[i] == pthread_self()) {
            return i;
        }
    }
}