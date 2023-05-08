#ifndef BBMA
#define BBMA
# include <common.h>

#define MAX_CPU 8

typedef struct buddy_block_* Buddy_block;
typedef enum BLOCK_SIZE_ BLOCK_SIZE;
typedef int spinlock_t;

enum BLOCK_SIZE_{
    S_4K=0,
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
    REFUSE
};

size_t block_sizes[] = {4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384};
size_t divide_sizes[] = {0, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192};
spinlock_t cpu_slab_lock_list[MAX_CPU];

struct buddy_block_{
    void* addr;
    Buddy_block near_buddy;
};

Buddy_block block_list[13];

#endif