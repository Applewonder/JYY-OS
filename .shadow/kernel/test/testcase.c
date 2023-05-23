#include "testcase.h"
#include "cbma.h"
#include "slab.h"
#include <time.h>

mutex_t mutex = MUTEX_INIT();
FILE *file;

extern unsigned long thread_id[];
extern BUDDY_BLOCK_STICK* buddy_blocks[];

void print_bbma_chain(size_t size) {
    BUDDY_BLOCK_SIZE bbma_size = determine_bbma_size(size);
    BUDDY_BLOCK_STICK* cur_stick = buddy_blocks[bbma_size - FIND_BBMA_OFFSET];
    assert(cur_stick != (void*)0x00000000000e);
    fprintf(file, "Size: %ld ", size);
    long print_count = 0;
    while (cur_stick != NULL)
    {
      // assert(0);
      assert(print_count < 10000);
      print_count++;
      assert(cur_stick != (void*)0x00000000000e);
      assert(cur_stick->next != (void*)0x00000000000e);
      fprintf(file, "%p -> ", convert_index_to_addr(cur_stick));
      cur_stick = cur_stick->next;
    }
    fprintf(file, "\n");
}

void write_in_file(void* ptr, size_t size, bool is_alloc, int test_id) {
    char str[20];
    sprintf(str, "%d", test_id);
    char origin_log[200] = "/home/appletree/JYY-OS/kernel/test/testlog";
    strcat(origin_log, str);
    strcat(origin_log, ".txt");
    file = fopen(origin_log, "a");
    if (is_alloc) {
        fprintf(file, "Alloc %p, Size %ld\n", ptr, size);
        fprintf(file, "End Alloc\n");
        print_bbma_chain(size);
    } else {
        fprintf(file, "Free %p\n", ptr);
        fprintf(file, "End Free\n");
        print_bbma_chain(size);
    }
    fprintf(file, "\n");
    fclose(file);
}

void test_alloc_and_free(size_t size, int test_id) {
    mutex_lock(&mutex);
    char str[20];
    sprintf(str, "%d", test_id);
    char origin_log[200] = "/home/appletree/JYY-OS/kernel/test/testlog";
    strcat(origin_log, str);
    strcat(origin_log, ".txt");
    file = fopen(origin_log, "a");
    fprintf(file, "Before Alloc\n");
    print_bbma_chain(size);

    void* ptr = pmm->alloc(size);
    if (ptr == NULL) {
        fprintf(file, "Failed to alloc\n");
    }
    fclose(file);
    write_in_file(ptr, size, true, test_id);


    mutex_unlock(&mutex);

    mutex_lock(&mutex);
    file = fopen(origin_log, "a");
    fprintf(file, "Before Free\n");
    print_bbma_chain(size);
    fclose(file);
    pmm->free(ptr);
    write_in_file(ptr, size, false, test_id);
    mutex_unlock(&mutex);
}

void once_slab_alloc(SLAB_SIZE size, char origin_log[], int test_id) {
    int real_size = 1 << (5 + size);
    mutex_lock(&mutex);
    file = fopen(origin_log, "a");
    fprintf(file, "Before Alloc\n");
    print_bbma_chain(real_size);

    void* ptr = pmm->alloc(real_size);
    if (ptr == NULL) {
        fprintf(file, "Failed to alloc\n");
    }
    fclose(file);
    write_in_file(ptr, real_size, true, test_id);
    mutex_unlock(&mutex);
}

void once_bbma_alloc(BUDDY_BLOCK_SIZE size, char origin_log[], int test_id) {
    int real_size = 1 << (14 + size);
    mutex_lock(&mutex);
    file = fopen(origin_log, "a");
    fprintf(file, "Before Alloc\n");
    print_bbma_chain(real_size);

    void* ptr = pmm->alloc(real_size);
    if (ptr == NULL) {
        fprintf(file, "Failed to alloc\n");
    }
    fclose(file);
    write_in_file(ptr, real_size, true, test_id);
    mutex_unlock(&mutex);
}

void once_slab_free(void* ptr, SLAB_SIZE size, char origin_log[], int test_id) {
    if (ptr == NULL) {
        return;
    }
    int real_size = 1 << (5 + size);
    mutex_lock(&mutex);
    file = fopen(origin_log, "a");
    fprintf(file, "Before Free\n");
    print_bbma_chain(real_size);
    fclose(file);
    pmm->free(ptr);
    write_in_file(ptr, real_size, false, test_id);
    mutex_unlock(&mutex);
}

