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

long align_to(long n, unsigned int align) {
    
    unsigned int align_number = 1 << align;
    unsigned int mask = align - 1;
    if ((align & mask) != 0) {
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
  unsigned int align_number = 1 << align;
  unsigned int mask = align_number - 1;
  if ((align_number & mask) != 0) {
    panic("align must be power of 2");
  }
  return ((uintptr_t)ptr & mask) == 0;
}

#endif