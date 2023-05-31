#ifndef SLAB_H
#define SLAB_H
# include <common.h>
# include <cbma.h>

#define SLAB_NUM 7
#define SLAB_REQUEST_SPACE 0x1000
#define SLAB_STICK_SIZE sizeof(SLAB_STICK)
#define SLAB_BLOCK_SIZE sizeof(SLAB_FREE_BLOCK)
#define CPU_FIND_SLAB_OFFSET 5

typedef enum SLAB_SIZE_ SLAB_SIZE;
typedef struct SLAB_STICK_ SLAB_STICK;

//TODO: local and tread
//TODO: try_lock



enum SLAB_SIZE_{
    S_32B=5,
    S_64B,
    S_128B,
    S_256B,
    S_512B,
    S_1KB,
    S_2KB,
    SLAB_REFUSE,
};

struct SLAB_STICK_{
#ifndef TEST
    spinlock_t slab_lock;
#else
    mutex_t slab_lock;
#endif
    uintptr_t current_slab_free_block_list;
    SLAB_STICK* next_slab_stick;
    unsigned int current_slab_free_space; 
};

typedef struct SLAB_FREE_BLOCK_{
    uintptr_t next_free_slab_block;
} SLAB_FREE_BLOCK;

SLAB_SIZE determine_slab_size(size_t size);
void* slab_alloc(int cpu_num, size_t size);
void* find_the_free_space_in_slab(int cpu_num, SLAB_SIZE slab_size);
void* request_a_slab_from_bbma(int cpu_num, SLAB_SIZE slab_size);
void* find_the_avaliable_page_in_slab_and_lock(int cpu_num, SLAB_SIZE slab_size);
void* slab_align_to_4kb(void* ptr);
void slab_free(void* ptr);
void slab_init();
void initialize_a_cpu_slab_area(int cpu_num);
#endif