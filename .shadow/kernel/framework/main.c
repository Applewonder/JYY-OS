#include <kernel.h>
#include <klib.h>

int main() {
  os->init();
  mpe_init(os->run);
#ifdef TEST
``for (size_t i = 0; i < 10000; i++)
{
  kmalloc(i);
}


#endif
  return 1;
}
