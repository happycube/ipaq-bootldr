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

#include "bootldr.h"
#include "heap.h"
#include "fs.h"
#include "ide.h"
#include "vfat.h"
#include "commands.h"
#include "serial.h"
#ifdef __linux__
#include <asm/errno.h>
#else
#include <errno.h>
#endif
#include "pcmcia.h"


#define N_DIR_BUF_ENTRIES 128
#define N_DIR_BUF_BYTES N_DIR_BUF_ENTRIES*sizeof(struct fat_dir_entry)

static int vfat_debug = 0;

static __inline__ int bpb_n_fats(struct bpb_info *info)
{
    return info->n_fats;
}
static __inline__ int bpb_bytes_per_sector(struct bpb_info *info)
{
    return (info->bytes_per_sector[1] << 8) + info->bytes_per_sector[0]; 
}
static __inline__ int bpb_sectors_per_cluster(struct bpb_info *info)
{
    return info->sectors_per_cluster;
}
static __inline__ int bpb_n_reserved_sectors(struct bpb_info *info)
{
    return info->n_reserved_sectors;
}

static __inline__ int bpb_n_root_entries(struct bpb_info *info)
{
    return (info->n_root_entries[1] << 8) + info->n_root_entries[0]; 
}

static __inline__ int bpb_root_dir_sectors(struct bpb_info *info)
{
    return (bpb_n_root_entries(info) * sizeof(struct fat_dir_entry) + (bpb_bytes_per_sector(info) - 1)) / bpb_bytes_per_sector(info);
}

static __inline__ int bpb_fat_size(struct bpb_info *info)
{
    int fat_size = 0;
    if (info->fat_size16 != 0) 
        fat_size = info->fat_size16;
    else 
        fat_size = info->fat_size32;
    return fat_size;
}
static __inline__ int bpb_first_root_dir_sector(struct bpb_info *info)
{
    return (bpb_n_reserved_sectors(info) 
            + (bpb_n_fats(info) * bpb_fat_size(info)));
}


static __inline__ int bpb_total_sectors(struct bpb_info *info)
{
    int total_sectors = 0;
    if (info->total_sectors16[0] || info->total_sectors16[1]) 
        total_sectors = (info->total_sectors16[1] << 8) | info->total_sectors16[0];
    else 
        total_sectors = info->total_sectors32;
    return total_sectors;
}

static __inline__ int bpb_n_data_sectors(struct bpb_info *info)
{
    return 
        bpb_total_sectors(info) - 
        (bpb_n_reserved_sectors(info) 
         + (bpb_n_fats(info) * bpb_fat_size(info))
         + bpb_root_dir_sectors(info));
}

static __inline__ int bpb_first_data_sector(struct bpb_info *info)
{
    return (bpb_n_reserved_sectors(info) 
            + (bpb_n_fats(info) * bpb_fat_size(info))
            + bpb_root_dir_sectors(info));
}

static __inline__ int bpb_n_clusters(struct bpb_info *info)
{
    return bpb_n_data_sectors(info) / bpb_sectors_per_cluster(info);
}

static __inline__ enum fat_type bpb_fat_type(struct bpb_info *info)
{
    u32 n_clusters = bpb_n_clusters(info);
    if (n_clusters < 4085)
        return ft_fat12;
    else if (n_clusters < 65535) 
        return ft_fat16;
    else 
        return ft_fat32;
}

static int bpb_seems_valid (struct bpb_info *info)
{
    int rc = 1;

    if (bpb_sectors_per_cluster(info) == 0) {
	putstr ("sectors_per_cluster=0 ");
	rc = 0;
    }

    if (bpb_bytes_per_sector(info) == 0) {
	putstr ("bytes_per_sector=0 ");
	rc = 0;
    }

    if (rc == 0)
	putstr("-- bpb_info invalid\r\n");

    return rc;
}

static int vfat_read_bpb_info(struct vfat_filesystem *vfat, struct iohandle *ioh)
{
    /* read bpb_info from first sector of partition */
    int rc = ioh->ops->read(ioh, vfat->buf, 0, 512); 
    if (!rc)
        memcpy(&vfat->info, vfat->buf, sizeof(struct bpb_info));
    return rc;
}

static __inline__ int fat_entry_first_clusterno(struct fat_dir_entry *entry)
{
    return (entry->first_cluster_high << 16) | entry->first_cluster_low;
}

void upcase(char *s) {
  char *t = s;
  while (*t) {
    if (*t >= 'a' && *t <= 'z')
      *t = *t + 'A' - 'a';
    t++;
  }
}

void fixup_shortname(unsigned char *dst, unsigned char *dosname)
{ 
    int i;
    int j = 0;
    int has_extension = 0;
    int dot_pos = 0;
    for (i = 0; i < 8; i++) {
        unsigned char c = dosname[i];
        if (c == ' ')
            break;
        if (c >= 'A' && c <= 'Z')
            c = c + 'a' - 'A';
        dst[j++] = c;
    } 
    dot_pos = j++;
    dst[dot_pos] = '.';
    for (i = 8; i < 11; i++) {
        unsigned char c = dosname[i];
        if (c == ' ')
            break;
        if (c >= 'A' && c <= 'Z')
            c = c + 'a' - 'A';
        has_extension = 1;
        dst[j++] = c;
    } 
    if (!has_extension)
        dst[dot_pos] = 0;
    dst[j++] = 0;
}

/* copies a segment of the name into the buffer, returns ordinal number read */
void fixup_longname(struct fat_ldir_entry *ldir, char *buf) {
  int offset;
  int i;
  char *name_ptr, *dir_ptr;
  /* build the name */
  if (ldir->ord & 128) {
    /* file has been deleted */
    buf[0] = '\0';
    return;
  }
  offset = ((ldir->ord & 0x3F) - 1) * 13;
  name_ptr = &buf[offset];

  dir_ptr = (char *) (ldir) + 1;  /* skip ordinal field */
  if (offset < 0) {
    putstr("Invalid long filename entry: ordinal < 0\r\n");
    return;
  } 
  if (offset >= 260) {
    putstr("Invalid long filename entry: filename too long\r\n");
    buf[260] = 0;
    return;
  }

  for (i = 5; i > 0; i--) {
    *name_ptr++ = *dir_ptr++;
    dir_ptr++; /* skip the empty byte of unicode */
  }

  dir_ptr += 3; /* skip flag byte, reserved field and checksum */
  for (i = 6; i > 0; i--) {
    *name_ptr++ = *dir_ptr++;
    dir_ptr++; /* skip the empty byte of unicode */
  }
  dir_ptr++; /* skip 0 field */
  dir_ptr++; /* skip 0 field */

  *name_ptr++ = *dir_ptr++;
  dir_ptr++; /* skip the empty byte of unicode */
  *name_ptr++ = *dir_ptr++; /* char 13 */
  if (ldir->ord & 0x40) {
    *name_ptr = 0;
  }
}

