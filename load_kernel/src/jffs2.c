/*
-------------------------------------------------------------------------
 * Filename:      jffs2.c
 * Version:       $Id: jffs2.c,v 1.12 2003/09/17 17:35:15 bavery Exp $
 * Copyright:     Copyright (C) 2001, Russ Dill
 * Author:        Russ Dill <Russ.Dill@asu.edu>
 * Description:   Module to load kernel from jffs2
 *-----------------------------------------------------------------------*/
/*
 * some portions of this code are taken from jffs2, and as such, the
 * following copyright notice is included.
 *
 * JFFS2 -- Journalling Flash File System, Version 2.
 *
 * Copyright (C) 2001 Red Hat, Inc.
 *
 * Created by David Woodhouse <dwmw2@cambridge.redhat.com>
 *
 * The original JFFS, from which the design for JFFS2 was derived,
 * was designed and implemented by Axis Communications AB.
 *
 * The contents of this file are subject to the Red Hat eCos Public
 * License Version 1.1 (the "Licence"); you may not use this file
 * except in compliance with the Licence.  You may obtain a copy of
 * the Licence at http://www.redhat.com/
 *
 * Software distributed under the Licence is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.
 * See the Licence for the specific language governing rights and
 * limitations under the Licence.
 *
 * The Original Code is JFFS2 - Journalling Flash File System, version 2
 *
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License version 2 (the "GPL"), in
 * which case the provisions of the GPL are applicable instead of the
 * above.  If you wish to allow the use of your version of this file
 * only under the terms of the GPL and not to allow others to use your
 * version of this file under the RHEPL, indicate your decision by
 * deleting the provisions above and replace them with the notice and
 * other provisions required by the GPL.  If you do not delete the
 * provisions above, a recipient may use your version of this file
 * under either the RHEPL or the GPL.
 *
 * $Id: jffs2.c,v 1.12 2003/09/17 17:35:15 bavery Exp $
 *
 */

/* Ok, so anyone who knows the jffs2 code will probably want to get a papar
 * bag to throw up into before reading this code. I looked through the jffs2
 * code, the caching scheme is very elegant. I tried to keep the version
 * for a bootloader as small and simple as possible. Instead of worring about
 * unneccesary data copies, node scans, etc, I just optimized for the known
 * common case, a kernel, which looks like:
 * 	(1) most pages are 4096 bytes
 *	(2) version numbers are somewhat sorted in acsending order
 *	(3) multiple compressed blocks making up one page is uncommon
 *
 * So I create a linked list of decending version numbers (insertions at the
 * head), and then for each page, walk down the list, until a matching page
 * with 4096 bytes is found, and then decompress the watching pages in
 * reverse order.
 *
 */


#include "types.h"
#include "load_kernel.h"
#include "jffs2.h"
#include "crc32.h"

#include <string.h>

//#define D_PUTSTR(x) putstr((x))
#define D_PUTSTR(x) 
//#define D_PUTLABELEDWORD(x,y) putLabeledWord((x),(y))
#define D_PUTLABELEDWORD(x,y) 
// for the lcd progress bar
/* default Compaq bootloader splash */

static struct jffs2_raw_dirent *find_inode(struct part_info *part, const char *name,
					   u32 pino);
void rtime_decompress   (unsigned char *data_in, unsigned char *cpage_out, 
		         __u32 srclen, __u32 destlen);
void dynrubin_decompress(unsigned char *data_in, unsigned char *cpage_out,
			 __u32 srclen, __u32 destlen);
long zlib_decompress    (unsigned char *data_in, unsigned char *cpage_out,
			 __u32 srclen, __u32 destlen);

struct data_strip {
	struct jffs2_raw_inode *inode;
	long int version;
	long int page;
	long int length;
	struct data_strip *next;
};


inline int hdr_crc(struct jffs2_unknown_node *node)
{
	if(node->hdr_crc != crc32(0, node, sizeof(struct jffs2_unknown_node) - 4))
		return 0;
	else return 1;
}

inline int dirent_crc(struct jffs2_raw_dirent *node)
{
	if (node->node_crc != crc32(0, node, sizeof(struct jffs2_raw_dirent) - 8)) {
		UDEBUG("bad dirent_crc\n");
		return 0;
	} else return 1;
}

inline int dirent_name_crc(struct jffs2_raw_dirent *node)
{
	if (node->name_crc != crc32(0, &(node->name), node->nsize)) {
		UDEBUG("bad dirent_name_crc\n");
		return 0;
	} else return 1;
}

inline int inode_crc(struct jffs2_raw_inode *node)
{
	if (node->node_crc != crc32(0, node, sizeof(struct jffs2_raw_inode) - 8)) {
		UDEBUG("bad inode_crc\n");
		return 0;
	} else return 1;
}

/* decompress a page from the flash, first contains a linked list of references
 * all the nodes of this file, sorted is decending order (newest first) */
static int jffs2_uncompress_page(char *dest, struct data_strip *first, u32 page)
{
	int i;
	char *src;

#if 1
	D_PUTSTR("jffs2_uncompress_page start entry\r\n");
	D_PUTLABELEDWORD("dest = 0x",dest);
	D_PUTLABELEDWORD("first = ",first);
	D_PUTLABELEDWORD("first->length = ",first->length);
	D_PUTLABELEDWORD("first->inode->offset = ",first->inode->offset);
	D_PUTLABELEDWORD("first->page = ",first->page);
	D_PUTLABELEDWORD("page = ",page);
	D_PUTLABELEDWORD("first->version = ",first->version);
	D_PUTLABELEDWORD("first->next = ",first->next);
	D_PUTSTR("jffs2_uncompress_page end entry\r\n");
#endif
	/* look for the next reference to this page */
	for (; first && first->page != page; first = first->next);
	{
#if 0
	    D_PUTLABELEDWORD("first = ",first);
#endif
	    if (!first) return 1;
	}
	
#if 0
	D_PUTLABELEDWORD("pre uncover check first->length = ",first->length);
#endif
	/* if we aren't covering what's behind us, uncompress that first */
	if (first->length != 4096)
		if (!(jffs2_uncompress_page(dest, first->next, page)))
			return 0;

	
	dest += first->inode->offset;
	src = ((char *) first->inode) + sizeof(struct jffs2_raw_inode);
	
	if (first->inode->data_crc != crc32(0, src, first->inode->csize)) {
		ldr_output_string("CRC Error!");
		return 0;
	}

	switch (first->inode->compr) {
	    case JFFS2_COMPR_NONE:
		ldr_memcpy(dest, src, first->length);
		break;
	case JFFS2_COMPR_ZERO:
		for (i = 0; i < first->length; i++) *(dest++) = 0;
		break;
	case JFFS2_COMPR_RTIME:
		rtime_decompress(src, dest, first->inode->csize, 
				 first->length);
		break;
	case JFFS2_COMPR_DYNRUBIN:
		dynrubin_decompress(src, dest, first->inode->csize, 
				    first->length);
		break;
	case JFFS2_COMPR_ZLIB:
		zlib_decompress(src, dest, first->inode->csize, first->length);
		break;
	default:
		/* unknown */
	        ;
	}
	return 1;
}

