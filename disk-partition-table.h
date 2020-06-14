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
 * DOS-style Disk Partition Table 
 */

#ifndef _DOS_PARTITION_TABLE_
#define _DOS_PARTITION_TABLE_

struct dos_ptable_entry {
   unsigned char is_active_partition;
   unsigned char starting_head;
   unsigned char starting_sector;
   unsigned char starting_cylinder;
   unsigned char partition_type;
   unsigned char ending_head;
   unsigned char ending_sector;
   unsigned char ending_cylinder;
   unsigned long starting_sector_lba;
   unsigned long n_sectors; 
};

#endif
