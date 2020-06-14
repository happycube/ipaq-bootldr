/* JFFS support for bootldr
 * ^^^^^^^^^^^^^^^^^^^^^^^^
 * Copyright (C) 2000, 2001  John G Dorsey
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * The author may be contacted via electronic mail at <john+@cs.cmu.edu>,
 * or at the following address:
 *
 *   John Dorsey
 *   Carnegie Mellon University
 *   HbH2201 - ICES
 *   5000 Forbes Avenue
 *   Pittsburgh, PA  15213
 *
 *
 * History:
 * ^^^^^^^^
 * 22 November, 2000 - created. (jd)
 *
 * 24 November, 2000 - bootldr.conf support added. (jd)
 *
 * 18 March, 2001 - JFFS2 support added. (jd)
 *
 */

#ifndef CONFIG_ACCEPT_GPL
#error This file covered by GPL but CONFIG_ACCEPT_GPL undefined.
#endif

#include "bootldr.h"
#include "btflash.h"
#include "jffs.h"
#include "crc.h"
#include "serial.h"
#include "heap.h"

#ifdef CONFIG_MD5
# include "md5.h"
#endif

#include <zlib.h>

static int jffs_initialized = 0;

static struct jffs_hash *jffs_table[JFFS_HASH_MODULUS];

static unsigned char *jffs_heap;

static struct jffs_conf_parse jffs_global_keywords[] = {
  JFFS_CONF_PARSE_GLOBAL("default", param_string, deflt, 0),
  JFFS_CONF_PARSE_GLOBAL("read-only", param_flag, access, JFFS_CONF_ACCESS_RO),
  JFFS_CONF_PARSE_GLOBAL("read-write", param_flag, access, JFFS_CONF_ACCESS_RW),
  JFFS_CONF_PARSE_GLOBAL("ramdisk", param_int, ramdisk, 0),
  JFFS_CONF_PARSE_GLOBAL("append", param_string, append, 0),
  JFFS_CONF_PARSE_GLOBAL("root", param_string, root, 0)
};

static struct jffs_conf_parse jffs_image_keywords[] = {
  JFFS_CONF_PARSE_IMAGE("image", param_string, image, 0),
  JFFS_CONF_PARSE_IMAGE("label", param_string, label, 0),
  JFFS_CONF_PARSE_IMAGE("alias", param_string, alias, 0),
  JFFS_CONF_PARSE_IMAGE("read-only", param_flag, access, JFFS_CONF_ACCESS_RO),
  JFFS_CONF_PARSE_IMAGE("read-write", param_flag, access, JFFS_CONF_ACCESS_RW),
  JFFS_CONF_PARSE_IMAGE("ramdisk", param_int, ramdisk, 0),
  JFFS_CONF_PARSE_IMAGE("root", param_string, root, 0),
  JFFS_CONF_PARSE_IMAGE("append", param_string, append, 0),
  JFFS_CONF_PARSE_IMAGE("literal", param_string, literal, 0)
};

static unsigned char *jffs_alloc(unsigned int size);

/* Inode names and data are each padded to 4-byte alignment: */
static inline unsigned int jffs_pad(unsigned int size)
{
  return (sizeof(size) - (size % sizeof(size))) % sizeof(size);
}

/* To which hashtable bucket does this inode belong? */
static inline unsigned int jffs_bucket(unsigned int inumber)
{
  return inumber % JFFS_HASH_MODULUS;
}

/* Irrespective of JFFS version, what's the inumber of this inode? */
static inline unsigned int jffs_inumber(union jffs_generic_inode *inode)
{
  if(inode->v1.magic == JFFS_MAGIC)
    return inode->v1.ino;

  else if(inode->v2.u.magic == JFFS2_MAGIC)
    switch(inode->v2.u.nodetype){
    case JFFS2_NODETYPE_DIRENT | JFFS2_NODE_ACCURATE:
      return inode->v2.d.ino;
    case JFFS2_NODETYPE_INODE | JFFS2_NODE_ACCURATE:
      return inode->v2.i.ino;
    }

  return JFFS_INVALID_INODE;
}

/* Irrespective of JFFS version, what's the inode version of this inode? */
static inline int jffs_inode_version(union jffs_generic_inode *inode)
{
  if(inode->v1.magic == JFFS_MAGIC)
    return inode->v1.version;

  else if(inode->v2.u.magic == JFFS2_MAGIC)
    switch(inode->v2.u.nodetype){
    case JFFS2_NODETYPE_DIRENT | JFFS2_NODE_ACCURATE:
      return JFFS2_DIRENT_VERSION + inode->v2.d.version;
    case JFFS2_NODETYPE_INODE | JFFS2_NODE_ACCURATE:
      return inode->v2.i.version;
    }

  return JFFS_INVALID_VERSION;
}

/* Irrespective of JFFS version, is this inode "accurate"? */
static inline unsigned int jffs_inode_accurate(union jffs_generic_inode
					       *inode)
{
  if(inode->v1.magic == JFFS_MAGIC)
    return inode->v1.accurate;

  else if(inode->v2.u.magic == JFFS2_MAGIC)
    return inode->v2.u.nodetype & JFFS2_NODE_ACCURATE;

  return 0;
}

/* Irrespective of JFFS version, what is the size of the (uncompressed)
 * data in this inode?
 */
static inline unsigned int jffs_inode_dsize(union jffs_generic_inode *inode)
{
  if(inode->v1.magic == JFFS_MAGIC)
    return inode->v1.dsize;

  else if(inode->v2.u.magic == JFFS2_MAGIC &&
	  inode->v2.u.nodetype == JFFS2_NODETYPE_DIRENT | JFFS2_NODE_ACCURATE)
    return inode->v2.i.dsize;
  
  return 0;
}

/* Number of characters remaining in `path' until a path separator
 * or the end of the '\0'-terminated string:
 */
static inline unsigned int jffs_path_component_length(char *path)
{
  char *p;
  unsigned int length;

  for(p = path, length = 0;
      *p != JFFS_PATH_SEPARATOR && *p != '\0';
      ++length, ++p);

  return length;
}

/* Would you want to see this character on your terminal? */
static inline int jffs_isprintable(char c)
{
  return (c > 32) && (c < 127);
}

/* Is this character a delimiter? */
static inline int jffs_isdelim(char c, char *delim)
{
  char *p;

  for(p = delim; *p != '\0'; ++p)
    if(c == *p)
      return 1;

  return 0;
}

/* Number of characters up to (and including) next newline or end of buffer: */
static inline unsigned int jffs_linelength(char *buf, unsigned int length)
{
  char *p;

  for(p = buf; p < (buf + length) && *p != '\n'; ++p);

  if(p < (buf + length))
    ++p;

  return p - buf;
}

/* Index of first occurence of character in buffer, or -1 if not found: */
static inline int jffs_strchr(char *buf, char c, unsigned int length)
{
  char *p;

  for(p = buf; p < (buf + length) && *p != c; ++p);

  return (p < (buf + length)) ? (p - buf) : -1;
}

/* Returns parse structure for given keyword, or NULL if not a keyword: */
static inline struct jffs_conf_parse *
jffs_conf_keyword(char *keyword, unsigned int length,
		  struct jffs_conf_parse *keywords,
		  unsigned int num_keywords)
{
  int i;

  for(i = 0; i < num_keywords; ++i)
    if(length == strlen(keywords[i].keyword) &&
       strncmp(keyword, keywords[i].keyword, length) == 0)
      return &keywords[i];

  return NULL;
}

/* Returns parse structure for global keyword, or NULL if not a keyword: */
static inline struct jffs_conf_parse *
jffs_conf_global_keyword(char *keyword, unsigned int length)
{
  return jffs_conf_keyword(keyword, length, jffs_global_keywords,
			   (sizeof(jffs_global_keywords) / 
			    sizeof(struct jffs_conf_parse)));
}

/* Returns parse structure for global keyword, or NULL if not a keyword: */
static inline struct jffs_conf_parse *
jffs_conf_image_keyword(char *keyword, unsigned int length)
{
  return jffs_conf_keyword(keyword, length, jffs_image_keywords,
			   (sizeof(jffs_image_keywords) / 
			    sizeof(struct jffs_conf_parse)));
}

/* Generic checksum algorithm for JFFS. It's hard to believe that
 * this simple sum provides enough error detection... given that
 * there are fast ways of computing CRC, we should use that.
 */
static inline unsigned int jffs_checksum(unsigned char *buf,
					 unsigned int length)
{
  unsigned int checksum;
  unsigned char *p;

  for(checksum = 0, p = buf; p < (buf + length); checksum += *(p++));

  return checksum;
}

/* Return the checksum for this inode. If the return value is
 * equal to inode->chksum, this inode is valid:
 */
static unsigned int jffs_inode_checksum(struct jffs_inode *inode)
{
  struct jffs_inode check_inode = *inode;

  check_inode.accurate = check_inode.chksum = 0;

  return jffs_checksum((unsigned char *)&check_inode, sizeof(check_inode));
}

/* Return the checksum for the name of this inode. If the return
 * value is equal to inode->nchksum, the name is valid:
 */
static unsigned int jffs_name_checksum(struct jffs_inode *inode)
{
  return jffs_checksum(((unsigned char *)inode) + sizeof(struct jffs_inode),
		       inode->nsize);
}

/* Return the checksum for the data of this inode. If the return
 * value is equal to inode->dchksum, the data is valid:
 */
static unsigned int jffs_data_checksum(struct jffs_inode *inode)
{
  return jffs_checksum(((unsigned char *)inode) + sizeof(struct jffs_inode) +
		       inode->nsize + jffs_pad(inode->nsize),
		       inode->dsize);
}

/* Return the CRC for the common fields in a generic inode. If the
 * return value is zero, this node is valid:
 */
static unsigned int jffs2_inode_hdr_crc(union jffs2_inode *inode)
{
  return crc32_buf((unsigned char *)inode,
		   sizeof(struct jffs2_unknown_node));
}

/* Return the CRC for this directory entry inode. If the return 
 * value is equal to inode->node_crc, this inode is valid:
 */
static unsigned int jffs2_dirent_node_crc(struct jffs2_raw_dirent *inode)
{
  return crc32_buf((unsigned char *)inode,
		   sizeof(struct jffs2_raw_dirent) - 8);
}

/* Return the CRC for the name of this directory entry inode. 
 * If the return value is equal to inode->name_crc, the name is valid:
 */
