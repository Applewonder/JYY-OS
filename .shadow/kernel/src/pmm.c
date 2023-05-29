#include <common.h>
#include <stdint.h>
#include <cbma.h>
#include <slab.h>
#include <string.h>

#define HEAP_SIZE 512 * 1024 * 1024

#ifdef TEST
struct {
  void *start, *end;
} heap;
#endif

static void *kalloc(size_t size) {
  int cpu_num = cpu_current();
#ifndef TEST
  return slab_alloc(cpu_num, size);
#else
  void* ptr = slab_alloc(cpu_num, size);
  
#endif
}

static void kfree(void *ptr) {
  if (is_align_to(ptr, 12)) {
    bbma_free(ptr);
  } else {
    slab_free(ptr);
  }
}

#ifndef TEST
// 框架代码中的 pmm_init (在 AbstractMachine 中运行)
static void pmm_init() {
  uintptr_t pmsize = ((uintptr_t)heap.end - (uintptr_t)heap.start);
  printf("Got %d MiB heap: [%p, %p)\n", pmsize >> 20, heap.start, heap.end);
  void* align_begin_address = (void*)align_to((uintptr_t)heap.start, 24);
  bbma_init(align_begin_address, heap.end);
  slab_init();
}
#else
// 测试代码的 pmm_init ()
static void pmm_init() {
  char *ptr  = malloc(HEAP_SIZE);
  heap.start = ptr;
  heap.end   = ptr + HEAP_SIZE;
  printf("Got %d MiB heap: [%p, %p)\n", HEAP_SIZE >> 20, heap.start, heap.end);
  // memset(ptr, 0, HEAP_SIZE);
  printf("memset done\n");
  void* align_begin_address = (void*)align_to((uintptr_t)heap.start, 24);
  bbma_init(align_begin_address, heap.end);
  printf("bbma_init done\n");
  slab_init();
  printf("slab_init done\n");
}
#endif

MODULE_DEF(pmm) = {
  .init  = pmm_init,
  .alloc = kalloc,
  .free  = kfree,
};
