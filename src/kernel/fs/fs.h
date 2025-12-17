#ifndef __FS_H__
#define __FS_H__

#include "../../lib/stdint.h"

#define FS_MAX_FILES        1024
#define FS_MAX_INODES       2048
#define FS_MAX_BLOCKS       10240
#define FS_BLOCK_SIZE       4096
#define FS_MAX_NAME_LEN     256
#define FS_MAX_OPEN_FILES   128

#define FS_TYPE_FILE        1
#define FS_TYPE_DIR         2

#define FS_MODE_READ        0x1
#define FS_MODE_WRITE       0x2
#define FS_MODE_EXECUTE     0x4

typedef struct {
    uint32_t block_size;
    uint32_t total_blocks;
    uint32_t total_inodes;
    uint32_t free_blocks;
    uint32_t free_inodes;
    uint32_t block_bitmap_start;
    uint32_t inode_bitmap_start;
    uint32_t inode_table_start;
    uint32_t data_blocks_start;
} superblock_t;

typedef struct {
    uint32_t inode_id;
    uint32_t type;              // FILE or DIR
    uint32_t size;
    uint32_t permissions;       // rwx
    uint32_t owner_id;
    uint32_t created_time;
    uint32_t modified_time;
    uint32_t accessed_time;
    uint32_t link_count;
    uint32_t block_count;
    uint32_t direct_blocks[12]; // Direct block pointers
    uint32_t indirect_block;    // Single indirect block
    uint8_t *data; 
} inode_t;

typedef struct {
    uint8_t data[FS_BLOCK_SIZE];
} block_t;

typedef struct {
    uint32_t inode_id;
    char name[FS_MAX_NAME_LEN];
    uint32_t type;
} dir_entry_t;

typedef struct {
    uint32_t fd;
    uint32_t inode_id;
    uint32_t offset;
    uint32_t mode;
    uint32_t ref_count;
} file_descriptor_t;

// File System Functions
void fs_init(void);
int32_t fs_open(const char *filename, uint32_t mode);
int32_t fs_close(uint32_t fd);
int32_t fs_read(uint32_t fd, void *buffer, uint32_t size);
int32_t fs_write(uint32_t fd, const void *buffer, uint32_t size);
int32_t fs_create_file(const char *filename);
int32_t fs_delete_file(const char *filename);
int32_t fs_create_dir(const char *dirname);
int32_t fs_list_dir(const char *dirname);
int32_t fs_seek(uint32_t fd, uint32_t offset);
uint32_t fs_get_file_size(uint32_t fd);

#endif
