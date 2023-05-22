#include "slab.h"
#include <assert.h>
#include <cbma.h>

static void* real_start_addr;
static void* begin_alloc_addr;


BUDDY_BLOCK_STICK* buddy_blocks[BBMA_NUM];
spinlock_t bbma_lock[BBMA_NUM];


BUDDY_BLOCK_SIZE determine_bbma_size(size_t size) {
    size_t real_size = size;
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
    delete_a_free_block_in_bbma_system(convert_addr_to_index(bbma_addr));
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
    if (bbma_stick->prev == NULL) {
        BUDDY_BLOCK_SIZE bbma_size = bbma_stick->alloc_spaces;
        buddy_blocks[bbma_size - FIND_BBMA_OFFSET] = bbma_stick->next;
        if (bbma_stick->next != NULL) {
            bbma_stick->next->prev = NULL;
        }
    } else {
        bbma_stick->prev->next = bbma_stick->next;
        if (bbma_stick->next != NULL) {
            bbma_stick->next->prev = bbma_stick->prev;
        }
    }
}

void* bbma_align_to_larger_block(void* ptr, BUDDY_BLOCK_SIZE bbma_size) {
    long mask = (1 << bbma_size) - 1;
    if (!((long)ptr & mask)) {
        return (void*)((long)ptr & (~mask)) + (1 << (bbma_size - 1));
    }
    return (void*)((long)ptr & (~mask));
}

void* divide_larger_bbma_block_from_bbma_system(BUDDY_BLOCK_SIZE bbma_size) {
    if (bbma_size == BBMA_REFUSE) {
        return NULL;
    }
    spin_lock(&bbma_lock[bbma_size - FIND_BBMA_OFFSET]);
    BUDDY_BLOCK_STICK* the_bbma_block_stick = buddy_blocks[bbma_size - FIND_BBMA_OFFSET];
    void* the_bbma_block_addr = NULL;
    if (the_bbma_block_stick == NULL) {
        spin_unlock(&bbma_lock[bbma_size - FIND_BBMA_OFFSET]);
        the_bbma_block_addr = divide_larger_bbma_block_from_bbma_system(bbma_size + 1);
        the_bbma_block_stick = convert_addr_to_index(the_bbma_block_addr);
        if (the_bbma_block_addr ==  NULL) {
            return NULL;
        }   
    } else {
        the_bbma_block_addr = convert_index_to_addr(the_bbma_block_stick);
    }
    BUDDY_BLOCK_STICK* left_divided_child = the_bbma_block_stick;
    BUDDY_BLOCK_STICK* right_divided_child = convert_addr_to_index(bbma_align_to_larger_block(the_bbma_block_addr, bbma_size));
    delete_a_free_block_in_bbma_system(the_bbma_block_stick);
    insert_two_new_divided_child_into_bbma_system(left_divided_child, right_divided_child, bbma_size - 1);
    spin_unlock(&bbma_lock[bbma_size - FIND_BBMA_OFFSET]);
    return ((void*)left_divided_child) + BBMA_STICK_SIZE;
}

