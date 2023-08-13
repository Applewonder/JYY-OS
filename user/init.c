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

  char fmt;
  if (pid) {
    fmt = 'P';
  } else {
    sleep(1);
    fmt = 'C';
  }

  while (1) {
    kputc(fmt);
    kputc('\n');
    sleep(2);
  }
}
