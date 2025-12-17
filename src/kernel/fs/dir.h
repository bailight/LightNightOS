#ifndef __DIR_H__
#define __DIR_H__

#include "../../lib/stdint.h"
#include "fs.h"

#define MAX_DIR_ENTRIES 256

typedef struct {
    uint32_t inode_id;
    char name[FS_MAX_NAME_LEN];
    uint32_t type;
    uint32_t entry_count;
} dir_t;

// Directory operations
dir_t* dir_create(const char *name);
int32_t dir_delete(uint32_t dir_inode_id);
int32_t dir_add_entry(uint32_t dir_inode_id, const char *name, uint32_t inode_id, uint32_t type);
int32_t dir_remove_entry(uint32_t dir_inode_id, const char *name);
dir_entry_t* dir_lookup(uint32_t dir_inode_id, const char *name);
dir_entry_t* dir_list(uint32_t dir_inode_id, uint32_t *entry_count);
int32_t dir_is_empty(uint32_t dir_inode_id);

#endif
