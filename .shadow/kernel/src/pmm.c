#include <common.h>
#include <stdint.h>
#include <bbma.h>
#include <slab.h>

#define HEAP_SIZE 0x100000

#ifdef TEST
struct {
  void *start, *end;
} heap;
#endif

static void *kalloc(size_t size) {
  int cpu_num = cpu_current();
  return slab_alloc(cpu_num, size);
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
  if (align_begin_address - heap.start >= BBMA_STICK_SIZE) {
    bbma_init(align_begin_address, heap.end);
  } else {
    bbma_init(align_begin_address + (1 << 24), heap.end);
  }
  slab_init();
  #ifdef NTEST
  for (size_t i = 0; i < 10000; i++)
  {
    void* int32_ptr = kalloc(32);
    void* int64_ptr = kalloc(64); 
    void* int128_ptr = kalloc(128);
    void* int256_ptr = kalloc(256);
    void* int512_ptr = kalloc(512);
    void* int1024_ptr = kalloc(1024);
    kfree(int32_ptr);
    kfree(int64_ptr);
    kfree(int128_ptr);
    kfree(int256_ptr);
    kfree(int512_ptr);
    kfree(int1024_ptr);
  }
#endif
}
#else
// 测试代码的 pmm_init ()
static void pmm_init() {
  char *ptr  = malloc(HEAP_SIZE);
  heap.start = ptr;
  heap.end   = ptr + HEAP_SIZE;
  printf("Got %d MiB heap: [%p, %p)\n", HEAP_SIZE >> 20, heap.start, heap.end);
}
#endif

MODULE_DEF(pmm) = {
  .init  = pmm_init,
  .alloc = kalloc,
  .free  = kfree,
};
