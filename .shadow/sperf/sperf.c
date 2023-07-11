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

#define SLASH 47
#define MAX_SYSCALLS 256

typedef struct {
    char* name;
    double time_cnt;
} Syscall;

Syscall syscalls[MAX_SYSCALLS];
int num_syscalls = 0;
double total_time = 0;

int compare(const void *a, const void *b) {
    double diff = ((Syscall*)b)->time_cnt - ((Syscall*)a)->time_cnt;
    return (diff > 0.0) - (diff < 0.0);
}

void parse_store_syscall(Syscall the_call) {
  printf("%s, (%.2f%%)\n", the_call.name, (the_call.time_cnt / (float)(total_time) * 100));
}

void print_max_five_syscall() {
  int print_count = 0;
  if (num_syscalls < 5) {
    print_count = num_syscalls;
  } else {
    print_count = 5;
  }
  for (int i = 0; i < print_count; i++)
  {
    parse_store_syscall(syscalls[i]);
  }
}

void print_stats(int signum) {
  print_max_five_syscall();
  alarm(1);
}

char** build_args(int argc, char* argv[], char* program) {
  char** args = malloc((argc + 4) * sizeof(char*));
  args[0] = "strace";
  args[1] = "-T";
  args[2] = program;

  for (int i = 2; i < argc; i++) {
      args[i + 1] = argv[i];
  }
  args[argc + 1] = ">";
  args[argc + 2] = "/dev/null";
  args[argc + 3] = NULL;
  return args;
}

char* get_prog_dir(char *command, bool need_dir) {
  if (need_dir) {
    char* program = command;
    char* path = getenv("PATH");
    char* the_path = strdup(path);
    printf("%s\n", path);
    char* directory = strtok(the_path, ":");
    while(directory != NULL) {
        DIR* dir = opendir(directory);
        struct dirent* entry;
        while((entry = readdir(dir)) != NULL) {
            if(strcmp(entry->d_name, program) == 0) {
              return directory;
            }
        }
        closedir(dir);
        directory = strtok(NULL, ":");
    }
  }
  return NULL;
}

char* get_exec_prog(char* command) {
  bool need_dir = false;
  if (command[0] != SLASH) {
    need_dir = true;
  }
  char* prog_dir = get_prog_dir(command, need_dir);
  if (prog_dir != NULL) {
    size_t length = strlen(prog_dir) + strlen(command) + 2;
    char *result = malloc(length);
    strcpy(result, prog_dir);
    strcat(result, "/");
    strcat(result, command);
    return result;
  } 
  return command;
}

char* build_sub_path() {
  char* main_path = getenv("PATH");
  char *path_env = malloc(strlen("PATH=") + strlen(main_path) + 1);
  strcpy(path_env, "PATH=");
  strcat(path_env, main_path);
  return path_env;
}

void store_in_matrix(char* buffer) {
  const char *l_ptr = strrchr(buffer, '<');
  if (l_ptr == NULL) {
    return;
  }
  size_t pos = l_ptr - buffer + 1;
  const char *r_ptr = strrchr(buffer, '>');
  if (r_ptr == NULL) {
    return;
  }
  size_t len = r_ptr - l_ptr - 1;
  if (len <= 0) {
    return;
  }
  char* time_used = malloc(sizeof(char*)*(len + 1));
  strncpy(time_used, l_ptr + 1, len);
  time_used[len] = '\0';
  double call_time = atof(time_used);
  if (call_time <= 0) {
    return;
  }
  const char *name_l_ptr = strrchr(buffer, '(');
  if (name_l_ptr == NULL) {
    return;
  }
  size_t name_len = name_l_ptr - buffer;
  char* name = malloc(sizeof(char*)*(name_len + 1));
  strncpy(name, buffer, name_len);
  name[len] = '\0';
  bool stored = false;
  for (size_t i = 0; i < num_syscalls; i++)
  {
    if (!strcmp(syscalls[i].name, name)) {
      syscalls[i].time_cnt += call_time;
      total_time += call_time;
      stored = true;
    }
    continue;
  }
  if (!stored) {
    if (num_syscalls < MAX_SYSCALLS) {
      syscalls[num_syscalls].name = name;
      syscalls[num_syscalls].time_cnt = call_time;
      num_syscalls += 1;
      total_time += call_time;
    } else {
      syscalls[MAX_SYSCALLS - 1].name = name;
      syscalls[MAX_SYSCALLS - 1].time_cnt = call_time;
      total_time += call_time;
    }
  }
  qsort(syscalls, num_syscalls, sizeof(Syscall), compare);
}

int main(int argc, char *argv[]) {
  char* exec_strace = get_exec_prog("strace");
  char* exec_prog = get_exec_prog(argv[1]);
  char** args = build_args(argc, argv, exec_prog);
  char* env_path = build_sub_path();
  int pipefd[2];
  char *envp[] = {env_path, NULL};
  pid_t pid;

  // 创建一个管道
  if (pipe(pipefd) == -1) {
      perror("pipe");
      exit(EXIT_FAILURE);
  }

  pid = fork();
  if (pid == -1) {
      perror("fork");
      exit(EXIT_FAILURE);
  }

  if (pid == 0) {
      close(pipefd[0]);

      dup2(pipefd[1], STDERR_FILENO);

      execve(exec_strace, args, envp);

      // perror("execvp");
      exit(EXIT_FAILURE);
  } else {
      close(pipefd[1]);
      signal(SIGALRM, print_stats);

      alarm(1);

      while (1) {
        char buffer[1024];
        ssize_t count;
        while (fgets(buffer, sizeof(buffer), fdopen(pipefd[0], "r"))) {
            store_in_matrix(buffer);
            // printf("%s", buffer);
        }
      }
      wait(NULL);
  }
  return 0;
}