static unsigned int jffs2_dirent_name_crc(struct jffs2_raw_dirent *inode)
{
  return crc32_buf(((unsigned char *)inode) + sizeof(struct jffs2_raw_dirent),
		   inode->nsize);
}

/* Return the CRC for this inode. If the return value is equal
 * to inode->node_crc, this inode is valid:
 */
static unsigned int jffs2_inode_node_crc(struct jffs2_raw_inode *inode)
{
  return crc32_buf((unsigned char *)inode,
		   sizeof(struct jffs2_raw_inode) - 8);
}

/* Return the CRC for the data of this inode. If the return value
 * is equal to inode->data_crc, the data is valid:
 */
static unsigned int jffs2_inode_data_crc(struct jffs2_raw_inode *inode)
{
  return crc32_buf(((unsigned char *)inode) + sizeof(struct jffs2_raw_inode),
		   inode->csize);
}

/* Dump the contents of `inode' to the terminal: */
static void jffs_print_inode(struct jffs_inode *inode)
{
  int i;

  putstr("JFFS inode "); putHexInt32(inode->ino);
  putstr(" at address "); putHexInt32((unsigned int)inode); putstr("\r\n");
  putLabeledWord("    magic: ", inode->magic);
  putLabeledWord("     pino: ", inode->pino);
  putLabeledWord("  version: ", inode->version);
  putLabeledWord("     mode: ", inode->mode);
  putLabeledWord("      uid: ", inode->uid);
  putLabeledWord("      gid: ", inode->gid);
  putLabeledWord("    atime: ", inode->atime);
  putLabeledWord("    mtime: ", inode->mtime);
  putLabeledWord("    ctime: ", inode->ctime);
  putLabeledWord("   offset: ", inode->offset);
  putLabeledWord("    dsize: ", inode->dsize);
  putLabeledWord("    rsize: ", inode->rsize);
  putLabeledWord("    nsize: ", inode->nsize);
  putLabeledWord("    nlink: ", inode->nlink);
  putLabeledWord("   rename: ", inode->rename);
  putLabeledWord("  deleted: ", inode->deleted);
  putLabeledWord(" accurate: ", inode->accurate);
  putLabeledWord("  dchksum: ", inode->dchksum);
  putLabeledWord("  nchksum: ", inode->nchksum);
  putLabeledWord("   chksum: ", inode->chksum);

  if(inode->nsize > 0){

    if(inode->nchksum != jffs_name_checksum(inode))
      putstr("     name: <BAD CHECKSUM>\r\n");
    else {
      putstr("     name: \""); 
      for(i = 0; i < inode->nsize; ++i)
	putc(((char *)(inode + 1))[i]);
      putstr("\"\r\n");
    }

  }

  if(inode->dsize > 0){
    
    putstr("     data: ");
    putstr((inode->dchksum != jffs_data_checksum(inode)) ? 
	   "<BAD CHECKSUM>\r\n" : "checksum OK\r\n");

  }

}

static void jffs2_print_dirent(struct jffs2_raw_dirent *inode)
{
  int i;

  putstr("JFFS2 dirent inode "); putHexInt32(inode->ino);
  putstr(" at address "); putHexInt32((unsigned int)inode); putstr("\r\n");
  putLabeledWord("    magic: ", inode->magic);
  putLabeledWord(" nodetype: ", inode->nodetype);
  putLabeledWord("   totlen: ", inode->totlen);
  putstr("  hdr_crc: ");
  putstr((jffs2_inode_hdr_crc((union jffs2_inode *)inode) != 0) ?
	 "<BAD CRC>\r\n" : "CRC OK\r\n");
  putLabeledWord("     pino: ", inode->pino);
  putLabeledWord("  version: ", inode->version);
  putLabeledWord("   mctime: ", inode->mctime);
  putLabeledWord("    nsize: ", inode->nsize);
  putLabeledWord("     type: ", inode->type);
  putLabeledWord(" node_crc: ", inode->node_crc);
  putLabeledWord(" name_crc: ", inode->name_crc);

  if(inode->nsize > 0){

    if(inode->name_crc != jffs2_dirent_name_crc(inode))
      putstr("     name: <BAD CRC>\r\n");
    else {
      putstr("     name: \"");
      for(i = 0; i < inode->nsize; ++i)
	putc(((char *)(inode + 1))[i]);
      putstr("\"\r\n");
    }

  }

}

static void jffs2_print_inode(struct jffs2_raw_inode *inode)
{
  putstr("JFFS2 inode "); putHexInt32(inode->ino);
  putstr(" at address "); putHexInt32((unsigned int)inode); putstr("\r\n");
  putLabeledWord("    magic: ", inode->magic);
  putLabeledWord(" nodetype: ", inode->nodetype);
  putLabeledWord("   totlen: ", inode->totlen);
  putstr("  hdr_crc: ");
  putstr((jffs2_inode_hdr_crc((union jffs2_inode *)inode) != 0) ?
	 "<BAD CRC>\r\n" : "CRC OK\r\n");
  putLabeledWord("  version: ", inode->version);
  putLabeledWord("     mode: ", inode->mode);
  putLabeledWord("      uid: ", inode->uid);
  putLabeledWord("      gid: ", inode->gid);
  putLabeledWord("    isize: ", inode->isize);
  putLabeledWord("    atime: ", inode->atime);
  putLabeledWord("    mtime: ", inode->mtime);
  putLabeledWord("    ctime: ", inode->ctime);
  putLabeledWord("   offset: ", inode->offset);
  putLabeledWord("    csize: ", inode->csize);
  putLabeledWord("    dsize: ", inode->dsize);
  putLabeledWord("    compr: ", inode->compr);
  putLabeledWord(" data_crc: ", inode->data_crc);
  putLabeledWord(" node_crc: ", inode->node_crc);

  if(inode->csize > 0){

    putstr("     data: ");
    putstr((inode->data_crc != jffs2_inode_data_crc(inode)) ?
	   "<BAD CRC>\r\n" : "CRC OK\r\n");

  }

}

/* Run through the entire JFFS filesystem, printing out inodes that
 * match `inumber' as we find them. This does not require that the
 * filesystem log have been already replayed.
 */
void jffs_dump_inode(unsigned int inumber)
{
  struct FlashRegion *region;
  union jffs_generic_inode *inode;
  unsigned int start, stop;

  if((region = btflash_get_partition("jffs")) == NULL){
    putstr("No flash partition configured; cannot examine JFFS\r\n");
    return;
  }

  if(crc32_init() < 0){
    putstr("Unable to generate CRC32 lookup table\r\n");
    return;
  }

  start = region->base;
  stop = region->base + region->size - sizeof(union jffs_generic_inode);

  for(inode = (union jffs_generic_inode *)start; 
      inode < (union jffs_generic_inode *)stop;){

    while(inode->v1.magic != JFFS_MAGIC && 
	  inode->v2.u.magic != JFFS2_MAGIC &&
	  inode < (union jffs_generic_inode *)stop)
      ++((unsigned int *)inode);

    if(inode->v1.magic == JFFS_MAGIC){

      if(inode->v1.chksum != jffs_inode_checksum(&inode->v1)){
	
	putstr("inode "); putHexInt32(inode->v1.ino);
	putstr(" at "); putHexInt32((unsigned int)inode);
	putstr(" with inode checksum "); putHexInt32(inode->v1.chksum);
	putstr(" is invalid\r\n");
	
	break;
	
      }
      
      if(inode->v1.ino == inumber)
	jffs_print_inode(&inode->v1);

      inode = (union jffs_generic_inode *)
	(((unsigned char *)inode) + sizeof(struct jffs_inode) +
	 inode->v1.nsize + jffs_pad(inode->v1.nsize) +
	 inode->v1.dsize + jffs_pad(inode->v1.dsize));

      continue;

    } else if(inode->v2.u.magic == JFFS2_MAGIC){

      /* Silently skip over inodes with an invalid header CRC, since we
       * can't make meaningful statements about them anyway:
       */
      if(jffs2_inode_hdr_crc(&inode->v2) == 0)
	switch(inode->v2.u.nodetype){
	case JFFS2_NODETYPE_DIRENT:
	  
	  if(inode->v2.d.node_crc != jffs2_dirent_node_crc(&inode->v2.d)){
	    
	    putstr("dirent inode "); putHexInt32(inode->v2.d.ino);
	    putstr(" at "); putHexInt32((unsigned int)inode);
	    putstr(" with inode CRC "); putHexInt32(inode->v2.d.node_crc);
	    putstr(" is invalid\r\n");
	    
	    break;
	    
	  }
	  
	  if(inode->v2.d.ino == inumber)
	    jffs2_print_dirent(&inode->v2.d);
	  
	  inode = (union jffs_generic_inode *)
	    (((unsigned char *)inode) + sizeof(struct jffs2_raw_dirent) +
	     inode->v2.d.nsize + jffs_pad(inode->v2.d.nsize));
	  
	  continue;
	  
	case JFFS2_NODETYPE_INODE:
	  
	  if(inode->v2.i.node_crc != jffs2_inode_node_crc(&inode->v2.i)){
	    
	    putstr("inode "); putHexInt32(inode->v2.i.ino);
	    putstr(" at "); putHexInt32((unsigned int)inode);
	    putstr(" with inode CRC "); putHexInt32(inode->v2.i.node_crc);
	    putstr(" is invalid\r\n");
	    
	    break;
	    
	  }
	  
	  if(inode->v2.i.ino == inumber)
	    jffs2_print_inode(&inode->v2.i);
	  
	  inode = (union jffs_generic_inode *)
	    (((unsigned char *)inode) + sizeof(struct jffs2_raw_inode) +
	     inode->v2.i.csize + jffs_pad(inode->v2.i.csize));
	  
	  continue;
	  
	}
	
    }

    /* Make progress past an unusable inode: */
    ++(unsigned int *)inode;

  }
}

