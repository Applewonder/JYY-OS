#ifndef __OS_H__
#define __OS_H__
#include <common.h>

#define PTE_P          0x001   // Present
#define PTE_W          0x002   // Writeable
#define PTE_U          0x004   // User
#define PTE_PS         0x080   // Large Page (1 GiB or 2 MiB)

void push_off();
void pop_off();

typedef struct i_cpu_ I_CPU;
typedef struct cpu_tasks_ CPU_TASKS;
typedef struct _irq IRQ;
typedef struct vm_area_struct VME_AREA;
typedef struct mapped_page M_PAGE;
typedef struct pid_queue PID_Q;

struct i_cpu_ {
  int noff;                   // Depth of push_off() nesting.
  int intena;                 // Were interrupts enabled before push_off()?
};

struct cpu_tasks_ {
  I_CPU interrupt;
  task_t* current_task;
  task_t* save_task;
  task_t* idle_task;
};

struct spinlock {
    my_spinlock_t lock;

    char name[K_LOCK_NAME];        // Name of lock.
    int cpu_num;       // The cpu holding the lock.
};

struct vm_area_struct {
    /* VM start and end addresses */
    void* vm_start;
    void* vm_end;

    /* linked list of VM areas per task, sorted by address */
    VME_AREA *vm_next;
    VME_AREA *vm_prev;

    /* The address space we belong to */
    AddrSpace* vm_as;

    /* Flags (protection bits, etc.) */
    uint8_t vm_prot;

    uint8_t vm_flags;
};

struct mapped_page {
    /* The address space we belong to */
    AddrSpace* as;

    /* The virtual address we are mapped at */
    void* va;

    void* pa;

    /* The next page in the list */
    M_PAGE* next;
};

struct task {
  uint8_t stack[STACK_SIZE];
  char name[K_TASK_NAME];
  spinlock_t status;
  Context   *context;
  AddrSpace* as; //add in L3
  VME_AREA* vm_area_head; //add in L3
  VME_AREA* vm_area_tail; //add in L3
  M_PAGE* mapped_page_head; //add in L3
  int ppid;
  int pid;
  bool block;
  bool is_running;
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

struct pid_queue {
  int pid;
  PID_Q* next;
};


#endif