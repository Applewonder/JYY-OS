#include "common.h"
#include "os.h"

CPU_TASKS cpu_list[MAX_CPU];
task_t* task_list[MAX_TASK];
int task_cnt = 1;