#include "cbma.h"
#include <alloca.h>
#include <common.h>
#include <os.h>

#ifdef DEBUG_DEV
  #include <devices.h>
#endif

#ifdef DEBUG_STARVE
  sem_t empty, fill;
  volatile int judger=0;
  #define P kmt->sem_wait
  #define V kmt->sem_signal
  #define N 1
  #define NPROD 1
  #define NCONS 1
#endif

static IRQ* irq_head = NULL;

#ifdef DEBUG_DEV
static void tty_reader(void *arg) {
  device_t *tty = dev->lookup(arg);
  char cmd[128], resp[128], ps[16];
  snprintf(ps, 16, "(%s) $ ", arg);
  while (1) {
    tty->ops->write(tty, 0, ps, strlen(ps));
    int nread = tty->ops->read(tty, 0, cmd, sizeof(cmd) - 1);
    cmd[nread] = '\0';
    sprintf(resp, "tty reader task: got %d character(s).\n", strlen(cmd));
    tty->ops->write(tty, 0, resp, strlen(resp));
  }
}
#endif

#ifdef DEBUG_PRINT
static void print_task(void *arg) {
  while (1) {
    printf("Hello from print_task!\n");
  }
}
#endif

#ifdef DEBUG_STARVE
// void Tproduce(void *arg) { 
//   while (1) { 
//     P(&empty);
     
//     putch('0' + ++judger); 
//     if (judger > N || judger < 0) {
//       putch(' ');
//     }
//     // putch('(');
//     V(&fill); 
//   } 
// }
// void Tconsume(void *arg) { 
//   while (1) { 
//     P(&fill);  
//     putch('0' + --judger); 
//     if (judger > N || judger < 0) {
//       putch(' ');
//     }
//     // putch(')');
//     V(&empty); 
//   } 
// }

int cnt_finish;
int para_p[10000], para_c[10000];
volatile int cnt_p[10000], cnt_c[10000];
volatile int cnt_cpu_p[10][1000], cnt_cpu_c[10][1000];
int cnt_cpu[10];
void Tproduce(void *arg) { 
    int me = *(int*)arg;
    int cnt = 0;
    int thres = 1;
    while (cnt <= 100) {
#ifdef DEBUG_PV
        P(&empty);
#endif
        // printf("%d", me);
        // putch('(');
        ++cnt_p[me];
        ++cnt_cpu[cpu_current()];
        ++cnt_cpu_p[cpu_current()][me];
        if (me == 0 && cnt_p[me] >= thres) {
            printf("Cpu : ");
            for (int i = 0; i < cpu_count(); ++i) printf("%d. ", cnt_cpu[i]);
            printf("\n");
            printf("%d Producer :", me);
            for (int i = 0; i < NPROD; ++i) printf("%d. ", cnt_p[i]);
            printf("\n%d Consumer :", me);
            for (int i = 0; i < NCONS; ++i) printf("%d. ", cnt_c[i]);
            printf("\nCpu producer:\n");
            for (int i = 0; i < cpu_count(); ++i) {
                for (int j = 0; j < NPROD; ++j) {
                    printf("%d. ", cnt_cpu_p[i][j]);
                } printf("\n");
            }
            printf("Cpu consumer:\n");
            for (int i = 0; i < cpu_count(); ++i) {
                for (int j = 0; j < NCONS; ++j) {
                    printf("%d. ", cnt_cpu_c[i][j]);
                } printf("\n");
            }
            printf("\n=============================\n");
            thres <<= 1;
        }
#ifdef DEBUG_PV
        V(&fill);
#endif
    }
    while(1);
}
void Tconsume(void *arg) {
    int me = *(int*)arg;//, cpu = cpu_current();
    int cnt = 0;
    // int thres = 1;
    while (cnt <= 100) {
#ifdef DEBUG_PV
        P(&fill);
#endif
        // putch(')'); //?????????????
        ++cnt_c[me];
        ++cnt_cpu[cpu_current()];
        ++cnt_cpu_c[cpu_current()][me];
        // if (me == 0 && cnt_p[me] >= thres) {
        //     printf("Cpu : ");
        //     for (int i = 0; i < cpu_count(); ++i) printf("%d ", cnt_cpu[i]);
        //     printf("\n");
        //     printf("%d Producer :", me);
        //     for (int i = 0; i < NPROD; ++i) printf("%d ", cnt_p[i]);
        //     printf("\n%d Consumer :", me);
        //     for (int i = 0; i < NCONS; ++i) printf("%d ", cnt_c[i]);
        //     printf("\n=============================\n");
        //     thres <<= 1;
        // }
        //++cnt_cpu[cpu_current()];
#ifdef DEBUG_PV
        V(&empty);
#endif
    }
    while(1) ;
}
#endif

