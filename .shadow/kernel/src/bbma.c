#include "slab.h"
#include <assert.h>
#include <bbma.h>

BUDDY_BLOCK_STICK* buddy_block_list[BBMA_NUM];
spinlock_t bbma_lock[BBMA_NUM];

BUDDY_BLOCK_SIZE determine_bbma_size(size_t size) {
    size_t real_size = size + BBMA_STICK_SIZE;
    if (real_size <= (1 << S_2K)) {
        return S_2K;
    } else if (real_size <= (1 << S_4K)) {
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
    }
    void* possible_bbma_addr = find_the_free_space_in_bbma_system(bbma_size);
    if (possible_bbma_addr == NULL) {
        possible_bbma_addr = divide_larger_bbma_block_from_bbma_system(bbma_size + 1);
        delete_a_free_block_in_bbma_system(possible_bbma_addr - BBMA_STICK_SIZE);
    }
    return possible_bbma_addr;
}

void* find_the_free_space_in_bbma_system(BUDDY_BLOCK_SIZE bbma_size) {
    spin_lock(&bbma_lock[bbma_size - FIND_BBMA_OFFSET]);
    BUDDY_BLOCK_STICK* the_bbma_block = buddy_block_list[bbma_size - FIND_BBMA_OFFSET];
    if (the_bbma_block == NULL) {
        spin_unlock(&bbma_lock[bbma_size - FIND_BBMA_OFFSET]);
        return NULL;
    }
    buddy_block_list[bbma_size - FIND_BBMA_OFFSET] = the_bbma_block->next;
    if (the_bbma_block->next != NULL) {
        the_bbma_block->next->prev = NULL;
    }
    spin_unlock(&bbma_lock[bbma_size - FIND_BBMA_OFFSET]);
    return ((void*)the_bbma_block) + BBMA_STICK_SIZE;
}

void insert_two_new_divided_child_into_bbma_system(BUDDY_BLOCK_STICK* left_divided_child, BUDDY_BLOCK_STICK* right_divided_child, BUDDY_BLOCK_SIZE bbma_size){
    left_divided_child->size = bbma_size;
    right_divided_child->size = bbma_size;
    left_divided_child->next = right_divided_child;
    right_divided_child->prev = left_divided_child;
    left_divided_child->prev = NULL;
    right_divided_child->next = NULL;
    spin_lock(&bbma_lock[bbma_size - FIND_BBMA_OFFSET]);
    BUDDY_BLOCK_STICK* the_cur_bbma_block = buddy_block_list[bbma_size - FIND_BBMA_OFFSET];
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
                buddy_block_list[bbma_size - FIND_BBMA_OFFSET] = left_divided_child;
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
    buddy_block_list[bbma_size - FIND_BBMA_OFFSET] = left_divided_child;
    spin_unlock(&bbma_lock[bbma_size - FIND_BBMA_OFFSET]);
}

void delete_a_free_block_in_bbma_system(BUDDY_BLOCK_STICK* block) {
    if (block->prev == NULL) {
        BUDDY_BLOCK_SIZE bbma_size = block->size;
        buddy_block_list[bbma_size - FIND_BBMA_OFFSET] = block->next;
        if (block->next != NULL) {
            block->next->prev = NULL;
        }
    } else {
        block->prev->next = block->next;
        if (block->next != NULL) {
            block->next->prev = block->prev;
        }
    }
}

void* divide_larger_bbma_block_from_bbma_system(BUDDY_BLOCK_SIZE bbma_size) {
    if (bbma_size == BBMA_REFUSE) {
        return NULL;
    }
    spin_lock(&bbma_lock[bbma_size - FIND_BBMA_OFFSET]);
    BUDDY_BLOCK_STICK* the_bbma_block = buddy_block_list[bbma_size - FIND_BBMA_OFFSET];
    void* the_bbma_block_addr = NULL;
    if (the_bbma_block == NULL) {
        spin_unlock(&bbma_lock[bbma_size - FIND_BBMA_OFFSET]);
        the_bbma_block_addr = divide_larger_bbma_block_from_bbma_system(bbma_size + 1);
        the_bbma_block = ((void*)the_bbma_block_addr) - BBMA_STICK_SIZE;
        if (the_bbma_block_addr ==  NULL) {
            return NULL;
        }   
    } else {
        the_bbma_block_addr = ((void*)the_bbma_block) + BBMA_STICK_SIZE;
    }
    BUDDY_BLOCK_STICK* left_divided_child = the_bbma_block_addr - BBMA_STICK_SIZE;
    BUDDY_BLOCK_STICK* right_divided_child = ((void*)left_divided_child) + (1 << (bbma_size - 1));
    delete_a_free_block_in_bbma_system(the_bbma_block);
    insert_two_new_divided_child_into_bbma_system(left_divided_child, right_divided_child, bbma_size - 1);
    spin_unlock(&bbma_lock[bbma_size - FIND_BBMA_OFFSET]);
    return ((void*)left_divided_child) + BBMA_STICK_SIZE;
}

