#include "slab.h"
#include "threads.h"
// #include <assert.h>
#include <cbma.h>
#include <stdio.h>

int calculate_addr_helper[] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512,
                               1024, 2048, 4096};
Tree all_trees[MAX_TREE + 1];//tree index begin from 1
Tree_Index cpu_trees[MAX_CPU][MAX_TREE];//for slab;
spinlock_t tree_locks[MAX_TREE];
static void* real_start_addr;
static void* begin_alloc_addr;
static int tree_num = 0;


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

void* bbma_alloc(size_t size) {
    BUDDY_BLOCK_SIZE bbma_size = determine_bbma_size(size);
    if (bbma_size == BBMA_REFUSE) {
        return NULL;
    }
    void* ptr = find_the_free_space_in_bbma_system(bbma_size);
    return ptr;
}

Tree_Index find_available_tree(BUDDY_BLOCK_SIZE bbma_size) {
    for (int i = 1; i <= tree_num; i++)
    {
        if (try_lock(&tree_locks[i])) {
            if (all_trees[i][1] < bbma_size) {
                spin_unlock(&tree_locks[i]);
                continue;
            }
            return i;
        }
    }
    return -1;
}

inline int left_child(int index) {
    return index * 2;
}

inline int right_child(int index) {
    return index * 2 + 1;
}

void* convert_index_to_addr(Tree tree, int index, BUDDY_BLOCK_SIZE cur_size) {
    assert(index <= MAX_NODE);
    assert(cur_size != BBMA_REFUSE);
    intptr_t tree_offset = (intptr_t)tree - (intptr_t)real_start_addr;
    int tree_gap = tree_offset / (sizeof(Tree_node) * MAX_NODE);
    intptr_t mem_offset = tree_gap << S_16M;
    intptr_t real_mem_start = (intptr_t)begin_alloc_addr + mem_offset;

    intptr_t node_offset = index - calculate_addr_helper[S_16M- cur_size];
    intptr_t mem_node_offset = node_offset << cur_size;

    return (void*)(real_mem_start + mem_node_offset);
}

void* get_the_free_space_in_tree(Tree tree, int index, BUDDY_BLOCK_SIZE cur_size, BUDDY_BLOCK_SIZE req_size) {
    assert(req_size != BBMA_REFUSE);
    assert(cur_size >= req_size);
    assert(tree[index] <= cur_size);
    // assert(tree[index] >= req_size);

    void* ptr = NULL;

    if (tree[index] < req_size) {
        return NULL;
    }

    if (cur_size == req_size) {
        if (tree[index] == req_size) {
            ptr = convert_index_to_addr(tree, index, cur_size);
            tree[index] = FULL_USED;
            return ptr;
        } else {
            return NULL;
        }
    } 

    int left_index = left_child(index);
    int right_index = right_child(index);
    if (tree[left_index] >= req_size) {
        ptr = get_the_free_space_in_tree(tree, left_index, cur_size - 1, req_size);
    } else {
        ptr = get_the_free_space_in_tree(tree, right_index, cur_size - 1, req_size);
    }

    if (ptr != NULL) {
        if (tree[index] == req_size) {
            tree[index] = GET_DEPARTED;
        } else {
            tree[index] = tree[left_index] >= tree[right_index] ? tree[left_index] : tree[right_index];
        }
    }

    return ptr;
}

void* find_the_free_space_in_bbma_system(BUDDY_BLOCK_SIZE bbma_size) {
    assert(bbma_size != BBMA_REFUSE);
    Tree_Index tree_index = find_available_tree(bbma_size);
    Tree tree = all_trees[tree_index];
    if (tree == NULL) {
        return NULL;
    }
    assert(tree[1] >= bbma_size);
    void* the_space = get_the_free_space_in_tree(tree, 1, S_16M, bbma_size);
    spin_unlock(&tree_locks[tree_index]);
    return the_space;
}

void initialize_tree(Tree tree) {
    int cur_stand = 1;
    int next_stand = 2;
    BUDDY_BLOCK_SIZE cur_size = S_16M;
    for (int i = 0; i < 13; i++)
    {
        cur_stand = calculate_addr_helper[i];
        if (i == 12) {
            next_stand = 8192;
        } else {
            next_stand = calculate_addr_helper[i + 1];
        }
        for (int i = cur_stand; i < next_stand; i++)
        {
            tree[i] = cur_size;
        }
        cur_size --;
    }
}

void distribute_tree(int tree_count) {
    int cpu_num = cpu_count();
    int normal_num = tree_count / cpu_num;
    int extra_num = tree_count % cpu_num;
    int cur_tree = 1;
    for (int i = 0; i < cpu_num; i++)
    {
        for (int t = 0; t < normal_num; t++)
        {
            cpu_trees[i][t] = cur_tree;
            cur_tree ++;
        }
        if (extra_num > 0) {
            cpu_trees[i][normal_num] = cur_tree;
            cur_tree ++;
            extra_num --;
        } 
    }
}

void bbma_init(void* start, void* end) {
    real_start_addr = start;
    intptr_t tree_size = sizeof(Tree_node) * MAX_NODE;
    intptr_t mem_gap = 1 << S_16M;
    intptr_t cur_tree_addr = (intptr_t)start;
    begin_alloc_addr = start + mem_gap;
    intptr_t cur_mem_addr = (intptr_t)begin_alloc_addr;
    int tree_cnt = 1;
    while (cur_mem_addr + mem_gap <= (intptr_t)end)
    {
        Tree tree = (Tree)cur_tree_addr;
        all_trees[tree_cnt] = tree;
        initialize_tree(tree);
        tree_cnt ++;
        cur_mem_addr += mem_gap;
        cur_tree_addr += tree_size;
    }
    tree_num = tree_cnt - 1;
    distribute_tree(tree_cnt);
}

Tree_Index determine_which_tree(void* ptr) {
    assert(ptr > begin_alloc_addr);
    intptr_t mask = (1 << S_16M) - 1;
    intptr_t mem_begin = (intptr_t)ptr & ~mask;
    return ((mem_begin - (intptr_t)begin_alloc_addr) >> S_16M); 
}

void free_tree_ptr(Tree tree, int index, void* ptr, BUDDY_BLOCK_SIZE cur_size) {
    assert(index <= MAX_NODE);
    assert(tree[index] <= cur_size);

    void* cur_addr = convert_index_to_addr(tree, index, cur_size);
    assert(cur_addr <= ptr);
    if (cur_addr == ptr) {
        if (tree[index] == FULL_USED) {
            tree[index] = cur_size;
            return;
        } else {
            free_tree_ptr(tree, left_child(index), ptr, cur_size - 1);
        }
    } else if (cur_addr + (1 << (cur_size - 1)) > ptr) {
        free_tree_ptr(tree, left_child(index), ptr, cur_size - 1);
    } else {
        free_tree_ptr(tree, right_child(index), ptr, cur_size - 1);
    }

    tree[index] = tree[left_child(index)] >= tree[right_child(index)] ? tree[left_child(index)] : tree[right_child(index)];
    assert(tree[index] != GET_DEPARTED);
    assert(tree[index] != FULL_USED);
    return;
}

void bbma_free(void* ptr) {
    if (ptr == NULL) {
        return;
    }
    Tree_Index tree_index = determine_which_tree(ptr);
    Tree tree = all_trees[tree_index];
    assert(tree != NULL);
    spin_lock(&tree_locks[tree_index]);
    free_tree_ptr(tree, 1, ptr, S_16M);
    spin_unlock(&tree_locks[tree_index]);
}
