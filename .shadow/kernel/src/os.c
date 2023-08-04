#include <alloca.h>
#include <common.h>
#include <os.h>

#ifdef DEBUG_DEV
  #include <devices.h>
#endif

#ifdef DEBUG_PV
  sem_t empty, fill;
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

#ifdef DEBUG_PV
void Tproduce(void *arg) { 
  while (1) { 
    P(&empty); 
    putch('('); 
    V(&fill); 
  } 
}
void Tconsume(void *arg) { 
  while (1) { 
    P(&fill);  
    putch(')'); 
    V(&empty); 
  } 
}
#endif

static void os_init() {
  pmm->init();
  kmt->init();
#ifdef DEBUG_PRINT
  kmt->create(pmm->alloc(sizeof(task_t)), "print_task", print_task, NULL);
#endif

#ifdef DEBUG_DEV
  dev->init();
  kmt->create(pmm->alloc(sizeof(task_t)), "tty_reader", tty_reader, "tty1");
  kmt->create(pmm->alloc(sizeof(task_t)), "tty_reader", tty_reader, "tty2");
#endif

#ifdef DEBUG_PV
  kmt->sem_init(&empty, "empty", N);
  kmt->sem_init(&fill,  "fill",  0);
  for (int i = 0; i < NPROD; i++) {
    kmt->create(pmm->alloc(sizeof(task_t)), "producer", Tproduce, NULL);
  }
  for (int i = 0; i < NCONS; i++) {
    kmt->create(pmm->alloc(sizeof(task_t)), "consumer", Tconsume, NULL);
  }
#endif
}

static void os_run() {
  // for (const char *s = "Hello World from CPU #*\n"; *s; s++) {
  //   // putch(*s == '*' ? '0' + cpu_current() : *s);
  // }
  
  iset(true);
  while (1) {
    panic("No user task!\n");
    putch('a');
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
  printf("cpu num: %d\n", cpu_count());
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