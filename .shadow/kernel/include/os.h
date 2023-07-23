#include <common.h>

struct task {
  // TODO
};

struct spinlock {
    my_spinlock_t lock;

    char name[K_LOCK_NAME];        // Name of lock.
    int cpu_num;       // The cpu holding the lock.
};

struct semaphore {
  // TODO
};
