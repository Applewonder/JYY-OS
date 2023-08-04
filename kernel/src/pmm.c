#include <common.h>
#include <stdint.h>
#include <cbma.h>
#include <slab.h>
#include <string.h>

#define HEAP_SIZE 128 * 1024 * 1024

#ifdef TEST
struct {
  void *start, *end;
} heap;
#endif

static void *kalloc(size_t size) {
  int cpu_num = cpu_current();
  // printf("Alloc Size: %ld\n", size);
#ifndef TEST
  return slab_alloc(cpu_num, size);
#else
  void* ptr = slab_alloc(cpu_num, size);
  printf("Slab_ptr: %p\n", ptr);
  if (ptr != NULL) {
    if (size > 2048) {
      int* mark = (ptr + sizeof(SLAB_STICK) + 8);
      assert(*mark == 0);
      *mark += 1;
    } else {
      int* mark = (ptr + 8);
      assert(*mark == 0);
      *mark += 1;
    }
  }
  return ptr;
#endif
}

static void kfree(void *ptr) {
  // printf("Free ptr: %p\n", ptr);
  if (is_align_to(ptr, 12)) {
#ifdef TEST
    int* mark = (ptr + sizeof(SLAB_STICK) + 8);
    assert(*mark == 1);
    *mark -= 1;
#endif
    bbma_free(ptr);
  } else {
#ifdef TEST
    int* mark = (ptr + 8);
    assert(*mark == 1);
    *mark -= 1;
#endif
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
  // for (int i = 1; i <= tree_num; i++)
  // {
  //   char* tree_node = align_begin_address + (i - 1) * (sizeof(Tree_node) * 8192) + 1;
  //   assert(*tree_node == S_16M);
  // }
  
  printf("bbma_init done\n");
  slab_init();
  printf("slab_init done\n");
}
#endif

static void *kalloc_safe(size_t size) {
  bool i = ienabled();
  iset(false);
  void *ret = kalloc(size);
  if (i) iset(true);
  return ret;
}

static void kfree_safe(void *ptr) {
  int i = ienabled();
  iset(false);
  kfree(ptr);
  if (i) iset(true);
}

MODULE_DEF(pmm) = {
  .init  = pmm_init,
  .alloc = kalloc_safe,
  .free  = kfree_safe,
};
