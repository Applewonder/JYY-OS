#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdbool.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct neighbour* Neighbour;

struct neighbour{
  int pid;
  Neighbour next;
};

Neighbour fa_node[4194304];
char name[4194304][50];

bool is_num(char* name) {
  int len = strlen(name);
  for (int i = 0; i < len; i ++) {
    if (name[i] < 48 || name[i] > 57) return false;
  }
  return true;
}

int find_fpid_and_name(char* path, int pid){
  char buffer[30];
  int ppid;
  char* token;
  char cur_name[50];
  FILE* fp = fopen(strcat(path, "/status"), "r");
  bool is_find_ppid = false;
  bool is_find_name = false;
  while (fgets(buffer, 20, fp) != NULL) {
      if (strncmp(buffer, "Name:", 5) == 0) {
          
          token = strtok(buffer, "\t");
          token = strtok(NULL, "\t");
          strcpy(cur_name, token);
          cur_name[strlen(cur_name)-1] = 0;
          is_find_ppid = true;
          if (pid == 492) {
            printf("%s\n", token);
            printf("%s\n", cur_name);
            exit(0);
          }
      }
      if (strncmp(buffer, "PPid:", 5) == 0) {
          token = strtok(buffer, "\t");
          token = strtok(NULL, "\t");
          token[strlen(token)-1] = 0;
          sscanf(token,"%d",&ppid);
          is_find_name = true;
      }
      if(is_find_name && is_find_ppid) {
        strcpy(name[pid], cur_name);
        break;
      }
  }
  
  return ppid;
}

void add_edge(int ppid, int pid){
  if (fa_node[ppid] == NULL) {
    Neighbour new_neib = malloc(sizeof(struct neighbour));
    new_neib->next = NULL;
    new_neib->pid = pid;
    fa_node[ppid] = new_neib;
    return;
  }
  Neighbour cur_neib = fa_node[ppid];
  while (cur_neib != NULL) {
    if (cur_neib->next == NULL) {
      Neighbour new_neib = malloc(sizeof(struct neighbour));
      new_neib->next = NULL;
      new_neib->pid = pid;
      cur_neib->next = new_neib;
      return;
    } else if(cur_neib->pid <= pid && cur_neib->next->pid >= pid) {
      Neighbour new_neib = malloc(sizeof(struct neighbour));
      new_neib->next = cur_neib->next;
      new_neib->pid = pid;
      cur_neib->next = new_neib;
      return;
    }
    cur_neib = cur_neib->next;
  }
}

void build_tree(){
  int dir_count = 0;
  struct dirent* dent;
  DIR* srcdir = opendir("/proc");
  while((dent = readdir(srcdir)) != NULL)
  {
      struct stat st;
      if(strcmp(dent->d_name, ".") == 0 || strcmp(dent->d_name, "..") == 0) continue;
      if (fstatat(dirfd(srcdir), dent->d_name, &st, 0) < 0) continue;
      if (S_ISDIR(st.st_mode) && is_num(dent->d_name)) {
        int pid = 0;
        sscanf(dent->d_name,"%d",&pid);

        char* tar_sub_dir = "/proc/";

        char *tmp_name = (char *) malloc(strlen(tar_sub_dir) + strlen(dent->d_name) + 10);
        strcpy(tmp_name, tar_sub_dir);
        strcat(tmp_name, dent->d_name);
        int ppid = find_fpid_and_name(tmp_name, pid);
        add_edge(ppid, pid);
      }
  }
  closedir(srcdir);
}

void tree_print(bool if_print_line_number, int pid, int count) {
  for (int i = 0; i < count; i++)
  {
    printf(" ");
  }
  Neighbour cur_neib = fa_node[pid];
  printf("%s", name[pid]);
  if (if_print_line_number) {
    printf("(%d)", pid);
  }
  printf("\n");
  while (cur_neib != NULL) {
    tree_print(if_print_line_number, cur_neib->pid, count + 1);
    cur_neib = cur_neib->next;
  }
  return;
}

void thread_tree_print(bool if_print_line_number) {
  build_tree();
  tree_print(if_print_line_number, 1, 0);
}

int main(int argc, char *argv[]) {
  bool is_p = false;
  bool is_n = false;
  for (int i = 1; i < argc; i++) {
    assert(argv[i]);
    // if (i == 0){
    //   assert(!strcmp(argv[i], "./pstree"));
    // }
    if (!strcmp(argv[i], "-V") || !strcmp(argv[i], "--version")) {
      printf("pstree (PSmisc) APPLETREE SPECIAL VERSION\n");
      return 0;
    }
    if (!strcmp(argv[1], "-n") || !strcmp(argv[1], "--numeric-sort")) {
      is_n = true;
    } else if (!strcmp(argv[1], "-p") || !strcmp(argv[1], "--show-pids")) {
      is_p = true;
    } else {
      return 0;
    }
  }
  // assert(0);
  thread_tree_print(is_p || is_n);
  assert(!argv[argc]);
  return 0;
}
