#include <alloca.h>
#include <common.h>
#include <os.h>

static IRQ* irq_head = NULL;

static void os_init() {
  pmm->init();
  kmt->init();
}

static void os_run() {
  for (const char *s = "Hello World from CPU #*\n"; *s; s++) {
    // putch(*s == '*' ? '0' + cpu_current() : *s);
  }
  
  while (1);
}

static void os_on_irq(int seq, int event, handler_t handler) {
  IRQ* new_handler = pmm->alloc(sizeof(IRQ));
  new_handler->seq = seq;
  new_handler->event = event;
  new_handler->handler = handler;
  new_handler->next = NULL;
  if (irq_head == NULL)
  {
    irq_head = new_handler;
    return;
  }
  
  IRQ* cur = irq_head;
  if (irq_head->seq >= seq)
  {
    new_handler->next = irq_head;
    irq_head = new_handler;
    return;
  }

  while (cur->next != NULL)
  {
    if (cur->next->seq >= seq)
    {
      new_handler->next = cur->next;
      cur->next = new_handler;
      return;
    }
    cur = cur->next;
  }
  
  cur->next = new_handler;
}

static Context *os_trap(Event ev, Context *context) {
  
  return NULL;
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