/*****************************************************************************
 *
 * Alternative functions
 *
 *
 */

/* decompress a page from the flash, first contains a linked list of references
 * all the nodes of this file, sorted is decending order (newest first) */
static int unordered_jffs2_uncompress_file(char *dest, struct data_strip *first)
{
	int i;
	char *src;
	long ret;
	struct data_strip  *curr;	
	char *lDest = dest;
	
#if 0
	if (first->inode->data_crc != crc32(0, src, first->inode->csize)) {
		ldr_output_string("CRC Error!");
		return -1;
	}
#endif

	curr = first;
	while (curr){
	    src = ((char *) curr->inode) + sizeof(struct jffs2_raw_inode);
	    lDest = dest + (curr->inode->offset & ~3);	    
	    D_PUTSTR("bdecompr: running:\n");
	    D_PUTLABELEDWORD("\tversion  ",curr->inode->version);		    
	    D_PUTLABELEDWORD("\toffset  ",curr->inode->offset);
	    D_PUTLABELEDWORD("\tcsize  ",curr->inode->csize);
	    D_PUTLABELEDWORD("\tdsize  ",curr->inode->dsize);
	    D_PUTLABELEDWORD("\tisize  ",curr->inode->isize);
	    D_PUTLABELEDWORD("\tpage  ",curr->page);
	    D_PUTLABELEDWORD("\tsrc  ",src);
	    D_PUTLABELEDWORD("\tlDest  ",lDest);
	    D_PUTLABELEDWORD("\tcompression  ",curr->inode->compr);
	    switch (curr->inode->compr) {
		case JFFS2_COMPR_NONE:
		    ret = (unsigned long) ldr_memcpy(lDest, src, curr->length);
		    break;
		case JFFS2_COMPR_ZERO:
		    ret = 0;		    
		    for (i = 0; i < curr->length; i++) *(lDest++) = 0;
		    break;
		case JFFS2_COMPR_RTIME:
		    ret = 0;		    
		    rtime_decompress(src, lDest, curr->inode->csize, 
				     curr->length);
		    break;
		case JFFS2_COMPR_DYNRUBIN:
		    ret = 0;		    
		    dynrubin_decompress(src, lDest, curr->inode->csize, 
					curr->length);
		    break;
		case JFFS2_COMPR_ZLIB:
		    ret = zlib_decompress(src, lDest, curr->inode->csize, curr->length);
		    break;
		default:
		    /* unknown */
		    D_PUTLABELEDWORD("UNKOWN COMPRESSION METHOD = ",curr->inode->compr);
		    return -1;
		    break;
	    }
	    D_PUTLABELEDWORD("\tcompression returned = ",ret);
	    curr = curr->next;	    
	}
	
	return 0;
}



static int jffs2_check_magic(struct part_info *part)
{		
	unsigned long offset;
	struct jffs2_unknown_node *node;
	
	/* search the start of each erase block for a magic number */
	offset = 0;
	for (offset = 0; offset < part->size; offset += part->erasesize) {
		node = (struct jffs2_unknown_node *) (part->offset + offset);
		if (node->magic == JFFS2_MAGIC_BITMASK && hdr_crc(node)) return 1;
	}
	return 0;
}

/* read in the data for inode inode_num */
static u32 read_inode(struct part_info *part, u32 *dest, u32 inode_num, int type)
{
	unsigned long size;
	struct jffs2_raw_inode *inode;
	unsigned long offset, inode_version, version;
	struct data_strip  *curr, *prev, *first, *new;
	int ret;
	

	
	// we have a mmalloc so we'll use it.
	//free = (struct data_strip *) fodder_ram_base;
	first = NULL;
	
	offset = size = inode_version = 0;
#if 1
	D_PUTLABELEDWORD("read_inode #  ",inode_num);
#endif
	/* Fill the list of nodes pertaining to this file */
	while (offset < part->size - sizeof(struct jffs2_raw_inode)) {
	    inode = (struct jffs2_raw_inode *) (part->offset + offset); 
	    if (inode->magic == JFFS2_MAGIC_BITMASK &&
		hdr_crc((struct jffs2_unknown_node *)inode)) {
		// this is here to see if we are skipping someone due to a crc error.
		if (inode->nodetype == JFFS2_NODETYPE_INODE &&
		    inode->ino == inode_num && !inode_crc(inode)) {
		    D_PUTSTR("read_inode: found a sad CRC node\n");
		    D_PUTLABELEDWORD("\tversion  ",inode->version);		    
		    D_PUTLABELEDWORD("\toffset  ",inode->offset);
		    D_PUTLABELEDWORD("\tcsize  ",inode->csize);
		    D_PUTLABELEDWORD("\tdsize  ",inode->dsize);
		}
		if (inode->nodetype == JFFS2_NODETYPE_INODE &&
		    inode->ino == inode_num && inode_crc(inode)) {
				/* add it in newest first order to our list */
		    D_PUTSTR("read_inode: found a valid node\n");
		    D_PUTLABELEDWORD("\tversion  ",inode->version);		    
		    D_PUTLABELEDWORD("\toffset  ",inode->offset);
		    D_PUTLABELEDWORD("\tcsize  ",inode->csize);
		    D_PUTLABELEDWORD("\tdsize  ",inode->dsize);

		    
		    prev = NULL;
		    curr = first;
		    version = inode->version;
		    //list is ordered from highest version to lowest.
		    // version == inode # in files e.g. lowest is
		    //start of file and highest is end of file.
		    while (curr && curr->version > version) {
			prev = curr;
			curr = curr->next;
		    }
		    //new = free++;
		    if ((new = (struct data_strip *)  mmalloc(sizeof(struct data_strip))) ==
			NULL){
			D_PUTSTR("read_inode: MMALLOC FAILED in list build\n");
			return 0;
		    }
		    
		    new->version = version;
		    new->inode = inode;
		    new->page = (inode->offset / 4096);
		    new->length = inode->dsize;
		    if (!prev) {
			// highest version yet found
			new->next = first;
			first = new;
		    } else {
			//stick it where it goes
			new->next = prev->next;
			prev->next = new;
		    } 
		    
		    if (version > inode_version) {
			inode_version = version;
			// since we are using only this is always the isize
			size = type == DT_LNK ? inode->dsize : inode->isize;
		    }
		}// if type == JFFS2_NODETYPE_INODE && ours & valid
		offset += ((inode->totlen + 3) & ~3);
	    }// if its a jffs2 inode
	    else offset += 4;
	} // while scanning the partition
	
	
	if (!first) UDEBUG("could not find any nodes!\n");
#if 1
	D_PUTLABELEDWORD("total size #  ",size);
	D_PUTLABELEDWORD("decompressing pages #  ",size/4096);
	D_PUTLABELEDWORD("first =  ",first);
#endif

	// let's list them and see if they are all here?
	D_PUTSTR("let's check the ordered list\n");
	curr = first;
	while (curr) {
	    D_PUTSTR("read_inode: ordered list:\n");
	    D_PUTLABELEDWORD("\tversion  ",curr->inode->version);		    
	    D_PUTLABELEDWORD("\toffset  ",curr->inode->offset);
	    D_PUTLABELEDWORD("\tcsize  ",curr->inode->csize);
	    D_PUTLABELEDWORD("\tdsize  ",curr->inode->dsize);
	    D_PUTLABELEDWORD("\tisize  ",curr->inode->isize);
	    D_PUTLABELEDWORD("\tpage  ",curr->page);
	    curr = curr->next;
	}
		
     
#if 0
	for (page = 0; page <= size / 4096; page++) {
	    /* Print a '.' every 0x40000 bytes */
	    if (!(page & 0x3F)) ldr_update_progress();
	    if ((jffs2_uncompress_page((char *) dest, first, page)) <= 0) {
		UDEBUG("jffs2_uncompress_page failed on page 0x%lx\n", page);
		return 0;
	    }
	}	
#endif

	if ((ret = unordered_jffs2_uncompress_file((char *) dest,first)))
	    D_PUTLABELEDWORD("bavery_jffs2_uncompress_file FAILED with a ",ret);
	
	
	
	return size;
}

