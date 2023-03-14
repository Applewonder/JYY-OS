#include <stdio.h>
#include <assert.h>
#include <string.h>

typedef struct neighbour* Neighbour;

struct neighbour{
  int pid;
  Neighbour next;
};

Neighbour fa_node[4194304];

void build_tree(){
  



  FILE *fp = fopen(filename, "r");
  if (fp) {
    // 用fscanf, fgets等函数读取
    fclose(fp);
  } else {
    // 错误处理
  }
}

void thread_tree_print(int if_print_line_number, int if_print_follow_pid) {
  build_tree();
}

int main(int argc, char *argv[]) {
  int is_p = 0;
  int is_n = 0;
  for (int i = 0; i < argc; i++) {
    assert(argv[i]);
    if (i == 0){
      assert(!strcmp(argv[i], "./pstree"));
    }
    if (!strcmp(argv[i], "-V") || !strcmp(argv[i], "--version")) {
      printf("pstree (PSmisc) APPLETREE SPECIAL VERSION\n");
      return 0;
    }
    if (!strcmp(argv[1], "-n") || !strcmp(argv[1], "--numeric-sort")) {
      is_n = 1;
    } else if (!strcmp(argv[1], "-p") || !strcmp(argv[1], "--show-pids")) {
      is_p = 1;
    } else {
      return 0;
    }
  }
  thread_tree_print(is_p || is_n, is_n);
  assert(!argv[argc]);
  return 0;
}