void jffs_statistics(void)
{
  struct FlashRegion *region;
  union jffs_generic_inode *inode;
  unsigned int start, stop, unit;
  int jffs_valid_inodes = 0, jffs_invalid_inodes = 0;
  int jffs2_valid_dirents = 0, jffs2_invalid_dirents = 0;
  int jffs2_valid_inodes = 0, jffs2_invalid_inodes = 0;
  int i, j, histogram[64], height;

  if((region = btflash_get_partition("jffs")) == NULL){
    putstr("No flash partition configured; cannot examine JFFS\r\n");
    return;
  }

  if(crc32_init() < 0){
    putstr("Unable to generate CRC32 lookup table\r\n");
    return;
  }

  memset(histogram, 0, sizeof(histogram));

  start = region->base;
  stop = region->base + region->size - sizeof(union jffs_generic_inode);

  unit = region->size / 64;

  for(inode = (union jffs_generic_inode *)start; 
      inode < (union jffs_generic_inode *)stop;){

    while(inode->v1.magic != JFFS_MAGIC && 
	  inode->v2.u.magic != JFFS2_MAGIC &&
	  inode < (union jffs_generic_inode *)stop)
      ++((unsigned int *)inode);

    if(inode->v1.magic == JFFS_MAGIC){

      if(inode->v1.chksum != jffs_inode_checksum(&inode->v1)){
	
	++jffs_invalid_inodes;
	
	break;
	
      }

      ++jffs_valid_inodes;
      ++histogram[((unsigned int)inode - start) / unit];
      
      inode = (union jffs_generic_inode *)
	(((unsigned char *)inode) + sizeof(struct jffs_inode) +
	 inode->v1.nsize + jffs_pad(inode->v1.nsize) +
	 inode->v1.dsize + jffs_pad(inode->v1.dsize));

      continue;

    } else if(inode->v2.u.magic == JFFS2_MAGIC){

      if(jffs2_inode_hdr_crc(&inode->v2) == 0)
	switch(inode->v2.u.nodetype){
	case JFFS2_NODETYPE_DIRENT:
	  
	  if(inode->v2.d.node_crc != jffs2_dirent_node_crc(&inode->v2.d)){
	    
	    ++jffs2_invalid_dirents;
	    
	    break;
	    
	  }
	  
	  ++jffs2_valid_dirents;
	  ++histogram[((unsigned int)inode - start) / unit];
	  
	  inode = (union jffs_generic_inode *)
	    (((unsigned char *)inode) + sizeof(struct jffs2_raw_dirent) +
	     inode->v2.d.nsize + jffs_pad(inode->v2.d.nsize));
	  
	  continue;
	  
	case JFFS2_NODETYPE_DIRENT & ~JFFS2_NODE_ACCURATE:
	  
	  ++jffs2_invalid_dirents;
	  
	  break;
	  
	case JFFS2_NODETYPE_INODE:
	  
	  if(inode->v2.i.node_crc != jffs2_inode_node_crc(&inode->v2.i)){
	    
	    ++jffs2_invalid_inodes;
	    
	    break;
	    
	  }
	  
	  ++jffs2_valid_inodes;
	  ++histogram[((unsigned int)inode - start) / unit];
	  
	  inode = (union jffs_generic_inode *)
	    (((unsigned char *)inode) + sizeof(struct jffs2_raw_inode) +
	     inode->v2.i.csize + jffs_pad(inode->v2.i.csize));
	  
	  continue;
	  
	case JFFS2_NODETYPE_INODE & ~JFFS2_NODE_ACCURATE:
	  
	  ++jffs2_invalid_inodes;
	  
	  break;
	  
	}

    }

    /* Make progress past an unusable inode: */
    ++(unsigned int *)inode;

  }

  for(j = 0, height = 0; j < JFFS_HISTOGRAM_WIDTH; ++j)
    if(histogram[j] > height)
      height = histogram[j];

  putstr("  inode distribution on "); putDecInt(region->size);
  putstr("-byte partition:\r\n\r\n");

  putDecIntWidth(height, 5);
  putstr(" _");
  for(j = 0; j < JFFS_HISTOGRAM_WIDTH; ++j)
    putc(' ');
  putstr("_\r\n");

  for(i = JFFS_HISTOGRAM_HEIGHT - 1; i >= 0; --i){

    putDecIntWidth((i * height) / JFFS_HISTOGRAM_HEIGHT, 5);

    putstr("  ");

    for(j = 0; j < JFFS_HISTOGRAM_WIDTH; ++j)
      putc((histogram[j] > (i * height) / JFFS_HISTOGRAM_HEIGHT) ? '#' : ' ');

    putstr("\r\n");

  }

  putstr("       ");
  for(j = 0; j < JFFS_HISTOGRAM_WIDTH; ++j)
    putc('=');
  putstr("\r\n       ");

  putHexInt32(start);
  for(j = 0; j < JFFS_HISTOGRAM_WIDTH - 16; ++j)
    putc(' ');
  putHexInt32(start + region->size);
  putstr("\r\n\r\n");

  putstr("  JFFS  inodes: ");
  putDecInt(jffs_valid_inodes);
  putstr(" valid, ");
  putDecInt(jffs_invalid_inodes);
  putstr(" invalid\r\n");

  putstr("  JFFS2 dirent: ");
  putDecInt(jffs2_valid_dirents);
  putstr(" valid, ");
  putDecInt(jffs2_invalid_dirents);
  putstr(" invalid\r\n");

  putstr("  JFFS2 inodes: ");
  putDecInt(jffs2_valid_inodes);
  putstr(" valid, ");
  putDecInt(jffs2_invalid_inodes);
  putstr(" invalid\r\n");

}

/* Linear allocator; cannot reclaim allocated memory. If we could know
 * that we'll only be replaying the filesystem metadata and at most a
 * few small files, we could just use the existing bootldr allocator.
 * I'm worried that someone will try to replay a ramdisk image or 
 * something similarly large, hence we use our own fenced-off region:
 */
static unsigned char *jffs_alloc(unsigned int size)
{
  unsigned char *ptr;

  if((jffs_heap + size + jffs_pad(size)) > (unsigned char *)JFFS_REPLAY_LIMIT)
    return NULL;

  ptr = jffs_heap;
  jffs_heap += size + jffs_pad(size); /* always return word-aligned regions */

  return ptr;
}

/* Returns an already-inserted hash entry if one exists, otherwise
 * allocates and initializes a new entry. Returns NULL if out of memory.
 */
static struct jffs_hash *jffs_make_hash(union jffs_generic_inode *inode)
{
  struct jffs_hash *hash, *newhash;
  unsigned int inumber;

  if((inumber = jffs_inumber(inode)) == JFFS_INVALID_INODE){
    putstr("invalid inode\r\n");
    return NULL;
  }

  hash = jffs_table[jffs_bucket(inumber)];
  
  if(hash == NULL){

    if((newhash = 
	(struct jffs_hash *)jffs_alloc(sizeof(struct jffs_hash))) == NULL)
      return NULL;
    
    memset(newhash, 0, sizeof(struct jffs_hash));

    newhash->inumber = inumber;
    newhash->next = NULL;
    newhash->log = NULL;
    
    hash = jffs_table[jffs_bucket(inumber)] = newhash;

  } else {
    
    while(hash->inumber != inumber && hash->next != NULL)
      hash = hash->next;

    if(hash->inumber != inumber){
      
      if((newhash = 
	  (struct jffs_hash *)jffs_alloc(sizeof(struct jffs_hash))) == NULL)
	return NULL;
    
      memset(newhash, 0, sizeof(struct jffs_hash));

      newhash->inumber = inumber;
      newhash->next = NULL;
      newhash->log = NULL;
      
      hash = hash->next = newhash;      

    }

  }

  return hash;
}

/* Returns an already inserted hash entry, or NULL if no match was found: */
static struct jffs_hash *jffs_get_hash(unsigned int inumber)
{
  struct jffs_hash *hash;

  hash = jffs_table[jffs_bucket(inumber)];

  if(hash == NULL)
    return NULL;

  while(hash->inumber != inumber && hash->next != NULL)
    hash = hash->next;

  if(hash->inumber != inumber)
    return NULL;

  return hash;
}

/* Allocates a new log entry for `inode' and links it (in version order)
 * with the replay list for `hash'. Returns -1 on allocation error or
 * if a version number inconsistency was found, 0 otherwise.
 */
static int jffs_add_log(struct jffs_hash *hash, 
			union jffs_generic_inode *inode)
{
  struct jffs_log *log, *newlog;
  int version;

  if((version = jffs_inode_version(inode)) == JFFS_INVALID_VERSION){
    putLabeledWord("invalid version for inode ", jffs_inumber(inode));
    putLabeledWord("nodetype is ", inode->v2.u.nodetype);
    putLabeledWord("version is ", version);
    return -1;
  }

  if((newlog = (struct jffs_log *)jffs_alloc(sizeof(struct jffs_log))) == NULL)
    return -1;

  newlog->inode = inode;

  log = hash->log;

  if(log == NULL){

    newlog->next = NULL;
    hash->log = newlog;

  } else if(version < jffs_inode_version(log->inode)){

    newlog->next = log;
    hash->log = newlog;

  } else if(version == jffs_inode_version(log->inode)){
      
    putstr("JFFS inconsistency: duplicate version ");
    putHexInt32(version);
    putLabeledWord(" for inode ", jffs_inumber(inode));
    
    return -1;

  } else {  /* adding version which is higher than that at start of list */

    /* Dumb linear search, but how long are these lists likely to be? */
    for(log = hash->log;
	log->next != NULL && version > jffs_inode_version(log->next->inode);
	log = log->next);

    if(version == jffs_inode_version(log->inode)){
      
      putstr("JFFS inconsistency: duplicate version ");
      putHexInt32(version);
      putLabeledWord(" for inode ", jffs_inumber(inode));
      
      return -1;

    }

    if(log->next == NULL)
      newlog->next = NULL;
    else 
      newlog->next = log->next;

    log->next = newlog;

  }

  return 0;
}

/* JFFS2 filesystems may not include an explicit root inode. During
 * replay, if we finish adding all of the logs and don't have a root
 * inode, we can just manufacture one.
 */
