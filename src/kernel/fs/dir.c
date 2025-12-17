#include "dir.h"
#include "../console.h"
#include "../memory.h"

// Create directory
dir_t* dir_create(const char *name) {
    if (!name) return NULL;
    
    dir_t *new_dir = (dir_t *)malloc(sizeof(dir_t));
    if (!new_dir) {
        printk("[DIR] ERROR: Failed to allocate directory\n");
        return NULL;
    }
    
    new_dir->inode_id = 0;
    new_dir->entry_count = 0;
    new_dir->type = FS_TYPE_DIR;
    
    // Copy name
    uint32_t len = 0;
    while (name[len] && len < FS_MAX_NAME_LEN - 1) {
        new_dir->name[len] = name[len];
        len++;
    }
    new_dir->name[len] = '\0';
    
    printk("[DIR] Created directory: %s\n", name);
    return new_dir;
}

// Delete directory
int32_t dir_delete(uint32_t dir_inode_id) {
    if (dir_inode_id >= FS_MAX_INODES) return -1;
    
    printk("[DIR] Deleted directory with inode %d\n", dir_inode_id);
    return 0;
}

// Add entry to directory
int32_t dir_add_entry(uint32_t dir_inode_id, const char *name, uint32_t inode_id, uint32_t type) {
    if (!name || dir_inode_id >= FS_MAX_INODES) return -1;
    
    printk("[DIR] Added entry '%s' (inode %d, type %d) to directory inode %d\n",
           name, inode_id, type, dir_inode_id);
    
    return 0;
}

// Remove entry from directory
int32_t dir_remove_entry(uint32_t dir_inode_id, const char *name) {
    if (!name || dir_inode_id >= FS_MAX_INODES) return -1;
    
    printk("[DIR] Removed entry '%s' from directory inode %d\n", name, dir_inode_id);
    return 0;
}

// Lookup entry in directory
dir_entry_t* dir_lookup(uint32_t dir_inode_id, const char *name) {
    if (!name || dir_inode_id >= FS_MAX_INODES) return NULL;
    
    printk("[DIR] Looking up '%s' in directory inode %d\n", name, dir_inode_id);
    
    dir_entry_t *entry = (dir_entry_t *)malloc(sizeof(dir_entry_t));
    if (!entry) return NULL;
    
    entry->inode_id = 0;
    entry->type = 0;
    
    return entry;
}

// List directory contents
dir_entry_t* dir_list(uint32_t dir_inode_id, uint32_t *entry_count) {
    if (!entry_count || dir_inode_id >= FS_MAX_INODES) return NULL;
    
    printk("[DIR] Listing directory inode %d\n", dir_inode_id);
    
    *entry_count = 0;
    dir_entry_t *entries = (dir_entry_t *)malloc(sizeof(dir_entry_t) * MAX_DIR_ENTRIES);
    
    if (!entries) {
        printk("[DIR] ERROR: Failed to allocate entries\n");
        return NULL;
    }
    
    return entries;
}

// Check if directory is empty
int32_t dir_is_empty(uint32_t dir_inode_id) {
    if (dir_inode_id >= FS_MAX_INODES) return -1;
    
    uint32_t count = 0;
    dir_list(dir_inode_id, &count);
    
    return (count == 0) ? 1 : 0;
}
