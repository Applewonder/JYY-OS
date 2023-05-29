#ifndef BBMA
#define BBMA
# include <common.h>

#define MAX_CPU 8
#define MAX_TREE 32
#define BBMA_NUM 13
#define FIND_ADDR_OFFSET 12
// #define BBMA_MAX_16_MB 32
#define FULL_USED 0
#define GET_DEPARTED 1
#define MAX_NODE 8192
#define BBMA_STICK_SIZE sizeof(BUDDY_BLOCK_STICK)

typedef struct buddy_block_ BUDDY_BLOCK_STICK;
typedef enum BLOCK_SIZE_ BUDDY_BLOCK_SIZE;
typedef char Tree_node;
typedef char* Tree;
typedef int Tree_Index;

enum BLOCK_SIZE_{
    S_4K=12,
    S_8K,
    S_16K,
    S_32K,
    S_64K,
    S_128K,
    S_256K,
    S_512K,
    S_1M,
    S_2M,
    S_4M,
    S_8M,
    S_16M,
    BBMA_REFUSE
};



BUDDY_BLOCK_SIZE determine_bbma_size(size_t size);
void* bbma_alloc(size_t size);
void* find_the_free_space_in_bbma_system(BUDDY_BLOCK_SIZE bbma_size);
BUDDY_BLOCK_STICK* divide_larger_bbma_block_from_bbma_system(BUDDY_BLOCK_SIZE bbma_size);
// BUDDY_BLOCK_STICK* find_the_position_where_inserting_the_free_bbma_block(BUDDY_BLOCK_STICK* inserted_bbma_block_stick, BUDDY_BLOCK_SIZE bbma_block_size);
void bbma_free(void* ptr);
void bbma_init(void* start, void* end);
void* convert_index_to_addr(Tree tree, int index, BUDDY_BLOCK_SIZE cur_size);
// void spy_insert_chain_block(BUDDY_BLOCK_STICK* position, BUDDY_BLOCK_STICK* item);
Tree_Index find_available_tree(BUDDY_BLOCK_SIZE bbma_size);
int left_child(int index);
int right_child(int index);
void* convert_index_to_addr(Tree tree, int index, BUDDY_BLOCK_SIZE cur_size);
void* get_the_free_space_in_tree(Tree tree, int index, BUDDY_BLOCK_SIZE cur_size, BUDDY_BLOCK_SIZE req_size);
void* find_the_free_space_in_bbma_system(BUDDY_BLOCK_SIZE bbma_size);
void initialize_tree(Tree tree);
void distribute_tree(int tree_count);
Tree_Index determine_which_tree(void* ptr);
void free_tree_ptr(Tree tree, int index, void* ptr, BUDDY_BLOCK_SIZE cur_size);

#endif