#ifdef DEBUG_NORMAL
#define TASK_NUM 2
static my_spinlock_t *idlelock[TASK_NUM];
static int* lock_id[TASK_NUM]; 
static char *idles_name[] = {"A", "B", "C", "D", "E", "F", "G", "H", "I", "J",
                             "K", "L"};
volatile int task_num[TASK_NUM];
volatile int cnt_cpu_task[MAX_CPU][TASK_NUM];
int cnt_cpu[MAX_CPU];
static void mock_task(void *arg) {
  // printf("Hello from task %d\n", *(int*)arg);
  // printf("Cpu %d interrupt %d\n", cpu_current(), ienabled());
  int thres = 1;
    while (1) {
        // printf("Cpu %d interrupt %d\n", cpu_current(), ienabled());
        spin_lock(idlelock[*(int*)arg]);
        ++task_num[*(int*)arg];
        ++cnt_cpu[cpu_current()];
        ++cnt_cpu_task[cpu_current()][*(int*)arg];
        // putch("ABCDEF"[*(int*)arg]);
        if (cpu_current() == 0 && cnt_cpu[*(int*)arg] >= thres) {
            printf("Cpu : ");
            for (int i = 0; i < cpu_count(); ++i) printf("%d. ", cnt_cpu[i]);
            printf("\n");
            printf("Task :");
            for (int i = 0; i < TASK_NUM; ++i) printf("%d. ", task_num[i]);
            printf("\nCpu Task:\n");
            for (int i = 0; i < cpu_count(); ++i) {
                for (int j = 0; j < TASK_NUM; ++j) {
                    printf("%d. ", cnt_cpu_task[i][j]);
                } printf("\n");
            }
            printf("\n=============================\n");
            thres <<= 1;
        }
        spin_unlock(idlelock[(*(int*)arg + 1 ) % TASK_NUM ]);
        yield();
//        for (int volatile i = 0; i < 100000; i++);
    }
}
#endif

static void os_init() {
  pmm->init();
  kmt->init();
#ifdef DEBUG_NORMAL
    for (size_t i = 0; i < TASK_NUM; i++)
    {
      idlelock[i] = pmm->alloc(sizeof(my_spinlock_t));
      lock_id[i] = pmm->alloc(sizeof(int));
      *idlelock[i] = SPIN_LOCK_INIT;
      *lock_id[i] = i;
    }
    for (size_t i = 1; i < TASK_NUM; i++)
    {
      spin_lock(idlelock[i]);
    }
    for(int i = 0; i < TASK_NUM; i++) {
        kmt->create(pmm->alloc(sizeof(task_t)), idles_name[i], mock_task, lock_id[i]);
    }
#endif

#ifdef DEBUG_PRINT
  kmt->create(pmm->alloc(sizeof(task_t)), "print_task", print_task, NULL);
#endif

#ifdef DEBUG_DEV
  dev->init();
  kmt->create(pmm->alloc(sizeof(task_t)), "tty_reader", tty_reader, "tty1");
  kmt->create(pmm->alloc(sizeof(task_t)), "tty_reader", tty_reader, "tty2");
#endif

#ifdef DEBUG_STARVE
  kmt->sem_init(&empty, "empty", N);
  kmt->sem_init(&fill,  "fill",  0);
  for (int i = 0; i < NPROD; i++) {
    para_p[i] = i;
    kmt->create(pmm->alloc(sizeof(task_t)), "producer", Tproduce, &para_p[i]);
  }
  for (int i = 0; i < NCONS; i++) {
    para_c[i] = i;
    kmt->create(pmm->alloc(sizeof(task_t)), "consumer", Tconsume, &para_c[i]);
  }
#endif
}

static void os_run() {
  // for (const char *s = "Hello World from CPU #*\n"; *s; s++) {
  //   // putch(*s == '*' ? '0' + cpu_current() : *s);
  // }
  
  iset(true);
  while (1) {
    // panic("No user task!\n");
    // putch('a');
    yield();
  }
}

