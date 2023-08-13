#include <os.h>
#include <syscall.h>
#include <threads.h>
#include "klib-macros.h"
#include "user.h"
#include "initcode.inc"

extern CPU_TASKS cpu_list[MAX_CPU];
extern task_t* task_list[MAX_TASK];
extern int task_cnt;
extern spinlock_t task_init_lock;



static spinlock_t pid_lock;
static spinlock_t cow_lock;

static int* cow_table;
// static int pgsize;
static PID_Q* p_head;
static PID_Q* p_tail;

//Warning! Code is not debugged!

int get_pid() {
    kmt->spin_lock(&pid_lock);
    if (p_head == NULL) {
        return 0;
    } else {
        int id = p_head->pid;
        pmm->free(p_head);
        p_head = p_head->next;
        if (p_head == NULL) {
            p_tail = NULL;
        }
        return id;
    }
    kmt->spin_unlock(&pid_lock);
}

void store_pid(int pid) {
    kmt->spin_lock(&pid_lock);
    PID_Q* new_node = pmm->alloc(sizeof(PID_Q));
    new_node->pid = pid;
    new_node->next = NULL;
    if (p_head == NULL) {
        panic_on(p_tail != NULL, "pid queue error");
        p_head = new_node;
        p_tail = new_node;
    } else {
        p_tail->next = new_node;
        p_tail = new_node;
    }
    kmt->spin_unlock(&pid_lock);
}

int new_pid() {
    if (task_cnt == 32768) {
        return get_pid();
    } else {
       return task_cnt++;
    }
}