static __inline__ u32 vfat_next_clusterno(struct vfat_filesystem *vfat, u32 clusterno)
{
    enum fat_type ftype = vfat->fat_type;
    u32 entry = 0;
    switch (ftype) {
    case ft_fat16: {
        u16 *fat = (u16 *)vfat->fat; 
        entry = fat[clusterno];
        if (entry >= 0xFFF8)
            entry = VFAT_EOC; 
    } break; 
    case ft_fat32: {
        u32 *fat = (u32 *)vfat->fat; 
        entry = fat[clusterno];
        entry &= 0x00FFFFFF;
        if (entry >= 0x00FFFFF8)
            entry = VFAT_EOC;
    } break;
    case ft_fat12: {
       u8 *fat = (u8 *)vfat->fat;
       memcpy(&entry, fat + (clusterno + clusterno/2), sizeof(u16));
       if (clusterno & 1) /* if odd entry, shift right 4 bits */
          entry >>= 4;
       else
          entry &= 0xFFF; /* else use lower 12 bits */
    } break;
    default:
        return 0;
    }  
    return entry;
}

static void vfat_set_next_clusterno(struct vfat_filesystem *vfat, u32 clusterno, u32 newnext)
{
    enum fat_type ftype = vfat->fat_type;
    switch (ftype) {
    case ft_fat16: {
        u16 *fat = (u16 *)vfat->fat; 
	if (newnext == VFAT_EOC) 
	  fat[clusterno] = 0xFFF8;
	else 
	  fat[clusterno] = newnext;
    } break; 
    case ft_fat32: {
        u32 *fat = (u32 *)vfat->fat;
	if (newnext == VFAT_EOC)
	  fat[clusterno] = 0x00FFFFFF;
	else
	  fat[clusterno] = newnext;
    } break;
    case ft_fat12: {
       u8 *fat = (u8 *)vfat->fat;
       u16 tempentry;
       memcpy(&tempentry, fat + (clusterno + clusterno / 2), sizeof(u16));
       if (clusterno & 1) 
          tempentry = (tempentry & 0xF) + (newnext << 4);
       else
	 tempentry = (tempentry & 0xF000) + newnext;
       memcpy((u8 *) (fat + (clusterno + clusterno/2)), &tempentry, sizeof(u16));
    } break;
    default:
    }  
}

u32 vfat_clusterno(struct vfat_filesystem *vfat, u32 clusterno, size_t byte_offset, size_t *offset_in_cluster)
{
    struct bpb_info *info = &vfat->info;
    u32 sectors_per_cluster = bpb_sectors_per_cluster(info);
    u32 bytes_per_sector = bpb_bytes_per_sector(info);
    u32 bytes_per_cluster = bytes_per_sector * sectors_per_cluster;
    while (byte_offset >= bytes_per_cluster && clusterno != VFAT_EOC) {
        u32 next_clusterno = vfat_next_clusterno(vfat, clusterno);
        byte_offset -= bytes_per_cluster;
        clusterno = next_clusterno;
    }
    if (offset_in_cluster) {
      *offset_in_cluster = byte_offset;
    }
    return clusterno;
}


int vfat_read_clusters_offset(struct vfat_filesystem *vfat, char *buf, u32 clusterno, size_t nbytes, size_t offset)
{
    struct bpb_info *info = &vfat->info;
    u32 sectors_per_cluster = bpb_sectors_per_cluster(info);
    u32 bytes_per_sector = bpb_bytes_per_sector(info);
    u32 bytes_per_cluster = bytes_per_sector * sectors_per_cluster;

    u32 sector_in_cluster = offset / bytes_per_sector;
    u32 offset_in_sector = offset % bytes_per_sector;
    u32 first_data_sector = bpb_first_data_sector(info);
    u32 sector_size = vfat->sector_size;
    size_t bytes_read = 0;
    if (nbytes == 0) {
      putstr("vfat_read_clusters called with nbytes=0\r\n");
      return 0;
    }
    // putLabeledWord("sector_in_cluster=", (offset / bytes_per_sector));
    if (offset > bytes_per_cluster) {
      putLabeledWord(__FUNCTION__ ": offset is not in this cluster: offset=", offset);
      return -EINVAL;
    }
    if (offset_in_sector) {
      putLabeledWord(__FUNCTION__ ": offset is not at beginning of sector: offset=", offset);
    }
    while (clusterno != VFAT_EOC && bytes_read < nbytes) {
        u32 sector = first_data_sector + sectors_per_cluster * (clusterno - 2) + sector_in_cluster;
        size_t bytes_read_this_cluster = sector_in_cluster * bytes_per_sector;
	if (vfat_debug > 1) {
	  putLabeledWord("Reading cluster = ", clusterno);
	}
        while (bytes_read < nbytes && bytes_read_this_cluster < bytes_per_cluster) {
            int rc;
	    if (vfat_debug > 1) {
	      putLabeledWord("Reading sector = ", sector);
	      putLabeledWord("Placing at = ", (unsigned long)(buf + bytes_read));
	    }
            rc = vfat->iohandle->ops->read(vfat->iohandle, buf+bytes_read, sector * sector_size, bytes_per_sector);
            if (rc < 0) return rc;
            bytes_read += bytes_per_sector;
	    if (bytes_read > nbytes) {
	      /* do not say we have more data than was asked for */
	      bytes_read = nbytes;
	    }
            bytes_read_this_cluster += bytes_per_sector;
            sector++;
        }
        clusterno = vfat_next_clusterno(vfat, clusterno);
	sector_in_cluster = 0;
    }
    if (clusterno == VFAT_EOC && bytes_read < nbytes) {
       putLabeledWord(__FUNCTION__ ": reached VFAT_EOC at bytes_read=", bytes_read);
    }
    return bytes_read;
}

/*
 * Note: first data clusterno is 2!
 */
int vfat_read_clusters(struct vfat_filesystem *vfat, char *buf, u32 clusterno, size_t nbytes)
{
  return vfat_read_clusters_offset(vfat, buf, clusterno, nbytes, 0);
}