static int jffs_add_root_inode(void)
{
  union jffs_generic_inode *root;
  struct jffs_hash *hash;

  if((root = (union jffs_generic_inode *)
      jffs_alloc(sizeof(union jffs_generic_inode))) == NULL)
    return -1;

  memset(root, 0, sizeof(union jffs_generic_inode));

  root->v2.d.magic = JFFS2_MAGIC;
  root->v2.d.nodetype = JFFS2_NODETYPE_DIRENT;
  root->v2.d.totlen = sizeof(struct jffs2_raw_inode);
  root->v2.d.hdr_crc = 0;
  root->v2.d.hdr_crc = jffs2_inode_hdr_crc(&root->v2); /* unsure about this */
  root->v2.d.pino = JFFS_ROOT_INODE;
  root->v2.d.ino = JFFS_ROOT_INODE;
  root->v2.d.type = JFFS2_TYPE_DIR;
  root->v2.d.node_crc = jffs2_dirent_node_crc(&root->v2.d);
  
  if((hash = jffs_make_hash(root)) == NULL){
    putstr("Unable to replay JFFS log: could not get root inode hash\r\n");
    return -1;
  }

  if(jffs_add_log(hash, root) < 0){
    putstr("Unable to replay JFFS log: could not add root inode log to hash\r\n");
    return -1;
  }

  return 0;
}

/* Increment the child count for the inode with inumber `parent'.
 * Used in the first pass of two to recreate the directory hierarchy;
 * once we know how many children a node has, we can later allocate
 * exactly enough space for its child inode pointers:
 */
static int jffs_add_child_count(unsigned int parent)
{
  int i;
  struct jffs_hash *hash;
  
  for(hash = jffs_table[jffs_bucket(parent)]; 
      hash != NULL;
      hash = hash->next){

    if(hash->inumber != parent)
      continue;

    ++hash->nchildren;
    
    return 0;
  }
  
  return -1;
}

/* "RTIME" and "DYNRUBIN" decompression support appears here as a nearly
 * direct copy of the implementations from compr_rtime.c and compr_rubin.c
 * in the JFFS2 tree for Linux. The original code is (C) 2001 Red Hat, 
 * Inc., was authored by Arjan van de Ven <arjanv@redhat.com>, and was
 * released under GPLv2 (or later).
 */
static int jffs2_rtime_decompress(unsigned char *dest, unsigned char *src,
				  unsigned int length,
				  struct jffs2_raw_inode *inode)
{
  int positions[256], outpos, pos, backoffs, repeat;
  unsigned char value;

  memset(positions, 0, sizeof(positions)); 
  
  for(outpos = 0, pos = 0; outpos < length;){
    
    value = src[pos++];
    dest[outpos++] = value;
    repeat = src[pos++];
    backoffs = positions[value];
    positions[value] = outpos;

    if(repeat){

      if(backoffs + repeat >= outpos){

	while(repeat){

	  dest[outpos++] = dest[backoffs++];
	  --repeat;

	}

      } else {

	memcpy(&dest[outpos], &dest[backoffs], repeat);
	outpos += repeat;

      }

    }

  }

  return 0;
}

static inline int jffs2_dynrubin_pullbit(struct pushpull *pp)
{
  int bit;

  bit = (pp->buf[pp->ofs >> 3] >> (7 - (pp->ofs & 7))) & 1;

  ++pp->ofs;

  return bit;
}

static int jffs2_dynrubin_decode(struct rubin_state *rs, long A, long B)
{
  char c;
  long i0, i1, threshold;
  int symbol;
  
  while(rs->q >= UPPER_BIT_RUBIN || (rs->p + rs->q) <= UPPER_BIT_RUBIN){

    c = jffs2_dynrubin_pullbit(&rs->pp);

    ++rs->bit_number;
    rs->q &= LOWER_BITS_RUBIN;
    rs->q <<= 1;
    rs->p <<= 1;
    rs->rec_q &= LOWER_BITS_RUBIN;
    rs->rec_q <<= 1;
    rs->rec_q += c;

  }

  i0 = A * rs->p / (A + B);

  if(i0 <= 0)
    i0 = 1;

  if(i0 >= rs->p)
    i0 = rs->p - 1;

  i1 = rs->p - i0;
  
  threshold = rs->q + i0;

  if(rs->rec_q < threshold){

    symbol = 0;

    rs->p = i0;

  } else {

    symbol = 1;

    rs->p = i1;
    rs->q += i0;

  }
  
  return symbol;
}

static int jffs2_dynrubin_inb(struct rubin_state *rs)
{
  int i, result;

  for (i=0, result = 0; i < 8; ++i)
    result |= jffs2_dynrubin_decode(rs, rs->bit_divider-rs->bits[i],
				    rs->bits[i]) << i;

  return result;
}

static int jffs2_dynrubin_decompress(unsigned char *dest, unsigned char *src,
				     unsigned int length,
				     struct jffs2_raw_inode *inode)
{
  int i;
  struct rubin_state rs;
  
  rs.pp.buf = src + 8;
  rs.pp.buflen = inode->csize - 8;
  rs.pp.ofs = 0;
  rs.pp.reserve = 0;
  rs.rec_q = 0;
  rs.q = 0;
  rs.p = 2 * UPPER_BIT_RUBIN;
  rs.bit_divider = 256;

  for(i = 0; i < 8; ++i)
    rs.bits[i] = src[i];

  for(rs.bit_number = 0; rs.bit_number < RUBIN_REG_SIZE; ++rs.bit_number)
    rs.rec_q = (rs.rec_q << 1) + jffs2_dynrubin_pullbit(&rs.pp);

  for(i = 0; i < length; ++i)
    dest[i] = jffs2_dynrubin_inb(&rs);

  return 0;
}

static void *jffs2_zlib_alloc(void *opaque, unsigned int items,
			      unsigned int size)
{
  return jffs_alloc(items * size);  /* Z_NULL == NULL */
}

static void jffs2_zlib_free(void *opaque, void *address)
{
  return;  /* can't free JFFS allocations */
}

/* calloc() to make zlib happy: */
void *calloc(unsigned int items, unsigned int size)
{
  char *ptr;

  if((ptr = jffs_alloc(items * size)) == NULL)
    return NULL;

  memset(ptr, 0, items * size);

  return ptr;
}

#ifndef CONFIG_LCD
/* LCD support uses zlib. The free() in heap.c is harmless if passed
 * a pointer not allocated by mmalloc().
 */
void free(void *ptr){}
#endif

static int jffs2_zlib_decompress(unsigned char *dest, unsigned char *src,
				 unsigned int length,
				 struct jffs2_raw_inode *inode)
{
  z_stream stream;
  int status;

  stream.zalloc = jffs2_zlib_alloc;
  stream.zfree  = jffs2_zlib_free;
  stream.next_in = src;
  stream.avail_in = inode->csize;
  stream.total_in = 0;
  stream.next_out = dest;
  stream.avail_out = length;
  stream.total_out = 0;

  if((status = inflateInit(&stream)) != Z_OK){
    putLabeledWord("error initializing zlib stream: ", status);
    return -1;
  }

  while((status = inflate(&stream, Z_FINISH)) == Z_OK);
  
  if(status != Z_STREAM_END){
    putLabeledWord("error inflating zlib stream: ", status);
    return -1;
  }

  inflateEnd(&stream);

  return 0;
}

/* Perform any manipulation necessary to place the data from `inode'
 * into the buffer which begins at `start', commencing at `offset'
 * bytes into the buffer. Do not exceed the maximum size of the buffer,
 * given by `size'.
 */
static int jffs2_inode_decompress(unsigned char *start,
				  unsigned int offset,
				  unsigned int size,
				  struct jffs2_raw_inode *inode)
{
  unsigned char *dest, *src;
  unsigned int length;

  /* If an inode later in the log history will eventually truncate us
   * to something smaller than the current offset, don't bother doing
   * any work.
   */
  if(offset >= size)
    return 0;

  dest = start + offset;
  src = (unsigned char *)(inode + 1);
  length = (size < (inode->offset + inode->dsize)) ? size - inode->offset :
    inode->dsize;

  switch(inode->compr){
  case JFFS2_COMPR_NONE:
    memcpy(dest, src, length);	   
    break;

  case JFFS2_COMPR_ZERO:
    putstr("compression type ZERO not implemented\r\n");
    return -1;

  case JFFS2_COMPR_RTIME:
    return jffs2_rtime_decompress(dest, src, length, inode);

  case JFFS2_COMPR_RUBINMIPS:
    putstr("compression type RUBINMIPS not implemented\r\n");
    return -1;

  case JFFS2_COMPR_COPY:
    putstr("compression type COPY not implemented\r\n");
    return -1;

  case JFFS2_COMPR_DYNRUBIN:
    return jffs2_dynrubin_decompress(dest, src, length, inode);

  case JFFS2_COMPR_ZLIB:
    return jffs2_zlib_decompress(dest, src, length, inode);

  default:
    putLabeledWord("unrecognized inode data compression type ", inode->compr);
    return -1;
  }

  return 0;
}

/* Assumes that all logs have been properly added to the hashtable.
 * Performs some consistency checks on the journal, determines final
 * file metadata, and updates parent counters as the first part of
 * a two-part method to recreate the directory hierarchy:
 */
