# include "am.h"

unsigned long thread_id[CPU_NUM];

int cpu_count() {
    return CPU_NUM;
}

int cpu_current() {
    for (int i = 0; i < CPU_NUM; i++)
    {
        unsigned long tid = pthread_self();
        if (thread_id[i] == tid) {
            return i;
        }
    }
    printf("NOOOOOOOOO\n");
}

inline int atomic_xchg(volatile int *addr, int newval) {
  int result;
  asm volatile ("lock xchg %0, %1":
    "+m"(*addr), "=a"(result) : "1"(newval) : "memory");
  return result;
}

void spin_lock(int *lk) {
  while (1) {
    int value = atomic_xchg(lk, 1);
    if (value == 0) {
      break;
    }
  }
}

inline bool try_lock(int *lk) {
  return !atomic_xchg(lk, 1);
}

void spin_unlock(int *lk) {
  atomic_xchg(lk, 0);
}

unsigned long align_to(unsigned long n, unsigned long align) {
    
    unsigned long align_number = 1 << align;
    unsigned long mask = align_number - 1;
    if ((align_number & mask) != 0) {
        // printf("align: %x\n", align);
        printf("align_number: %lx\n", align_number);
        printf("mask: %lx\n", mask);
        //panic("align must be power of 2");
    }
    if ((n & mask) == 0) {
        return n;
    } else {
        return (n & ~mask) + align_number;
    }
}

bool is_align_to(void *ptr, unsigned int align) {
  unsigned int align_number = 1 << align;
  unsigned int mask = align_number - 1;
  if ((align_number & mask) != 0) {
    //panic("align must be power of 2");
  }
  return ((uintptr_t)ptr & mask) == 0;
}