static int vfat_free_clusters(struct vfat_filesystem *vfat, u32 first_clusterno, u8 write_fat)
{
  u32 clusterno;
  while (first_clusterno != VFAT_EOC && first_clusterno != 0) {
    if (vfat_debug) {
      putLabeledWord("Freeing cluster ", first_clusterno);
    }
    clusterno = vfat_next_clusterno(vfat, first_clusterno);
    vfat_set_next_clusterno(vfat, first_clusterno, 0);
    first_clusterno = clusterno;
  }
  vfat_set_next_clusterno(vfat, first_clusterno, 0);
  if (write_fat) vfat_write_fat(vfat);
  return 0;
}

/* Allocates a cluster chain efficiently */
static int vfat_allocate_clusters(struct vfat_filesystem *vfat, u32 parentcluster, struct fat_dir_entry *entry, size_t nbytes)
{
  struct bpb_info *info = &vfat->info;
  u32 first_clusterno = fat_entry_first_clusterno(entry);
  u32 n_clusters = bpb_n_clusters(info);
  u32 sectors_per_cluster = bpb_sectors_per_cluster(info);
  u32 bytes_per_sector = bpb_bytes_per_sector(info);
  u32 bytes_per_cluster = bytes_per_sector * sectors_per_cluster;
  u32 clusterno, next;
  size_t bytes_seen = 0;

  if (entry->n_bytes == nbytes) return first_clusterno;  /* already all allocated */
  
  if ((nbytes - 1) / bytes_per_cluster != (entry->n_bytes - 1) / bytes_per_cluster) {
    if (first_clusterno) {
      if (vfat_debug) {
		putstr("Destroying currently allocated chain...\r\n");
      }
      vfat_free_clusters(vfat, first_clusterno, 0);
    }
    if (vfat_count_free(vfat) < (nbytes + bytes_per_cluster - 1) / bytes_per_cluster) {
      putstr("Not enough free space on device.\r\n");
      return -EINVAL;
    }
    first_clusterno = clusterno = 0;
    if (nbytes > 0) {
       /* go to the first data cluster */   
       for (next = 3;
	   next < n_clusters && bytes_seen <= nbytes;
	   next++)
	{
	  if (vfat_next_clusterno(vfat, next) == 0) {
	    if (vfat_debug) {
	      putLabeledWord("Allocating cluster ", next);
	    }
	    if (first_clusterno == 0) {
	      first_clusterno = clusterno = next;
	      entry->first_cluster_high = first_clusterno >> 16;
	      entry->first_cluster_low = first_clusterno & 0xFFFF;
	    } else {
	      vfat_set_next_clusterno(vfat, clusterno, next);
	      clusterno = next;
	    }
	    bytes_seen += bytes_per_cluster;
	  }
	}
      vfat_set_next_clusterno(vfat, clusterno, VFAT_EOC);
    }
    vfat_write_fat(vfat);
  }

  if (entry->n_bytes != nbytes) {
    if (nbytes == 0) {
      entry->first_cluster_high = entry->first_cluster_low = 0;
    }
    entry->n_bytes = nbytes;
    vfat_update_file_entry_cluster(vfat, parentcluster, entry);
  }
  return first_clusterno;
}


struct vfat_iohandle {
   struct iohandle         ioh; /* must be first */
   struct vfat_filesystem *vfat;  
   struct fat_dir_entry    entry;
   u32                     parentcluster;
};

static int vfat_iohandle_prewrite(struct iohandle *ioh, size_t offset, size_t nbytes)
{
   if (ioh) {
      struct vfat_iohandle *vioh = (struct vfat_iohandle *)ioh->pdata;
      struct fat_dir_entry *entry = &vioh->entry;
      struct vfat_filesystem *vfat = vioh->vfat;
      u32 parentcluster = vioh->parentcluster;
      return vfat_allocate_clusters(vfat, parentcluster, entry, nbytes);
   } else {
      return -EINVAL;
   }
}

/*
 * Writes files of the allocated length. (see vfat_allocate_clusters)
 */
int vfat_write_clusters_offset(struct vfat_filesystem *vfat, char *buf, u32 clusterno, size_t nbytes, size_t offset)
{
  struct bpb_info *info = &vfat->info;
  u32 sectors_per_cluster = bpb_sectors_per_cluster(info);
  u32 bytes_per_sector = bpb_bytes_per_sector(info);
  u32 bytes_per_cluster = bytes_per_sector * sectors_per_cluster;
  u32 sector_in_cluster;
  u32 offset_in_sector;

  u32 first_data_sector = bpb_first_data_sector(info);
  u32 sector_size = vfat->sector_size;
  u32 next = VFAT_EOC;
  size_t bytes_written = 0;

  offset = offset % bytes_per_cluster;
  sector_in_cluster = offset / bytes_per_sector;
  offset_in_sector = offset % bytes_per_sector;
  if (vfat_debug >= 2) {
    putLabeledWord("vfat_write_clusters_offset offset = ", offset);
    if (vfat_debug >= 3)
      hex_dump(buf, nbytes);
  }
  if (nbytes == 0)
    nbytes = sector_size;
  if (offset > bytes_per_cluster) {
    putLabeledWord(__FUNCTION__ ": offset is not in this cluster: offset=", offset);
	return -EINVAL;
  }
  if (offset_in_sector) {
    putLabeledWord(__FUNCTION__ ": offset is not at beginning of sector: offset=", offset);
	return -EINVAL;
  }
  while (bytes_written < nbytes) {
    u32 sector = first_data_sector + sectors_per_cluster * (clusterno - 2) + sector_in_cluster;
    size_t bytes_written_this_cluster = sector_in_cluster * bytes_per_sector;
    if (vfat_debug) {
      putLabeledWord("Writing cluster = ", clusterno);
    }
    while (bytes_written < nbytes && bytes_written_this_cluster < bytes_per_cluster) {
      int rc;
      if (vfat_debug) {
		putLabeledWord("Writing sector = ", sector);
		putLabeledWord("bytes_written = ", bytes_written);
      }
      rc = vfat->iohandle->ops->write(vfat->iohandle, buf+bytes_written, sector * sector_size, bytes_per_sector);

      if (rc < 0) return rc;
      if (bytes_written + bytes_per_sector >= nbytes) {
	bytes_written = nbytes;
      } else {
	bytes_written += bytes_per_sector;
      }
      bytes_written_this_cluster += bytes_per_sector;
      sector++;
    }

    next = vfat_next_clusterno(vfat, clusterno);
    if (bytes_written >= nbytes) 
      break; /* done writing; clusterno points to last sector */
    sector_in_cluster = 0; // only nonzero offset into cluster for first cluster because of offset
    clusterno = next;
  }
  return bytes_written;
}
/*
 * returns the clusterno of the first free cluster, or -1 if none available
 */