static int jffs_replay(void)
{
  int i;
  struct jffs_hash *hash;
  struct jffs_log *log;
  struct jffs_name *name, **namep;
  unsigned int version, length;

  for(i=0; i<JFFS_HASH_MODULUS; ++i){

    for(hash = jffs_table[i]; hash != NULL; hash = hash->next){

      /* First pass through the log list to determine journal integrity,
       * final file size, and deletion status:
       */
      for(log = hash->log, version = 1, length = 0;
	  log != NULL; 
	  version = jffs_inode_version(log->inode), log = log->next){

	if(jffs_inode_version(log->inode) < version){
	  putLabeledWord("Inconsistent log sequence for inode ", 
			 hash->inumber);
	  return -1;
	}

	/* Want to find the final data size for this file. For version 1,
	 * we have to run through the logs. For version 2, we can just check
	 * the isize of the last inode in the sequence (but since the last
	 * log entry may be a dirent rather than an inode, we still have
	 * to traverse the logs).
	 */
	if(log->inode->v1.magic == JFFS_MAGIC){

	  if(!log->inode->v1.accurate)
	    continue;
	  
	  if(log->inode->v1.dsize + log->inode->v1.offset > length)
	    length = log->inode->v1.dsize + log->inode->v1.offset;
	  
	  /* Nonzero dsize and rsize indicates a swap; no net change in size: */
	  if(log->inode->v1.dsize == 0)
	    length -= log->inode->v1.rsize;
	  
	  /* Assume that files don't "come back to life" after being deleted: */
	  if(log->inode->v1.deleted)
	    hash->dsize = -1;

	  /* For these fields, only the most recent version counts: */
	  if(log->next == NULL){
	    
	    hash->mtime = log->inode->v1.mtime;
	    hash->mode = log->inode->v1.mode;
	    
	    if(log->inode->v1.nchksum != jffs_name_checksum(&log->inode->v1)){
	      
	      putstr("inode "); putHexInt32(hash->inumber);
	      putstr(" version "); putHexInt32(log->inode->v1.version);
	      putstr(" with name checksum "); 
	      putHexInt32(log->inode->v1.nchksum);
	      putstr(" is invalid\r\n");
	      
	      /* Don't add file: */
	      hash->dsize = -1;
	      
	    } else {
	      
	      for(namep = &hash->names; *namep != NULL; 
		  namep = &(*namep)->next);
	      
	      if((*namep = (struct jffs_name *)
		  jffs_alloc(sizeof(struct jffs_name))) == NULL)
		return -1;

	      (*namep)->parent = log->inode->v1.pino;
	      (*namep)->nsize = log->inode->v1.nsize;
	      (*namep)->name = (char *)((&log->inode->v1) + 1); /* in flash */
	      (*namep)->next = NULL;
	      
	    }
	  
	  }

	} else if(log->inode->v2.u.magic == JFFS2_MAGIC){

	  /* Again, it's really only the last one that counts, but since
	   * the metadata is distributed over dirents and inodes, we'd
	   * have to actually search to find when the last of each occur
	   * in the log list.
	   */

	  switch(log->inode->v2.u.nodetype){
	  case JFFS2_NODETYPE_DIRENT:

	    for(namep = &hash->names; *namep != NULL; namep = &(*namep)->next);

	    if((*namep = (struct jffs_name *)
		jffs_alloc(sizeof(struct jffs_name))) == NULL)
	      return -1;

	    (*namep)->parent = log->inode->v2.d.pino;
	    (*namep)->nsize = log->inode->v2.d.nsize;
	    (*namep)->name = (char *)((&log->inode->v2.d) + 1); /* in flash */
	    (*namep)->next = NULL;

	    ++hash->nlink;
	    hash->mtime = log->inode->v2.d.mctime;

	    break;

	  case JFFS2_NODETYPE_INODE:

	    hash->mode = log->inode->v2.i.mode;
	    length = log->inode->v2.i.isize;
	    
	    break;

	  default:
	    putstr("Invalid node type "); 
	    putHexInt32(log->inode->v2.u.nodetype);
	    putLabeledWord(" for inode ", hash->inumber);
	    return -1;
	  }

	} else {
	  putLabeledWord("Invalid magic number for inode ", hash->inumber);
	  return -1;
	}

      }  /* determine log consistency, final size, deletion status */

      if(hash->dsize == -1)
	continue;
      
      hash->dsize = length;

      /* The test for pino == INVALID catches orphan inodes which have
       * no valid dirent; we don't add these to the user's view of the
       * filesystem.
       */
      if(hash->inumber != JFFS_ROOT_INODE)
	for(name = hash->names; name != NULL; name = name->next)
	  if(name->parent != JFFS_INVALID_INODE &&
	     jffs_add_child_count(name->parent) < 0){
	    putstr("Unable to add inode "); putHexInt32(hash->inumber);
	    putstr(" to parent "); putHexInt32(name->parent);
	    putstr(" child count\r\n");
	    return -1;
	  }

    }  /* traverse linear chain on i'th hash bucket */

  }  /* loop over each hash bucket */

  return 0;
}

/* Assumes that enough space has been allocated for the directory
 * inode indicated by `parent' to contain pointers for all its children.
 * Assumes that free child pointers are set to NULL. Adds a pointer
 * to `child' (having name `name') in the directory list for `parent':
 */
static int jffs_add_child(unsigned int parent, struct jffs_hash *child,
			  struct jffs_name *name)
{
  int i, j;
  struct jffs_hash *hash;
  
  for(hash = jffs_table[jffs_bucket(parent)]; 
      hash != NULL;
      hash = hash->next){

    if(hash->inumber != parent)
      continue;

    for(j = 0; j < hash->nchildren; ++j){

      if(hash->children[j].child == NULL && hash->children[j].name == NULL){
	hash->children[j].child = child;
	hash->children[j].name = name;
	break;
      }

    }

    if(j == hash->nchildren){
      putstr("child array (size "); putHexInt32(hash->nchildren);
      putLabeledWord(") is full for inode ", hash->inumber);
      return -1;
    }
    
    return 0;
  }
  
  return -1;
}

/* Allocates and initializes the child pointer lists for each entry in
 * the hashtable with nonzero child count. Assumes that the child counts
 * have been set up by a previous call to jffs_replay().
 */
static int jffs_build_directories(void)
{
  int i;
  struct jffs_hash *hash;
  struct jffs_name *name;
  
  for(i=0; i<JFFS_HASH_MODULUS; ++i){

    for(hash = jffs_table[i]; hash != NULL; hash = hash->next){

      if(hash->dsize == -1)  /* deleted */
	continue;

      if(hash->nchildren > 0){

	if((hash->children = 
	    (struct jffs_child *)
	    jffs_alloc(hash->nchildren * sizeof(struct jffs_child))) == NULL)
	  return -1;
	
	memset(hash->children, 0, 
	       hash->nchildren * sizeof(struct jffs_child));

      }

    }

  }

  /* Child inode pointers are allocated. Now walk through every file
   * in the filesystem and fill in the pointers.
   */

  for(i=0; i<JFFS_HASH_MODULUS; ++i){

    for(hash = jffs_table[i]; hash != NULL; hash = hash->next){

      if(hash->dsize == -1)  /* deleted */
	continue;

      /* See jffs_replay(); we don't bother to add an orphan if it
       * has no dirent (i.e., if our replay thinks the parent inode
       * is zero).
       */
      if(hash->inumber != JFFS_ROOT_INODE)
	for(name = hash->names; name != NULL; name = name->next)
	  if(name->parent != JFFS_INVALID_INODE &&
	     jffs_add_child(name->parent, hash, name) < 0){
	    putstr("Unable to add inode "); putHexInt32(hash->inumber);
	    putLabeledWord(" to parent ", name->parent);
	    return -1;
	  }

    }

  }  

  return 0;
}

/* Initializes a JFFS filesystem by reading its raw inodes from the 
 * flash device, inserting them into a hashtable. Determines final
 * inode metadata after replaying the logs, and recreates the final
 * directory hierarchy. Does not cause any file _data_ to be replayed.
 *
 * If jffs_init() has already been called, this routine returns 0
 * without doing any work. This behavior can be overridden by passing
 * a nonzero argument `force', which completely resets the JFFS
 * replay memory allocator, hashtable, and other data. A re-init 
 * should be forced any time the JFFS data on flash changes, such as
 * after downloading a new filesystem image.
 *
 * Returns 0 on success, -1 on any error.
 */
int jffs_init(int force)
{
  struct FlashRegion *region;
  union jffs_generic_inode *inode;
  struct jffs_hash *hash;
  unsigned int start, stop, magic = 0, update;
  char timebuf[16];

  if(jffs_initialized && !force)
    return 0;

  if((region = btflash_get_partition("root")) == NULL){
    putstr("No \"root\" partition configured; cannot initialize JFFS\r\n");
    return -1;
  }

  jffs_initialized = 0;

  start = region->base;
  stop = region->base + region->size - sizeof(struct jffs_inode);

  memset(jffs_table, 0, sizeof(jffs_table));

  jffs_heap = (unsigned char *)JFFS_REPLAY_BASE;

  if(crc32_init() < 0){
    putstr("Unable to generate CRC32 lookup table\r\n");
    return -1;
  }

  for(inode = (union jffs_generic_inode *)start;
      inode < (union jffs_generic_inode *)stop;){

    /* We update the `inode' pointer in the "slow" way if we ever get
     * lost in the flash (for example, as the result of running across
     * an invalid inode, or because of excess padding between log
     * entries:
     */
    while(inode->v1.magic != JFFS_MAGIC && 
	  inode->v2.u.magic != JFFS2_MAGIC &&
	  inode < (union jffs_generic_inode *)stop)
      ++((unsigned int *)inode);

    /* Make progress past an unusable inode, if necessary: */
    update = sizeof(unsigned int);
	
    if(inode->v1.magic == JFFS_MAGIC){

      if(!magic)
	magic = JFFS_MAGIC;
      else if(magic != JFFS_MAGIC){
	putstr("Cannot load hybrid version JFFS\r\n");
	return -1;
      }

      if(inode->v1.chksum != jffs_inode_checksum(&inode->v1)){
	
	putstr("inode "); putHexInt32(inode->v1.ino);
	putstr(" version "); putHexInt32(inode->v1.version);
	putstr(" with inode checksum "); putHexInt32(inode->v1.chksum);
	putstr(" is invalid\r\n");
	
      } else
	update = sizeof(struct jffs_inode) +
	  inode->v1.nsize + jffs_pad(inode->v1.nsize) +
	  inode->v1.dsize + jffs_pad(inode->v1.dsize);

    } else if(inode->v2.u.magic == JFFS2_MAGIC){

      if(!magic)
	magic = JFFS2_MAGIC;
      else if(magic != JFFS2_MAGIC){
	putstr("Cannot load hybrid version JFFS2\r\n");
	return -1;
      }

      if(jffs2_inode_hdr_crc(&inode->v2) == 0)
	switch(inode->v2.u.nodetype){
	case JFFS2_NODETYPE_DIRENT:
	  
	  if(inode->v2.d.node_crc != jffs2_dirent_node_crc(&inode->v2.d)){
	    
	    putstr("dirent inode "); putHexInt32(inode->v2.d.ino);
	    putstr(" at "); putHexInt32((unsigned int)inode);
	    putstr(" with inode CRC "); putHexInt32(inode->v2.d.node_crc);
	    putstr(" is invalid\r\n");
	    
	  } else if(inode->v2.d.ino != JFFS_INVALID_INODE)
	    update = sizeof(struct jffs2_raw_dirent) +
	      inode->v2.d.nsize + jffs_pad(inode->v2.d.nsize);
	  
	  break;
	  
	case JFFS2_NODETYPE_INODE:
	  
	  if(inode->v2.i.node_crc != jffs2_inode_node_crc(&inode->v2.i)){
	    
	    putstr("inode "); putHexInt32(inode->v2.i.ino);
	    putstr(" at "); putHexInt32((unsigned int)inode);
	    putstr(" with inode CRC "); putHexInt32(inode->v2.i.node_crc);
	    putstr(" is invalid\r\n");
	    
	  } else if(inode->v2.i.ino != JFFS_INVALID_INODE)
	    update = sizeof(struct jffs2_raw_inode) +
	      inode->v2.i.csize + jffs_pad(inode->v2.i.csize);
	  
	}

    } else break;
      
    /* Need to rethink the loop structure to avoid sketchy control logic
     * like this:
     */
    if(update > sizeof(unsigned int)){

      if((hash = jffs_make_hash(inode)) == NULL){
	putstr("Unable to replay JFFS log: could not get inode hash\r\n");
	return -1;
      }
      
      if(jffs_add_log(hash, inode) < 0){
	putstr("Unable to replay JFFS log: could not add log to hash\r\n");
	return -1;
      }

    }
    
    /* Can only update `inode' pointer in this way if we're sure that
     * the inode data itself is correct:
     */
    inode = (union jffs_generic_inode *)(((unsigned char *)inode) + update);

  }

  if(!jffs_get_hash(JFFS_ROOT_INODE) && jffs_add_root_inode() < 0){
    putstr("Unable to replay JFFS log: could not add root inode\r\n");
    return -1;
  }

  if(jffs_replay() < 0){
    putstr("Unable to replay JFFS log: could not reassemble files\r\n");
    return -1;
  }
  
  if(jffs_build_directories() < 0){
    putstr("Unable to replay JFFS log: could not build directories\r\n");
    return -1;
  }

  jffs_initialized = 1;

  return 0;
}

