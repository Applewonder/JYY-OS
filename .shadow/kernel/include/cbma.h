#ifndef BBMA
#define BBMA
# include <common.h>

#define MAX_CPU 8
#define BBMA_NUM 13
#define FIND_BBMA_OFFSET 12
#define BBMA_MAX_16_MB 32
#define BBMA_STICK_SIZE sizeof(BUDDY_BLOCK_STICK)

typedef struct buddy_block_ BUDDY_BLOCK_STICK;
typedef enum BLOCK_SIZE_ BUDDY_BLOCK_SIZE;

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

struct buddy_block_{
    BUDDY_BLOCK_SIZE alloc_spaces;
};

void* get_the_free_space_by_dividing(BUDDY_BLOCK_SIZE bbma_size);
void* bbma_alloc(size_t size, bool is_from_slab);
BUDDY_BLOCK_SIZE determine_bbma_size(size_t size);
void delete_a_free_block_in_bbma_system(BUDDY_BLOCK_STICK* block);
void* divide_larger_bbma_block_from_bbma_system(BUDDY_BLOCK_SIZE bbma_size);
void* find_the_free_space_in_bbma_system(BUDDY_BLOCK_SIZE bbma_size);

#endif