int vfat_find_first_free(struct vfat_filesystem *vfat)
{
  int i;
  int n = bpb_n_clusters(&vfat->info);
  for (i = 2; i < n; i++)
    if (vfat_next_clusterno(vfat, i) == 0) return i;
  return -1;
}

/*
 * returns the number of free clusters
 */
int vfat_count_free(struct vfat_filesystem *vfat)
{
  int i;
  int ctr = 0; 
  int n = bpb_n_clusters(&vfat->info);
  for (i = 2; i < n; i++)
    if (vfat_next_clusterno(vfat, i) == 0) ctr++;
  return ctr;
}

void get_basename(char *bname, const char *fname)
{
    char *sep_pos = strrchr(fname, '/');
    if (sep_pos)
        strcpy(bname, sep_pos+1);
    else
        strcpy(bname, fname);
}

void get_dirname(char *dirname, const char *fname)
{
    char *sep_pos = strrchr(fname, '/');
    if (sep_pos) {
        while (fname < sep_pos) {
            *dirname++ = *fname++;
        }
    }
    *dirname = 0;
}

int vfat_find_dir_entry(struct vfat_filesystem *vfat, struct fat_dir_entry *outdir, const char *fname)
{
    char basename[128];
    char dirname[128];
    char longbuf[261];
    int use_long_name = 0;
    struct fat_dir_entry dir_storage;
    struct fat_dir_entry *dir = &dir_storage;
    struct fat_dir_entry entries[N_DIR_BUF_ENTRIES];
    int rc = 0;
    get_basename(basename, fname);
    get_dirname(dirname, fname);
    putstr("vfat_find_dir_entry: fname='"); putstr(fname); putstr("'\r\n"); 
    putstr("                   dirname='"); putstr(dirname); putstr("'\r\n"); 
    putstr("                  basename='"); putstr(basename); putstr("'\r\n"); 
    if (dirname[0]) {
        if ((rc = vfat_find_dir_entry(vfat, &dir_storage, dirname)) == 0) {
            size_t n_bytes = dir->n_bytes;
            if (n_bytes == 0) n_bytes = N_DIR_BUF_BYTES;
            // entries = mmalloc(n_bytes); 
	    memset(entries, 0, n_bytes);
            n_bytes = vfat_read_clusters(vfat, (char *)entries, fat_entry_first_clusterno(dir), n_bytes);
            dir = entries;
        } else {
            return rc; 
        }
    } else {
        putstr("  searching root_dir_entries\r\n");
        dir = vfat->root_dir_entries;
    }
    if (!basename[0])
        strcpy(basename, ".");
    upcase(basename);
    
    while (dir->name[0]) {
        if (dir->attr == 0) {
	  /* skip if attr is 0 */
        } else if (dir->name[0] == 0xE5) {
            /* skip deleted entries */
        } else if (dir->attr & vfat_attr_long_name) {
	  use_long_name = 1;
	  fixup_longname((struct fat_ldir_entry *) dir, longbuf);
	} else {
	  int result = 1;
	  if (use_long_name) {
	    upcase(longbuf);
	    result = strcmp(longbuf, basename);
	  }
	  if (result) {
            char name[128];	    
            fixup_shortname(name, dir->name);
	    upcase(name);
            result = strcmp(name, basename);
	  }
	  if (result == 0) {
            if (vfat_debug) {
              putstr("  vfat_find_dir_entry:\r\n");
              hex_dump((char *)dir, sizeof(struct fat_dir_entry));
            }
	    memcpy(outdir, dir, sizeof(struct fat_dir_entry));
	    // if (entries)
	    //    mfree(entries);
	    if (!(outdir->attr & vfat_attr_directory))
	      return -ENOTDIR;
	    return 0;
	  }
	  use_long_name = 0;
        }
        dir++;
    }
    return -ENOENT;
}

struct fat_dir_entry *vfat_find_file_in_dir(struct fat_dir_entry *dir, char *basename, int max) {
  int use_long_name = 0;
  int ctr = 0;
  char longbuf[261];
  while (dir->name[0] && (max == 0 || ctr < max)) {
    ctr++;
    if (dir->attr == 0) {
      /* skip if attr is zero */
    } else if (dir->name[0] == 0xe5) {
      /* skip if entry is deleted */
    } else if (dir->attr & vfat_attr_long_name) {
      use_long_name = 1;
      fixup_longname((struct fat_ldir_entry *) dir, longbuf);
    } else {
      int result = 1;
      if (use_long_name) {
	upcase(longbuf);
	putstr("find_file_in_dir: "); vfat_list_one_entry(dir, longbuf);
	result = strcmp(longbuf, basename);
      }
      if (result) {
	char name[15];	    
	putstr("find_file_in_dir: "); vfat_list_one_entry(dir, longbuf);
	fixup_shortname(name, dir->name);
	upcase(name);
	result = strcmp(name, basename);
      }
      if (result == 0) return dir;
      use_long_name = 0;
    }
    dir++;
  }
  return (struct fat_dir_entry *) 0;
}

int vfat_find_file_entry(struct vfat_filesystem *vfat, struct fat_dir_entry *outparent, struct fat_dir_entry *outentry, const char *fname)
{
    char basename[128];
    char dirname[128];
    size_t n_bytes;
    struct fat_dir_entry dir_storage;
    struct fat_dir_entry *dir = &dir_storage;
    struct fat_dir_entry entries[128];
    int rc = 0;
    get_basename(basename, fname);
    upcase(basename);
    get_dirname(dirname, fname);
    putstr("vfat_find_file_entry: fname='"); putstr(fname); putstr("'\r\n"); 
    putstr("                    dirname='"); putstr(dirname); putstr("'\r\n"); 
    putstr("                   basename='"); putstr(basename); putstr("'\r\n"); 
    if (dirname[0]) {
        if ((rc = vfat_find_dir_entry(vfat, dir, dirname)) == 0) {
		  if (outparent)
			memcpy(outparent, dir, sizeof(struct fat_dir_entry));
		  n_bytes = dir->n_bytes;
		  if (vfat_debug) {
			putstr("vfat_find_file_entry: got dir entry for "); putstr(dirname); putstr("\r\n");
			putLabeledWord("  attr=", dir->attr);
			putLabeledWord("  clusterno=", fat_entry_first_clusterno(dir));
			putLabeledWord("  n_bytes=", n_bytes);
		  }
		  if (n_bytes == 0) n_bytes = N_DIR_BUF_BYTES;
		  memset(entries, 0, n_bytes);
		  n_bytes = vfat_read_clusters(vfat, (char *)entries, fat_entry_first_clusterno(dir), n_bytes);
		  dir = entries;
        } else {
		  return rc; 
        }
    } else {
      if (outparent) {
		outparent->attr = 0;
      }
      if (vfat_debug)
        putstr("  looking for file entry in root_dir_entries:\r\n");
      dir = vfat->root_dir_entries;
    }
    
    {
      struct fat_dir_entry *file = vfat_find_file_in_dir(dir, basename, 0);
      if (file) {
		if (vfat_debug) {
		  putstr("  vfat_find_file_entry succeeded:\r\n");
		  hex_dump((char *) file, sizeof(struct fat_dir_entry));
		}
		memcpy(outentry, file, sizeof(struct fat_dir_entry));
		if (outentry->attr & vfat_attr_directory)
		  return -EISDIR;
		return 0;
      } else {
		putstr("Could not find file.");
		return -EINVAL;
      }
    }
}

