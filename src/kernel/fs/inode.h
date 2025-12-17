#ifndef __INODE_H__
#define __INODE_H__

#include "../../lib/stdint.h"
#include "fs.h"

#define INODE_CACHE_SIZE 256

typedef struct {
    uint32_t inode_id;
    inode_t data;
    uint32_t dirty;     // 1 if modified, 0 if clean
    uint32_t ref_count; // Reference count
} inode_cache_entry_t;

// Inode operations
inode_t* inode_create(uint32_t type);
int32_t inode_delete(uint32_t inode_id);
inode_t* inode_read(uint32_t inode_id);
int32_t inode_write(uint32_t inode_id, inode_t *data);
inode_t* inode_lookup(uint32_t inode_id);
void inode_cache_init(void);
void inode_cache_flush(void);
int32_t inode_add_block(uint32_t inode_id, uint32_t block_num);
int32_t inode_remove_block(uint32_t inode_id, uint32_t block_num);

#endif