/* follow a symlink */
static struct jffs2_raw_dirent *do_symlink(struct part_info *part, 
	struct jffs2_raw_dirent *node)
{
	char name[JFFS2_MAX_NAME_LEN + 1];
	unsigned long size;
	
	UDEBUG(" finding symlink ");
	
	/* read in the symlink */
	if((size = read_inode(part, (u32 *) name, node->ino, DT_LNK))) {
		name[size] = '\0';
		UDEBUG(" '%s'\n", name);
		
		/* if it points to /, start over at inode 1, otherwise, stay
		 * in the same directory */
		if (name[0] == '/') return find_inode(part, name + 1, 1);
		else return find_inode(part, name, node->pino);
	} else {
		UDEBUG("could not load symlink!");
		return NULL;
	}
}

/* find a dirent with the name "name", and parent "pino" return NULL on 
 * error */
static struct jffs2_raw_dirent *find_inode(struct part_info *part, const char *name,
					   u32 pino)
{
	char curr_name[JFFS2_MAX_NAME_LEN + 1];
	int len, i, up = 0;
	struct jffs2_raw_dirent *node, *curr;
	unsigned long version, off;

	i = 0;
	do {
		/* parse the next dir/file/symlink */
		len = 0;
		while (name[i] != '\0' && name[i] != '/') 
			curr_name[len++] = name[i++];
			
		curr_name[len] = '\0';
		
		/* go up an inode if its a '..' */
		up = (len == 2 && !ldr_strncmp("..", curr_name, 2));
			
		UDEBUG("\nlooking for '%s' ...", curr_name);
		
		/* start again...at the beginning */
		version = off = 0;
		
		node = NULL;
		// start at the beginning of the partition
		while (off < part->size - sizeof(struct jffs2_raw_inode)) {
			curr = (struct jffs2_raw_dirent *)(part->offset + off);
			if (curr->magic == JFFS2_MAGIC_BITMASK && 
			    hdr_crc((struct jffs2_unknown_node *)curr)) {
				if (curr->nodetype == JFFS2_NODETYPE_DIRENT &&
				    up && curr->ino == pino && dirent_crc(curr) &&
				    curr->version > version) {
				    	version = curr->version;
				    	node = curr;
				} else if (curr->nodetype == JFFS2_NODETYPE_DIRENT &&
				    !up && curr->pino == pino &&
				    curr->nsize == len && dirent_crc(curr) &&
				    dirent_name_crc(curr) &&
				    !ldr_strncmp(curr_name, curr->name, len) &&
				    curr->version > version) {
				    	UDEBUG("found matching dirent\n");
					D_PUTSTR("find_inode: matching dirent\n");			
					D_PUTLABELEDWORD("\tnew version = ",curr->version);
					D_PUTLABELEDWORD("\tnew ino = ",curr->ino);
					D_PUTLABELEDWORD("\tnew pino = ",curr->pino);
				    	version = curr->version;
				    	node = curr;
				}
				// word aligns it.  are we word aligned?
				off += (curr->totlen + 3) & ~3;
			} else off += 4;
		}
		
	
		if (!node) return NULL;

		if (node->type == DT_LNK)
			if (!(node = do_symlink(part, node))) 
				return NULL;
		
		/* if we are going up a dir, we want to look for the parent */
		if (up) {
			pino = node->pino;
			up = 0;
		}
		else pino = node->ino;

		UDEBUG(" '%s' found at ino %ld\n", curr_name, pino);
		
		D_PUTLABELEDWORD("find_inode: found file at ino = ",node->ino);
		
		
	} while (name[i++] == '/');
	
	return node;	
}

static u32 jffs2_load_kernel(u32 *dest, struct part_info *part, const char *kernel_filename)
{
	struct jffs2_raw_dirent *node;
	
	if (!(node = find_inode(part, kernel_filename, 1))) {
		UDEBUG("could not find /linux\n");
		return 0;
	}
	if (node->type != DT_REG) {
		UDEBUG("/linux is not a regular file\n");
		return 0;
	}
	return read_inode(part, dest, node->ino, DT_REG);
}

u32 jffs2_test_load(u32 *dest, struct part_info *part, const char *filename)
{
	struct jffs2_raw_dirent *node;
	
	if (!(node = find_inode(part, filename, 1))) {
	    //UDEBUG("could not find /linux\n");
	    D_PUTSTR("could not find ");
	    D_PUTSTR(filename);	    
	    D_PUTSTR("\r\n ");
		return 0;
	}
	else {
#if 0
	    D_PUTLABELEDWORD("found node of type = ", node->type);	    
	    D_PUTLABELEDWORD("reg file type is = ", DT_REG);	    
#endif
	}
	

	if (node->type != DT_REG) {
		UDEBUG("/linux is not a regular file\n");
		return 0;
	}
	return read_inode(part, dest, node->ino, DT_REG);

}