/* Creates the file referenced by the hash `file' in memory by allocating
 * a buffer and then replaying the log.
 *
 * Returns 0 on success, -1 if out of memory or an inconsistency was found
 */
static int jffs_build_file(struct jffs_hash *file)
{
  struct jffs_log *log;
  unsigned int length;

  if(file->dsize == 0 || file->data != NULL)
    return 0;

  length = file->dsize;

#ifdef CONFIG_MD5
  length += md5_extend(file->dsize);
#endif

  if((file->data = jffs_alloc(length)) == NULL){
    putstr("Out of memory!\r\n");
    return -1;
  }

  for(log = file->log; log != NULL; log = log->next){

    if(log->inode->v1.magic == JFFS_MAGIC){

      if(log->inode->v1.dchksum != jffs_data_checksum(&log->inode->v1)){
	
	putstr("inode "); putHexInt32(log->inode->v1.ino);
	putstr(" version "); putHexInt32(log->inode->v1.version);
	putstr(" checksum "); putHexInt32(log->inode->v1.dchksum);
	putstr(" is invalid\r\n");
	
	return -1;
	
      }

      memcpy(file->data + log->inode->v1.offset,
	     ((unsigned char *)log->inode) + sizeof(struct jffs_inode) +
	     log->inode->v1.nsize + jffs_pad(log->inode->v1.nsize),
	     (file->dsize < (log->inode->v1.offset + log->inode->v1.dsize)) ?
	     (file->dsize - log->inode->v1.offset) : log->inode->v1.dsize);
    
    } else if(log->inode->v2.u.magic == JFFS2_MAGIC){

      if(log->inode->v2.u.nodetype == JFFS2_NODETYPE_INODE){

	if(log->inode->v2.i.data_crc != 
	   jffs2_inode_data_crc(&log->inode->v2.i)){
	  
	  putstr("inode "); putHexInt32(log->inode->v2.i.ino);
	  putstr(" version "); putHexInt32(log->inode->v2.i.version);
	  putstr(" CRC "); putHexInt32(log->inode->v2.i.data_crc);
	  putstr(" is invalid\r\n");
	  
	  return -1;
	  
	}
	
	if(jffs2_inode_decompress(file->data, log->inode->v2.i.offset,
				  file->dsize, &log->inode->v2.i) < 0){
	  putLabeledWord("could not decompress inode ",
			 jffs_inumber(log->inode));
	  return -1;
	}

      }  /* valid JFFS2 inode */

    } else  /* unknown JFFS version */
      return -1;

  }

  return 0;
}

/* Fetches the hash entry for the file described by the pathname
 * (a '\0'-terminated string) given in `path', beginning from the 
 * filesystem node `start' (if `start' is NULL, traversal begins from
 * the filesystem root). Fills in `pathp' (if non-NULL) with a pointer
 * to the last pathname component of the requested file, which can be
 * used in determining the correct name for multiply-linked inodes. 
 * Returns NULL if the filesystem cannot be traversed, or if a directory
 * in the path does not contain the indicated next component as a child.
 */
static struct jffs_hash *jffs_get_file(struct jffs_hash *start,
				       char *path, char **pathp)
{
  char *p;
  struct jffs_hash *hash;
  struct jffs_name *name;
  unsigned int length;
  int i, j;
  char symlink_path[JFFS_MAXPATHLEN];
  static unsigned int loop_check[JFFS_LOOP_CHECK_LENGTH];
  
  if(start && *path != JFFS_PATH_SEPARATOR)
    hash = start;
  else {

    if((hash = jffs_get_hash(JFFS_ROOT_INODE)) == NULL){
      putstr("Unable to fetch root inode!\r\n");
      return NULL;
    }

    memset(loop_check, 0, sizeof(loop_check));

  }

  for(p = path; ;){
    while(*p == JFFS_PATH_SEPARATOR)
      ++p;

    if(*p == '\0')
      break;

    /* We have a new path component to examine. */

    length = jffs_path_component_length(p);

    if(strncmp(p, JFFS_PARENT_DIR, length) == 0){

      /* Assume no hardlinks to directories, so there is only one
       * name for this node:
       */

      if(hash->inumber != JFFS_ROOT_INODE &&
	 (hash = jffs_get_hash(hash->names[0].parent)) == NULL)
	return NULL;

      p += length;

      continue;

    }

    /* Path component wasn't "..", so it must be a directory. */

    if(hash->nchildren == 0)
      return NULL;
    
    for(i = 0; i < hash->nchildren; ++i){
      
      if(length != hash->children[i].name->nsize)
	continue;
      
      if(strncmp(p, hash->children[i].name->name, length) == 0)
	break;
      
    }
    
    if(pathp)
      *pathp = p;
    
    p += length;

    if(i == hash->nchildren)
      return NULL;

    if((hash->children[i].child->mode & JFFS_MODE_FMT) == JFFS_MODE_FLNK){

      /* Recursively process a symbolic link: */

      for(j = 0; 
	  j < JFFS_LOOP_CHECK_LENGTH && loop_check[j] != JFFS_INVALID_INODE;
	  ++j)
	if(hash->children[i].child->inumber == loop_check[j]){
	  putstr("symbolic link loop detected\r\n");
	  return NULL;
	}

      if(j < JFFS_LOOP_CHECK_LENGTH)
	loop_check[j] = hash->children[i].child->inumber;

      if(jffs_build_file(hash->children[i].child) < 0)
	return NULL;

      for(j = 0; j < hash->children[i].child->dsize; ++j)
	symlink_path[j] = hash->children[i].child->data[j];
      symlink_path[j] = '\0';

      if((hash = jffs_get_file(hash, symlink_path, NULL)) == NULL)
	return NULL;

      /* Resume traversing the original path. */

    } else
      hash = hash->children[i].child;

  }

  return hash;
}

/* Prints the first `size' bytes of the file described by the absolute
 * pathname (a '\0'-terminated string) `path'.
 */
void jffs_dump_file(char *path, unsigned int size)
{
  int i;
  struct jffs_hash *file;
  char ascii[JFFS_DUMP_LINEWIDTH + 1];

  if((file = jffs_get_file(NULL, path, NULL)) == NULL){
    putstr("Could not dump \""); putstr(path);
    putstr("\": no such file or directory\r\n");
    return;
  }

  if(file->dsize == 0)
    return;

  if(jffs_build_file(file) < 0){
    putstr("Error building file.\r\n");
    return;
  }

  ascii[JFFS_DUMP_LINEWIDTH] = '\0';

  for(i = 0; i < size && i < file->dsize; i += sizeof(unsigned short)){

    if((i % JFFS_DUMP_LINEWIDTH) == 0){

      if(i != 0) {
	putstr("     ");
	putstr(ascii);
	putstr("\r\n");
      }

      putHexInt32(i);

    }

    putstr(" ");
    putHexInt16(*((unsigned short *)(file->data+i)));

    ascii[i % JFFS_DUMP_LINEWIDTH] = jffs_isprintable(*(file->data+i)) ?
      *(file->data+i) : '.';
    ascii[(i+1) % JFFS_DUMP_LINEWIDTH] = jffs_isprintable(*(file->data+i+1)) ?
      *(file->data+i+1) : '.';

  }

  while((i % JFFS_DUMP_LINEWIDTH) != 0){

    putstr("     ");

    ascii[i % JFFS_DUMP_LINEWIDTH] = ' ';
    ascii[(i+1) % JFFS_DUMP_LINEWIDTH] = ' ';

    i += sizeof(unsigned short);

  }

  putstr("     ");
  putstr(ascii);
  putstr("\r\n");
}

/* Prints out an `ls'-esque string describing the file `node' using the
 * name `name' (multiply-linked files can have different names):
 */
