#include <os.h>
#include <syscall.h>

// #include "initcode.inc"

void uproc_init() {
    vme_init(pmm->alloc, pmm->free);
}

int kputc(task_t *task, char ch) {
    putch(ch); // safe for qemu even if not lock-protected
    return 0;
}

int kgetpid(task_t *task) {
    assert(task != NULL);
    assert(task->id >= 1 && task->id < 32768);
    return task->id;
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