void vfat_list_one_entry(struct fat_dir_entry *dir, char *long_name)
{
  putstr(" "); 
  if (long_name == NULL) {
    char name[13];
    fixup_shortname(name, dir->name);
    putstr(name); 
  } else {
    putstr(long_name);
  }
  putstr("\r\n");
  putLabeledWord("   attr=", dir->attr);
  putLabeledWord("   first_cluster=", (dir->first_cluster_high << 16) | dir->first_cluster_low);
  putLabeledWord("   n_bytes=", dir->n_bytes);
}

void vfat_list_dir_entries(struct fat_dir_entry *dir)
{
  int use_long_name = 0;
  char longbuf[261];  /* maximum length of vfat name is 260 */
  if (vfat_debug)
    putLabeledWord("  vfat_list_dir_entries: attr=", dir->attr);
  while (dir->name[0]) {
    if (dir->attr == 0) {
      /* skip if attr is zero */
    } else if (dir->name[0] == 0xe5) {
      /* skip if entry is deleted */
    } else if (dir->attr & vfat_attr_long_name) {
      use_long_name = 1;
      fixup_longname((struct fat_ldir_entry *) dir, longbuf);
    } else {
      if (use_long_name) {
		vfat_list_one_entry(dir, longbuf);
      } else {
		vfat_list_one_entry(dir, NULL);
      }
      use_long_name = 0;
    }
    dir++;
  }
}


/*
 * lists the file or directory named by fname
 */
int vfat_list(struct vfat_filesystem *vfat, const char *fname)
{
    struct fat_dir_entry entry;
    size_t n_bytes = N_DIR_BUF_BYTES;
    int rc = vfat_find_dir_entry(vfat, &entry, fname);
    if (!rc)
        return rc;
    if (entry.n_bytes)
        n_bytes = entry.n_bytes;
    if (entry.attr & vfat_attr_directory) {
        struct fat_dir_entry entries[N_DIR_BUF_ENTRIES];
        // entries = mmalloc(n_bytes);
        if (entries) {
	    memset(entries, 0, n_bytes);
	    n_bytes = vfat_read_clusters(vfat, (char *)entries, fat_entry_first_clusterno(&entry), n_bytes);
            vfat_list_dir_entries(entries);
            if (entries != vfat->root_dir_entries)
                mfree(entries);
        } else {
            return -EINVAL;
        }
    } else {
        /* file entry */
        vfat_list_one_entry(&entry, NULL);
    }
    return 0;
}



static int vfat_iohandle_file_read(struct iohandle *ioh, char *buf, size_t offset, size_t nbytes)
{
  if (ioh) {
      struct vfat_iohandle *vioh = (struct vfat_iohandle *)ioh->pdata;
      struct fat_dir_entry *entry = &vioh->entry;
      struct vfat_filesystem *vfat = vioh->vfat;
      u32 first_clusterno = fat_entry_first_clusterno(entry);
      size_t offset_in_cluster = 0;
      u32 clusterno = vfat_clusterno(vfat, first_clusterno, offset, &offset_in_cluster);
      return vfat_read_clusters_offset(vfat, buf, clusterno, nbytes, offset_in_cluster);
  } else {
    return -EINVAL;
  }
}

static int vfat_iohandle_file_write(struct iohandle *ioh, char *buf, size_t offset, size_t nbytes)
{
   if (ioh) {
      struct vfat_iohandle *vioh = (struct vfat_iohandle *)ioh->pdata;
      struct fat_dir_entry *entry = &vioh->entry;
      struct vfat_filesystem *vfat = vioh->vfat;
      u32 first_clusterno = fat_entry_first_clusterno(entry);
      size_t offset_in_cluster = 0;
      u32 clusterno = vfat_clusterno(vfat, first_clusterno, offset, &offset_in_cluster);
      return vfat_write_clusters_offset(vfat, buf, clusterno, nbytes, offset_in_cluster);
   } else {
      return -EINVAL;
   }
}

int vfat_iohandle_release(struct iohandle *ioh)
{
    // mfree(ioh);
    return 0;
}

static int vfat_iohandle_len(struct iohandle *ioh)
{
   if (ioh) {
      struct vfat_iohandle *vioh = (struct vfat_iohandle *)ioh->pdata;
      struct fat_dir_entry *entry = &vioh->entry;
      return entry->n_bytes;
   } else {
      return 0;
   }
}

struct iohandle_ops vfat_iohandle_ops = {
    read: vfat_iohandle_file_read,
    write: vfat_iohandle_file_write,
    close: vfat_iohandle_release,
    len: vfat_iohandle_len,
    prewrite: vfat_iohandle_prewrite
};

struct iohandle *vfat_get_file_entry_iohandle(struct vfat_filesystem *vfat, struct fat_dir_entry *parent, struct fat_dir_entry *entry)
{
    struct vfat_iohandle *vioh = (struct vfat_iohandle *)malloc(sizeof(struct vfat_iohandle));
    struct iohandle *ioh = &vioh->ioh;
    memset(vioh, 0, sizeof(struct vfat_iohandle));
    ioh->ops = &vfat_iohandle_ops;
    ioh->pdata = vioh;
    vioh->vfat = vfat;
    memcpy(&vioh->entry, entry, sizeof(struct fat_dir_entry));
    if (parent->attr)
       vioh->parentcluster = fat_entry_first_clusterno(parent);
    return ioh;
}


