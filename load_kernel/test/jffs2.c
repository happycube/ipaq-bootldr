/*
-------------------------------------------------------------------------
 * Filename:      jffs2.c
 * Version:       $Id: jffs2.c,v 1.1 2001/08/14 21:17:40 jamey Exp $
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
 * $Id: jffs2.c,v 1.1 2001/08/14 21:17:40 jamey Exp $
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

static struct jffs2_raw_dirent *find_inode(struct part_info *part, const char *name,
					   u32 pino);
void rtime_decompress   (unsigned char *data_in, unsigned char *cpage_out, 
		         __u32 srclen, __u32 destlen);
void dynrubin_decompress(unsigned char *data_in, unsigned char *cpage_out,
			 __u32 srclen, __u32 destlen);
void zlib_decompress    (unsigned char *data_in, unsigned char *cpage_out,
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

	/* look for the next reference to this page */
	for (; first && first->page != page; first = first->next);
	if (!first) return 1;
	
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
	
	}
	return 1;
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
	unsigned long offset, inode_version, version, page;
	struct data_strip *free, *curr, *prev, *first, *new;
	
	free = (struct data_strip *) fodder_ram_base;
	first = NULL;
	
	offset = size = inode_version = 0;

	/* Fill the list of nodes pertaining to this file */
	while (offset < part->size - sizeof(struct jffs2_raw_inode)) {
		inode = (struct jffs2_raw_inode *) (part->offset + offset); 
		if (inode->magic == JFFS2_MAGIC_BITMASK && hdr_crc(inode)) {
			if (inode->nodetype == JFFS2_NODETYPE_INODE &&
			    inode->ino == inode_num && inode_crc(inode)) {
				/* add it in newest first order to our list */
				prev = NULL;
				curr = first;
				version = inode->version;
				while (curr && curr->version > version) {
					prev = curr;
					curr = curr->next;
				}
				new = free++;
				new->version = version;
				new->inode = inode;
				new->page = (inode->offset / 4096);
				new->length = inode->dsize;
				if (!prev) {
					new->next = first;
					first = new;
				} else {
					new->next = prev->next;
					prev->next = new;
				} 
				if (version > inode_version) {
					inode_version = version;
					size = type == DT_LNK ? inode->dsize : inode->isize;
				}
			}
			offset += ((inode->totlen + 3) & ~3);
		} else offset += 4;
	}
	
	if (!first) UDEBUG("could not find any nodes!\n");

	for (page = 0; page <= size / 4096; page++) {
		/* Print a '.' every 0x40000 bytes */
		if (!(page & 0x3F)) ldr_update_progress();
		if ((jffs2_uncompress_page((char *) dest, first, page)) <= 0) {
			UDEBUG("jffs2_uncompress_page failed on page 0x%lx\n", page);
			return 0;
		}
	}
	
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
		while (off < part->size - sizeof(struct jffs2_raw_inode)) {
			curr = (struct jffs2_raw_dirent *)(part->offset + off);
			if (curr->magic == JFFS2_MAGIC_BITMASK && 
			    hdr_crc(curr)) {
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
				    	version = curr->version;
				    	node = curr;
				} 
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

struct kernel_loader jffs2_load = {
	check_magic: jffs2_check_magic,
	load_kernel: jffs2_load_kernel,
	name:	     "jffs2"
};
	
