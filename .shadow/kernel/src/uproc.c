#include <os.h>
#include <syscall.h>

extern CPU_TASKS cpu_list[MAX_CPU];
extern task_t* task_list[MAX_TASK];
extern int task_cnt;

// #include "initcode.inc"
static spinlock_t task_init_lock;

int uproc_create(task_t *task, const char *name, void (*entry)(void *arg), void *arg) {
    TRACE_ENTRY;
    kmt->spin_lock(&task_init_lock);
    
    memset(task->name, '\0', strlen(name));
    strcpy(task->name, name);
    memset(task->stack, '\0', sizeof(uint8_t) * STACK_SIZE);
    task->context = kcontext((Area) {(void *) task->stack, (void *) (task->stack + STACK_SIZE)}, entry, arg);
    kmt->spin_init(&task->status, name);
    task->block = false;
    task->is_running = false;
    task->pid = task_cnt;
    task_list[task_cnt++] = task;
    kmt->spin_unlock(&task_init_lock);
    TRACE_EXIT;
    return 0;
}

void uproc_init() {
    vme_init(pmm->alloc, pmm->free);
}

int kputc(task_t *task, char ch) {
    putch(ch); // safe for qemu even if not lock-protected
    return 0;
}

int kgetpid(task_t *task) {
    assert(task != NULL);
    assert(task->pid >= 1 && task->pid < 32768);
    return task->pid;
}

int ksleep(task_t *task, int second) {
    uint64_t wake_time = io_read(AM_TIMER_UPTIME).us + second * 1000000;
    while (io_read(AM_TIMER_UPTIME).us < wake_time) {
        yield();
    }
    return 0;
}





MODULE_DEF(uproc) = {
    .init = uproc_init,
    .kputc = kputc,
    .getpid = kgetpid,
    .sleep = ksleep,
    
};
