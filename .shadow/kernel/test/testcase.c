#include "testcase.h"
#include "cbma.h"
#include "slab.h"
#include <stdlib.h>
#include <time.h>

mutex_t mutex = MUTEX_INIT();
mutex_t record_addr = MUTEX_INIT();
FILE *file;
void* already_alloc[500000][2];
int remain_cap = 128 * 1024 * 1024;
int end_index = 0;
extern unsigned long thread_id[];
extern int tree_num;
extern Tree all_trees[];
extern int calculate_addr_helper[];
// extern BUDDY_BLOCK_STICK* buddy_blocks[];

// void print_bbma_chain(size_t size) {
//     BUDDY_BLOCK_SIZE bbma_size = determine_bbma_size(size);
//     BUDDY_BLOCK_STICK* cur_stick = buddy_blocks[bbma_size - FIND_BBMA_OFFSET];
//     assert(cur_stick != (void*)0x00000000000e);
//     fprintf(file, "Size: %ld ", size);
//     long print_count = 0;
//     while (cur_stick != NULL)
//     {
//       // assert(0);
//       assert(print_count < 10000);
//       print_count++;
//       assert(cur_stick != (void*)0x00000000000e);
//       assert(cur_stick->next != (void*)0x00000000000e);
//       fprintf(file, "%p -> ", convert_index_to_addr(cur_stick));
//       cur_stick = cur_stick->next;
//     }
//     fprintf(file, "\n");
// }

void write_in_file(void* ptr, size_t size, bool is_alloc, int test_id) {
    char str[20];
    sprintf(str, "%d", test_id);
    char origin_log[200] = "/home/appletree/JYY-OS/kernel/test/testlog";
    strcat(origin_log, str);
    strcat(origin_log, ".txt");
    file = fopen(origin_log, "a");
    
    BUDDY_BLOCK_SIZE bbma_size = determine_bbma_size(size);
    SLAB_SIZE slab_size = determine_slab_size(size);

    if (size > 2 * 1024) {
      size = (1 << bbma_size);
    } else {
      size = (1 << slab_size);
    }

    if (is_alloc) {
        fprintf(file, "Alloc %p, Size %ld\n", ptr, size);
        // print_bbma_chain(size);
    } else {
        fprintf(file, "Free %p\n", ptr);
        // print_bbma_chain(size);
    }
    // fprintf(file, "\n");
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
    // print_bbma_chain(size);

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
    // print_bbma_chain(size);
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
    // print_bbma_chain(real_size);

    void* ptr = pmm->alloc(real_size);
    if (ptr == NULL) {
        fprintf(file, "Failed to alloc\n");
    }
    fclose(file);
    write_in_file(ptr, real_size, true, test_id);
    mutex_unlock(&mutex);
}

void once_bbma_alloc(BUDDY_BLOCK_SIZE size, char origin_log[], int test_id) {
    int real_size = 1 << (12 + size);
    mutex_lock(&mutex);
    file = fopen(origin_log, "a");
    fprintf(file, "Before Alloc\n");
    // print_bbma_chain(real_size);

    void* ptr = pmm->alloc(real_size);
    if (ptr == NULL) {
        fprintf(file, "Failed to alloc\n");
    }
    fclose(file);
    write_in_file(ptr, real_size, true, test_id);
    mutex_unlock(&mutex);
}

