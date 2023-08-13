#include "ulib.h"

void hello_test();
void dfs_test();

int main() {
  kputc('H');
  kputc('\n');
  hello_test();
  // dfs_test();
  while (1);
}

void hello_test() {
  int pid = fork();

  if (pid) {
    kputc('P');
  } else {
    sleep(1);
    kputc('C');
  }

  while (1) {
    kputc('J');
    kputc('\n');
    sleep(2);
  }
}