static void jffs_list_node(struct jffs_hash *node, struct jffs_name *name)
{
  int i;

  switch(node->mode & JFFS_MODE_FMT){
  case JFFS_MODE_FSOCK:
    putc('s');
    break;
  case JFFS_MODE_FLNK:
    putc('l');
    break;
  case JFFS_MODE_FREG:
    putc('-');
    break;
  case JFFS_MODE_FBLK:
    putc('b');
    break;
  case JFFS_MODE_FDIR:
    putc('d');
    break;
  case JFFS_MODE_FCHR:
    putc('c');
    break;
  case JFFS_MODE_FIFO:
    putc('f');  /* I don't think I've ever seen what `ls' does for this. */
    break;
  default:
    putc('-');
  }

  putc((node->mode & JFFS_MODE_RUSR) ? 'r' : '-');
  putc((node->mode & JFFS_MODE_WUSR) ? 'w' : '-');
  putc((node->mode & JFFS_MODE_XUSR) ? 
       ((node->mode & JFFS_MODE_SUID) ? 's' : 'x') : '-');

  putc((node->mode & JFFS_MODE_RGRP) ? 'r' : '-');
  putc((node->mode & JFFS_MODE_WGRP) ? 'w' : '-');
  putc((node->mode & JFFS_MODE_XGRP) ? 
       ((node->mode & JFFS_MODE_SGID) ? 's' : 'x') : '-');

  putc((node->mode & JFFS_MODE_ROTH) ? 'r' : '-');
  putc((node->mode & JFFS_MODE_WOTH) ? 'w' : '-');
  putc((node->mode & JFFS_MODE_XOTH) ? 'x' : '-');

  putstr("   ");
  putDecIntWidth((node->nchildren > 0) ? node->nchildren : node->nlink, 
		 JFFS_LIST_LINKS_WIDTH);
  putstr("   ");
  putDecIntWidth(node->inumber, JFFS_LIST_INUMBER_WIDTH);
  putstr("   ");
  putHexInt32(node->mtime);
  putstr("   ");
  putDecIntWidth(node->dsize, JFFS_LIST_SIZE_WIDTH);
  putstr("   ");
  
  for(i = 0; i < name->nsize; ++i)
    putc(name->name[i]);

  if(node->nchildren > 0 || node->mode & JFFS_MODE_FDIR)
    putc(JFFS_PATH_SEPARATOR);

  if((node->mode & JFFS_MODE_FMT) == JFFS_MODE_FLNK){

    if(jffs_build_file(node) < 0){
      putstr(" (bad symlink)\r\n");
      return;
    }

    putstr(" -> ");

    for(i = 0; i < node->dsize; ++i)
      putc(jffs_isprintable(node->data[i]) ? node->data[i] : '.');

  }

  putstr("\r\n");
}

/* Lists a single file: */
static void jffs_list_file(struct jffs_hash *file, struct jffs_name *name)
{

  putstr(JFFS_LIST_HEADER);
  jffs_list_node(file, name);

}

/* Lists the contents of a directory: */
static void jffs_list_directory(struct jffs_hash *directory)
{ 
  int i;

  putstr(JFFS_LIST_HEADER);
  for(i = 0; i < directory->nchildren; ++i)
    jffs_list_node(directory->children[i].child,
		   directory->children[i].name);

}

/* Lists the file or the contents of the directory described by the 
 * absolute path (a '\0'-terminated string) `path':
 */
void jffs_list(char *path)
{
  struct jffs_hash *hash;
  struct jffs_name *name;
  char *pathp;
 
  if((hash = jffs_get_file(NULL, path, &pathp)) == NULL){
    putstr("Could not list \""); putstr(path);
    putstr("\": no such file or directory\r\n");
    return;
  }

  if(hash->nchildren > 0)
    jffs_list_directory(hash);
  else {

    putstr("listing file \""); putstr(pathp); putstr("\"\r\n");

    for(name = hash->names; name != NULL; name = name->next)
      if(strncmp(name->name, pathp, name->nsize) == 0)
	jffs_list_file(hash, name);

  }
}

/* Causes the file described by the absolute path (a '\0'-terminated
 * string) `path' to be built in memory by replaying its version log.
 * A pointer to the built file is returned, and the final length of the
 * file in memory is returned in `length'. Returns NULL if the file
 * has zero length, or if an error occurred while replaying the log.
 */
void *jffs_file(char *path, unsigned int *length)
{
  struct jffs_hash *file;

  if((file = jffs_get_file(NULL, path, NULL)) == NULL){
    putstr("Could not open \""); putstr(path);
    putstr("\": no such file or directory\r\n");
    return NULL;
  }

  if(file->dsize == 0)
    return NULL;

  if(jffs_build_file(file) < 0){
    putstr("Error building file.\r\n");
    return NULL;
  }

  *length = file->dsize;

  return (void *)file->data;
}

/* Examines the given buffer for a substring which is delimited by the
 * given character set, up to the specified buffer length. Delimiter
 * characters are stripped on either side of the resulting substring.
 * A pointer to the substring is returned (or NULL, if an error occurred).
 * The length of the substring (which is NOT terminated) is filled in,
 * as is the total number characters which were processed in the buffer
 * during tokenization.
 */
static char *jffs_tokenize(char *buf, const char *delim, unsigned int length, 
			   unsigned int *token_length, 
			   unsigned int *update_length)
{
  char *p, *token;

  /* Weed out any delimiters at the start of the buffer: */
  for(p = buf; p < (buf + length) && jffs_isdelim(*p, (char *)delim); ++p);

  if(p == (buf + length)){
    *token_length = 0;
    *update_length = p - buf;
    return NULL;
  }

  if(*p == '\"'){  /* Special case for string constants: */

    for(token = p, ++p; p < (buf + length) && *p != '\"'; ++p);

    if(p < (buf + length))
      ++p;
    else  /* Unterminated string; back up to first non-delim character: */
      for(; p > token && jffs_isdelim(*(p - 1), (char *)delim); --p);

  } else {

    for(token = p; p < (buf + length) && !jffs_isdelim(*p, (char *)delim);
	++p);

  }

  *token_length = p - token;

  /* Weed out any delimiters after token: */
  for(; p < (buf + length) && jffs_isdelim(*p, (char *)delim); ++p);

  *update_length = p - buf;

  return token;
}

/* Displays a formatted diagnostic message for use during conf file
 * processing:
 */
static void jffs_conf_message(char *path, int line, char *token, 
			      unsigned int token_length, char *message)
{
  int i;
  char decimal[16];

  memset(decimal, 0, sizeof(decimal));
  dwordtodecimal(decimal, line);
  
  putstr(path); putc(' '); putstr(decimal);
  putstr(": token \"");
  for(i = 0; i < token_length; ++i)
    putc(token[i]);
  putstr("\" ");
  putstr(message);
  putstr("\r\n");
}

/* Parses the contents of the file described by `file' (`path' is used
 * to provide meaningful diagnostic messages). Returns 0 on success,
 * -1 on any error.
 */
static int jffs_conf_parse(char *path, struct jffs_hash *file)
{
  char *token, *p;
  unsigned int file_length, token_length, update_length, line_length;
  unsigned int consumed_length;
  int i, line, image_block;
  struct jffs_conf_parse *keyword;
  void *field;
  char int_param[16];
  unsigned long int_param_parsed;
  extern byte strtoul_err;

  if((file->misc.conf = 
      (struct jffs_conf *)jffs_alloc(sizeof(struct jffs_conf))) == NULL){
    putstr("Out of memory!\r\n");
    return -1;
  }
  
  memset(file->misc.conf, 0, sizeof(struct jffs_conf));
  
  /* Quick run through the file; how many "image" blocks do we have? */
  
  for(p = file->data, file_length = file->dsize; 
      file_length > 0; 
      p += line_length, file_length -= line_length){
    
    line_length = jffs_linelength(p, file_length);
    
    if((token = jffs_tokenize(p, JFFS_CONF_TOKEN_DELIM, line_length,
			      &token_length, &update_length)) == NULL)
      continue;
    
    if(token_length == strlen(JFFS_CONF_KEYWORD_IMAGE) &&
       strncmp(token, JFFS_CONF_KEYWORD_IMAGE, token_length) == 0)
      ++file->misc.conf->nimages;
    
  }
  
  if(file->misc.conf->nimages == 0){
    putstr("No bootable images defined in \"");
    putstr(path);
    putstr("\"\r\n");
    return -1;
  }
  
  if((file->misc.conf->images = 
      (struct jffs_conf_image **)
      jffs_alloc(file->misc.conf->nimages *
		 sizeof(struct jffs_conf_image *))) == NULL){
    putstr("Out of memory!\r\n");
    return -1;
  }
  
  for(i = 0; i < file->misc.conf->nimages; ++i){
    
    if((file->misc.conf->images[i] =
	(struct jffs_conf_image *)
	jffs_alloc(sizeof(struct jffs_conf_image))) == NULL){
      putstr("Out of memory!\r\n");
      return -1;
    }
    
    memset(file->misc.conf->images[i], 0, sizeof(struct jffs_conf_image));
    
  }
  
  /* Image blocks are allocated and initialized. Full parse: */
  
  for(p = file->data, file_length = file->dsize, line = 1, image_block = -1;
      file_length > 0;
      p += line_length, file_length -= line_length, ++line){
    
    line_length = jffs_linelength(p, file_length);
    consumed_length = 0;
    
    /* Keyword: */
    if((token = jffs_tokenize(p, JFFS_CONF_TOKEN_DELIM, line_length,
			      &token_length, &update_length)) == NULL)
      continue;
    
    consumed_length += update_length;
    
    if(token_length == strlen(JFFS_CONF_KEYWORD_IMAGE) &&
       strncmp(token, JFFS_CONF_KEYWORD_IMAGE, token_length) == 0)
      ++image_block;
    
    if((keyword = ((image_block < 0) ?
		   jffs_conf_global_keyword(token, token_length) :
		   jffs_conf_image_keyword(token, token_length))) == NULL){
      
      jffs_conf_message(path, line, token, token_length, 
			"unrecognized (ignoring)");
      continue;
      
    }
    
    field = (void *)((image_block < 0) ? 
		     ((char *)file->misc.conf + keyword->field) :
		     ((char *)file->misc.conf->images[image_block] + 
		      keyword->field));
    
    /* Duplicate appearances of a keyword in a given context are
     * permitted. (We could reliably detect this for strings, but
     * not necessarily for all ints or flags without additional
     * bookkeeping.) Note that in the case of string parameters, 
     * duplicates result in memory leaks because of the way
     * jffs_alloc() works.
     */
    
    switch(keyword->param){
      
    case param_string:
      
      /* jffs_tokenize() eats trailing delimiters, so if there's a '='
       * right after the keyword, it will be in the range consumed during
       * tokenization:
       */
      if(jffs_strchr(p, '=', consumed_length) < 0){
	
	jffs_conf_message(path, line, keyword->keyword, 
			  strlen(keyword->keyword),
			  "requires value assignment (ignoring)");
	continue;
	
      }
      
      if((token = jffs_tokenize(p + consumed_length, 
				JFFS_CONF_TOKEN_WHITESPACE, 
				line_length - consumed_length,
				&token_length, &update_length)) == NULL){
	
	jffs_conf_message(path, line, keyword->keyword, 
			  strlen(keyword->keyword),
			  "requires value after '=' (ignoring)");
	continue;
	
      }
      
      consumed_length += update_length;
      
      /* Is this a quoted string literal? */
      if(token[0] == '\"'){
	
	if(token[token_length - 1] != '\"'){
	  jffs_conf_message(path, line, token, token_length,
			    "is unterminated string (ignoring)");
	  continue;
	}
	
	/* Strip leading and trailing quotes: */
	++token;
	token_length -= 2;
	
      }
      
      if((*(char **)field = jffs_alloc(token_length + 1)) == NULL){
	putstr("Out of memory!\r\n");
	return -1;
      }
      
      memcpy(*(char **)field, token, token_length);
      (*(char **)field)[token_length] = '\0';
      
      if(consumed_length < line_length)
	jffs_conf_message(path, line, keyword->keyword,
			  strlen(keyword->keyword),
			  "takes one value; ignoring extra tokens");
      
      break;
      
    case param_int:
      
      /* jffs_tokenize() eats trailing delimiters, so if there's a '='
       * right after the keyword, it will be in the range consumed during
       * tokenization:
       */
      if(jffs_strchr(p, '=', consumed_length) < 0){
	
	jffs_conf_message(path, line, keyword->keyword, 
			  strlen(keyword->keyword),
			  "requires value assignment (ignoring)");
	continue;
	
      }
      
      if((token = jffs_tokenize(p + consumed_length, 
				JFFS_CONF_TOKEN_WHITESPACE, 
				line_length - consumed_length,
				&token_length, &update_length)) == NULL){
	
	jffs_conf_message(path, line, keyword->keyword, 
			  strlen(keyword->keyword),
			  "requires value after '=' (ignoring)");
	continue;
	
      }
      
      consumed_length += update_length;
      
      memcpy(int_param, token, token_length);
      int_param[token_length] = '\0';
      
      if((int_param_parsed = strtoul(int_param, NULL, 0)) == 0 &&
	 strtoul_err != 0){
	
	jffs_conf_message(path, line, int_param, strlen(int_param),
			  "could not be parsed (ignoring)");
	continue;
	
      }
      
      *(unsigned int *)field = int_param_parsed;
      
      if(consumed_length < line_length)
	jffs_conf_message(path, line, keyword->keyword,
			  strlen(keyword->keyword),
			  "takes no values; ignoring extra tokens");
      
      break;
      
    case param_flag:
      
      *(unsigned int *)field |= keyword->flag;
      
      if(consumed_length < line_length)
	jffs_conf_message(path, line, keyword->keyword,
			  strlen(keyword->keyword),
			  "takes no values; ignoring extra tokens");
      
      break;
      
    default:
      
      jffs_conf_message(path, line, keyword->keyword,
			strlen(keyword->keyword),
			"has unrecognized keyword type (error)");
      
    }
    
  }
  
  return 0;
}