void insert_two_new_divided_child_into_bbma_system(BUDDY_BLOCK_STICK* left_divided_child, BUDDY_BLOCK_STICK* right_divided_child, BUDDY_BLOCK_SIZE bbma_size){
    left_divided_child->alloc_spaces = bbma_size;
    right_divided_child->alloc_spaces = bbma_size;
    left_divided_child->next = right_divided_child;
    right_divided_child->prev = left_divided_child;
    left_divided_child->prev = NULL;
    right_divided_child->next = NULL;
    spin_lock(&bbma_lock[bbma_size - FIND_BBMA_OFFSET]);
    BUDDY_BLOCK_STICK* the_cur_bbma_block = buddy_blocks[bbma_size - FIND_BBMA_OFFSET];
    while (the_cur_bbma_block != NULL) {
        if (the_cur_bbma_block < left_divided_child) {
            if (the_cur_bbma_block->next == NULL) {
                the_cur_bbma_block->next = left_divided_child;
                left_divided_child->prev = the_cur_bbma_block;
                return;
            } else {
                the_cur_bbma_block = the_cur_bbma_block->next;
            }
        } else {
            if (the_cur_bbma_block->prev == NULL) {
                buddy_blocks[bbma_size - FIND_BBMA_OFFSET] = left_divided_child;
                right_divided_child->next = the_cur_bbma_block;
                the_cur_bbma_block->prev = right_divided_child;
                return;
            } else {
                the_cur_bbma_block->prev->next = left_divided_child;
                left_divided_child->prev = the_cur_bbma_block->prev;
                right_divided_child->next = the_cur_bbma_block;
                the_cur_bbma_block->prev = right_divided_child;
                return;
            }
        }
    }
    buddy_blocks[bbma_size - FIND_BBMA_OFFSET] = left_divided_child;
    spin_unlock(&bbma_lock[bbma_size - FIND_BBMA_OFFSET]);
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

void bbma_init(void* start, void* end) {
    real_start_addr = start;
    unsigned long heap_space = (unsigned long)end - (unsigned long)start;
    unsigned long block_num = heap_space >> (S_4K);
    unsigned long reduce_sticks_started_addr = (unsigned long)real_start_addr + block_num * BBMA_STICK_SIZE;
    begin_alloc_addr = (void*)align_to(reduce_sticks_started_addr, S_16M);

    unsigned long bbma_init_block_size = (1 << S_16M);
    unsigned long bbma_init_block_stick_gap = (bbma_init_block_size >> S_4K) * BBMA_STICK_SIZE;
    void* cur_buddy_block_addr = begin_alloc_addr;
    BUDDY_BLOCK_STICK* cur_bbma_block_stick = convert_addr_to_index(cur_buddy_block_addr);
    cur_bbma_block_stick->prev = NULL;
    cur_bbma_block_stick->next = NULL;
    buddy_blocks[S_16M - FIND_BBMA_OFFSET] = cur_bbma_block_stick;
    while(cur_buddy_block_addr + bbma_init_block_size <= end) {
        cur_bbma_block_stick->alloc_spaces = S_16M;
        if (cur_buddy_block_addr + 2 * bbma_init_block_size <= end) {
            cur_bbma_block_stick->next = ((void*)cur_bbma_block_stick) + bbma_init_block_stick_gap;
            cur_bbma_block_stick->next->prev = cur_bbma_block_stick;
        } else {
            cur_bbma_block_stick->next = NULL;
        }
        cur_buddy_block_addr += bbma_init_block_size;
        cur_bbma_block_stick = cur_bbma_block_stick->next;
    }
}

BUDDY_BLOCK_STICK* find_the_position_where_inserting_the_free_bbma_block(BUDDY_BLOCK_STICK* inserted_bbma_block_stick, BUDDY_BLOCK_SIZE bbma_block_size) {
    BUDDY_BLOCK_STICK* the_cur_bbma_block = buddy_blocks[bbma_block_size - FIND_BBMA_OFFSET];
    while (the_cur_bbma_block != NULL) {
        if (the_cur_bbma_block < inserted_bbma_block_stick) {
            if (the_cur_bbma_block->next == NULL) {
                return the_cur_bbma_block;
            } else {
                the_cur_bbma_block = the_cur_bbma_block->next;
            }
        } else {
            if (the_cur_bbma_block->prev == NULL) {
                return NULL;
            } else {
                return the_cur_bbma_block->prev;
            }
        }
    }
    return NULL;
}

bool judge_if_can_merge(BUDDY_BLOCK_STICK* inserted_bbma_block_stick, BUDDY_BLOCK_STICK* the_begin_bbma_block, BUDDY_BLOCK_STICK* the_position_where_inserting_the_free_bbma_block, BUDDY_BLOCK_STICK* the_cur_bbma_expected_neighbor_block) {
    if (inserted_bbma_block_stick->alloc_spaces == S_16M) {
        return false;
    }
    bool where_is_the_neighbor = the_cur_bbma_expected_neighbor_block < inserted_bbma_block_stick;
    if (where_is_the_neighbor) {
        if (the_position_where_inserting_the_free_bbma_block == NULL) {
            if (the_cur_bbma_expected_neighbor_block == the_begin_bbma_block) {
                return true;
            } 
        } else {
            if (the_cur_bbma_expected_neighbor_block == the_position_where_inserting_the_free_bbma_block) {
                return true;
            } 
        }
    } else {
        if (the_position_where_inserting_the_free_bbma_block == NULL) {
            return false;
        } else {
            if (the_cur_bbma_expected_neighbor_block == the_position_where_inserting_the_free_bbma_block->next) {
                return true;
            } 
        }
    }
    return false;
}

BUDDY_BLOCK_STICK* merge_the_block(BUDDY_BLOCK_STICK* inserted_bbma_block_stick, BUDDY_BLOCK_STICK* the_cur_bbma_expected_neighbor_block, bool where_is_the_neighbor) {
    if (where_is_the_neighbor) {
        delete_a_free_block_in_bbma_system(the_cur_bbma_expected_neighbor_block);
        the_cur_bbma_expected_neighbor_block->next = NULL;
        the_cur_bbma_expected_neighbor_block->prev = NULL;
        the_cur_bbma_expected_neighbor_block->alloc_spaces ++;
        return the_cur_bbma_expected_neighbor_block;
    } else {
        inserted_bbma_block_stick->next = NULL;
        inserted_bbma_block_stick->prev = NULL;
        inserted_bbma_block_stick->alloc_spaces ++;
        return inserted_bbma_block_stick;
    }
}

void insert_free_bbma_block_into_bbma_system(BUDDY_BLOCK_STICK* inserted_bbma_block_stick, BUDDY_BLOCK_SIZE bbma_block_size) {
    spin_lock(&bbma_lock[bbma_block_size - FIND_BBMA_OFFSET]);
    BUDDY_BLOCK_STICK* the_begin_bbma_block_stick = buddy_blocks[bbma_block_size - FIND_BBMA_OFFSET];
    BUDDY_BLOCK_STICK* the_cur_bbma_expected_neighbor_block_stick = convert_addr_to_index(bbma_align_to_larger_block(convert_index_to_addr(inserted_bbma_block_stick), bbma_block_size + 1));
    BUDDY_BLOCK_STICK* the_position_where_inserting_the_free_bbma_block_stick = find_the_position_where_inserting_the_free_bbma_block(inserted_bbma_block_stick, bbma_block_size);
    bool where_is_the_neighbor = the_cur_bbma_expected_neighbor_block_stick < inserted_bbma_block_stick;

    if (the_begin_bbma_block_stick == NULL) {
        buddy_blocks[bbma_block_size - FIND_BBMA_OFFSET] = inserted_bbma_block_stick;
        spin_unlock(&bbma_lock[bbma_block_size - FIND_BBMA_OFFSET]);
        return;
    }

    if (judge_if_can_merge(inserted_bbma_block_stick, the_begin_bbma_block_stick, the_position_where_inserting_the_free_bbma_block_stick, the_cur_bbma_expected_neighbor_block_stick)) {
        BUDDY_BLOCK_STICK* ready_to_insert = merge_the_block(inserted_bbma_block_stick, the_cur_bbma_expected_neighbor_block_stick, where_is_the_neighbor);
        insert_free_bbma_block_into_bbma_system(ready_to_insert, bbma_block_size + 1);
        spin_unlock(&bbma_lock[bbma_block_size - FIND_BBMA_OFFSET]);
        return;
    }
    // deal with the situation that the inserted block is the first block in the list
    if (the_position_where_inserting_the_free_bbma_block_stick == NULL) {
        inserted_bbma_block_stick->next = the_begin_bbma_block_stick;
        the_begin_bbma_block_stick->prev = inserted_bbma_block_stick;
        buddy_blocks[bbma_block_size - FIND_BBMA_OFFSET] = inserted_bbma_block_stick;
    } else {
        inserted_bbma_block_stick->next = the_position_where_inserting_the_free_bbma_block_stick->next;
        inserted_bbma_block_stick->prev = the_position_where_inserting_the_free_bbma_block_stick;
        the_position_where_inserting_the_free_bbma_block_stick->next = inserted_bbma_block_stick;
        if (inserted_bbma_block_stick->next != NULL) {
            inserted_bbma_block_stick->next->prev = inserted_bbma_block_stick;
        }
    }
    spin_unlock(&bbma_lock[bbma_block_size - FIND_BBMA_OFFSET]);
}

void bbma_free(void* ptr) {
    if (ptr == NULL) {
        return;
    }
    BUDDY_BLOCK_STICK* cur_bbma_block_stick = convert_addr_to_index(ptr);
    BUDDY_BLOCK_SIZE cur_bbma_block_size = cur_bbma_block_stick->alloc_spaces;
    cur_bbma_block_stick->prev = NULL;
    cur_bbma_block_stick->next = NULL;
    insert_free_bbma_block_into_bbma_system(cur_bbma_block_stick, cur_bbma_block_size);
}