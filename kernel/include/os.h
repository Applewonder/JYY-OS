#include <common.h>

void push_off();
void pop_off();

typedef struct i_cpu_ I_CPU;

struct i_cpu_ {
  int noff;                   // Depth of push_off() nesting.
  int intena;                 // Were interrupts enabled before push_off()?
};

union task {
  struct {
    int status;
    const char *name;
    union task *next;
    Context   *context;
  };
  uint8_t stack[4096];
} Task;

struct spinlock {
    my_spinlock_t lock;

    char name[K_LOCK_NAME];        // Name of lock.
    int cpu_num;       // The cpu holding the lock.
};

struct semaphore {
  // TODO
  int resource;
  spinlock_t lock;
  task_t* task_list[K_MAX_TASK];
  int task_cnt;
};
