#ifndef TEST
#include <common.h>

void spin_lock(int *lk) {
  while (1) {
    int value = atomic_xchg(lk, 1);
    if (value == 0) {
      break;
    }
  }
}

void spin_unlock(int *lk) {
  atomic_xchg(lk, 0);
}

inline bool try_lock(int *lk) {
  return !atomic_xchg(lk, 1);
}

unsigned long align_to(unsigned long n, unsigned long align) {
    
    unsigned long align_number = 1 << align;
    unsigned long mask = align_number - 1;
    if ((align_number & mask) != 0) {
        printf("align: %x\n", align);
        printf("align_number: %x\n", align_number);
        printf("mask: %x\n", mask);
        panic("align must be power of 2");
    }
    if ((n & mask) == 0) {
        return n;
    } else {
        return (n & ~mask) + align_number;
    }
}

bool is_align_to(void *ptr, unsigned int align) {
  unsigned long align_number = 1 << align;
  unsigned long mask = align_number - 1;
  if ((align_number & mask) != 0) {
    panic("align must be power of 2");
  }
  return ((uintptr_t)ptr & mask) == 0;
}

#endif