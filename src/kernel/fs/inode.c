#include "inode.h"
#include "../console.h"
#include "../memory.h"

static inode_cache_entry_t inode_cache[INODE_CACHE_SIZE] = {0};

// Initialize inode cache
void inode_cache_init(void) {
    printk("[INODE] Initializing inode cache\n");
    for (uint32_t i = 0; i < INODE_CACHE_SIZE; i++) {
        inode_cache[i]. inode_id = 0xFFFFFFFF;
        inode_cache[i].dirty = 0;
        inode_cache[i].ref_count = 0;
    }
}

// Create new inode
inode_t* inode_create(uint32_t type) {
    inode_t *new_inode = (inode_t *)malloc(sizeof(inode_t));
    if (!new_inode) {
        printk("[INODE] ERROR: Failed to allocate inode\n");
        return NULL;
    }
    
    new_inode->type = type;
    new_inode->size = 0;
    new_inode->permissions = 0644;
    new_inode->owner_id = 0;
    new_inode->link_count = 1;
    new_inode->block_count = 0;
    
    for (int i = 0; i < 12; i++) {
        new_inode->direct_blocks[i] = 0xFFFFFFFF;
    }
    new_inode->indirect_block = 0xFFFFFFFF;
    
    printk("[INODE] Created inode of type %d\n", type);
    return new_inode;
}

// Delete inode
int32_t inode_delete(uint32_t inode_id) {
    if (inode_id >= FS_MAX_INODES) return -1;
    
    // Find in cache and mark for deletion
    for (uint32_t i = 0; i < INODE_CACHE_SIZE; i++) {
        if (inode_cache[i].inode_id == inode_id) {
            inode_cache[i].inode_id = 0xFFFFFFFF;
            inode_cache[i].ref_count = 0;
            inode_cache[i].dirty = 0;
            break;
        }
    }
    
    printk("[INODE] Deleted inode %d\n", inode_id);
    return 0;
}

// Read inode from cache or disk
inode_t* inode_read(uint32_t inode_id) {
    if (inode_id >= FS_MAX_INODES) return NULL;
    
    // Check cache first
    for (uint32_t i = 0; i < INODE_CACHE_SIZE; i++) {
        if (inode_cache[i]. inode_id == inode_id && inode_cache[i].ref_count > 0) {
            inode_cache[i]. ref_count++;
            printk("[INODE] Cache hit for inode %d\n", inode_id);
            return &inode_cache[i].data;
        }
    }
    
    // Find empty cache slot
    uint32_t cache_slot = 0xFFFFFFFF;
    for (uint32_t i = 0; i < INODE_CACHE_SIZE; i++) {
        if (inode_cache[i].ref_count == 0) {
            cache_slot = i;
            break;
        }
    }
    
    if (cache_slot == 0xFFFFFFFF) {
        printk("[INODE] ERROR: Cache full\n");
        return NULL;
    }
    
    printk("[INODE] Cache miss for inode %d, loading from disk\n", inode_id);
    
    // In a real implementation, read from disk here
    // For now, just allocate and initialize
    inode_cache[cache_slot].inode_id = inode_id;
    inode_cache[cache_slot].ref_count = 1;
    inode_cache[cache_slot].dirty = 0;
    
    return &inode_cache[cache_slot].data;
}

// Write inode
int32_t inode_write(uint32_t inode_id, inode_t *data) {
    if (!data || inode_id >= FS_MAX_INODES) return -1;
    
    for (uint32_t i = 0; i < INODE_CACHE_SIZE; i++) {
        if (inode_cache[i].inode_id == inode_id) {
            inode_cache[i].data = *data;
            inode_cache[i].dirty = 1;
            printk("[INODE] Wrote inode %d (marked dirty)\n", inode_id);
            return 0;
        }
    }
    
    return -1;
}

// Lookup inode by ID
inode_t* inode_lookup(uint32_t inode_id) {
    return inode_read(inode_id);
}

// Flush cache to disk
void inode_cache_flush(void) {
    printk("[INODE] Flushing inode cache to disk\n");
    
    for (uint32_t i = 0; i < INODE_CACHE_SIZE; i++) {
        if (inode_cache[i].dirty) {
            // In a real implementation, write to disk here
            printk("[INODE] Flushed inode %d\n", inode_cache[i].inode_id);
            inode_cache[i].dirty = 0;
        }
    }
}

// Add block to inode
int32_t inode_add_block(uint32_t inode_id, uint32_t block_num) {
    inode_t *inode = inode_read(inode_id);
    if (!inode) return -1;
    
    // Add to direct blocks first
    for (int i = 0; i < 12; i++) {
        if (inode->direct_blocks[i] == 0xFFFFFFFF) {
            inode->direct_blocks[i] = block_num;
            inode->block_count++;
            printk("[INODE] Added block %d to inode %d\n", block_num, inode_id);
            return 0;
        }
    }
    
    // TODO: Handle indirect blocks
    printk("[INODE] ERROR: Direct blocks full, indirect blocks not implemented\n");
    return -1;
}

// Remove block from inode
int32_t inode_remove_block(uint32_t inode_id, uint32_t block_num) {
    inode_t *inode = inode_read(inode_id);
    if (!inode) return -1;
    
    for (int i = 0; i < 12; i++) {
        if (inode->direct_blocks[i] == block_num) {
            inode->direct_blocks[i] = 0xFFFFFFFF;
            inode->block_count--;
            printk("[INODE] Removed block %d from inode %d\n", block_num, inode_id);
            return 0;
        }
    }
    
    return -1;
}
