#include <common.h>

void push_off();
void pop_off();

typedef struct i_cpu_ I_CPU;
typedef struct cpu_tasks_ CPU_TASKS;

struct i_cpu_ {
  int noff;                   // Depth of push_off() nesting.
  int intena;                 // Were interrupts enabled before push_off()?
};

struct cpu_tasks_ {
  I_CPU interrupt;
  task_t* current_task;
};

union task {
  struct {
    bool block;
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
  char name[K_SEM_NAME];
  volatile int resource;
  volatile int task_cnt;
  spinlock_t lock;//Too slow remove volatile
  volatile task_t* task_list[K_MAX_TASK];
};
