#include "cbma.h"
#include <os.h>

CPU_TASKS cpu_list[MAX_CPU];
task_t* task_list[MAX_TASK];
int task_cnt;

spinlock_t sem_init_lock;
spinlock_t task_init_lock;

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
    if (cpu_list[cpu_current()].interrupt.noff == 0) {
        cpu_list[cpu_current()].interrupt.intena = old;
    }
    cpu_list[cpu_current()].interrupt.noff += 1;
}

void pop_off() {
    assert(ienabled() == false);
    if (cpu_list[cpu_current()].interrupt.noff < 1) {
        panic("pop_off() called without push_off()\n");
    }
    cpu_list[cpu_current()].interrupt.noff -= 1;
    if (cpu_list[cpu_current()].interrupt.noff == 0 && cpu_list[cpu_current()].interrupt.intena) {
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
        if (cpu_list[cpu_id].current_task) {
            sem->task_list[sem->task_cnt++] = cpu_list[cpu_id].current_task;
            cpu_list[cpu_id].current_task->block = true;
        }
    }
    kmt_spin_unlock(&sem->lock);
    if (sem->resource < 0) {  // 如果 P 失败，不能继续执行
                // (注意此时可能有线程执行 V 操作)
        yield(); // 引发一次上下文切换
    }
}

void kmt_sem_signal(sem_t *sem) {
    // spin_lock(&sem->lock); // 获得自旋锁
    kmt_spin_lock(&sem->lock);
    sem->resource++; // 自旋锁保证原子性
    if (sem->resource <= 0 && sem->task_cnt > 0) {
        int task_id = rand() % sem->task_cnt;
        sem->task_list[task_id]->block = false;
        sem->task_list[task_id] = sem->task_list[--sem->task_cnt];
    }
    kmt_spin_unlock(&sem->lock);
}

void kmt_sem_init(sem_t *sem, const char *name, int value) {
    // TODO
    // sem->resource = 0;
    // sem->task_cnt = 0;
    // spin_init(&sem->lock, "sem");
    // for (int i = 0; i < K_MAX_TASK; i++) {
    //     sem->task_list[i] = NULL;
    // }
    kmt_spin_lock(&sem_init_lock);
    memset(sem->name, '\0', strlen(name));
    strcpy(sem->name, name);
    sem->resource = value;
    sem->task_cnt = 0;
    kmt_spin_init(&sem->lock, "sem");
    memset(sem->task_list, '\0', sizeof(task_t *) * K_MAX_TASK);
    kmt_spin_unlock(&sem_init_lock);
}

int kmt_create(task_t *task, const char *name, void (*entry)(void *arg), void *arg) {
    kmt_spin_lock(&task_init_lock);
    
    memset(task->name, '\0', strlen(name));
    strcpy(task->name, name);
    memset(task->stack, '\0', sizeof(uint8_t) * STACK_SIZE);
    task->context = kcontext((Area) {(void *) task->stack, (void *) (task->stack + STACK_SIZE)}, entry, arg);
    kmt_spin_init(&task->status, name);

    task->id = task_cnt;
    task_list[task_cnt++] = task;
    kmt_spin_unlock(&task_init_lock);
    return 0;
}

void kmt_teardown(task_t *task) {
    kmt_spin_lock(&task_init_lock);
    int id = task->id;
    task_list[id] = task_list[--task_cnt];
    kmt_spin_unlock(&task_init_lock);
}

void kmt_init() {
    task_cnt = 0;
    kmt_spin_init(&sem_init_lock, "sem_init_lock");
    kmt_spin_init(&task_init_lock, "task_init_lock");
    for (int i = 0; i < cpu_count(); i++) {
        cpu_list[i].current_task = NULL;
        cpu_list[i].interrupt.noff = 0;
        cpu_list[i].interrupt.intena = 0;
    }
    for (int i = 0; i < MAX_TASK; i++) {
        task_list[i] = NULL;
    }
}



MODULE_DEF(kmt) = {
    // .init  = kmt_init,
    .create = kmt_create,
    .teardown  = kmt_teardown,
    .spin_init = kmt_spin_init,
    .spin_lock = kmt_spin_lock,
    .spin_unlock = kmt_spin_unlock,
    .sem_init = kmt_sem_init,
    .sem_wait = kmt_sem_wait,
    .sem_signal = kmt_sem_signal
};
