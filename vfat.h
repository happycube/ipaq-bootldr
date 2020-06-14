/****************************************************************************/
/* Copyright 2002 Compaq Computer Corporation.                              */
/*                                           .                              */
/* Copying or modifying this code for any purpose is permitted,             */
/* provided that this copyright notice is preserved in its entirety         */
/* in all copies or modifications.  COMPAQ COMPUTER CORPORATION             */
/* MAKES NO WARRANTIES, EXPRESSED OR IMPLIED, AS TO THE USEFULNESS          */
/* OR CORRECTNESS OF THIS CODE OR ITS FITNESS FOR ANY PARTICULAR            */
/* PURPOSE.                                                                 */
/****************************************************************************/
/*
 * Microsoft FAT filesystem support
 */

enum fat_type {
  ft_unknown,
  ft_fat12,
  ft_fat16,
  ft_fat32
};

/* 
 * Boot Sector and BPB Structure
 *  Found at sector 0 of partition
 */
struct bpb_info {
  /* offset  0 */ u8 jmpboot[3]; /* ignore this */
  /* offset  3 */ u8 oemname[8];
  /* offset 11 */ u8 bytes_per_sector[2]; /* unaligned, so u8 [2] */
  /* offset 13 */ u8  sectors_per_cluster;
  /* offset 14 */ u16 n_reserved_sectors; 
  /* offset 16 */ u8 n_fats;  
  /* offset 17 */ u8 n_root_entries[2];
  /* offset 19 */ u8 total_sectors16[2];
  /* offset 21 */ u8 media;
  /* offset 22 */ u16 fat_size16;
  /* offset 24 */ u16 sectors_per_track;  
  /* offset 26 */ u16 n_heads;  
  /* offset 28 */ u32 n_hidden_sectors;  
  /* offset 32 */ u32 total_sectors32;

  /* following fields for fat32 only */
  /* offset 36 */ u32 fat_size32;
  /* offset 40 */ u16 ext_flags;
  /* offset 42 */ u16 fsver;
  /* offset 44 */ u32 first_root_cluster;
  /* offset 48 */ u16 fsinfo;
  /* offset 50 */ u16 backup_bootblock_sector;
  /* offset 52 */ u8 reserved52[12];
  /* offset 64 */ u8 drive_number;
  /* offset 65 */ u8 reserved65;
  /* offset 66 */ u8 bootsig;
  /* offset 67 */ u8 volume_id[4];
  /* offset 71 */ u8 volume_label[11];
  /* offset 82 */ u8 filesystem_tyep[8];
};

struct bpb_info16 {
  /* offset  0 */ u8 jmpboot[3]; /* ignore this */
  /* offset  3 */ u8 oemname[8];
  /* offset 11 */ u8 bytes_per_sector[2]; /* unaligned, so u8 [2] */
  /* offset 13 */ u8  sectors_per_cluster;
  /* offset 14 */ u16 n_reserved_sectors; 
  /* offset 16 */ u8 n_fats;  
  /* offset 17 */ u8 n_root_entries[2];
  /* offset 19 */ u8 total_sectors16[2];
  /* offset 21 */ u8 media;
  /* offset 22 */ u16 fat_size16;
  /* offset 24 */ u16 sectors_per_track;  
  /* offset 26 */ u16 n_heads;  
  /* offset 28 */ u32 n_hidden_sectors;  
  /* offset 32 */ u32 total_sectors32[2];
};

enum vfat_attribute {
  vfat_attr_read_only = 0x01,
  vfat_attr_hidden    = 0x02,
  vfat_attr_system    = 0x04,
  vfat_attr_volume_id = 0x08,
  vfat_attr_directory = 0x10,
  vfat_attr_archive   = 0x20,
  vfat_attr_long_name = 0x0f
};

struct fat_dir_entry {
  /* offset 00: */ unsigned char name[11];
  /* offset 11: */ u8 attr;
  /* offset 12: */ u8 should_be_zero;
  /* offset 13: */ u8 creation_time_tenth;
  /* offset 14: */ u16 creation_time;
  /* offset 16: */ u16 creation_date;
  /* offset 18: */ u16 last_access_date;
  /* offset 20: */ u16 first_cluster_high;
  /* offset 22: */ u16 last_write_time;
  /* offset 24: */ u16 last_write_date;
  /* offset 26: */ u16 first_cluster_low;
  /* offset 28: */ u32 n_bytes;
};

struct fat_ldir_entry {
   /* offset 00: */ u8 ord;
   /* offset 01: */ u8 name1[10]; /* unicode */
   /* offset 11: */ u8 attr;
   /* offset 12: */ u8 ldir_type;
   /* offset 13: */ u8 chksum;
   /* offset 14: */ u8 name2[12];
   /* offset 26: */ u8 must_be_zero[2];
   /* offset 28: */ u8 name3[4];
};

#define VFAT_EOC 0x00FFFFF8ul

struct vfat_filesystem {
  /* raw boot parameter block info */
  struct bpb_info info;
  /* io handle (i.e., block device) that is mounted */
  struct iohandle *iohandle;
  /* cooked info */
  u32 sector_size;
  /* fat info */ 
  enum fat_type fat_type;
  u32 fat_size;
  char *fat;
  /* root directory info */
  u32 n_root_entries;
  struct fat_dir_entry *root_dir_entries;
  char buf[4096];
};

extern int vfat_find_first_free(struct vfat_filesystem *vfat);
extern int vfat_write_fat(struct vfat_filesystem *vfat);
extern int vfat_count_free(struct vfat_filesystem *vfat);
extern int vfat_update_file_entry_cluster(struct vfat_filesystem *vfat, u32 parentcluster, struct fat_dir_entry *entry);
extern void vfat_list_one_entry(struct fat_dir_entry *dir, char *long_name);
extern int vfat_make_ready(void);

extern void command_vfat(int argc, const char **argv);

/*
 * Mount vfat partition on ioh device
 */
int vfat_mount(struct vfat_filesystem *vfat, struct iohandle *ioh);

/*
 * Mount vfat partition on ide partition partno (numbered from zero)
 */
int vfat_mount_partition(int partno);

/* 
 * Reads nbytes of data from the named file into the provided buffer. 
 *  If nbytes is 0, read whole file.
 */
int vfat_read_file(char *buf, const char *filename, size_t nbytes);


/*
 * returns an iohandle for a file in the vfat filesystem 
 */
struct iohandle *vfat_file_open(const char *filename);
