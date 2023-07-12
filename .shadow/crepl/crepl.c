#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <stdlib.h>
#include <signal.h>
#include <assert.h>
#include <fcntl.h>
#include <dlfcn.h>

int exp_cnt = 0;

bool judge_eval_or_func(char* line) {
  if (line[0] == 'i' && line[1] == 'n' && line[2] == 't') {
    return true;
  } 
  return false;
}

char* get_only_name() {
  char* wrapper_name = malloc(sizeof(char) * 256);
  sprintf(wrapper_name, "____expr_wrapper_%d", exp_cnt);
  exp_cnt ++;
  return wrapper_name;
}

char* get_the_name(int num) {
  char* wrapper_name = malloc(sizeof(char) * 256);
  sprintf(wrapper_name, "____expr_wrapper_%d", num);
  return wrapper_name;
}

char* build_wrapper(char* line) {
  char* func_start = "int ";
  char* func_name = get_only_name();
  char* func_middle = "() {return ";
  char* func_end = ";}";
  char* func_full = malloc(sizeof(char) * 512);
  strcpy(func_full, func_start);
  strcat(func_full, func_name);
  strcat(func_full, func_middle);
  strcat(func_full, line);
  strcat(func_full, func_end);
  return func_full;
}

int try_compile(char* line, char* dl_name, int* exp_num, bool is_exp) {
    char template[] = "/tmp/tempfileXXXXXX";
    int fd = mkstemp(template);

    char* output_file = malloc(sizeof(char) * 256);
    snprintf(output_file, 256, "%s.so", template);

    if (fd == -1) {
        perror("mkstemp");
        exit(EXIT_FAILURE);
    }

    char new_filename[256];
    snprintf(new_filename, sizeof(new_filename), "%s.c", template);

    // Rename the file
    if (rename(template, new_filename) == -1) {
        perror("rename");
        exit(EXIT_FAILURE);
    }

    FILE* file = fdopen(fd, "w");
    if (file == NULL) {
        perror("fdopen");
        exit(EXIT_FAILURE);
    }

    fprintf(file, "%s", line);
    fclose(file);

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process
        
        execlp("gcc", "gcc", "-w", "-shared", "-o", output_file, new_filename, (char *) NULL);
        perror("execlp");  // execlp returns only on error
        exit(EXIT_FAILURE);
    } else {
        // Parent process
        int status;
        waitpid(pid, &status, 0);  // Wait for the child to finish
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
          if (!is_exp) {
            printf("OK\n");
          } else {
            sprintf(dl_name, "%s", output_file);
            *exp_num = exp_cnt - 1;
          }
          return true;
        } else {
          if (!is_exp) {
            printf("Compile Error\n");
          } else {
            printf("Invalid Exp\n");
          }
          return false;
        }
    }
}

int get_exp_res(char* dl_path, int num) {
  void* handle = dlopen(dl_path, RTLD_LAZY);
  if (!handle) {
      fprintf(stderr, "%s\n", dlerror());
      return 1;
  }

  dlerror();
  int (*func)() = dlsym(handle, get_the_name(num));
  char* error = dlerror();
  if (error) {
        fprintf(stderr, "%s\n", error);
        return 1;
  }
  int res = func();

  dlclose(handle);
  return res;
}

int main(int argc, char *argv[]) {
  static char line[4096];
  while (1) {
    printf("crepl> ");
    fflush(stdout);
    if (!fgets(line, sizeof(line), stdin)) {
      break;
    }
    line[strcspn(line, "\n")] = '\0';
    bool is_func = judge_eval_or_func(line);
    if (is_func) {
      //TODO: compile is to dl
      try_compile(line, NULL, NULL, false);
    } else {
      // bool is_eval = eval_judger(line);
      char* ready_to_eval = build_wrapper(line);
      char* dl_path = malloc(sizeof(char) * 256);
      int* the_exp = malloc(sizeof(int));
      bool is_valid = try_compile(ready_to_eval, dl_path, the_exp, true);
      if (is_valid) {
        int res = get_exp_res(dl_path, *the_exp);
        printf("%d\n", res);
      }
    }
  }
}
