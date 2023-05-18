# include <slab.h>
#include <stdint.h>

SLAB_STICK* cpu_own_area[MAX_CPU][SLAB_NUM];

SLAB_SIZE determine_slab_size(size_t size) {
    // give a size, return the slab size
    if (size <= 32) {
        return S_32B;
    } else if (size <= 64) {
        return S_64B;
    } else if (size <= 128) {
        return S_128B;
    } else if (size <= 256) {
        return S_256B;
    } else if (size <= 512) {
        return S_512B;
    } else if (size <= 1024) {
        return S_1KB;
    } else {
        return SLAB_REFUSE;
    }
}

void* slab_alloc(int cpu_num, size_t size) {
    printf("This is cpu %d\n, alloc %ld space\n", cpu_num, size);
    SLAB_SIZE slab_size = determine_slab_size(size);
    if (slab_size == SLAB_REFUSE) {
        return bbma_alloc(size, false);
    }
    void* possible_slab_addr = find_the_free_space_in_slab(cpu_num, slab_size);
    if (possible_slab_addr == NULL) {
        return request_a_slab_from_bbma(cpu_num, slab_size);
    }
    void* the_slab_addr = possible_slab_addr;
    return the_slab_addr;
}

void* find_the_avaliable_page_in_slab(int cpu_num, SLAB_SIZE slab_size) {
    // find the avaliable page in the slab
    
    void* slab_addr = cpu_own_area[cpu_num][slab_size - CPU_FIND_SLAB_OFFSET];
    if (slab_addr == NULL) {
        return NULL;
    }
    SLAB_STICK* free_space = slab_addr;
    while (free_space != NULL) {
        if (free_space->current_slab_free_space == 0) {
            free_space = (SLAB_STICK*)free_space->next_slab_stick;
            continue;
        }
        void* page_addr = (void*)free_space;
        return page_addr;
    }

    return NULL;
}

void* find_the_free_space_in_slab(int cpu_num, SLAB_SIZE slab_size) {
    // find the free space in the slab
    SLAB_STICK* free_space = find_the_avaliable_page_in_slab(cpu_num, slab_size);
    if (free_space == NULL) {
        return NULL;
    }
    spin_lock(&free_space->slab_lock);
    SLAB_FREE_BLOCK* avaliable_slab_block = (SLAB_FREE_BLOCK*)free_space->current_slab_free_block_list;
    free_space->current_slab_free_block_list = avaliable_slab_block->next_free_slab_block;
    free_space->current_slab_free_space -= 1;
    spin_unlock(&free_space->slab_lock);
    return avaliable_slab_block;
}

void initialize_a_slab_new_page(int cpu_num, SLAB_SIZE slab_size, SLAB_STICK* slab_page) {
    long slab_stick_size = align_to(SLAB_STICK_SIZE, slab_size);
    long bbma_stick_size = align_to(BBMA_STICK_SIZE, slab_size);
    slab_page->current_slab_free_space = (SLAB_REQUEST_SPACE - slab_stick_size - bbma_stick_size) >> slab_size;
    slab_page->current_slab_free_block_list = (uintptr_t)NULL;
    unsigned int slab_free_space = slab_page->current_slab_free_space;
    uintptr_t slab_block_addr = (uintptr_t)slab_page + slab_stick_size;
    while (slab_free_space > 0) {
        SLAB_FREE_BLOCK* free_block = (SLAB_FREE_BLOCK*)slab_block_addr;
        free_block->next_free_slab_block = slab_page->current_slab_free_block_list;
        slab_page->current_slab_free_block_list = (uintptr_t)free_block;
        slab_free_space -= 1;
        slab_block_addr += (1 << slab_size);
    }
}

long align_to_slab(void* addr, SLAB_SIZE slab_size) {
    // align the addr to the slab size
    return align_to((long)addr, slab_size);
}

void* request_a_slab_from_bbma(int cpu_num, SLAB_SIZE slab_size) {
    // request a slab from bbma
    void* slab_addr = bbma_alloc(SLAB_REQUEST_SPACE, true);
    if (slab_addr == NULL) {
        return NULL;
    }
    SLAB_STICK* free_space = slab_addr;
    free_space->next_slab_stick = cpu_own_area[cpu_num][slab_size - CPU_FIND_SLAB_OFFSET];
    cpu_own_area[cpu_num][slab_size - CPU_FIND_SLAB_OFFSET] = free_space;
    initialize_a_slab_new_page(cpu_num, slab_size, free_space);
    return find_the_free_space_in_slab(cpu_num, slab_size);
}

void* slab_align_to_4kb(void* ptr) {
    // align the ptr to 4kb
    long mask = (1 << 12) - 1;
    return (void*)((long)ptr & (~mask));
}

void slab_free(void* ptr) {
    SLAB_FREE_BLOCK* slab_block = (SLAB_FREE_BLOCK*)ptr;
    SLAB_STICK* slab_stick = (SLAB_STICK*)slab_align_to_4kb(ptr);
    slab_block->next_free_slab_block = slab_stick->current_slab_free_block_list;
    spin_lock(&slab_stick->slab_lock);
    slab_stick->current_slab_free_block_list = (uintptr_t)slab_block;
    slab_stick->current_slab_free_space += 1;
    spin_unlock(&slab_stick->slab_lock);
}

void initialize_a_cpu_slab_area(int cpu_num) {
    for (int i = 0; i < SLAB_NUM; i++) {
        request_a_slab_from_bbma(cpu_num, i + CPU_FIND_SLAB_OFFSET);
    }
}

void slab_init() {
    int cpu_num = cpu_count();
    for (int i = 0; i < cpu_num; i++) {
        initialize_a_cpu_slab_area(i);
    }
}