u32 jffs2_scan_test( struct part_info *part)
{
    struct jffs2_raw_dirent *node;
    unsigned long version, off;
    unsigned long four_off_cnt,totlen_off_cnt,obsolete_cnt;
    
    
    obsolete_cnt = four_off_cnt = totlen_off_cnt = 0;    
    version = off = 0;
    // start at the beginning of the partition
    while (off < part->size - sizeof(struct jffs2_raw_inode)) {
	node = (struct jffs2_raw_dirent *)(part->offset + off);
	//putLabeledWord("Nodetype = ",node->nodetype);	
	if (node->magic == JFFS2_MAGIC_BITMASK &&
	    hdr_crc((struct jffs2_unknown_node *)node)){
	    totlen_off_cnt++;	    
	    off += ((node->totlen + 3) & ~3);
	    if (node->nodetype == JFFS2_NODETYPE_INODE)
		if(((struct jffs2_raw_inode *)node)->offset & 1) 
		    obsolete_cnt++;
	}	
	else{
	    four_off_cnt++;	    
	    off += 4;
	}
    }
    putLabeledWord("4 offset cnt = ",four_off_cnt);
    putLabeledWord("node offset cnt = ",totlen_off_cnt);
    putLabeledWord("obsolete cnt = ",obsolete_cnt);
    return 1;
}

struct b_node
{
    u32 offset;
    struct b_node *next;
};

struct b_lists
{
    char *partOffset;    
    struct b_node *dirListTail;
    struct b_node *dirListHead;
    u32 dirListCount;
    u32 dirListMemBase;    
    struct b_node *fragListTail;
    struct b_node *fragListHead;
    u32 fragListCount;
    u32 fragListMemBase;    
    
};
struct b_compr_info
{
    u32 num_frags;
    u32 compr_sum;
    u32 decompr_sum;
};

#define NUM_JFFS2_COMPR 7
struct b_jffs2_info 
{
    struct b_compr_info compr_info[NUM_JFFS2_COMPR];
};


static struct b_lists g_1PassList;
#define MALLOC_CHUNK (10*1024)  

// 1pass fxns
struct b_node *add_node(struct b_node *tail,u32 *count,u32 *memBase);
void jffs_init_1pass_list(void);
long jffs2_1pass_read_inode(struct b_lists *pL,u32 inode,char *dest);
u32 jffs2_1pass_find_inode(struct b_lists *pL,const char *name,u32 pino);
u32 jffs2_1pass_list_inodes(struct b_lists *pL,u32 pino);
u32 jffs2_1pass_resolve_inode(struct b_lists *pL,u32 ino);
u32 jffs2_1pass_search_inode(struct b_lists *pL,const char *fname,u32 pino);
u32 jffs2_1pass_search_list_inodes(struct b_lists *pL,const char *fname,u32 pino);
u32 jffs2_1pass_build_lists(struct part_info *part,struct b_lists *pL);
u32 jffs2_1pass_ls(struct part_info *part,const char *fname);
u32 jffs2_1pass_load(char *dest, struct part_info *part,const char *fname);
void jffs2_1pass_fill_info(struct b_lists *pL,struct b_jffs2_info *piL);
void jffs2_info(struct part_info *part);









struct b_node *add_node(struct b_node *tail,u32 *count,u32 *memBase)
{
    u32 index;
    u32 memLimit;
    struct b_node *b;
    
    index = (*count) * sizeof(struct b_node) % MALLOC_CHUNK;
    memLimit = MALLOC_CHUNK;

#if 0
    putLabeledWord("add_node: index = ",index);
    putLabeledWord("add_node: memLimit = ",memLimit);
    putLabeledWord("add_node: memBase = ",*memBase);
#endif
    
    // we need not keep a list of bases since we'll never free the
    // memory, just jump the the kernel
    if ( (index == 0) || (index > memLimit)){ // we need mode space before we continue
	if ((*memBase = (u32)  mmalloc(MALLOC_CHUNK)) ==
	    (u32) NULL){
	    putstr("add_node: malloc failed\n");
	    return NULL;
	}
#if 0
	putLabeledWord("add_node: alloced a new membase at ",*memBase);
#endif
	
    }
    
    // now we have room to add it.
    b = (struct b_node *)(*memBase + index);    

    // null on first call
    if (tail)
	tail->next = b;

#if 0
    putLabeledWord("add_node: tail = ",(u32) tail);
    if (tail)
	putLabeledWord("add_node: tail->next = ",(u32) tail->next);

#endif


#if 0
    putLabeledWord("add_node: mb+i = ",(u32) (*memBase+index));
    putLabeledWord("add_node: b = ",(u32) b);
#endif
    (*count)++;
    b->next = (struct b_node *) NULL;
    return b;
    
}

void jffs_init_1pass_list()
{
    g_1PassList.dirListHead = g_1PassList.dirListTail = NULL;
    g_1PassList.fragListHead = g_1PassList.fragListTail = NULL;
    g_1PassList.dirListCount = 0;
    g_1PassList.dirListMemBase = 0;
    g_1PassList.fragListCount = 0;
    g_1PassList.fragListMemBase = 0;
    g_1PassList.partOffset = 0x0;
}