int vfat_mount(struct vfat_filesystem *vfat, struct iohandle *ioh)
{
    char oemname[9];
    struct bpb_info *info = &vfat->info;
    int rc = 0;
    size_t sector_size = ioh->sector_size;
    memset(vfat, 0, sizeof(struct vfat_filesystem));
    vfat->iohandle = ioh;
    putstr("vfat mount: reading bpb_info\r\n");
    if ((vfat_read_bpb_info(vfat, ioh) == 0)
	&& bpb_seems_valid(info)) {
        u32 sectors_per_cluster = bpb_sectors_per_cluster(info);
        u32 n_reserved_sectors = bpb_n_reserved_sectors(info);
        u32 n_root_entries = bpb_n_root_entries(info);
        u32 root_dir_sectors = bpb_root_dir_sectors(info);
        u32 first_root_dir_sector = bpb_first_root_dir_sector(info);
        u32 fat_size = bpb_fat_size(info);
        u32 fat_size_bytes = fat_size * sector_size;
        u32 n_fats = bpb_n_fats(info);
        u32 total_sectors = bpb_total_sectors(info);
        u32 first_data_sector = bpb_first_data_sector(info);
        u32 n_data_sectors = bpb_n_data_sectors(info);
        u32 n_clusters = bpb_n_clusters(info);
        u32 fat_type = bpb_fat_type(info);

        if (vfat_debug)
          hex_dump((unsigned char *)&vfat->info, sizeof(struct bpb_info));
        memcpy(oemname, vfat->info.oemname, 8);
        putstr("  oemname="); putstr(oemname); putstr("\r\n");
        if (vfat_debug) {
          putLabeledWord("  sectors_per_cluster=", sectors_per_cluster);
          putLabeledWord("  n_reserved_sectors=", n_reserved_sectors);
          putLabeledWord("  n_root_entries=", n_root_entries);
          putLabeledWord("  root_dir_sectors=", root_dir_sectors);
          putLabeledWord("  first_root_dir_sector=", first_root_dir_sector);
          putLabeledWord("  fat_size=", fat_size);
          putLabeledWord("  fat_size_bytes=", fat_size_bytes);
          putLabeledWord("  n_fats=", n_fats);
          putLabeledWord("  total_sectors=", total_sectors);
          putLabeledWord("  n_data_sectors=", n_data_sectors);
          putLabeledWord("  first_data_sector=", first_data_sector);
          putLabeledWord("  n_clusters=", n_clusters);
          putLabeledWord("  fat_type=", fat_type);
        }

        /* gather data */ 
        sector_size = bpb_bytes_per_sector(info);
        vfat->sector_size = sector_size;

        vfat->fat_type = fat_type;

        /* read the root_dir_entries */
        vfat->n_root_entries = n_root_entries;
        vfat->root_dir_entries = (struct fat_dir_entry *)mmalloc(n_root_entries * sizeof(struct fat_dir_entry));
        { 
            int offset = 0;
            u32 start_byte = sector_size * first_root_dir_sector;
            u32 nbytes = n_root_entries * sizeof(struct fat_dir_entry);
            char *buf = (char *)vfat->root_dir_entries;
            for (offset = 0; offset < nbytes; offset += sector_size) {
                rc = ioh->ops->read(ioh, buf+offset, start_byte+offset, sector_size);
                if (rc)
                    return rc;
            }
        }
        if (vfat_debug) {
          putstr("root_dir_entries:\r\n");
          hex_dump((char *)vfat->root_dir_entries, 0x96);
        }

        /* read the fat */
        vfat->fat_size = fat_size;
        vfat->fat = mmalloc(sector_size * bpb_fat_size(info));
        { 
            int offset = 0;
            u32 start_byte = sector_size * bpb_n_reserved_sectors(info);
            u32 nbytes = sector_size * bpb_fat_size(info);
            char *buf = (char *)vfat->fat;
            for (offset = 0; offset < nbytes; offset += sector_size) {
                rc = ioh->ops->read(ioh, buf+offset, start_byte+offset, sector_size);
                if (rc)
                    return rc;
            }
        }
        if (vfat_debug) {
          putstr("fat:\r\n");
          hex_dump((char *)vfat->fat, 0x40);
        }

        /* read some data */
        if (vfat_debug) {
          rc = ioh->ops->read(ioh, vfat->buf, sector_size * first_root_dir_sector, sector_size);
          putstr("first data:\r\n");
          hex_dump(vfat->buf, 0x40);
        }
    }
    return rc;
}

int vfat_write_fat(struct vfat_filesystem *vfat) {
  int offset = 0;
  int rc;
  u32 start_byte = vfat->sector_size * bpb_n_reserved_sectors(&vfat->info);
  u32 nbytes = vfat->sector_size * vfat->fat_size;
  char *buf = (char *)vfat->fat;
  for (offset = 0; offset < nbytes; offset += vfat->sector_size) {
    rc = vfat->iohandle->ops->write(vfat->iohandle, buf+offset, start_byte+offset, vfat->sector_size);
    if (rc)
      return rc;
  }
  return 0;
}

struct vfat_filesystem *vfat = 0; 

SUBCOMMAND(vfat, mount, command_vfat_mount, "[partition] -- fat partition on IDE partition", BB_RUN_FROM_RAM, 0);
void command_vfat_mount(int argc, const char **argv) {
  struct bpb_info *info;
  u32 partid = 2;
  struct iohandle *ioh = NULL;
  int rc = 0;
  
  vfat = mmalloc(sizeof(struct vfat_filesystem));;
  if (!vfat) {
    putstr("mmalloc failed\r\n");
    return;
  }
  info = &vfat->info;
  if (argv[2])
    partid = strtoul(argv[2], 0, 0);
  putLabeledWord("cmd vfat mount: partid=", partid);
  ioh = get_ide_partition_iohandle(partid);
  if (!ioh) {
    putstr("get_ide_partition_iohandle failed\r\n");
    return;
  }
  rc = vfat_mount(vfat, ioh);
  if (rc) {
    putLabeledWord("vfat_mount failed with rc=", rc);
    mfree(vfat);
    vfat = NULL;
    return;
  }          
  putstr("cmd vfat mount: listing the root directory \r\n");
  vfat_list_dir_entries(vfat->root_dir_entries);
}

