#include "am.h"
#include "cbma.h"
#include <klib.h>
#include <os.h>

#define MIN_SEQ 0
#define MAX_SEQ 10000

extern CPU_TASKS cpu_list[MAX_CPU];
extern task_t* task_list[MAX_TASK];
extern int task_cnt;

extern spinlock_t sem_init_lock;
extern spinlock_t task_init_lock;

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

bool kmt_try_spin_lock(spinlock_t *lk) {
    TRACE_ENTRY;
    push_off();
    if (try_lock(&lk->lock)) {
#ifdef TRACE_F
        printf("get lock %s\n", lk->name);
#endif
        lk->cpu_num = cpu_current();
        return true;
    }
    printf("cpu %d pop off %s\n", cpu_current());
    pop_off();
    TRACE_EXIT;
    return false;
}


void kmt_spin_lock(spinlock_t *lk) {
    TRACE_ENTRY;
    push_off();
    spin_lock(&lk->lock);
#ifdef TRACE_F
    printf("get lock %s\n", lk->name);
#endif
    lk->cpu_num = cpu_current();
    TRACE_EXIT;
}

void kmt_spin_unlock(spinlock_t *lk) {

    if (!holding(lk)) {
        //TODO: print lock name
        printf("cpu %d try to release lock %s\n", cpu_current(), lk->name);
        panic("release");
    }
    lk->cpu_num = -1;
#ifdef TRACE_F
    printf("release lock %s\n", lk->name);
#endif
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
    TRACE_ENTRY;
    // spin_lock(&sem->lock); // 获得自旋锁
    kmt_spin_lock(&sem->lock);
    sem->resource--; // 自旋锁保证原子性
    if (sem->resource < 0) {
        int cpu_id = cpu_current();
        if (cpu_list[cpu_id].current_task) {
            sem->task_list[sem->task_cnt++] = cpu_list[cpu_id].current_task;
            kmt_spin_lock(&cpu_list[cpu_id].current_task->status);
            cpu_list[cpu_id].current_task->block = true;
            kmt_spin_unlock(&cpu_list[cpu_id].current_task->status);
        }
    }
    kmt_spin_unlock(&sem->lock);
    if (sem->resource < 0) {  // 如果 P 失败，不能继续执行
                // (注意此时可能有线程执行 V 操作)
        yield(); // 引发一次上下文切换
    }
    TRACE_EXIT;
}

void kmt_sem_signal(sem_t *sem) {
    TRACE_ENTRY;
    // spin_lock(&sem->lock); // 获得自旋锁
    kmt_spin_lock(&sem->lock);
    sem->resource++; // 自旋锁保证原子性
    if (sem->resource <= 0 && sem->task_cnt > 0) {
        int task_id = rand() % sem->task_cnt;
        sem->task_list[task_id]->block = false;
        sem->task_list[task_id] = sem->task_list[--sem->task_cnt];
    }
    kmt_spin_unlock(&sem->lock);
    TRACE_EXIT;
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
    kmt_spin_init(&sem->lock, name);
    memset(sem->task_list, '\0', sizeof(task_t *) * K_MAX_TASK);
    kmt_spin_unlock(&sem_init_lock);
}

int kmt_create(task_t *task, const char *name, void (*entry)(void *arg), void *arg) {
    TRACE_ENTRY;
    kmt_spin_lock(&task_init_lock);
    
    memset(task->name, '\0', strlen(name));
    strcpy(task->name, name);
    memset(task->stack, '\0', sizeof(uint8_t) * STACK_SIZE);
    task->context[0] = kcontext((Area) {(void *) task->stack, (void *) (task->stack + STACK_SIZE)}, entry, arg);
    kmt_spin_init(&task->status, name);
    task->block = false;
    task->is_running = false;
    task->pid = task_cnt;
    task->nested_interrupt = 0;
    task_list[task_cnt++] = task;
    kmt_spin_unlock(&task_init_lock);
    TRACE_EXIT;
    return 0;
}

void kmt_teardown(task_t *task) {
    kmt_spin_lock(&task_init_lock);
    // int pid = task->pid;
    // task_list[pid] = task_list[--task_cnt];
    kmt_spin_unlock(&task_init_lock);
}

void idle_thread(void *arg) {
    while (1) {
        yield();
    }
}

Context* kmt_context_save(Event ev, Context *c){
    TRACE_ENTRY;
    int cpu_id = cpu_current();
    task_t* cur_task = cpu_list[cpu_id].current_task;
    if (cur_task->pid > 0) {
        cur_task->context[cur_task->nested_interrupt++] = c;
    } else {
        cur_task->context[0] = c;
    }
    // cur_task->context[0] = c;
    // printf("task %d\n", cpu_id, c);
    // assert(cur_task->nested_interrupt < 3 && cur_task->nested_interrupt >= 0);
    if (cpu_list[cpu_id].save_task && cpu_list[cpu_id].save_task != cur_task) {
        if (cpu_list[cpu_id].save_task->pid > 0) {
            kmt_spin_lock(&cpu_list[cpu_id].save_task->status);
            cpu_list[cpu_id].save_task->is_running = false;
            cpu_list[cpu_id].save_task->nested_interrupt --;
            assert(cpu_list[cpu_id].save_task->nested_interrupt >= 0 && cpu_list[cpu_id].save_task->nested_interrupt < 3);
            kmt_spin_unlock(&cpu_list[cpu_id].save_task->status);
        }
    }
    cpu_list[cpu_id].save_task = cur_task;
    TRACE_EXIT;
    return NULL;
}

