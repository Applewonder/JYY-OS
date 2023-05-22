// #include <common.h>
// #include <buddy.h>

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
//   // for (const char *s = "Hello World from CPU #*\n"; *s; s++) {
//   //   putch(*s == '*' ? '0' + cpu_current() : *s);
//   // }
//   printf("Hello World from CPU #%d\n", cpu_current());
//   // test_alloc(1);
//   // test_alloc(2);
//   // test_alloc(4);
//   // test_alloc(8);
//   void *p1 = test_alloc(1024 * 1024);
//   // void *p2 = test_alloc(1024 * 1024);
//   // void *p3 = test_alloc(1024 * 1024);
//   // void *p4 = test_alloc(1024 * 1024);
//   // void *p5 = test_alloc(1024 * 1024 + 1);
//   // // buddy_debug_print();
//   // printf("--------free-------\n");
//   test_free(p1);
//   // test_free(p2);
//   // test_free(p3); 
//   // test_free(p4);
//   // test_free(p5);
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
//   // size_t size = 16 * 1024 * 1024;
//   // void *p = pmm->alloc(size);
//   // printf("CPU #%d Allocating in %x, %d byte(s) %x\n", cpu_current(), (uintptr_t)p, size, size);
//   // for (volatile int i = 0; i < 10000; i ++);
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

#include "testcase.h"

mutex_t mutex = MUTEX_INIT();
FILE *file;

extern unsigned long thread_id[];

void write_in_file(void* ptr, size_t size, bool is_alloc, int test_id) {
    char str[20];
    sprintf(str, "%d", test_id);
    char origin_log[200] = "/home/appletree/JYY-OS/kernel/test/testlog";
    strcat(origin_log, str);
    strcat(origin_log, ".txt");
    mutex_lock(&mutex);
    file = fopen(origin_log, "a");
    if (is_alloc) {
        fprintf(file, "Alloc %p, Size %ld\n", ptr, size);
    } else {
        fprintf(file, "Free %p\n", ptr);
    }
    fclose(file);
    mutex_unlock(&mutex);
}

void test_alloc_and_free(size_t size, int test_id) {
    void* ptr = pmm->alloc(size);
    write_in_file(ptr, size, true, test_id);
    pmm->free(ptr);
    write_in_file(ptr, size, false, test_id);
}

static void entry_0(int tid) { 
  int cur_cpu = tid - 1;
  thread_id[cur_cpu] = pthread_self();
//   printf("thread_id[%d]: %ld\n", cur_cpu, thread_id[cur_cpu]);
  for (int i = 0; i < 10000; i++)
  {
    int choose_type = i % 6;
    test_alloc_and_free(1 << (5 + choose_type), 0);
  }
}

static void entry_1(int tid) { 
  int cur_cpu = tid - 1;
  thread_id[cur_cpu] = pthread_self();
//   printf("thread_id[%d]: %ld\n", cur_cpu, thread_id[cur_cpu]);
  for (int i = 0; i < 14; i++)
  {
    int choose_type = i % 14;
    test_alloc_and_free(1 << (11 + choose_type), 1);
  }
}


void do_test_0() {
    printf("\033[32m Test 0 begin\n\033[0m");
    file = fopen("/home/appletree/JYY-OS/kernel/test/testlog0.txt", "w");
    fclose(file);
    pmm->init();
    for (int i = 0; i < CPU_NUM; i++){
        create(entry_0);
    }
    join();
}

void do_test_1() {
    printf("\033[32m Test 1 begin\n\033[0m");
    file = fopen("/home/appletree/JYY-OS/kernel/test/testlog1.txt", "w");
    fclose(file);
    pmm->init();
    for (int i = 0; i < CPU_NUM; i++){
        create(entry_1);
    }
    join();
}

void do_test_2() {
    printf("\033[32m Test 2 begin\n\033[0m");
    file = fopen("/home/appletree/JYY-OS/kernel/test/testlog2.txt", "w");
    fclose(file);
    pmm->init();
    for (int i = 0; i < 1; i++){
        create(entry_1);
    }
    join();
}