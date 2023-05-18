#include "threads.h"
#include "common.h"
#include "am.h"

extern int thread_id[];
static void entry(int tid) { 
  int cur_cpu = tid - 1;
  thread_id[cur_cpu] = pthread_self();
  pmm->alloc(128);
}
static void goodbye()      { printf("End.\n"); }
int main() {
  pmm->init();
  for (int i = 0; i < CPU_NUM; i++)
    create(entry);
  join(goodbye);
}