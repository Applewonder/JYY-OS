#include <kernel.h>
#include <klib.h>

int main() {
  os->init();
  mpe_init(os->run);
#ifdef NTEST
  for (size_t i = 0; i < 10000; i++)
  {
    void* int32_ptr = kmalloc(32);
    void* int64_ptr = kmalloc(64); 
    void* int128_ptr = kmalloc(128);
    void* int256_ptr = kmalloc(256);
    void* int512_ptr = kmalloc(512);
    void* int1024_ptr = kmalloc(1024);
    kfree(int32_ptr);
    kfree(int64_ptr);
    kfree(int128_ptr);
    kfree(int256_ptr);
    kfree(int512_ptr);
    kfree(int1024_ptr);
  }


#endif
  return 1;
}
