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

inline int atomic_xchg(volatile int *addr, int newval) {
  int result;
  asm volatile ("lock xchg %0, %1":
    "+m"(*addr), "=a"(result) : "1"(newval) : "memory");
  return result;
}