#include "threads.h"
#include "common.h"

int cur_cpu;

static void entry(int tid) { 
  cur_cpu = tid - 1;
  pmm->alloc(128);
}
static void goodbye()      { printf("End.\n"); }
int main() {
  pmm->init();
  for (int i = 0; i < 4; i++)
    create(entry);
  join(goodbye);
}