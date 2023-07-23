#include "cbma.h"
#include <exception>
#include <os.h>

I_CPU cpu_list[MAX_CPU];

int holding(spinlock_t *lk)
{
    push_off();
    int r;
    r = (lk->lock && lk->cpu_num == cpu_current());
    pop_off();
    return r;
}

void push_off() {
    int old = ienabled();
    iset(false);
    if (cpu_list[cpu_current()].noff == 0) {
        cpu_list[cpu_current()].intena = old;
    }
    cpu_list[cpu_current()].noff += 1;
}

void pop_off() {
    assert(ienabled() == false);
    if (cpu_list[cpu_current()].noff < 1) {
        panic("pop_off() called without push_off()\n");
    }
    cpu_list[cpu_current()].noff -= 1;
    if (cpu_list[cpu_current()].noff == 0 && cpu_list[cpu_current()].intena) {
        iset(true);
    }
}

void kmt_spin_lock(spinlock_t *lk) {
    push_off();
    if (holding(lk)) {
        //TODO: print lock name
        panic("acquire");
    }
    spin_lock(&lk->lock);
    lk->cpu_num = cpu_current();
}

void kmt_spin_unlock(spinlock_t *lk) {
    if (holding(lk)) {
        //TODO: print lock name
        panic("release");
    }
    lk->cpu_num = -1;
    spin_unlock(&lk->lock);
    pop_off();
}

void kmt_spin_init(spinlock_t *lk, const char *name) {
    //TODO: lock and unlock
    lk->lock = SPIN_LOCK_INIT;
    lk->cpu_num = -1;
    strcpy(lk->name, name);
}

void kmt_sem_wait(sem_t *sem) {
    // spin_lock(&sem->lock); // 获得自旋锁
    kmt_spin_lock(&sem->lock);
    sem->resource--; // 自旋锁保证原子性
    if (sem->resource < 0) {
        int cpu_id = cpu_current();
        if (current[cpu_id]) {
            sem->task_list[sem->task_cnt++] = current[cpu_id];
            current[cpu_id]->block = 1;
        }
    }
    kmt_spin_unlock(&sem->lock);
    if (sem->resource < 0) {  // 如果 P 失败，不能继续执行
                // (注意此时可能有线程执行 V 操作)
        yield(); // 引发一次上下文切换
    }
}

MODULE_DEF(kmt) = {
    // .init  = kmt_init,
    // .create = kcreate,
    // .teardown  = kteardown,
    .spin_init = kmt_spin_init,
    .spin_lock = kmt_spin_lock,
    .spin_unlock = kmt_spin_unlock,
    // .sem_init = ksem_init,
    .sem_wait = kmt_sem_wait,
    // .sem_signal = ksem_signal
};