/* Fill in a pointer to a structure describing the configuration data in
 * the named file. Returns 0 on success, -1 if the file could not be
 * processed.
 */
int jffs_conf(char *path, struct jffs_conf **conf)
{
  struct jffs_hash *file;
  extern unsigned int *assabet_scr;

  if((file = jffs_get_file(NULL, path, NULL)) == NULL){
    putstr("Error opening conf file \""); putstr(path);
    putstr("\": no such file or directory\r\n");
    return -1;
  }

  if(file->dsize == 0){
    putstr("Error reading conf file \""); putstr(path);
    putstr("\": file has zero length\r\n");
    return -1;
  }

  if(jffs_build_file(file) < 0){
    putstr("Error building file \""); putstr(path); putstr("\"\r\n");
    return -1;
  }

  if(file->misc.conf == NULL){

    if(jffs_conf_parse(path, file) < 0){
      putstr("Could not parse \""); putstr(path); putstr("\"\r\n");
      return -1;
    }

  }

  *conf = file->misc.conf;

  return 0;
}

/* List the available labels and their aliases. Mark default with a '*': */
void jffs_conf_list(struct jffs_conf *conf)
{
  int i;

  for(i = 0; i < conf->nimages; ++i){
    
    if(conf->images[i]->label){
      putstr(conf->images[i]->label);
      if((conf->deflt != NULL && 
	  strcmp(conf->images[i]->label, conf->deflt) == 0) ||
	 (conf->deflt == NULL && i == 0))
	putc('*');
      putc(' ');
    }
    
    if(conf->images[i]->alias){
      putstr("("); putstr(conf->images[i]->alias);
      if((conf->deflt != NULL && 
	  strcmp(conf->images[i]->alias, conf->deflt) == 0) ||
	 (conf->deflt == NULL && i == 0 && 
	  conf->images[i]->label == NULL))
	putc('*');
      putstr(") ");
    }
    
    if(conf->images[i]->label || conf->images[i]->alias)
      putstr("   ");
    
  }
  
  putstr("\r\n");
}

/* Fill in path of a (hopefully) bootable image and kernel argument list: */
int jffs_conf_image(struct jffs_conf *conf, const char *boot_file,
		    char *imagename, char *bootargs)
{
  char *bootlabel, *root;
  unsigned int access, ramdisk;
  char decimal[16];
  int i;

  /* command_set() sets the param value of zero-length strings to
   * be a NULL pointer rather than a pointer to a zero-length
   * string:
   */
  if(boot_file == 0){  /* use default boot image */
    
    if(conf->deflt == NULL){  /* no "default" keyword in conf file */
      
      if(conf->nimages > 0){
	
	/* For the first image found, try its label, then its alias,
	 * and finally just the raw image filename:
	 */
	
	if(conf->images[0]->label)
	  bootlabel = conf->images[0]->label;
	else if(conf->images[0]->alias)
	  bootlabel = conf->images[0]->alias;
	else if(conf->images[0]->image)
	  bootlabel = conf->images[0]->image;
	else {
	  putstr("No default image or usable first image in conf file\r\n");
	  return -1;
	}
	
      } else {
	putstr("No images listed in conf file\r\n");
	return -1;
      }
      
    } else
      bootlabel = conf->deflt;
    
  } else {  /* use named boot image */
    
    bootlabel = (char *)boot_file;
    
  }
  
  /* `bootlabel' now points to a string which is either (supposedly)
   * a label or alias for a boot image, or a pathname for a bootable
   * file.
   */
  
  for(i = 0; i < conf->nimages; ++i)
    if((conf->images[i]->label &&
	strcmp(conf->images[i]->label, bootlabel) == 0) ||
       (conf->images[i]->alias &&
	strcmp(conf->images[i]->alias, bootlabel) == 0))
      break;
  
  if(i == conf->nimages){
    
    putstr("Could not find image label or alias \"");
    putstr(bootlabel); putstr("\"\r\n");
    
    /* Maybe the bootlabel was an actual pathname? */
    
    strcpy(imagename, bootlabel);

  } else {
    
    /* Found it. Try and dig up the image file and perform any
     * necessary arg list edits.
     */
    
    if(conf->images[i]->image == NULL){
      putstr("No boot image in configuration for label \"");
      putstr(bootlabel); putstr("\"\r\n");
      return -1;
    }
    
    strcpy(imagename, conf->images[i]->image);

    putstr("Label or alias \""); putstr(bootlabel);
    putstr("\" names image \""); putstr(conf->images[i]->image); 
    putstr("\"\r\n");
    
    if(conf->images[i]->literal){
      
      if(strlen(bootargs) > 0 || conf->access || conf->ramdisk ||
	 conf->append || conf->root || conf->images[i]->access || 
	 conf->images[i]->ramdisk || conf->images[i]->root || 
	 conf->images[i]->append)
	putstr("Keyword \"literal\" overrides all other argument list "
	       "elements\r\n");
      
      strcpy(bootargs, conf->images[i]->literal);
      
    } else {
      
      /* Root filesystem access permissions: */
      
      if(conf->access){
	
	if(conf->images[i]->access){
	  putstr("Image access flags override global access flags\r\n");
	  access = conf->images[i]->access;
	} else
	  access = conf->access;
	
      } else 
	access = conf->images[i]->access;
      
      switch(access){
      case 0:
	break;
	
      case JFFS_CONF_ACCESS_RO:
	strcat(bootargs, "ro ");
	break;
	
      case JFFS_CONF_ACCESS_RW:
	strcat(bootargs, "rw ");
	break;
	
      default:
	putstr("Read-only root filesystem access overrides other "
	       "flags\r\n");
	strcat(bootargs, "ro ");
	
      }
      
      /* Ramdisk size: */
      
      if(conf->ramdisk){
	
	if(conf->images[i]->ramdisk){
	  putstr("Image ramdisk size overrides global ramdisk size\r\n");
	  ramdisk = conf->images[i]->ramdisk;
	} else
	  ramdisk = conf->ramdisk;
	
      } else
	ramdisk = conf->images[i]->ramdisk;
      
      if(ramdisk > 0){

	memset(decimal, 0, sizeof(decimal));
	dwordtodecimal(decimal, ramdisk);

	strcat(bootargs, "ramdisk_size=");
	strcat(bootargs, decimal);
	strcat(bootargs, " ");

      }

      /* Root filesystem: */

      if(conf->root){

	if(conf->images[i]->root){
	  putstr("Image root filesystem overrides global root filesystem\r\n");
	  root = conf->images[i]->root;
	} else
	  root = conf->root;

      } else
	root = conf->images[i]->root;

      if(root){
	strcat(bootargs, "root=");
	strcat(bootargs, root);
	strcat(bootargs, " ");
      }

      /* Append to argument list: */
      
      if(conf->append){
	strcat(bootargs, conf->append);
	strcat(bootargs, " ");
      }
      
      if(conf->images[i]->append){
	strcat(bootargs, conf->images[i]->append);
	strcat(bootargs, " ");
      }
      
    }
    
  }

  return 0;
}

#ifdef CONFIG_MD5

/* Compute and display the MD5 checksum of the file referenced by
 * `path':
 */
void jffs_md5sum(char *path)
{
  struct jffs_hash *file;
  unsigned int sum[MD5_SUM_WORDS];
  int i, j;
  char *pathp;

  if((file = jffs_get_file(NULL, path, &pathp)) == NULL){
    putstr("Could not open \""); putstr(path);
    putstr("\": no such file or directory\r\n");
    return;
  }

  if(file->dsize == 0)
    return;

  if(jffs_build_file(file) < 0){
    putstr("Error building file.\r\n");
    return;
  }

  md5_sum((unsigned char *)file->data, file->dsize, sum);
  md5_display(sum);
  putstr("  "); putstr(pathp); putstr("\r\n");
}

#endif  /* CONFIG_MD5 */