static void os_on_irq(int seq, int event, handler_t handler) {
  // putch('a');
  TRACE_ENTRY;
  IRQ* new_handler = pmm->alloc(sizeof(IRQ));
  new_handler->seq = seq;
  new_handler->event = event;
  new_handler->handler = handler;
  new_handler->next = NULL;
  if (irq_head == NULL)
  {
    irq_head = new_handler;
    TRACE_EXIT;
    return;
  }
  
  IRQ* cur = irq_head;
  if (irq_head->seq >= seq)
  {
    new_handler->next = irq_head;
    irq_head = new_handler;
    TRACE_EXIT;
    return;
  }

  while (cur->next != NULL)
  {
    if (cur->next->seq >= seq)
    {
      new_handler->next = cur->next;
      cur->next = new_handler;
      TRACE_EXIT;
      return;
    }
    cur = cur->next;
  }

  cur->next = new_handler;
  panic_on(irq_head == NULL, "irq_head is NULL");
  TRACE_EXIT;
}

static Context *os_trap(Event ev, Context *context) {
  // printf("cpu num: %d\n", cpu_count());
  Context *next = NULL;
  IRQ* irq_ptr = irq_head;
  panic_on(irq_ptr == NULL, "no irq handler");
  while(irq_ptr != NULL) {
    if (irq_ptr->event == EVENT_NULL || irq_ptr->event == ev.event) {
      Context *r = irq_ptr->handler(ev, context);
      panic_on(r && next, "returning multiple contexts");
      if (r) next = r;
    }
    irq_ptr = irq_ptr->next;
  }
  panic_on(!next, "returning NULL context");
  // panic_on(sane_context(next), "returning to invalid context");
  return next;
}

MODULE_DEF(os) = {
  .init = os_init,
  .run  = os_run,
  .on_irq = os_on_irq,
  .trap = os_trap,
};


// #include <common.h>

// static void os_init() {
//   pmm->init();
// }

// static void *test_alloc(int size) {
//   void *p = pmm->alloc(size);
// #ifndef TEST
//   printf("CPU #%d Allocating in %p, %d byte(s) (%x)\n", cpu_current(), p, size, size);
// #else
//   printf("CPU Allocating in %p, %d byte(s) (%x)\n", p, size, size);
// #endif
//   assert((size | ((uintptr_t)p == size + (uintptr_t)p)) || ((size-1) | (uintptr_t)p) == (size-1) + (uintptr_t)p);
//   return p;
// }

// static void test_free(void *addr) {
//   printf("CPU #%d Freeing in %p\n", cpu_current(), addr);
//   assert(addr != NULL);
//   pmm->free(addr);
// #ifndef TEST
// #else
//   printf("CPU Freeing in %p\n", addr);
// #endif
//   // buddy_debug_print();
// }

// #ifndef TEST
// static void os_run() {
//   for (const char *s = "Hello World from CPU #*\n"; *s; s++) {
//     putch(*s == '*' ? '0' + cpu_current() : *s);
//   }
//   printf("Hello World from CPU #%d\n", cpu_current());
//   test_alloc(1);
//   test_alloc(2);
//   test_alloc(4);
//   test_alloc(8);
//   void *p1 = test_alloc(1024 * 1024);
//   void *p2 = test_alloc(1024 * 1024);
//   void *p3 = test_alloc(1024 * 1024);
//   void *p4 = test_alloc(1024 * 1024);
//   void *p5 = test_alloc(1024 * 1024 + 1);
//   // buddy_debug_print();
//   printf("--------free-------\n");
//   test_free(p1);
//   test_free(p2);
//   test_free(p3); 
//   test_free(p4);
//   test_free(p5);
//   typedef struct Task {
//     void *alloc;
//     int size;
//   } Task;
//   #define TEST_SIZE 10000
//   Task tasks[TEST_SIZE];
//   for (int i = 0; i < TEST_SIZE; i++) {
//     tasks[i].size = (1 << (rand() % 3 + 13));
//     tasks[i].alloc = test_alloc(tasks[i].size);
//     // assert((size | ((uintptr_t)p == size + (uintptr_t)p)) || ((size-1) | (uintptr_t)p) == (size-1) + (uintptr_t)p);
//   }
//   for (int i = 0; i < TEST_SIZE; i++) {
//     if (tasks[i].alloc)
//     test_free(tasks[i].alloc);
//   }
//   size_t size = 16 * 1024 * 1024;
//   void *p = pmm->alloc(size);
//   printf("CPU #%d Allocating in %x, %d byte(s) %x\n", cpu_current(), (uintptr_t)p, size, size);
//   for (volatile int i = 0; i < 10000; i ++);
//   printf("SUCCESS\n");
//   while (1);
// }
// #else 
// static void os_run() {
//   // for (const char *s = "Hello World from CPU #*\n"; *s; s++) {
//   //   putch(*s == '*' ? '0' + cpu_current() : *s);
//   // }
//   printf("Testing\n");
//   while (1) ;
// }
// #endif

// MODULE_DEF(os) = {
//   .init = os_init,
//   .run  = os_run,
// };