SUBCOMMAND(vfat, ls, command_vfat_ls,    "[dir] -- lists directory contents on mounted vfat system", BB_RUN_FROM_RAM, 0);
void command_vfat_ls(int argc, const char **argv)
{
  const char *fname = argv[2];
  struct fat_dir_entry entries[N_DIR_BUF_ENTRIES];
  
  vfat_make_ready();
  if (vfat) {
    struct fat_dir_entry entry;
    memset(&entry, 0, sizeof(entry));
    if (!fname) {
      vfat_list_dir_entries(vfat->root_dir_entries); 
    } else {
      int rc = vfat_find_dir_entry(vfat, &entry, fname);
      putLabeledWord("vfat ls: rc=", rc);
      putLabeledWord("vfat ls: entry.n_bytes=", entry.n_bytes);
      memset(entries, 0, N_DIR_BUF_BYTES);
      vfat_read_clusters(vfat, (char *)entries, fat_entry_first_clusterno(&entry), N_DIR_BUF_BYTES);
      vfat_list_dir_entries(entries);
    }
  } else {
    putstr("no vfat mounted\r\n");
  }
}

SUBCOMMAND(vfat, next_cluster, command_vfat_next_cluster, "[clusterno] -- computes next clusterno from vfat", BB_RUN_FROM_RAM, 0); 
void command_vfat_next_cluster(int argc, const char **argv) {
  u32 clusterno = strtoul(argv[2], 0, 0);
  putLabeledWord("clusterno=", clusterno);
  putLabeledWord("nextcluster=", vfat_next_clusterno(vfat, clusterno));
}

SUBCOMMAND(vfat, read_cluster, command_vfat_read_cluster, "<dstaddr> <clusterno> <nbytes>", BB_RUN_FROM_RAM, 0); 
void command_vfat_read_cluster(int argc, const char **argv) {
  u32 dstaddr = strtoul(argv[2], 0, 0);
  u32 clusterno = strtoul(argv[3], 0, 0);
  u32 nbytes = strtoul(argv[4], 0, 0);
  if (!argv[2] || !argv[3] || !argv[4]) putstr("vfat read_cluster <dstaddr> <clusterno> <nbytes>\r\n"); 
  else vfat_read_clusters(vfat, (char *)dstaddr, clusterno, nbytes);
}

SUBCOMMAND(vfat, debug, command_vfat_debug, " [level] -- toggle/set debug verbosity", BB_RUN_FROM_RAM, 2); 
void command_vfat_debug(int argc, const char **argv) 
{
  if (argc == 0)
    vfat_debug ^= 1;
  else
    vfat_debug = strtoul(argv[0], 0, 0);
  putLabeledWord("vfat_debug = ", vfat_debug);
}
/*
 * Mount vfat partition on ide partition partno (numbered from zero)
 */
int vfat_mount_partition(int partid)
{
   struct bpb_info *info;
   struct iohandle *ioh = NULL;

   vfat = mmalloc(sizeof(struct vfat_filesystem));;
   info = &vfat->info;
   putLabeledWord("cmd vfat mount: partid=", partid);
   ioh = get_ide_partition_iohandle(partid);
   return vfat_mount(vfat, ioh);
}

int vfat_make_ready(void)
{
  if (!vfat) {
#ifdef CONFIG_H3600_SLEEVE
    extern void h3600_sleeve_insert(void);
    h3600_sleeve_insert();
#endif
#ifdef CONFIG_PCMCIA
    pcmcia_insert();
#endif    
    vfat_mount_partition(0);
  }
  return 0;
}

struct iohandle *vfat_file_open(const char *filename)
{
   vfat_make_ready();
   if (vfat) {
      struct fat_dir_entry entry;
      struct fat_dir_entry parent;
      int rc = vfat_find_file_entry(vfat, &parent, &entry, filename);
      if (rc) {
 	 putLabeledWord(__FUNCTION__ ": file not found errno=", -rc);
         return NULL;
      }
      return vfat_get_file_entry_iohandle(vfat, &parent, &entry);
   } else {
      return NULL;
   }
}

/* 
 * Reads nbytes of data from the named file into the provided buffer. 
 *  If nbytes is 0, read whole file.
 */
int vfat_read_file(char *buf, const char *filename, size_t nbytes)
{
   vfat_make_ready();
   if (vfat) {
      struct fat_dir_entry entry;
      int rc = vfat_find_file_entry(vfat, 0, &entry, filename);
      u32 first_clusterno;
      if (rc)
         return rc;
      if (nbytes == 0)
         nbytes = entry.n_bytes;
      first_clusterno = fat_entry_first_clusterno(&entry);
      return vfat_read_clusters(vfat, buf, first_clusterno, nbytes);
   } else {
      return -EINVAL;
   }
}

static int vfat_update_file_entry(struct fat_dir_entry *parent, struct fat_dir_entry *entry)
{
  struct fat_dir_entry entries[N_DIR_BUF_ENTRIES];
  struct fat_dir_entry *my_entry;
  char basename[14];
  fixup_shortname(basename, entry->name);
  upcase(basename);

  if (parent) {
    u32 dir_n_bytes = N_DIR_BUF_BYTES;
    if (vfat_debug) putstr("Reading in parent entries...\r\n"); 
    memset(entries, 0, dir_n_bytes);
    dir_n_bytes = vfat_read_clusters(vfat, (char *) entries, fat_entry_first_clusterno(parent), dir_n_bytes);
    my_entry = vfat_find_file_in_dir(entries, basename, 0);
    if (!my_entry) { putstr("Cannot find file.\r\n"); return -EINVAL; }
    memcpy(my_entry, entry, sizeof(struct fat_dir_entry));
    return vfat_write_clusters_offset(vfat, (char *) entries, fat_entry_first_clusterno(parent), dir_n_bytes, 0);
  } else { /* under root directory */
    int offset = 0;
    u32 start_byte = vfat->sector_size * bpb_first_root_dir_sector(&vfat->info);
    /* write the root_dir_entries */
    my_entry = vfat_find_file_in_dir(vfat->root_dir_entries, basename, vfat->n_root_entries);
    if (!my_entry) { putstr("Cannot find file.\r\n"); return -EINVAL; }
    memcpy(my_entry, entry, sizeof(struct fat_dir_entry));
    offset = (my_entry - vfat->root_dir_entries);
    offset = offset - offset % vfat->sector_size;
    return vfat->iohandle->ops->write(vfat->iohandle,
				      (char *) (vfat->root_dir_entries + offset),
				      start_byte + offset, vfat->sector_size);
  }
}