// find the inode from the slashless name given a parent 
long jffs2_1pass_read_inode(struct b_lists *pL,u32 inode,char *dest)
{
    struct b_node *b;
    struct jffs2_raw_inode *jNode;
    u32 totalSize = 1;
    u32 oldTotalSize = 0;
    u32 size = 0;
    char *lDest = (char *) dest;    
    char *src;
    long ret;
    int i;
    u32 counter = 0;
    char totalSizeSet = 0;
    
    

#if 0
    b = pL->fragListHead;
    while (b){
	jNode = (struct jffs2_raw_inode *) (b->offset);
	if ((inode == jNode->ino)){
	    putLabeledWord("\r\n\r\nread_inode: totlen = ",jNode->totlen);
	    putLabeledWord("read_inode: inode = ",jNode->ino);
	    putLabeledWord("read_inode: version = ",jNode->version);
	    putLabeledWord("read_inode: isize = ",jNode->isize);
	    putLabeledWord("read_inode: offset = ",jNode->offset);
	    putLabeledWord("read_inode: csize = ",jNode->csize);
	    putLabeledWord("read_inode: dsize = ",jNode->dsize);
	    putLabeledWord("read_inode: compr = ",jNode->compr);
	    putLabeledWord("read_inode: usercompr = ",jNode->usercompr);
	    putLabeledWord("read_inode: flags = ",jNode->flags);
	}
	
	b = b->next;
    }
    
#endif

#if 1
    b = pL->fragListHead;
    while (b && (size < totalSize)){
	jNode = (struct jffs2_raw_inode *) (b->offset);
	if ((inode == jNode->ino)){
	    if ((jNode->isize == oldTotalSize) && (jNode->isize > totalSize) ){
		// 2 consecutive isizes indicate file length
		totalSize = jNode->isize;
		totalSizeSet = 1;
	    }
	    else if (!totalSizeSet){
		totalSize = size + jNode->dsize+1;
	    }
	    oldTotalSize = jNode->isize;
	    
	    src = ((char *) jNode) + sizeof(struct jffs2_raw_inode);
	    //lDest = (char *) (dest + (jNode->offset & ~3));
	    lDest = (char *) (dest + jNode->offset);	    
#if 0
	    putLabeledWord("\r\n\r\nread_inode: src = ",src);
	    putLabeledWord("read_inode: dest = ",lDest);
	    putLabeledWord("read_inode: dsize = ",jNode->dsize);
	    putLabeledWord("read_inode: csize = ",jNode->csize);	    
	    putLabeledWord("read_inode: version = ",jNode->version);
	    putLabeledWord("read_inode: isize = ",jNode->isize);
	    putLabeledWord("read_inode: offset = ",jNode->offset);
	    putLabeledWord("read_inode: compr = ",jNode->compr);
	    putLabeledWord("read_inode: flags = ",jNode->flags);
#endif
	    switch (jNode->compr) {
		case JFFS2_COMPR_NONE:
#if 0
		{
		    int i;
		    if ((dest > 0xc0092ff0) && (dest < 0xc0093000))
			for (i=0; i < first->length; i++){
			    putLabeledWord("\tCOMPR_NONE: src =",src+i);
			    putLabeledWord("\tCOMPR_NONE: length =",first->length);
			    putLabeledWord("\tCOMPR_NONE: dest =",dest+i);
			    putLabeledWord("\tCOMPR_NONE: data =",(unsigned char) *(src+i));
			}
		}
#endif
	    
		    ret = (unsigned long) ldr_memcpy(lDest, src, jNode->dsize);
		    break;
		case JFFS2_COMPR_ZERO:
		    ret = 0;		    
		    for (i = 0; i < jNode->dsize; i++) *(lDest++) = 0;
		    break;
		case JFFS2_COMPR_RTIME:
		    ret = 0;		    
		    rtime_decompress(src, lDest, jNode->csize, 
				     jNode->dsize);
		    break;
		case JFFS2_COMPR_DYNRUBIN:
		    // this is slow but it works
		    ret = 0;		    		    
		    dynrubin_decompress(src, lDest, jNode->csize, 
					jNode->dsize);
		    break;
		case JFFS2_COMPR_ZLIB:
		    ret = zlib_decompress(src, lDest, jNode->csize, jNode->dsize);
		    break;
		default:
		    /* unknown */
		    putLabeledWord("UNKOWN COMPRESSION METHOD = ",jNode->compr);
		    return -1;
		    break;
	    }
	    
	    size += jNode->dsize;	    
#if 0
	    putLabeledWord("read_inode: size = ",size);
	    putLabeledWord("read_inode: totalSize = ",totalSize);
	    putLabeledWord("read_inode: compr ret = ",ret);	    
#endif
	}
	b = b->next;
	counter++;
    }
#endif

#if 0
    putLabeledWord("read_inode: returning = ",size);
#endif
    return size;
}
// find the inode from the slashless name given a parent 
u32 jffs2_1pass_find_inode(struct b_lists *pL,const char *name,u32 pino)
{
    struct b_node *b;
    struct jffs2_raw_dirent *jDir;
    int len;
    u32 counter;
    u32 version = 0;
    u32 inode = 0;
    
    
    // name is assumed slash free
    len = strlen(name);

    b = pL->dirListHead;
    counter = 0;    
    // we need to search all and return the inode with the highest version
    while (b){
	jDir = (struct jffs2_raw_dirent *) (b->offset);
	if ((pino == jDir->pino) &&
	    (len == jDir->nsize) &&
	    (jDir->ino) &&  // 0 for unlink
	    (!strncmp(jDir->name,name,len)))// a match
	    if (jDir->version >= version){
		inode =  jDir->ino;
		version = jDir->version;
	    }
#if 0
	putstr("\r\nfind_inode:p&l ->");putnstr(jDir->name,jDir->nsize);putstr("\r\n");
	putLabeledWord("pino = ",jDir->pino);
	putLabeledWord("nsize = ",jDir->nsize);
	putLabeledWord("b = ",(u32)b);
	putLabeledWord("counter = ",counter);
#endif
	counter++;
	b = b->next;
    }
    return inode;
}

// list inodes witht the given pino
u32 jffs2_1pass_list_inodes(struct b_lists *pL,u32 pino)
{
    struct b_node *b;
    struct jffs2_raw_dirent *jDir;
    u32 counter;
    
    b = pL->dirListHead;
    counter = 0;    
    while (b){
	jDir = (struct jffs2_raw_dirent *) (b->offset);
	if ((pino == jDir->pino) && (jDir->ino)){ // 0 inode for unlink
	    putnstr(jDir->name,jDir->nsize);putstr("\r\n");

//#define SHOW_LINK
#if SHOW_LINK
	    if (jDir->type == DT_LNK){
	            struct b_node *b2;
		    b2 = pL->fragListHead;
		    while (b2){
		        struct jffs2_raw_inode *jNode;
			jNode = (struct jffs2_raw_inode *) (b2->offset);
			if (jNode->ino == jDir-> ino){
			    unsigned char * src;
			    src = (unsigned char *) (b2->offset+sizeof(struct jffs2_raw_inode));
			    
			    putstr("\t-----Symlink info--------\r\n");
			    putLabeledWord("\t\t dsize = ",jNode->dsize);
			    putstr("\t\t target = ");
			    putnstr(src,jNode->dsize);putstr("\r\n");
			}
			b2 = b2->next;
		    }
	    }
#endif // SHOW_LINK

	}
	counter++;
	b = b->next;
    }
    return pino;
}

u32 jffs2_1pass_resolve_inode(struct b_lists *pL,u32 ino)
{
    struct b_node *b;
    struct b_node *b2;
    struct jffs2_raw_dirent *jDir;
    struct jffs2_raw_inode *jNode;
    struct jffs2_raw_dirent *jDirFound = NULL;
    char tmp[256];
    u32 version = 0;
    u32 pino;
    unsigned char *src;
    

    b = pL->dirListHead;
    // we need to search all and return the inode with the highest version
    while (b){
	jDir = (struct jffs2_raw_dirent *) (b->offset);
	if (ino == jDir->ino)
	    if (jDir->version > version){
		jDirFound =  jDir;
		version = jDir->version;
	    }	
	b = b->next;
    }
    // now we found the right entry again. (shoulda returned inode*)
    if (jDirFound && (jDirFound->type != DT_LNK))
	return jDirFound->ino;
    // so its a soft link so we follow it again.
    b2 = pL->fragListHead;
    while (b2){
	jNode = (struct jffs2_raw_inode *) (b2->offset);
	if (jNode->ino == jDirFound-> ino){
	    src = (unsigned char *) (b2->offset+sizeof(struct jffs2_raw_inode));

#if 0
	    putLabeledWord("\t\t dsize = ",jNode->dsize);
	    putstr("\t\t target = ");
	    putnstr(src,jNode->dsize);putstr("\r\n");
#endif
	    strncpy(tmp,src,jNode->dsize);
	    tmp[jNode->dsize]='\0';
	    break;
	}
	b2 = b2->next;
    }
    // ok so the name of the new file to find is in tmp
    // if it starts with a slash it is root based else shared dirs
    if (tmp[0] == '/')
	pino = 1;
    else
	pino = jDirFound->pino;
    
    return jffs2_1pass_search_inode(pL,tmp,pino);
}