int uproc_create(task_t *task, const char *name) {
    TRACE_ENTRY;
    kmt->spin_lock(&task_init_lock);
    
    memset(task->name, '\0', strlen(name));
    strcpy(task->name, name);
    memset(task->stack, '\0', sizeof(uint8_t) * STACK_SIZE);
    task->as = pmm->alloc(sizeof(AddrSpace));
    protect(task->as);
    task->context[0] = ucontext(task->as, (Area) {(void *) task->stack, (void *) (task->stack + STACK_SIZE)}, task->as->area.start);
    kmt->spin_init(&task->status, name);
    kmt->spin_init(&task->vme_lock, name);
    kmt->spin_init(&task->mp_lock, name);
    task->block = false;
    task->is_running = false;
    task->pid = new_pid();
    task->ppid = 0;
    task->vm_area_head = NULL;
    task->vm_area_tail = NULL;
    task->mapped_page_head = NULL;
    task->state = 0;
    task->retstate = 0;
    task->killed = false;
    task->nested_interrupt = 0;
    task_list[task->pid] = task;
    kmt->spin_unlock(&task_init_lock);
    TRACE_EXIT;
    return 0;
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

int64_t kuptime(task_t *task) {
    return io_read(AM_TIMER_UPTIME).us / 1000;
}

void* get_suitable_addr(task_t *task, void *start_addr, int length) {
    VME_AREA* head = task->vm_area_head;
    VME_AREA* cur = head;
    void* addr = start_addr;
    void* end_addr = (void*) ROUNDUP((uintptr_t) start_addr + length, task->as->pgsize);

    if (head == NULL) {
        return addr;
    }

    while (cur != NULL) {
        if (cur->vm_start >= end_addr) {
            break;
        } else if (cur->vm_end <= addr) {
            cur = cur->vm_next;
            continue;
        } else {
            addr = cur->vm_end;
            end_addr = (void*) ROUNDUP((uintptr_t) addr + length, task->as->pgsize);
            cur = cur->vm_next;
        }
    }
    assert(end_addr <= task->as->area.end);
    return addr;
}

VME_AREA* new_vme_node(task_t *task, void* addr, int length, int prot, int flags) {
    VME_AREA* node = (VME_AREA*) pmm->alloc(sizeof(VME_AREA));
    void* start_addr = (void*) ROUNDUP((uintptr_t) addr, task->as->pgsize);
    void* right_addr = get_suitable_addr(task, start_addr, length);
    node->vm_start = right_addr;
    node->vm_end = (void*) ROUNDUP((uintptr_t) right_addr + length, task->as->pgsize);
    node->vm_flags = flags;
    node->vm_prot = prot;

    node->vm_next = NULL;
    node->vm_prev = NULL;

    node->vm_as = task->as;
    return node;
}

bool insert_vme_node(task_t *task, VME_AREA* node) {
    VME_AREA* head = task->vm_area_head;
    VME_AREA* cur = head;
    if (head == NULL) {
        task->vm_area_head = node;
        task->vm_area_tail = node;
        return true;
    }

    while (cur != NULL) {
        if (cur->vm_start >= node->vm_end) {
            break;
        } else if (cur->vm_end <= node->vm_start) {
            cur = cur->vm_next;
            continue;
        } else {
            return false;
        }
    }

    if (cur == NULL) {
        VME_AREA* tail = task->vm_area_tail;
        tail->vm_next = node;
        node->vm_prev = tail;
        task->vm_area_tail = node;
    } else {
        VME_AREA* prev = cur->vm_prev;
        if (prev == NULL) {
            task->vm_area_head = node;
        } else {
            prev->vm_next = node;
        }
        node->vm_prev = prev;
        node->vm_next = cur;
        cur->vm_prev = node;
    }
    return true;
}

VME_AREA* find_vme_node(task_t *task, uintptr_t addr, uintptr_t end_addr) {
    VME_AREA* head = task->vm_area_head;
    VME_AREA* cur = head;
    while (cur != NULL) {
        if ((uintptr_t)cur->vm_start <= addr && (uintptr_t)cur->vm_end >= end_addr) {
            return cur;
        } else if ((uintptr_t)cur->vm_end <= addr) {
            cur = cur->vm_next;
        }
        break;
    }
    return NULL;
}

M_PAGE* new_mapped_page(task_t* task, void* va, void* pa) {
    M_PAGE* node = (M_PAGE*) pmm->alloc(sizeof(M_PAGE));
    node->va = va;
    node->pa = pa;
    node->as = task->as;
    node->next = NULL;
    return node;
}

void insert_map_page(task_t* task, M_PAGE* pg) {
    pg->next = task->mapped_page_head;
    task->mapped_page_head = pg;
}

void delete_map_page(task_t* task, void* va) {
    M_PAGE* head = task->mapped_page_head;
    M_PAGE* cur = head;
    M_PAGE* prev = NULL;
    while (cur != NULL) {
        if (cur->va == va) {
            if (prev == NULL) {
                task->mapped_page_head = cur->next;
            } else {
                prev->next = cur->next;
            }
            pmm->free(cur);
            return;
        }
        prev = cur;
        cur = cur->next;
    }
    panic_on(cur == NULL, "mapped page not found");
}

M_PAGE* find_mapped_page(task_t* task, void* va) {
    M_PAGE* head = task->mapped_page_head;
    M_PAGE* cur = head;
    while (cur != NULL) {
        if (cur->va == va) {
            return cur;
        }
        cur = cur->next;
    }
    return NULL;
}

int convert_mmap_prot_to_vm_prot(int prot) {
    int vm_prot = 0;
    if (prot & PROT_READ) {
        vm_prot |= MMAP_READ;
    }
    if (prot & PROT_WRITE) {
        vm_prot |= MMAP_WRITE;
    }
    if (prot == PROT_NONE) {
        vm_prot = MMAP_NONE;
    }
    return vm_prot;
}

int get_cow_cnt(task_t* task, void* pa) {
    intptr_t index = ROUNDDOWN((intptr_t) pa, task->as->pgsize) / task->as->pgsize;
    kmt->spin_lock(&cow_lock);
    int ret = cow_table[index];
    kmt->spin_unlock(&cow_lock);
    return ret;
}

void set_cow_cnt(task_t* task, void* pa, int cnt) {
    intptr_t index = ROUNDDOWN((intptr_t) pa, task->as->pgsize) / task->as->pgsize;
    kmt->spin_lock(&cow_lock);
    cow_table[index] = cnt;
    kmt->spin_unlock(&cow_lock);
}

void inc_cow_cnt(task_t* task, void* pa) {
    intptr_t index = ROUNDDOWN((intptr_t) pa, task->as->pgsize) / task->as->pgsize;
    kmt->spin_lock(&cow_lock);
    cow_table[index]++;
    kmt->spin_unlock(&cow_lock);
}

void dec_cow_cnt(task_t* task, void* pa) {
    intptr_t index = ROUNDDOWN((intptr_t) pa, task->as->pgsize) / task->as->pgsize;
    kmt->spin_lock(&cow_lock);
    cow_table[index]--;
    kmt->spin_unlock(&cow_lock);
}

void pgmap(task_t* task, void* va, void* pa, int prot, bool cow_cnt) {
    M_PAGE* page = new_mapped_page(task, va, pa);
    if (cow_cnt) {
        inc_cow_cnt(task, pa);
    }
    insert_map_page(task, page);
    int vm_prot = convert_mmap_prot_to_vm_prot(prot);
    map(task->as, va, pa, vm_prot);
}

void pgunmap(task_t* task, void* va) {
    delete_map_page(task, va);
    map(task->as, va, NULL, MMAP_NONE);
}

void delete_mapping(task_t *task, uintptr_t start_addr, uintptr_t end_addr) {
    uintptr_t cur = start_addr;  
    for(; cur < end_addr; cur += task->as->pgsize) {
        M_PAGE* pg = find_mapped_page(task, (void*) cur);
        if(pg != NULL) {
            pgunmap(task, (void*) cur);
        }
    }
}


/*
mmap:
Alloc a virtual memory area with size of length, start from addr (not forced to be addr). 
Maintain a linked list to record flags and prot level. 
Maintain a linked list of all mapped pages.

unmap:
remove the mapping from the linked list, and free the physical memory.
*/

void *kmmap(task_t *task, void *addr, int length, int prot, int flags) {
    assert(task != NULL);
    assert(task->as != NULL);
    assert(addr != NULL);
    assert(length > 0);
    if (flags == MAP_UNMAP) {
        assert(prot == PROT_NONE);
        uintptr_t start_addr = ROUNDDOWN((uintptr_t) addr, task->as->pgsize);
        uintptr_t end_addr = ROUNDUP((uintptr_t) addr + length, task->as->pgsize);
        kmt->spin_lock(&task->vme_lock);
        VME_AREA* own_space = find_vme_node(task, start_addr, end_addr);
        kmt->spin_unlock(&task->vme_lock);
        if (own_space != NULL) {
            kmt->spin_lock(&task->mp_lock);
            delete_mapping(task, start_addr, end_addr);
            kmt->spin_unlock(&task->mp_lock);
        }
        return NULL;
    }
    
    VME_AREA* node = new_vme_node(task, addr, length, prot, flags);
    kmt->spin_lock(&task->vme_lock);
    bool success = insert_vme_node(task, node);
    panic_on(!success, "mmap failed");
    kmt->spin_unlock(&task->vme_lock);
    return node->vm_start;
}

// void divide_vme_node(task_t *task, VME_AREA *node, uintptr_t addr, int new_prot) {
//     assert(node != NULL);
//     assert((uintptr_t)node->vm_start <= addr && (uintptr_t)node->vm_end >= addr);
//     uintptr_t new_end = addr + task->as->pgsize;
//     uintptr_t old_end = (uintptr_t) node->vm_end;
//     uintptr_t old_start = (uintptr_t) node->vm_start;
//     if (old_start == addr) {
//         if (old_end == new_end) {
//             node->vm_prot = new_prot;
//             return;
//         } else {
//             node->vm_end = (void*) new_end;
//             VME_AREA* new_node = new_vme_node(task, (void*) new_end, old_end - new_end, node->vm_prot, node->vm_flags);
//             node->vm_prot = new_prot;
//             insert_vme_node(task, new_node);
//             return;
//         }
//     } else {
//         if (old_end == new_end) {
//             node->vm_end = (void*) addr;
//             VME_AREA* new_node = new_vme_node(task, (void*) addr, old_end - addr, new_prot, node->vm_flags);
//             insert_vme_node(task, new_node);
//             return;
//         } else {
//             node->vm_end = (void*) addr;
//             VME_AREA* new_node = new_vme_node(task, (void*) addr, task->as->pgsize, new_prot, node->vm_flags);
//             insert_vme_node(task, new_node);
//             VME_AREA* new_node2 = new_vme_node(task, (void*) new_end, old_end - new_end, node->vm_prot, node->vm_flags);
//             insert_vme_node(task, new_node2);
//             return;
//         }
//     }
//     panic("divide_vme_node failed");
//     return;
// }
/*
get a node from the mapped page linked list.
unmap;
get the page's protect level from vma;
remove its write permission;
divide the vma node;
map the page with the new protect level;
*/
void fork_cow_mapping(task_t *task) {
    kmt->spin_lock(&task->mp_lock);
    M_PAGE* head = task->mapped_page_head;
    M_PAGE* cur = head;
    while (cur != NULL) {
        if (get_cow_cnt(task, cur->pa) == 0) {
            uintptr_t va = (uintptr_t) cur->va;
            uintptr_t pa = (uintptr_t) pmm->alloc(task->as->pgsize);
            kmt->spin_lock(&task->vme_lock);
            VME_AREA* vma = find_vme_node(task, va, va + task->as->pgsize);
            kmt->spin_unlock(&task->vme_lock);
            if ((vma->vm_prot & PROT_WRITE) && (vma->vm_flags & MAP_PRIVATE)) {
                pgunmap(task, (void*) va);
                int new_prot = vma->vm_prot & ~PROT_WRITE;
                // divide_vme_node(task, vma, va, new_prot);
                pgmap(task, (void*) va, (void*) pa, new_prot, 1);
            }
        }
        cur = cur->next;
    }
    kmt->spin_unlock(&task->mp_lock);
}

void fork_copying_vme_pages(task_t *task, task_t *new_task) {
    VME_AREA* head = task->vm_area_head;
    VME_AREA* cur = head;
    VME_AREA* new_cur = NULL;
    bool is_head = true;
    if(cur == NULL) {
        new_task->vm_area_head = NULL;
        new_task->vm_area_tail = NULL;
        return;
    }
    kmt->spin_lock(&task->vme_lock);
    while (cur != NULL) {
        new_cur = new_vme_node(new_task, cur->vm_start, cur->vm_end - cur->vm_start, cur->vm_prot, cur->vm_flags);
        insert_vme_node(new_task, new_cur);
        if(is_head) {
            new_task->vm_area_head = new_cur;
            is_head = false;
        }
        if (cur == task->vm_area_tail) {
            new_task->vm_area_tail = new_cur;
        }
        cur = cur->vm_next;
    }
    kmt->spin_unlock(&task->vme_lock);
}

void fork_coppying_new_mapped_pages(task_t *task, task_t *new_task) {
    kmt->spin_lock(&task->mp_lock);
    M_PAGE* head = task->mapped_page_head;
    M_PAGE* cur = head;
    if(cur == NULL) {
        new_task->mapped_page_head = NULL;
        return;
    }

    while (cur != NULL) {
        kmt->spin_lock(&task->vme_lock);
        VME_AREA* vma = find_vme_node(task, (uintptr_t) cur->va, (uintptr_t) cur->va + task->as->pgsize);
        pgmap(new_task, cur->va, cur->pa, vma->vm_prot, 1);
        kmt->spin_unlock(&task->vme_lock);
        cur = cur->next;
    }
    kmt->spin_unlock(&task->mp_lock);
}

int kfork(task_t *task) {
    task_t* new_task = pmm->alloc(sizeof(task_t)); 
    
    uproc_create(new_task, strcat(task->name, "_fork"));

    uintptr_t rsp0 = new_task->context[0]->rsp0;
    void* cr3 = new_task->context[0]->cr3;

    new_task->context[0] = task->context[0];
    new_task->context[0]->rsp0 = rsp0;
    new_task->context[0]->cr3 = cr3;
    new_task->context[0]->GPRx = 0;

    // kmt->spin_lock(&task->vme_lock);
    fork_cow_mapping(task);

    fork_copying_vme_pages(task, new_task);

    fork_coppying_new_mapped_pages(task, new_task);
    // kmt->spin_unlock(&task->vme_lock);

    new_task->ppid = task->pid;
    int new_id = new_pid();
    new_task->pid = new_id;
    kmt->spin_lock(&task_init_lock);
    task_list[new_id] = new_task;
    kmt->spin_unlock(&task_init_lock);
    return new_id;
}

int kwait(task_t *task, int *status) {
    task->retstate=0;
    for(int i = 0; i < 32768 ; i++){
        task_t *the_task = task_list[i];

        if(the_task->ppid == task->pid && !the_task->killed){
            kmt->sem_init(&task->wait_sem, "wait_sem", 0);

            kmt->sem_wait(&task->wait_sem);

            *status=task->retstate;
            return 0;
        }
    }
    return -1;
}

int kkill(task_t *task, int pid) {
    task_t *t = task_list[pid];
    if (t == NULL) {
        return -1;
    }
    if (t->killed) {
        return -1;
    }
    t->killed = true;
    return 0;
}

void destroy_mapped_pages(task_t* task) {
    M_PAGE* head = task->mapped_page_head;
    M_PAGE* cur = head;
    if(cur == NULL) {
        return;
    }

    while (cur != NULL) {
        M_PAGE* next = cur->next;
        dec_cow_cnt(task, cur->pa);
        if (get_cow_cnt(task, cur->pa) == 0) {
            pmm->free(cur->pa);
        }
        pmm->free(cur);
        cur = next;
    }
}

void destroy_vme_area(task_t* task) {
    VME_AREA* head = task->vm_area_head;
    VME_AREA* cur = head;
    if(cur == NULL) {
        return;
    }

    while (cur != NULL) {
        VME_AREA* next = cur->vm_next;
        pmm->free(cur);
        cur = next;
    }
}

void destroy_task(task_t* task) {
    kmt->spin_lock(&task->vme_lock);
    destroy_mapped_pages(task);
    destroy_vme_area(task);
    unprotect(task->as);
    kmt->spin_lock(&task->vme_lock);
}

int kexit(task_t *task, int status) {
    cpu_list[cpu_current()].current_task->killed = true;
    task_t* father_task = NULL;
    if(task->ppid!=0){
        for(int i = 1; i < 32768 ; i++){
            father_task = task_list[i];
            if(father_task->pid == task->ppid){
                father_task->retstate = status;
                break;
            }
        }
        kmt->sem_signal(&father_task->wait_sem);
    }
    store_pid(task->pid);
    destroy_task(task);
    kmt->teardown(task);
    pmm->free(task);
    return status;
}

Context* page_fault(Event ev, Context *c) {
    task_t *task = (cpu_list[cpu_current()].current_task);
    AddrSpace *as = task->as;
    void *pa=pmm->alloc(as->pgsize);
    void *va=(void *)(ev.ref & ~(as->pgsize-1L));
    if(va==as->area.start){
        //TODO: copy the code from the init process
        memcpy(pa,_init,_init_len);
    }
    kmt->spin_lock(&task->vme_lock);
    M_PAGE* the_page = find_mapped_page(task, va);
    VME_AREA* vma = find_vme_node(task, (uintptr_t) va, (uintptr_t) va + as->pgsize);
    if (the_page == NULL) {
        if (vma == NULL) {
            kmt->spin_unlock(&task->vme_lock);
            kmmap(task, va, as->pgsize, PROT_READ | PROT_WRITE, MAP_PRIVATE);
            kmt->spin_lock(&task->mp_lock);
            pgmap(task, va, pa, PROT_READ | PROT_WRITE, 0);
            kmt->spin_unlock(&task->mp_lock);
        } else {
            kmt->spin_lock(&task->mp_lock);
            pgmap(task, va, pa, vma->vm_prot, 0);
            kmt->spin_unlock(&task->mp_lock);
            kmt->spin_unlock(&task->vme_lock);
        }
    } else {
        kmt->spin_lock(&task->mp_lock);
        memcpy(pa, the_page->pa, as->pgsize);
        pgunmap(task, va);
        pgmap(task, va, pa, vma->vm_prot, 0);
        kmt->spin_unlock(&task->mp_lock);
        dec_cow_cnt(task, the_page->pa);
        if (get_cow_cnt(task, the_page->pa) == 0) {       
            pmm->free(the_page->pa);            
        }
        kmt->spin_unlock(&task->vme_lock);
    }
    return NULL;
}

Context *syscall(Event e,Context *c){
    panic_on(ienabled()==1,"cli");
    iset(true);
    task_t *current=cpu_list[cpu_current()].current_task;
    switch(c->GPRx){
        case SYS_kputc: {
            c->GPRx = kputc(current,c->GPR1); 
            break;
        }
        case SYS_exit: {
            c->GPRx = kexit(current,c->GPR1);
            break;
        }
        case SYS_sleep: {
            c->GPRx = ksleep(current,c->GPR1);
            break;
        }
        case SYS_uptime: {
            c->GPRx = kuptime(current);
            break;
        }
        case SYS_fork: {
            c->GPRx = kfork(current);
            break;
        }
        case SYS_getpid: {
            c->GPRx = kgetpid(current);
            break;
        }
        case SYS_wait: {
            c->GPRx = kwait(current,(int*)c->GPR1);
            break;
        }
        case SYS_kill: {
            c->GPRx = kkill(current, c->GPR1);
            break;
        }
        case SYS_mmap: {
            c->GPRx = (uint64_t)kmmap(current, (void*)c->GPR1, (size_t)c->GPR2, (int)c->GPR3, (int)c->GPR4);
            break;
        }
        default:assert(0);
    }
    panic_on(ienabled()==0,"cli");
    iset(false);
    return NULL;
}

void uproc_init() {
    vme_init(pmm->alloc, pmm->free);

    os->on_irq(0,EVENT_SYSCALL,syscall);
    os->on_irq(0,EVENT_PAGEFAULT,page_fault);

    kmt->spin_init(&pid_lock, "pid_lock");
    kmt->spin_init(&cow_lock, "cow_lock");

    uintptr_t pmsize = (((uintptr_t)heap.end - (uintptr_t)heap.start)) >> 12;

    cow_table = (int*)pmm->alloc(pmsize * sizeof(int));
    memset(cow_table, 0, pmsize * sizeof(int));

    task_cnt = 1;

    task_t *t=pmm->alloc(sizeof(task_t));
    uproc_create(t, "initi");
    t->pid = get_pid();
}

MODULE_DEF(uproc) = {
    .init = uproc_init,
    .kputc = kputc,
    .getpid = kgetpid,
    .sleep = ksleep,
    .uptime = kuptime,
    .mmap = kmmap,
    .fork = kfork,
    .kill = kkill,
    .wait = kwait,
    .exit = kexit,
};
