#include "threads.h"
#include "common.h"
#include "am.h"

extern unsigned long thread_id[];
static void entry(int tid) { 
  int cur_cpu = tid - 1;
  thread_id[cur_cpu] = pthread_self();
  printf("thread_id[%d]: %ld\n", cur_cpu, thread_id[cur_cpu]);
  pmm->alloc(128);
}
static void goodbye()      { printf("End.\n"); }
int main() {
  pmm->init();
  for (int i = 0; i < CPU_NUM; i++)
    create(entry);
  join();
}