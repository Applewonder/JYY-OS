#include "threads.h"
#include "common.h"

static void entry(int tid) { pmm->alloc(128); }
static void goodbye()      { printf("End.\n"); }
int main() {
  pmm->init();
  for (int i = 0; i < 4; i++)
    create(entry);
  join(goodbye);
}