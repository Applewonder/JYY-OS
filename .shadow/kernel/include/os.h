#ifndef __OS_H__
#define __OS_H__
#include <common.h>

void push_off();
void pop_off();

typedef struct i_cpu_ I_CPU;
typedef struct cpu_tasks_ CPU_TASKS;
typedef struct _irq IRQ;

struct i_cpu_ {
  int noff;                   // Depth of push_off() nesting.
  int intena;                 // Were interrupts enabled before push_off()?
};

struct cpu_tasks_ {
  I_CPU interrupt;
  task_t* current_task;
  task_t* save_task;
};

struct spinlock {
    my_spinlock_t lock;

    char name[K_LOCK_NAME];        // Name of lock.
    int cpu_num;       // The cpu holding the lock.
};

struct task {
  uint8_t stack[STACK_SIZE];
  char name[K_TASK_NAME];
  spinlock_t status;
  Context   *context;
  int id;
  bool block;
};

struct semaphore {
  // TODO
  char name[K_SEM_NAME];
  volatile int resource;
  volatile int task_cnt;
  spinlock_t lock;//Too slow remove volatile
  volatile task_t* task_list[K_MAX_TASK];
};

struct _irq {
  // TODO
  int seq;
  int event;
  handler_t handler;
  IRQ* next;
};


#endif