void* bbma_align_to_larger_block(void* ptr, BUDDY_BLOCK_SIZE bbma_size) {
    long mask = (1 << bbma_size) - 1;
    if (!((long)ptr & mask)) {
        return (void*)((long)ptr & (~mask)) + (1 << bbma_size);
    }
    return (void*)((long)ptr & (~mask));
}
BUDDY_BLOCK_STICK* find_the_position_where_inserting_the_free_bbma_block(BUDDY_BLOCK_STICK* inserted_bbma_block_stick, BUDDY_BLOCK_SIZE bbma_block_size) {
    BUDDY_BLOCK_STICK* the_cur_bbma_block = buddy_block_list[bbma_block_size - FIND_BBMA_OFFSET];
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

void insert_free_bbma_block_into_bbma_system(BUDDY_BLOCK_STICK* inserted_bbma_block_stick, BUDDY_BLOCK_SIZE bbma_block_size) {
    spin_lock(&bbma_lock[bbma_block_size - FIND_BBMA_OFFSET]);
    BUDDY_BLOCK_STICK* the_cur_bbma_block = buddy_block_list[bbma_block_size - FIND_BBMA_OFFSET];
    BUDDY_BLOCK_STICK* the_cur_bbma_neighbor_block = bbma_align_to_larger_block((void*)inserted_bbma_block_stick + BBMA_STICK_SIZE, bbma_block_size + 1) - BBMA_STICK_SIZE;
    BUDDY_BLOCK_STICK* the_position_where_inserting_the_free_bbma_block = find_the_position_where_inserting_the_free_bbma_block(inserted_bbma_block_stick, bbma_block_size);
    bool where_is_the_neighbor = the_cur_bbma_neighbor_block < inserted_bbma_block_stick;
    if (where_is_the_neighbor) {
        if (the_position_where_inserting_the_free_bbma_block == NULL) {
            inserted_bbma_block_stick->next = the_cur_bbma_block;
            the_cur_bbma_block->prev = inserted_bbma_block_stick;
            buddy_block_list[bbma_block_size - FIND_BBMA_OFFSET] = inserted_bbma_block_stick;
        } else {
            if (the_position_where_inserting_the_free_bbma_block == the_cur_bbma_neighbor_block) {
                if (the_position_where_inserting_the_free_bbma_block->prev == NULL) {
                    buddy_block_list[bbma_block_size - FIND_BBMA_OFFSET] = the_position_where_inserting_the_free_bbma_block->next;
                } else {
                    the_position_where_inserting_the_free_bbma_block->prev->next = the_position_where_inserting_the_free_bbma_block->next;
                }
                if (the_position_where_inserting_the_free_bbma_block->next != NULL) {
                    the_position_where_inserting_the_free_bbma_block->next->prev = the_position_where_inserting_the_free_bbma_block->prev;
                }
                the_position_where_inserting_the_free_bbma_block->next = NULL;
                the_position_where_inserting_the_free_bbma_block->prev = NULL;
                the_position_where_inserting_the_free_bbma_block->size ++;
                insert_free_bbma_block_into_bbma_system(the_position_where_inserting_the_free_bbma_block, the_position_where_inserting_the_free_bbma_block->size);
            } else {
                inserted_bbma_block_stick->next = the_position_where_inserting_the_free_bbma_block->next;
                the_position_where_inserting_the_free_bbma_block->next = inserted_bbma_block_stick;
                inserted_bbma_block_stick->prev = the_position_where_inserting_the_free_bbma_block;
                if (inserted_bbma_block_stick->next != NULL) {
                    inserted_bbma_block_stick->next->prev = inserted_bbma_block_stick;
                }
            }
        }
    } else {
        if (the_position_where_inserting_the_free_bbma_block == NULL) {
            if (buddy_block_list[bbma_block_size - FIND_BBMA_OFFSET] == NULL) {
                buddy_block_list[bbma_block_size - FIND_BBMA_OFFSET] = inserted_bbma_block_stick;
            } else {
                if (the_cur_bbma_neighbor_block == buddy_block_list[bbma_block_size - FIND_BBMA_OFFSET]) {
                    buddy_block_list[bbma_block_size - FIND_BBMA_OFFSET] = buddy_block_list[bbma_block_size - FIND_BBMA_OFFSET]->next;
                    inserted_bbma_block_stick->size = inserted_bbma_block_stick->size + 1;
                    insert_free_bbma_block_into_bbma_system(inserted_bbma_block_stick, inserted_bbma_block_stick->size);
                } else {
                    inserted_bbma_block_stick->next = buddy_block_list[bbma_block_size - FIND_BBMA_OFFSET];
                    buddy_block_list[bbma_block_size - FIND_BBMA_OFFSET]->prev = inserted_bbma_block_stick;
                    buddy_block_list[bbma_block_size - FIND_BBMA_OFFSET] = inserted_bbma_block_stick;
                }
            } 
        } else {
            if (the_position_where_inserting_the_free_bbma_block->next == NULL) {
                the_position_where_inserting_the_free_bbma_block->next = inserted_bbma_block_stick;
                inserted_bbma_block_stick->prev = the_position_where_inserting_the_free_bbma_block;
            } else {
                if (the_position_where_inserting_the_free_bbma_block->next == the_cur_bbma_neighbor_block) {
                    the_position_where_inserting_the_free_bbma_block->next = the_cur_bbma_neighbor_block->next;
                    if (the_cur_bbma_neighbor_block->next != NULL) {
                        the_cur_bbma_neighbor_block->next->prev = the_position_where_inserting_the_free_bbma_block;
                    }
                    inserted_bbma_block_stick->size ++;
                    insert_free_bbma_block_into_bbma_system(inserted_bbma_block_stick, inserted_bbma_block_stick->size);
                } else {
                    inserted_bbma_block_stick->next = the_position_where_inserting_the_free_bbma_block->next;
                    the_position_where_inserting_the_free_bbma_block->next = inserted_bbma_block_stick;
                    inserted_bbma_block_stick->prev = the_position_where_inserting_the_free_bbma_block;
                    if (inserted_bbma_block_stick->next != NULL) {
                        inserted_bbma_block_stick->next->prev = inserted_bbma_block_stick;
                    }
                }
            }
        }
    }
    spin_unlock(&bbma_lock[bbma_block_size - FIND_BBMA_OFFSET]);
}

void bbma_free(void* ptr) {
    BUDDY_BLOCK_STICK* cur_bbma_block_stick = ptr - BBMA_STICK_SIZE;
    BUDDY_BLOCK_SIZE cur_bbma_block_size = cur_bbma_block_stick->size;
    cur_bbma_block_stick->prev = NULL;
    cur_bbma_block_stick->next = NULL;
    insert_free_bbma_block_into_bbma_system(cur_bbma_block_stick, cur_bbma_block_size);
}

void bbma_init(void* start, void* end) {
    unsigned int bbma_init_block_size = (1 << S_16M);
    void* cur_buddy_block_addr = start;
    BUDDY_BLOCK_STICK* cur_bbma_block_stick = cur_buddy_block_addr - BBMA_STICK_SIZE;
    cur_bbma_block_stick->prev = NULL;
    cur_bbma_block_stick->next = NULL;
    buddy_block_list[S_16M - FIND_BBMA_OFFSET] = cur_bbma_block_stick;
    while(cur_buddy_block_addr + bbma_init_block_size <= end) {
        cur_bbma_block_stick->size = S_16M;
        if (cur_buddy_block_addr + 2 * bbma_init_block_size <= end) {
            cur_bbma_block_stick->next = ((void*)cur_bbma_block_stick) + bbma_init_block_size;
            cur_bbma_block_stick->next->prev = cur_bbma_block_stick;
        } else {
            cur_bbma_block_stick->next = NULL;
        }
        cur_buddy_block_addr += bbma_init_block_size;
        cur_bbma_block_stick = cur_bbma_block_stick->next;
    }
}