int vfat_update_file_entry_cluster(struct vfat_filesystem *vfat, u32 parentcluster, struct fat_dir_entry *entry)
{
  struct fat_dir_entry entries[N_DIR_BUF_ENTRIES];
  struct fat_dir_entry *my_entry;
  char basename[14];
  fixup_shortname(basename, entry->name);
  upcase(basename);

  if (parentcluster) {
    u32 dir_n_bytes = N_DIR_BUF_BYTES;
    if (vfat_debug) putstr("Reading in parent entries...\r\n"); 
    memset(entries, 0, dir_n_bytes);
    dir_n_bytes = vfat_read_clusters(vfat, (char *) entries, parentcluster, dir_n_bytes);
    my_entry = vfat_find_file_in_dir(entries, basename, 0);
    if (my_entry) { 
      memcpy(my_entry, entry, sizeof(struct fat_dir_entry));
      return vfat_write_clusters_offset(vfat, (char *) entries, parentcluster, dir_n_bytes, 0);
    }
  }
  {
    int offset = 0;
    u32 start_byte = vfat->sector_size * bpb_first_root_dir_sector(&vfat->info);
    /* write the root_dir_entries */
    my_entry = vfat_find_file_in_dir(vfat->root_dir_entries, basename, vfat->n_root_entries);
    if (!my_entry) { putstr("Cannot find file.\r\n"); return -EINVAL; }
    memcpy(my_entry, entry, sizeof(struct fat_dir_entry));
    offset = (my_entry - vfat->root_dir_entries);
    offset = offset - offset % vfat->sector_size;
    return vfat->iohandle->ops->write(vfat->iohandle,
				      (char *) (vfat->root_dir_entries + offset),
				      start_byte + offset, vfat->sector_size);
  }
}

/* 
 * Writes nbytes of data into the named file from the provided buffer. 
 * if 0, assumes same size as file already existing
 */
int vfat_write_file(char *buf, const char *filename, size_t nbytes)
{
  vfat_make_ready();
  if (vfat) {
    u32 first_clusterno;
    int rc;
    struct fat_dir_entry parent_storage;
    struct fat_dir_entry *parent = &parent_storage;
    struct fat_dir_entry entry_storage;
    struct fat_dir_entry *entry = &entry_storage;

    rc = vfat_find_file_entry(vfat, parent, entry, filename);
    if (rc) 
      return rc;
    if (nbytes == 0)
      nbytes = entry->n_bytes;
	first_clusterno = vfat_allocate_clusters(vfat, 
						   fat_entry_first_clusterno(parent), 
						   entry, nbytes);
    entry->n_bytes = vfat_write_clusters_offset(vfat, buf, 
												first_clusterno, nbytes, 0);
    return entry->n_bytes;
  } else {
    return -EINVAL;
  }
}



SUBCOMMAND(vfat, read, command_vfat_read, "<dstaddr> <filename> [bytes]", BB_RUN_FROM_RAM, 2);
void command_vfat_read(int argc, const char **argv)
{
  vfat_make_ready();
  if (vfat) {
    if (argc >= 2) {
      int bytes_read = 
        vfat_read_file((char *) strtoul(argv[0], 0, 0),
                       argv[1],
                       argv[2] ? strtoul(argv[2], 0, 0) : 0);
      putLabeledWord("bytes read=", bytes_read);
    } else putstr("vfat read <dstaddr> <filename> [bytes]\r\n");
  } else putstr("no vfat mounted\r\n");
}

SUBCOMMAND(vfat, write, command_vfat_write, "<srcaddr> <filename> [bytes]", BB_RUN_FROM_RAM, 2);
void command_vfat_write(int argc, const char **argv)
{
  vfat_make_ready();
  if (vfat) {
    if (argc >= 2) {
      int bytes_written =
        vfat_write_file((char *) strtoul(argv[0], 0, 0),
                        argv[1],
                        argv[2] ? strtoul(argv[2], 0, 0) : 0);
      putLabeledWord("bytes written=", bytes_written);
    } else putstr("vfat write_file <srcaddress> <filename> [bytes]\r\n");
  } else putstr("no vfat mounted\r\n");
}

SUBCOMMAND(vfat, set_filesize, command_vfat_set_nbytes, "<file> <bytes> -- resets file size", BB_RUN_FROM_RAM, 2);
void command_vfat_set_nbytes(int argc, const char **argv)
{
  struct fat_dir_entry entry;
  struct fat_dir_entry parent;
  int rc;
  vfat_make_ready();
  if ((rc = vfat_find_file_entry(vfat, &parent, &entry, argv[0])) >= 0) {
    vfat_allocate_clusters(vfat, fat_entry_first_clusterno(&parent), &entry, strtoul(argv[1], 0, 0));
  } else {
	putstr(argv[0]); 
     if (rc == -EISDIR)
	   putstr(" is a directory.\r\n");
	else if (rc == -EINVAL)
	   putstr(" could not be found.\r\n");
	else 
	   putLabeledWord(" error = ", rc);
  }
}

#ifdef CONFIG_VFAT_DEBUG
SUBCOMMAND(vfat, set_next_cluster, command_vfat_set_next_cluster, "<clusterno> <next> -- sets the next cluster #", BB_RUN_FROM_RAM, 2);
void command_vfat_set_next_cluster(int argc, const char **argv)
{
  vfat_make_ready();
  if (strncmp(argv[1], "EOC", 3))
    vfat_set_next_clusterno(vfat, strtoul(argv[0], 0, 0), VFAT_EOC);
  else
    vfat_set_next_clusterno(vfat, strtoul(argv[0], 0, 0), strtoul(argv[1], 0, 0));
}

SUBCOMMAND(vfat, findfree, command_vfat_findfree, "-- returns the cluster # of the first free cluster", BB_RUN_FROM_RAM, 2);
void command_vfat_findfree(int argc, const char **argv)
{
  vfat_make_ready();
  putLabeledWord("clusterno = ", vfat_find_first_free(vfat));
}

SUBCOMMAND(vfat, countfree, command_vfat_countfree, "-- returns the number of free clusters", BB_RUN_FROM_RAM, 2);
void command_vfat_countfree(int argc, const char **argv)
{
  vfat_make_ready();
  putLabeledWord("free clusters = ", vfat_count_free(vfat));
}

SUBCOMMAND(vfat, list_clusters, command_vfat_list_clusters, "<file> -- lists the clusters allocated to a particular file", BB_RUN_FROM_RAM, 2);
void command_vfat_list_clusters(int argc, const char **argv)
{
  u32 clusterno;
  struct fat_dir_entry parent, entry;

  vfat_make_ready();
  if (vfat_find_file_entry(vfat, &parent, &entry, argv[0]) == -EINVAL) {
	putstr("Not found.\r\n");
	return;
  } 
  clusterno = fat_entry_first_clusterno(&entry);
  while (clusterno != VFAT_EOC) {
	putHexInt32(clusterno);
	putstr(" ");
	clusterno = vfat_next_clusterno(vfat, clusterno);
  }
  putstr("\r\n");
}
     
#endif