void once_bbma_free(void* ptr, BUDDY_BLOCK_SIZE size, char origin_log[], int test_id) {
    if (ptr == NULL) {
        return;
    }
    int real_size = 1 << (12 + size);
    mutex_lock(&mutex);
    file = fopen(origin_log, "a");
    fprintf(file, "Before Free\n");
    // print_bbma_chain(real_size);
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

    // once_slab_alloc(size_1, origin_log, test_id);
    // once_slab_alloc(size_2, origin_log, test_id);
    // once_slab_alloc(size_3, origin_log, test_id);

    // once_slab_free(NULL, size_3, origin_log, test_id);
    // once_slab_free(NULL, size_2, origin_log, test_id);
    // once_slab_free(NULL, size_1, origin_log, test_id);
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

    // once_bbma_alloc(size_1, origin_log, test_id);
    // once_bbma_alloc(size_2, origin_log, test_id);
    // once_bbma_alloc(size_3, origin_log, test_id);

    // once_bbma_free(NULL, size_3, origin_log, test_id);
    // once_bbma_free(NULL, size_2, origin_log, test_id);
    // once_bbma_free(NULL, size_1, origin_log, test_id);
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

// void test_multi_alloc(int test_id) {
//     char str[20];
//     sprintf(str, "%d", test_id);
//     char origin_log[200] = "/home/appletree/JYY-OS/kernel/test/testlog";
//     strcat(origin_log, str);
//     strcat(origin_log, ".txt");

//     for (int i = 0; i < 3; i ++) {
//       for (int i = 0; i < 10000; i++)
//       {
//         pmm->alloc(1 << (5 + i % SLAB_NUM));
//         write_in_file(ptr, size, true, test_id);

//       }
      
//     }
// }

void print_chain_in_file(int test_id, int size, bool is_end) {
    char str[20];
    sprintf(str, "%d", test_id);
    char origin_log[200] = "/home/appletree/JYY-OS/kernel/test/testlog";
    strcat(origin_log, str);
    strcat(origin_log, ".txt");
    file = fopen(origin_log, "a+");
    if (size > 2 *1024) {
      int log_size = determine_bbma_size(size);
      fprintf(file, "Size: %dKB\n", 1 << (log_size - 10));
      // print_bbma_chain(size);
    } else {
      int log_size = determine_slab_size(size);
      fprintf(file, "Slab alloc: %dB\n", 1 << log_size);
    }
    if (is_end) {
      fprintf(file, "\n");
    }
    fclose(file);
}

static void entry_6(int tid) { 
  printf("entry_6\n");
  int cur_cpu = tid - 1;
  thread_id[cur_cpu] = pthread_self();
//   printf("thread_id[%d]: %ld\n", cur_cpu, thread_id[cur_cpu]);

  void* already_alloc[50000];
  int end_index = 0;
  int round_cnt = 0;
  while (round_cnt < 10000) {
    // printf("round_cnt: %d\n", round_cnt);
    int choose_type = rand() % 2;
    if (choose_type && end_index) {
      int index = rand() % end_index;
      // BUDDY_BLOCK_STICK* stick_addr =convert_addr_to_index(already_alloc[index]);
      // int size = 1 << stick_addr->alloc_spaces;
      // mutex_lock(&mutex);
      // print_chain_in_file(6, size, false);
      // mutex_unlock(&mutex);
      pmm->free(already_alloc[index]);
      // mutex_lock(&mutex);
      // write_in_file(already_alloc[index], 0, false, 6);
      // print_chain_in_file(6, size, true);
      // mutex_unlock(&mutex);
      already_alloc[index] = already_alloc[end_index - 1];
      end_index --;
    } else {
      int size = (rand() % 16 * 1024 * 1024) + 1;
      // mutex_lock(&mutex);
      // print_chain_in_file(6, size, false);
      // mutex_unlock(&mutex);
      void* ptr = pmm->alloc(size);
      // mutex_lock(&mutex);
      if (ptr == NULL) {
        file = fopen("/home/appletree/JYY-OS/kernel/test/testlog6.txt", "a");
        fprintf(file, "Try to alloc Size: %d. Can not alloc\n", size);
        fclose(file);
      } else {
        // write_in_file(ptr, size, true, 6);

        // print_chain_in_file(6, size, true);

        already_alloc[end_index] = ptr;
        end_index ++;
      }
      // mutex_unlock(&mutex);
    }
    round_cnt ++;
  }
}

char* turn_num_into_size(char bma_size) {
  // switch (bma_size){
  //   case S_4K:
  //     return "4K";
  // }
  switch (bma_size)
  {
  case S_4K:
    return "4K  ";
    break;
  case S_8K:
    return "8K  ";
    break;
  case S_16K:
    return "16K ";
    break;
  case S_32K:
    return "32K ";
    break;
  case S_64K:
    return "64K ";
    break;
  case S_128K: 
    return "128K";
    break;
  case S_256K:
    return "256K";
    break;
  case S_512K:
    return "512K";
    break;
  case S_1M:
    return "1M  ";
    break;
  case S_2M:
    return "2M  ";
    break;
  case S_4M:
    return "4M  ";
    break;
  case S_8M:
    return "8M  ";
    break;
  case S_16M:
    return "16M ";
    break;
  case 1:
    return "DEPA";
    break;
  case 0:
    return "USED";
    break;
  }
  return NULL;
}

void print_tree_status() {
  file = fopen("/home/appletree/JYY-OS/kernel/test/testlog7.txt", "a");
  fprintf(file, "Remain cap: %d\n", remain_cap);
  for (int i = 1; i <= tree_num; i ++) {
    Tree tree = all_trees[i];
    fprintf(file, "Tree %d\n", i);
    int cur_stand = 1;
    int next_stand = 2;
    for (int j = 0; j < 13; j ++) {
      cur_stand = calculate_addr_helper[j];
      if (j == 12) {
        next_stand = 8192;
      } else {
        next_stand = calculate_addr_helper[j + 1];
      }
      for (int k = cur_stand; k < next_stand; k++)
      {
        fprintf(file, "%s ", turn_num_into_size(tree[k]));
      }
      fprintf(file, "\n");
    }
    fprintf(file, "\n");
  }
  fclose(file);
}

static void entry_7(int tid) { 
  printf("entry_7\n");
  int cur_cpu = tid - 1;
  thread_id[cur_cpu] = pthread_self();
//   printf("thread_id[%d]: %ld\n", cur_cpu, thread_id[cur_cpu]);



  int round_cnt = 0;
  while (1) {
  //   // printf("round_cnt: %d\n", round_cnt);
    int choose_type = rand() % 2;
    if (choose_type) {
      mutex_lock(&record_addr);
      if (end_index) {
        int index = rand() % end_index;
        
        void* addr = already_alloc[index][0];
        remain_cap += (uintptr_t)already_alloc[index][1];
        already_alloc[index][0] = already_alloc[end_index - 1][0];
        already_alloc[index][1] = already_alloc[end_index - 1][1];
        end_index --;
        mutex_unlock(&record_addr);
        pmm->free(addr);
      } else {
        mutex_unlock(&record_addr);
      }
    } else {
      int size = (rand() % SLAB_NUM) + CPU_FIND_SLAB_OFFSET;
      uintptr_t real_size = 1 << size;
      int choose_size = 0;
      if (choose_size) {
        real_size = 1 << S_4K;
      }
      void* ptr = pmm->alloc(real_size);
      if (ptr == NULL) {
        
        break;
      } else {
        mutex_lock(&record_addr);
        remain_cap -= real_size;
        already_alloc[end_index][0] = ptr;
        already_alloc[end_index][1] = (void*)real_size;
        end_index ++;
        mutex_unlock(&record_addr);
      }
    }
      // pmm->free(ptr);
    // print_tree_status();
    round_cnt ++;
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

void do_test_7() {
    printf("\033[32m Test 7 begin\n\033[0m");
    file = fopen("/home/appletree/JYY-OS/kernel/test/testlog7.txt", "w");
    fclose(file);
    pmm->init();
    printf("init done\n");
    for (int i = 0; i < CPU_NUM; i++){
        create(entry_7);
    }
    join();
    file = fopen("/home/appletree/JYY-OS/kernel/test/testlog7.txt", "a");
    fprintf(file, "Try to alloc Failed, remain capacity %d. Can not alloc\n", remain_cap);
    fclose(file);
    print_tree_status();
        
}