u32 jffs2_1pass_search_inode(struct b_lists *pL,const char *fname,u32 pino)
{
    int i;    
    char tmp[256];
    char working_tmp[256];
    char *c;

    
    // discard any leading slash
    i=0;
    while (fname[i] == '/')
	i++;
    strcpy(tmp,&fname[i]);
    

    while ((c = (char *) strchr(tmp,'/'))) // we are still dired searching
    {
	strncpy(working_tmp,tmp,c - tmp);
	working_tmp[c - tmp] = '\0';
#if 0
	putstr("search_inode: tmp = ");putstr(tmp);putstr("\r\n");
	putstr("search_inode: wtmp = ");putstr(working_tmp);putstr("\r\n");
	putstr("search_inode: c = ");putstr(c);putstr("\r\n");
#endif
	for (i=0; i < strlen(c) - 1;i++)
	    tmp[i] = c[i+1];
	tmp[i] = '\0';
#if 0
	putstr("search_inode: post tmp = ");putstr(tmp);putstr("\r\n");
#endif
	

	if (!(pino = jffs2_1pass_find_inode(pL,working_tmp,pino))){
	    putstr("find_inode failed for name=");putstr(working_tmp);putstr("\r\n");
	    return 0;
	}
    }
    // this is for the bare filename, directories have already been mapped
    if (!(pino = jffs2_1pass_find_inode(pL,tmp,pino))){
	putstr("find_inode failed for name=");putstr(tmp);putstr("\r\n");
	return 0;
    }
    return pino;

}


u32 jffs2_1pass_search_list_inodes(struct b_lists *pL,const char *fname,u32 pino)
{
    int i;    
    char tmp[256];
    char working_tmp[256];
    char *c;

    
    // discard any leading slash
    i=0;
    while (fname[i] == '/')
	i++;
    strcpy(tmp,&fname[i]);
    working_tmp[0] = '\0';
    while ((c = (char *) strchr(tmp,'/'))) // we are still dired searching
    {
	strncpy(working_tmp,tmp,c - tmp);
	working_tmp[c - tmp] = '\0';
	for (i=0; i < strlen(c) - 1;i++)
	    tmp[i] = c[i+1];
	tmp[i] = '\0';
	// only a failure if we arent looking at top level
	if (!(pino = jffs2_1pass_find_inode(pL,working_tmp,pino)) && (working_tmp[0])){
	    putstr("find_inode failed for name=");putstr(working_tmp);putstr("\r\n");
	    return 0;
	}
    }

    if (tmp[0] && !(pino = jffs2_1pass_find_inode(pL,tmp,pino))){
	putstr("find_inode failed for name=");putstr(tmp);putstr("\r\n");
	return 0;
    }

    // this is for the bare filename, directories have already been mapped
    if (!(pino = jffs2_1pass_list_inodes(pL,pino))){
	putstr("find_inode failed for name=");putstr(tmp);putstr("\r\n");
	return 0;
    }
    return pino;

}

// we know we have empties at the start offset so we will hop
// t points that would be non F if there were a node here to speed this up.
struct jffs2_empty_node
{
    u32 first;
    u32 second;
};


u32 jffs2_scan_empty(u32 start_offset,struct part_info *part)
{
    u32 max = part->size - sizeof(struct jffs2_raw_inode);
    // this would be either dir node_crc or frag isize
    u32 offset = start_offset + 32;
    struct jffs2_empty_node *node;

    start_offset += 4;    
    while (offset < max){
	node = (struct jffs2_empty_node *)(part->offset + offset);
	if ((node->first == 0xFFFFFFFF) && (node->second == 0xFFFFFFFF)){
	    // we presume that there were no nodes in between and advance in a hop
	    //putLabeledWord("\t\tjffs2_scan_empty: empty at offset=",offset);	    
	    start_offset = offset + 4;
	    offset = start_offset + 32; // orig 32 + 4 bytes for the second==0xfffff
	}
	else{
	    return start_offset;
	}
    }
    return start_offset;
}

unsigned char  jffs2_1pass_rescan_needed(struct part_info *part,struct b_lists *pL)
{
    unsigned char ret = 0;
    struct b_node *b;
    struct jffs2_unknown_node *node;

    // if we have no list, we need to rescan
    if (pL->fragListCount == 0)
	return 1;
    // or if we are scanninga new partition
    if (pL->partOffset != part->offset) 
	return 1;
    // but suppose someone reflashed the root partition at the same offset...
    b = pL->dirListHead;
    while (b){
	node = (struct jffs2_unknown_node *) (b->offset);
	if (node->nodetype != JFFS2_NODETYPE_DIRENT)
	    return 1;
	b= b->next;
    }
    return ret;
}