void test_multi_slab_alloc_and_free(int test_id) {
    char str[20];
    sprintf(str, "%d", test_id);
    char origin_log[200] = "/home/appletree/JYY-OS/kernel/test/testlog";
    strcat(origin_log, str);
    strcat(origin_log, ".txt");

    SLAB_SIZE size_1 = rand() % SLAB_NUM;
    SLAB_SIZE size_2 = rand() % SLAB_NUM;
    SLAB_SIZE size_3 = rand() % SLAB_NUM;

    once_slab_alloc(size_1, origin_log, test_id);
    once_slab_alloc(size_2, origin_log, test_id);
    once_slab_alloc(size_3, origin_log, test_id);

    once_slab_free(NULL, size_3, origin_log, test_id);
    once_slab_free(NULL, size_2, origin_log, test_id);
    once_slab_free(NULL, size_1, origin_log, test_id);
}


void test_multi_bbma_alloc_and_free(int test_id) {
    char str[20];
    sprintf(str, "%d", test_id);
    char origin_log[200] = "/home/appletree/JYY-OS/kernel/test/testlog";
    strcat(origin_log, str);
    strcat(origin_log, ".txt");

    SLAB_SIZE size_1 = rand() % SLAB_NUM;
    SLAB_SIZE size_2 = rand() % SLAB_NUM;
    SLAB_SIZE size_3 = rand() % SLAB_NUM;

    once_bbma_alloc(size_1, origin_log, test_id);
    once_bbma_alloc(size_2, origin_log, test_id);
    once_bbma_alloc(size_3, origin_log, test_id);

    once_bbma_free(NULL, size_3, origin_log, test_id);
    once_bbma_free(NULL, size_2, origin_log, test_id);
    once_bbma_free(NULL, size_1, origin_log, test_id);
}

static void entry_0(int tid) { 
  int cur_cpu = tid - 1;
  thread_id[cur_cpu] = pthread_self();
//   printf("thread_id[%d]: %ld\n", cur_cpu, thread_id[cur_cpu]);
  for (int i = 0; i < 10000; i++)
  {
    int choose_type = i % SLAB_NUM;
    test_alloc_and_free(1 << (5 + choose_type), 0);
  }
}

static void entry_1(int tid) { 
  int cur_cpu = tid - 1;
  thread_id[cur_cpu] = pthread_self();
//   printf("thread_id[%d]: %ld\n", cur_cpu, thread_id[cur_cpu]);
  for (int i = 0; i < BBMA_NUM; i++)
  {
    int choose_type = i % BBMA_NUM;
    test_alloc_and_free(1 << (12 + choose_type), 1);
  }
}

static void entry_2(int tid) { 
  int cur_cpu = tid - 1;
  thread_id[cur_cpu] = pthread_self();
//   printf("thread_id[%d]: %ld\n", cur_cpu, thread_id[cur_cpu]);
  for (int i = 0; i < BBMA_NUM; i++)
  {
    int choose_type = i % BBMA_NUM;
    test_alloc_and_free(1 << (12 + choose_type), 2);
  }
}

static void entry_3(int tid) { 
  int cur_cpu = tid - 1;
  thread_id[cur_cpu] = pthread_self();
//   printf("thread_id[%d]: %ld\n", cur_cpu, thread_id[cur_cpu]);
  for (int i = 0; i < 10000; i++)
  {
    int choose_type = rand() % BBMA_NUM;
    test_alloc_and_free(1 << (12 + choose_type), 3);
  }
}

static void entry_4(int tid) { 
  int cur_cpu = tid - 1;
  thread_id[cur_cpu] = pthread_self();
//   printf("thread_id[%d]: %ld\n", cur_cpu, thread_id[cur_cpu]);
  for (int i = 0; i < 10000; i++)
  {
    int choose_type = rand() % SLAB_NUM;
    test_alloc_and_free(1 << (5 + choose_type), 4);
  }
}

static void entry_5(int tid) { 
  int cur_cpu = tid - 1;
  thread_id[cur_cpu] = pthread_self();
//   printf("thread_id[%d]: %ld\n", cur_cpu, thread_id[cur_cpu]);
  for (int i = 0; i < 1000; i++)
  {
    test_multi_slab_alloc_and_free(5);
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
    for (int i = 0; i < 6; i++){
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
        create(entry_2);
    }
    join();
}

void do_test_3() {
    printf("\033[32m Test 3 begin\n\033[0m");
    file = fopen("/home/appletree/JYY-OS/kernel/test/testlog3.txt", "w");
    fclose(file);
    pmm->init();
    for (int i = 0; i < 6; i++){
        create(entry_3);
    }
    join();
}

void do_test_4() {
    printf("\033[32m Test 4 begin\n\033[0m");
    file = fopen("/home/appletree/JYY-OS/kernel/test/testlog4.txt", "w");
    fclose(file);
    pmm->init();
    for (int i = 0; i < 6; i++){
        create(entry_4);
    }
    join();
}

void do_test_5() {
    printf("\033[32m Test 5 begin\n\033[0m");
    file = fopen("/home/appletree/JYY-OS/kernel/test/testlog5.txt", "w");
    fclose(file);
    pmm->init();
    for (int i = 0; i < 6; i++){
        create(entry_5);
    }
    join();
}