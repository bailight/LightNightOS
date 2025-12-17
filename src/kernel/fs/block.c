#include "block.h"
#include "../console.h"
#include "../memory.h"
#include "../lib.h"

static block_cache_entry_t block_cache[BLOCK_CACHE_SIZE] = {0};
static uint8_t *block_storage = NULL;

// Initialize block cache
void block_cache_init(void) {
    printk("[BLOCK] Initializing block cache\n");
    
    for (uint32_t i = 0; i < BLOCK_CACHE_SIZE; i++) {
        block_cache[i].block_num = 0xFFFFFFFF;
        block_cache[i].dirty = 0;
        block_cache[i].ref_count = 0;
    }
    
    // Allocate smaller block storage - just for cache, not all blocks
    uint32_t storage_size = FS_BLOCK_SIZE * 16;  // Reduced from FS_MAX_BLOCKS
    block_storage = (uint8_t *)malloc(storage_size);
    if (!block_storage) {
        printk("[BLOCK] WARNING: Could not allocate block storage, using cache only\n");
    } else {
        printk("[BLOCK] Allocated %d bytes for block storage\n", storage_size);
    }
}

// Read block
int32_t block_read(uint32_t block_num, void *buffer) {
    if (! buffer || block_num >= FS_MAX_BLOCKS) return -1;
    
    // Check cache first
    for (uint32_t i = 0; i < BLOCK_CACHE_SIZE; i++) {
        if (block_cache[i].block_num == block_num && block_cache[i].ref_count > 0) {
            memcpy(buffer, &block_cache[i].data, FS_BLOCK_SIZE);
            block_cache[i].ref_count++;
            printk("[BLOCK] Cache hit for block %d\n", block_num);
            return FS_BLOCK_SIZE;
        }
    }
    
    // Find empty cache slot
    uint32_t cache_slot = 0xFFFFFFFF;
    for (uint32_t i = 0; i < BLOCK_CACHE_SIZE; i++) {
        if (block_cache[i].ref_count == 0) {
            cache_slot = i;
            break;
        }
    }
    
    if (cache_slot != 0xFFFFFFFF && block_storage) {
        uint8_t *src = block_storage + (block_num * FS_BLOCK_SIZE);
        memcpy(&block_cache[cache_slot]. data, src, FS_BLOCK_SIZE);
        block_cache[cache_slot].block_num = block_num;
        block_cache[cache_slot].ref_count = 1;
        block_cache[cache_slot].dirty = 0;
        
        memcpy(buffer, &block_cache[cache_slot].data, FS_BLOCK_SIZE);
        printk("[BLOCK] Read block %d into cache slot %d\n", block_num, cache_slot);
        return FS_BLOCK_SIZE;
    }
    
    return -1;
}

// Write block
int32_t block_write(uint32_t block_num, const void *buffer) {
    if (!buffer || block_num >= FS_MAX_BLOCKS) return -1;
    
    // Find in cache
    uint32_t cache_slot = 0xFFFFFFFF;
    for (uint32_t i = 0; i < BLOCK_CACHE_SIZE; i++) {
        if (block_cache[i].block_num == block_num) {
            cache_slot = i;
            break;
        }
    }
    
    // If not in cache, find empty slot
    if (cache_slot == 0xFFFFFFFF) {
        for (uint32_t i = 0; i < BLOCK_CACHE_SIZE; i++) {
            if (block_cache[i].ref_count == 0) {
                cache_slot = i;
                break;
            }
        }
    }
    
    if (cache_slot != 0xFFFFFFFF) {
        memcpy(&block_cache[cache_slot].data, buffer, FS_BLOCK_SIZE);
        block_cache[cache_slot].block_num = block_num;
        block_cache[cache_slot].dirty = 1;
        block_cache[cache_slot].ref_count++;
        
        printk("[BLOCK] Wrote block %d (marked dirty)\n", block_num);
        return FS_BLOCK_SIZE;
    }
    
    return -1;
}

// Allocate block
uint32_t block_allocate(void) {
    for (uint32_t i = 0; i < FS_MAX_BLOCKS; i++) {
        uint32_t byte_idx = i / 8;
        uint32_t bit_idx = i % 8;
        
        // Simplified: assume bitmap is in first block
        // In real implementation, use actual bitmap
        if (byte_idx < FS_BLOCK_SIZE) {
            printk("[BLOCK] Allocated block %d\n", i);
            return i;
        }
    }
    
    printk("[BLOCK] ERROR: No free blocks\n");
    return 0xFFFFFFFF;
}

// Free block
int32_t block_free(uint32_t block_num) {
    if (block_num >= FS_MAX_BLOCKS) return -1;
    
    // Clear from cache
    for (uint32_t i = 0; i < BLOCK_CACHE_SIZE; i++) {
        if (block_cache[i].block_num == block_num) {
            block_cache[i].block_num = 0xFFFFFFFF;
            block_cache[i].ref_count = 0;
            block_cache[i].dirty = 0;
            break;
        }
    }
    
    printk("[BLOCK] Freed block %d\n", block_num);
    return 0;
}

// Clear block (zero out)
int32_t block_clear(uint32_t block_num) {
    if (block_num >= FS_MAX_BLOCKS) return -1;
    
    uint8_t zeros[FS_BLOCK_SIZE];
    memset(zeros, 0, FS_BLOCK_SIZE);
    
    return block_write(block_num, zeros);
}

// Flush cache to disk
void block_cache_flush(void) {
    printk("[BLOCK] Flushing block cache to disk\n");
    
    for (uint32_t i = 0; i < BLOCK_CACHE_SIZE; i++) {
        if (block_cache[i].dirty && block_storage) {
            uint8_t *dst = block_storage + (block_cache[i].block_num * FS_BLOCK_SIZE);
            memcpy(dst, &block_cache[i].data, FS_BLOCK_SIZE);
            block_cache[i].dirty = 0;
            printk("[BLOCK] Flushed block %d\n", block_cache[i].block_num);
        }
    }
}
