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
 * IDE drive support
 */

#include "disk-partition-table.h" 

extern int ide_attach(volatile char *ioport);
extern int ide_detach(volatile char *ioport);
extern int ide_read(char *buf, unsigned long offset, unsigned long len);
int ide_read_ptable(struct dos_ptable_entry *ptable);
int ide_read_partition(char *buf, int partid, size_t *lenp);
int ide_get_partition_start_byte(int partid);
int ide_get_partition_n_bytes(int partid);
struct iohandle *get_ide_partition_iohandle(u8 partid);
extern void command_ide(int argc, const char* argv[]);

