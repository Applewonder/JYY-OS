#include "slab.h"
#include <assert.h>
#include <cbma.h>

static void* real_start_addr;
static void* begin_alloc_addr;


BUDDY_BLOCK_STICK* buddy_blocks[BBMA_NUM];
spinlock_t bbma_lock[BBMA_NUM];


BUDDY_BLOCK_SIZE determine_bbma_size(size_t size) {
    size_t real_size = size + BBMA_STICK_SIZE;
    if (real_size <= (1 << S_4K)) {
        return S_4K;
    } else if (real_size <= (1 << S_8K)) {
        return S_8K;
    } else if (real_size <= (1 << S_16K)) {
        return S_16K;
    } else if (real_size <= (1 << S_32K)) {
        return S_32K;
    } else if (real_size <= (1 << S_64K)) {
        return S_64K;
    } else if (real_size <= (1 << S_128K)) {
        return S_128K;
    } else if (real_size <= (1 << S_256K)) {
        return S_256K;
    } else if (real_size <= (1 << S_512K)) {
        return S_512K;
    } else if (real_size <= (1 << S_1M)) {
        return S_1M;
    } else if (real_size <= (1 << S_2M)) {
        return S_2M;
    } else if (real_size <= (1 << S_4M)) {
        return S_4M;
    } else if (real_size <= (1 << S_8M)) {
        return S_8M;
    } else if (real_size <= (1 << S_16M)) {
        return S_16M;
    } else {
        return BBMA_REFUSE;
    }
}

void* get_the_free_space_by_dividing(BUDDY_BLOCK_SIZE bbma_size) {
    void* bbma_addr = divide_larger_bbma_block_from_bbma_system(bbma_size + 1);
    delete_a_free_block_in_bbma_system(bbma_addr - BBMA_STICK_SIZE);
    return bbma_addr;
}

void* bbma_alloc(size_t size, bool is_from_slab) {
    BUDDY_BLOCK_SIZE bbma_size = BBMA_REFUSE;
    if (is_from_slab) {
        if (size != SLAB_REQUEST_SPACE) {
            // panic_on(true, "slab size error");
            return NULL;
        }
        bbma_size = S_4K;
    } else {
        bbma_size = determine_bbma_size(size);
        if (bbma_size == BBMA_REFUSE) {
            // panic_on(true, "bbma size error");
            return NULL;
        }
    }
    void* possible_bbma_addr = find_the_free_space_in_bbma_system(bbma_size);//TODO: find the free space in bbma system
    if (possible_bbma_addr == NULL) {
        possible_bbma_addr = get_the_free_space_by_dividing(bbma_size);
    }
    return possible_bbma_addr;
}

void* convert_index_to_addr(void* index) {
    unsigned long gap = index - real_start_addr;
    unsigned long num = gap / BBMA_STICK_SIZE;
    unsigned long offset = num << S_4K;
    return begin_alloc_addr + offset;
}

void* convert_addr_to_index(void* addr) {
    unsigned long gap = addr - begin_alloc_addr;
    unsigned long num = gap >> S_4K;
    unsigned long offset = num * BBMA_STICK_SIZE;
    return real_start_addr + offset;
}

void delete_a_free_block_in_bbma_system(BUDDY_BLOCK_STICK* bbma_stick) {

}

void* divide_larger_bbma_block_from_bbma_system(BUDDY_BLOCK_SIZE bbma_size) {
    return NULL;
}

void* find_the_free_space_in_bbma_system(BUDDY_BLOCK_SIZE bbma_size) {
    void* bbma_addr = NULL;
    spin_lock(&bbma_lock[bbma_size]);
    if (buddy_blocks[bbma_size - FIND_BBMA_OFFSET] != NULL) {
        bbma_addr = buddy_blocks[bbma_size - FIND_BBMA_OFFSET];
        delete_a_free_block_in_bbma_system(bbma_addr);
        BUDDY_BLOCK_STICK* bbma_stick = (BUDDY_BLOCK_STICK*)bbma_addr;
        bbma_stick->alloc_spaces = bbma_size;
        bbma_addr = convert_index_to_addr(bbma_addr);
    }
    spin_unlock(&bbma_lock[bbma_size]);
    return bbma_addr;
}