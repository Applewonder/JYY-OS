#include "threads.h"
#include "common.h"
#include "am.h"

extern unsigned long thread_id[];
static void entry(int tid) { 
  int cur_cpu = tid - 1;
  thread_id[cur_cpu] = pthread_self();
  printf("thread_id[%d]: %ld\n", cur_cpu, thread_id[cur_cpu]);
  for (int i = 0; i < 10000; i++)
  {
    void* int32_ptr = pmm->alloc(32);
    printf("int32_ptr: %p\n", int32_ptr);
    void* int64_ptr = pmm->alloc(64);
    printf("int64_ptr: %p\n", int64_ptr);
    void* int128_ptr = pmm->alloc(128);
    printf("int128_ptr: %p\n", int128_ptr);
    void* int256_ptr = pmm->alloc(256);
    printf("int256_ptr: %p\n", int256_ptr);
    void* int512_ptr = pmm->alloc(512);
    printf("int512_ptr: %p\n", int512_ptr);
    void* int1024_ptr = pmm->alloc(1024);
    printf("int1024_ptr: %p\n", int1024_ptr);
    pmm->free(int32_ptr);
    pmm->free(int64_ptr);
    pmm->free(int128_ptr);
    pmm->free(int256_ptr);
    pmm->free(int512_ptr);
    pmm->free(int1024_ptr);
  }
  
}
static void goodbye()      { printf("End.\n"); }
int main() {
  pmm->init();
  for (int i = 0; i < CPU_NUM; i++)
    create(entry);
  join();
}