u32 jffs2_1pass_build_lists(struct part_info *part,struct b_lists *pL)
{
    struct jffs2_unknown_node *node;
    u32 offset;
    u32 max = part->size - sizeof(struct jffs2_raw_inode);
    u32 counter = 0;
    u32 counter4 = 0;
    u32 counterF = 0;
    int last_button = 0;
    
    // turn off the lcd.  Refreshing the lcd adds 50% overhead to the
   // jffs2 list building enterprise nope.  in newer versions the overhead is
    // only about 5 %.  not enough to inconvenience people for.
    // lcd_off();

    
    //if we are building a list we need to refresh the cache.
    // note that since we don't free our memory, eventually this will be bad.
    // but we're a bootldr so what the hell.
    jffs_init_1pass_list();    
    pL->partOffset = part->offset;    
    offset = 0;
    putstr("Scanning all of flash for your JFFS2 convenience. Est Time: 6-12 seconds!\r\n");
    // give visual feedback that we are scanning the flash
#if CONFIG_LCD
    led_blink(0x1,0x0,0x1,0x1); // on, forever, on 100ms, off 100ms
#endif

    // start at the beginning of the partition
    putLabeledWord("build_list: max = ",max);
    // ok, buttons. we check and latch them but don't execute until the
    // end so you can't Q up more than 1 buttonpress
    // also will exec the fxns in your params file rather than the default.
#if !defined(CONFIG_MACH_SKIFF)
    set_exec_buttons_automatically(0);
    set_last_buttonpress(0);
#endif // CONFIG_MACH_SKIFF

    while (offset < max) {
	if (!(counter++ % 10000)){
	    putstr(".");
#if LCD_CONFIG
	    lcd_bar(0xFFF, (offset * 100) / max);
#endif

#if !defined(CONFIG_MACH_SKIFF)
	    button_check();
	    if (get_last_buttonpress() > 0){
		// unhilite the old
		if (last_button > 0)
		    hilite_button(last_button);
		// hilite the new
		last_button = get_last_buttonpress();
		hilite_button(last_button);		    
		set_last_buttonpress(0);
	    }
#endif // !defined(CONFIG_MACH_SKIFF)            
	}
	
	if (!(counter++ % (80*10000)))
	    putstr("\r\n");
	if (!(counter++ % (80*10000)))
	    putLabeledWord("Offset = ",offset);
	
	
	node = (struct jffs2_unknown_node *)(part->offset + offset);
	if (node->magic == JFFS2_MAGIC_BITMASK){
	    // if its a fragment add it
	    if (node->nodetype == JFFS2_NODETYPE_INODE && 
		inode_crc((struct jffs2_raw_inode *)node)){
		if (!(pL->fragListTail = add_node(pL->fragListTail,&(pL->fragListCount),
						 &(pL->fragListMemBase)))){
		    putstr("add_node failed!\r\n");
		    return 0;
		}
		pL->fragListTail->offset = (u32) (part->offset + offset);		
		if (!pL->fragListHead)
		    pL->fragListHead = pL->fragListTail;		    
	    }
	    else if (node->nodetype == JFFS2_NODETYPE_DIRENT && 
		     dirent_crc((struct jffs2_raw_dirent *)node) &&
		     dirent_name_crc((struct jffs2_raw_dirent *)node)){
#if 0
		putstr("\r\nbuild_lists:p&l ->");putnstr(((struct jffs2_raw_dirent *)node)->name,((struct jffs2_raw_dirent *)node)->nsize);putstr("\r\n");
		putLabeledWord("\tpino = ",((struct jffs2_raw_dirent *)node)->pino);
		putLabeledWord("\tnsize = ",((struct jffs2_raw_dirent *)node)->nsize);
#endif

		if (!(pL->dirListTail = add_node(pL->dirListTail,&(pL->dirListCount),
						 &(pL->dirListMemBase)))){
		    putstr("add_node failed!\r\n");
		    return 0;
		}
		pL->dirListTail->offset = (u32) (part->offset + offset);
#if 0
		putLabeledWord("\ttail = ",(u32) pL->dirListTail);
		putstr("\ttailName ->");putnstr(((struct jffs2_raw_dirent *)(pL->dirListTail->offset))->name,((struct jffs2_raw_dirent *)(pL->dirListTail->offset))->nsize);putstr("\r\n");
#endif
		if (!pL->dirListHead)
		    pL->dirListHead = pL->dirListTail;		    
		
	    }
	    offset += ((node->totlen + 3) & ~3);
	    counterF++;
	    
	}
	else if ((node->magic == JFFS2_EMPTY_BITMASK) && (node->magic == JFFS2_EMPTY_BITMASK)){
	    //putLabeledWord("build_list: Empty entry at offset =",offset);	    
	    offset = jffs2_scan_empty(offset,part);
	    //putLabeledWord("build_list: Empty returned at offset =",offset);	    
	}
	else{ // if we know nothing of the filesystem, we just step and look.
	    offset += 4;
	    counter4++;
	    
	}
    }

    putstr("\r\n"); // close off the dots
    // turn the lcd back on.
    //splash();

#if 1
    putLabeledWord("dir entries = ",pL->dirListCount);
    putLabeledWord("frag entries = ",pL->fragListCount);
    putLabeledWord("+4 increments = ",counter4);
    putLabeledWord("+file_offset increments = ",counterF);

#endif

//#define SHOW_ALL
#ifdef SHOW_ALL
    {
        struct b_node *b;
	struct b_node *b2;
	struct jffs2_raw_dirent *jDir;
	struct jffs2_raw_inode *jNode;

	putstr("\r\n\r\n******The directory Entries******\r\n");
	b = pL->dirListHead;
	while (b){
	    jDir = (struct jffs2_raw_dirent *) (b->offset);
	    putstr("\r\n");
	    putnstr(jDir->name,jDir->nsize);
	    putLabeledWord("\r\n\tbuild_list: magic = ",jDir->magic);
	    putLabeledWord("\tbuild_list: nodetype = ",jDir->nodetype);
	    putLabeledWord("\tbuild_list: hdr_crc = ",jDir->hdr_crc);
	    putLabeledWord("\tbuild_list: pino = ",jDir->pino);
	    putLabeledWord("\tbuild_list: version = ",jDir->version);
	    putLabeledWord("\tbuild_list: ino = ",jDir->ino);
	    putLabeledWord("\tbuild_list: mctime = ",jDir->mctime);
	    putLabeledWord("\tbuild_list: nsize = ",jDir->nsize);
	    putLabeledWord("\tbuild_list: type = ",jDir->type);
	    putLabeledWord("\tbuild_list: node_crc = ",jDir->node_crc);
	    putLabeledWord("\tbuild_list: name_crc = ",jDir->name_crc);
	    b = b->next;
	}
    

	putstr("\r\n\r\n******The fragment Entries******\r\n");
	b = pL->fragListHead;
	while (b){
	    jNode = (struct jffs2_raw_inode *) (b->offset);
	    putLabeledWord("\r\n\tbuild_list: FLASH_OFFSET = ",b->offset);
	    putLabeledWord("\tbuild_list: totlen = ",jNode->totlen);
	    putLabeledWord("\tbuild_list: inode = ",jNode->ino);
	    putLabeledWord("\tbuild_list: version = ",jNode->version);
	    putLabeledWord("\tbuild_list: isize = ",jNode->isize);
	    putLabeledWord("\tbuild_list: atime = ",jNode->atime);
	    putLabeledWord("\tbuild_list: offset = ",jNode->offset);
	    putLabeledWord("\tbuild_list: csize = ",jNode->csize);
	    putLabeledWord("\tbuild_list: dsize = ",jNode->dsize);
	    putLabeledWord("\tbuild_list: compr = ",jNode->compr);
	    putLabeledWord("\tbuild_list: usercompr = ",jNode->usercompr);
	    putLabeledWord("\tbuild_list: flags = ",jNode->flags);
	    b = b->next;
	}
    
    }
    
#endif // SHOW_ALL
    // give visual feedback that we are done scanning the flash
#if CONFIG_LCD
    led_blink(0x0,0x0,0x1,0x1); // off, forever, on 100ms, off 100ms
#endif
    //reset the screen
#if LCD_CONFIG
    lcd_bar_clear();
#endif


#if !defined(CONFIG_MACH_SKIFF)
  
    // exec whatever button was pressed and unhilite it.
    if (last_button > 0){
	hilite_button(last_button);
	exec_button(last_button);
    }
    // return to normal processing mode.
    set_exec_buttons_automatically(1);
#endif // !defined(CONFIG_MACH_SKIFF)    
    return 1;    
}




