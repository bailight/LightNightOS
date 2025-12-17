#ifndef __BLOCK_H__
#define __BLOCK_H__

#include "../../lib/stdint.h"
#include "fs.h"

#define BLOCK_CACHE_SIZE 64

typedef struct {
    uint32_t block_num;
    block_t data;
    uint32_t dirty;
    uint32_t ref_count;
} block_cache_entry_t;

// Block operations
int32_t block_read(uint32_t block_num, void *buffer);
int32_t block_write(uint32_t block_num, const void *buffer);
uint32_t block_allocate(void);
int32_t block_free(uint32_t block_num);
void block_cache_init(void);
void block_cache_flush(void);
int32_t block_clear(uint32_t block_num);

#endif
