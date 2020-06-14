/* nandif.h - nand flash interface */

#include "bootconfig.h"

#ifdef	CONFIG_YAFFS

void *yaffs_fs_readfile(const char *path, void *buffer, unsigned int buffer_length, size_t *read_length);
int yaffs_fs_writefile(const char *path, char *data, unsigned int length);

void yaffs_fs_reset(void);
int yaffs_fs_init(const char *partition_name);
int yaffs_fs_ls(const char *path);
int yaffs_fs_read(void *yaffs_obj, char *buffer, unsigned int offset, unsigned int length);
int yaffs_fs_write(void *yaffs_obj, char *buffer, unsigned int offset, unsigned int length);
void *yaffs_fs_open(const char *path);
int yaffs_fs_file_length(void *yaffs_obj);

#endif