u32 jffs2_1pass_ls(struct part_info *part,const char *fname)
{

    struct b_lists *pL;
    u32 inode;
    long ret = 0;
    
    pL = &g_1PassList;

#if 0
    putLabeledWord("j1ls: fragListCount = ",pL->fragListCount);
    putLabeledWord("j1ls: pl->offset = ",pL->partOffset);
    putLabeledWord("j1ls: part->offset = ",part->offset);
#endif
    
    //if ((pL->fragListCount == 0) || (pL->partOffset != part->offset)) // build the list if we are first
    if (jffs2_1pass_rescan_needed(part,pL))
	if (!jffs2_1pass_build_lists(part,pL)){
	    putstr("Failed to scan jffs2 file structure\r\n");
	    return 0;
	}
    
    if (!(inode = jffs2_1pass_search_list_inodes(pL,fname,1))){
	putstr("Failed to scan jffs2 file structure\r\n");
	return 0;
    }
    
#if 0
    putLabeledWord("found file at inode = ",inode);
    putLabeledWord("read_inode returns = ",ret);
#endif

    return ret;
}


u32 jffs2_1pass_load(char *dest, struct part_info *part,const char *fname)
{

    struct b_lists *pL;
    u32 inode;
    long ret = 0;
    
    pL = &g_1PassList;

#if 0
    putLabeledWord("j1l: fragListCount = ",pL->fragListCount);
    putLabeledWord("j1l: pl->offset = ",pL->partOffset);
    putLabeledWord("j1l: part->offset = ",part->offset);
#endif
    
    
    //if ((pL->fragListCount == 0) || (pL->partOffset != part->offset)) // build the list if we are first
    if (jffs2_1pass_rescan_needed(part,pL))
	if (!jffs2_1pass_build_lists(part,pL)){
	    putstr("Failed to scan jffs2 file structure\r\n");
	    return 0;
	}
    
    if (!(inode = jffs2_1pass_search_inode(pL,fname,1))){
	putstr("Failed to find inode \r\n");
	return 0;
    }
    // ok, but suppose the entry we found was a softlink
    // rather than a file
#if 1
    if (!(inode = jffs2_1pass_resolve_inode(pL,inode))){
	putstr("Failed to resolve inode file structure\r\n");
	return 0;
    }
#endif
    
    
    if ((ret = jffs2_1pass_read_inode(pL,inode,dest)) < 0 ){
	putstr("Failed to read inode\r\n");
	return 0;
    }

    
#if 0
    putLabeledWord("found file at inode = ",inode);
    putLabeledWord("read_inode returns = ",ret);
#endif

    return ret;
}

void jffs2_1pass_fill_info(struct b_lists *pL,struct b_jffs2_info *piL)
{
    struct b_node *b;
    struct jffs2_raw_inode *jNode;
    int i;
    
    b = pL->fragListHead;
    for (i=0; i < NUM_JFFS2_COMPR; i++){
	piL->compr_info[i].num_frags=0;
	piL->compr_info[i].compr_sum=0;
	piL->compr_info[i].decompr_sum=0;
    }
    
    while (b){
	jNode = (struct jffs2_raw_inode *) (b->offset);
	if (jNode->compr < NUM_JFFS2_COMPR)
	{
	    piL->compr_info[jNode->compr].num_frags++;
	    piL->compr_info[jNode->compr].compr_sum += jNode->csize;
	    piL->compr_info[jNode->compr].decompr_sum += jNode->dsize;
	}
	b = b->next;
    }
}





void jffs2_info(struct part_info *part)
{
    struct b_jffs2_info iL;
    struct b_jffs2_info *piL;
    struct b_lists *pL;
    int i;

    piL = &iL;
    pL = &g_1PassList;
	
    //if ((pL->fragListCount == 0) || (pL->partOffset != part->offset)) // build the list if we are first
    if (jffs2_1pass_rescan_needed(part,pL))
	if (!jffs2_1pass_build_lists(part,pL)){
	    putstr("Failed to scan jffs2 file structure\r\n");
	    return;
	}
    jffs2_1pass_fill_info(pL,piL);
    i=0;    
    putstr("Compr = NONE:\r\n");
    putLabeledWord("\tfrag count: ",piL->compr_info[i].num_frags);
    putLabeledWord("\tcompressed sum: ",piL->compr_info[i].compr_sum);
    putLabeledWord("\tuncompressed sum: ",piL->compr_info[i].decompr_sum);
    i++;
    
    putstr("Compr = ZERO:\r\n");
    putLabeledWord("\tfrag count: ",piL->compr_info[i].num_frags);
    putLabeledWord("\tcompressed sum: ",piL->compr_info[i].compr_sum);
    putLabeledWord("\tuncompressed sum: ",piL->compr_info[i].decompr_sum);
    i++;
    
    putstr("Compr = RTIME:\r\n");
    putLabeledWord("\tfrag count: ",piL->compr_info[i].num_frags);
    putLabeledWord("\tcompressed sum: ",piL->compr_info[i].compr_sum);
    putLabeledWord("\tuncompressed sum: ",piL->compr_info[i].decompr_sum);
    i++;
    
    putstr("Compr = RUBINMIPS:\r\n");
    putLabeledWord("\tfrag count: ",piL->compr_info[i].num_frags);
    putLabeledWord("\tcompressed sum: ",piL->compr_info[i].compr_sum);
    putLabeledWord("\tuncompressed sum: ",piL->compr_info[i].decompr_sum);
    i++;

    putstr("Compr = COPY:\r\n");
    putLabeledWord("\tfrag count: ",piL->compr_info[i].num_frags);
    putLabeledWord("\tcompressed sum: ",piL->compr_info[i].compr_sum);
    putLabeledWord("\tuncompressed sum: ",piL->compr_info[i].decompr_sum);
    i++;

    putstr("Compr = DYNRUBIN:\r\n");
    putLabeledWord("\tfrag count: ",piL->compr_info[i].num_frags);
    putLabeledWord("\tcompressed sum: ",piL->compr_info[i].compr_sum);
    putLabeledWord("\tuncompressed sum: ",piL->compr_info[i].decompr_sum);
    i++;

    putstr("Compr = ZLIB:\r\n");
    putLabeledWord("\tfrag count: ",piL->compr_info[i].num_frags);
    putLabeledWord("\tcompressed sum: ",piL->compr_info[i].compr_sum);
    putLabeledWord("\tuncompressed sum: ",piL->compr_info[i].decompr_sum);
    i++;
    
}


struct kernel_loader jffs2_load = {
	check_magic: jffs2_check_magic,
	load_kernel: jffs2_load_kernel,
	name:	     "jffs2"
};
	
