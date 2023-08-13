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
  } else {
    sleep(1);
  }

  while (1) {
    kputc('P');
    kputc('\n');
    sleep(2);
  }
}