Context* kmt_schedule(Event ev, Context *c) {
    TRACE_ENTRY;
    int cpu_id = cpu_current();
    
    if (cpu_list[cpu_id].current_task == NULL) {
        cpu_list[cpu_id].current_task = cpu_list[cpu_id].idle_task;
    }
    bool fine_task = false;
    for (int i = 0; i < task_cnt * 10; i++) {
        int rand_id = (rand() % (task_cnt - 1)) + 1;
        if (task_list[rand_id] == cpu_list[cpu_id].current_task) {
            if (task_list[rand_id]->block) {
                continue;
            }
            cpu_list[cpu_id].current_task = task_list[rand_id];
            cpu_list[cpu_id].current_task->nested_interrupt --;
            fine_task = true;
            panic_on(cpu_list[cpu_id].current_task->block  && fine_task, "Current task is blocked");
            break;
        }
        kmt_spin_lock(&task_list[rand_id]->status);
        if (task_list[rand_id]->is_running || task_list[rand_id]->block) {
            kmt_spin_unlock(&task_list[rand_id]->status);
            continue;
        }
        task_list[rand_id]->is_running = true;
        kmt_spin_unlock(&task_list[rand_id]->status);
        // panic_on(task_list[rand_id]->status.cpu_num != cpu_current(), "cpu num error");
        if (!task_list[rand_id]->block) {
            cpu_list[cpu_id].current_task = task_list[rand_id];
            fine_task = true;
            panic_on(cpu_list[cpu_id].current_task->block  && fine_task, "Current task is blocked");
            break;
        }
    }
    panic_on(cpu_list[cpu_id].current_task->block  && fine_task, "Current task is blocked");
    panic_on(cpu_list[cpu_id].current_task == NULL, "No task to schedule");
    TRACE_EXIT;
    if(!fine_task) {
        cpu_list[cpu_id].current_task = cpu_list[cpu_id].idle_task;
    }
    panic_on(cpu_list[cpu_id].current_task->block, "Current task is blocked");
    Context* ret = cpu_list[cpu_id].current_task->context[cpu_list[cpu_id].current_task->nested_interrupt];
    // Context* ret = cpu_list[cpu_id].current_task->context[0];
    if (!fine_task) {
        ret = cpu_list[cpu_id].idle_task->context[0];
    } else {
        // if (ret == NULL) {
        //     printf("w");
        // }
        // printf("task %s, nest %d\n", cpu_list[cpu_id].current_task->name, cpu_list[cpu_id].current_task->nested_interrupt);
        panic_on(ret == NULL, "No context to schedule");
    }
    // panic_on(ret == NULL && !fine_task, "No context to schedule");
    return ret;
}

void initialize_idle_task(task_t* idle) {
    memset(idle->name, '\0', strlen("idle"));
    strcpy(idle->name, "idle");
    memset(idle->stack, '\0', sizeof(uint8_t) * STACK_SIZE);
    idle->context[0] = kcontext((Area) {(void *) idle->stack, (void *) (idle->stack + STACK_SIZE)}, idle_thread, NULL);
    assert(idle->context);
    kmt_spin_init(&idle->status, "idle");
    idle->pid = 0;
    idle->block = false;
    idle->is_running = false;
    idle->nested_interrupt = 0;
}

void kmt_init() {
    TRACE_ENTRY;
    os->on_irq(MIN_SEQ, EVENT_NULL, kmt_context_save);   
    os->on_irq(MAX_SEQ, EVENT_NULL, kmt_schedule);       

    task_cnt = 1;
    kmt_spin_init(&sem_init_lock, "sem_init_lock");
    kmt_spin_init(&task_init_lock, "task_init_lock");
    for (int i = 0; i < cpu_count(); i++) {
        cpu_list[i].idle_task = pmm->alloc(sizeof(task_t));
        initialize_idle_task(cpu_list[i].idle_task);
        cpu_list[i].save_task = NULL;
        cpu_list[i].current_task = NULL;
        cpu_list[i].interrupt.noff = 0;
        cpu_list[i].interrupt.intena = 0;
    }
    memset(task_list, '\0', sizeof(task_t *) * K_MAX_TASK);
    TRACE_EXIT;
}

MODULE_DEF(kmt) = {
    .init  = kmt_init,
    .create = kmt_create,
    .teardown  = kmt_teardown,
    .spin_init = kmt_spin_init,
    .spin_lock = kmt_spin_lock,
#ifdef DEBUG_NORMAL
    .spin_try_lock = kmt_try_spin_lock,
#endif
    .spin_unlock = kmt_spin_unlock,
    .sem_init = kmt_sem_init,
    .sem_wait = kmt_sem_wait,
    .sem_signal = kmt_sem_signal
};
