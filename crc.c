/* CRC-16 and CRC-32 support for bootldr
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 * Copyright (C) 2001  John G Dorsey
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
 * 13 March, 2001 - created by separating from jffs.c and ymodem.c. (jd)
 *
 */

#ifndef CONFIG_ACCEPT_GPL
#error This file covered by GPL but CONFIG_ACCEPT_GPL undefined.
#endif

#include "crc.h"
#include "bootldr.h"

#undef CRC32_INCLUDED			// zlib has a better crc32

static unsigned short *crc16_table = NULL;
#ifdef CRC32_INCLUDED
static unsigned int   *crc32_table = NULL;
#endif

/* Generate the table of constants used in executing the CRC32 algorithm: */
int crc16_init(void){
  int i, j;
  unsigned short crc;

  if(crc16_table == NULL){

    /* This table is currently _not_ freed: */
    if((crc16_table = 
	(unsigned short *)mmalloc(CRC_TABLE_SIZE *
				  sizeof(unsigned short))) == NULL)
      return -1;

    for(i = 0; i < CRC_TABLE_SIZE; ++i){

      crc = i << 8;

      for(j = 8; j > 0; --j){

	if(crc & 0x8000)
	  crc = (crc << 1) ^ CRC16_POLYNOMIAL;
	else
	  crc <<= 1;

      }

      crc16_table[i] = crc;

    }

  }

  return 0;
}

#ifdef CRC32_INCLUDED
/* Generate the table of constants used in executing the CRC32 algorithm: */
int crc32_init(void){
  int i, j;
  unsigned int crc;

  if(crc32_table == NULL){

    /* This table is currently _not_ freed: */
    if((crc32_table = 
	(unsigned int *)mmalloc(CRC_TABLE_SIZE *
				sizeof(unsigned int))) == NULL)
      return -1;
    
    for(i = 0; i < CRC_TABLE_SIZE; ++i){
      
      crc = i;
      
      for(j = 8; j > 0; --j){
	
	if(crc & 0x1)
	  crc = (crc >> 1) ^ CRC32_POLYNOMIAL;
	else
	  crc >>= 1;
	
      }
      
      crc32_table[i] = crc;
      
    }

  }

  return 0;
}
#endif

/* Perform a CRC16 computation over `buf'. This method was derived from
 * an algorithm (C) 1986 by Gary S. Brown, and was checked against an
 * implementation (C) 2000 by Compaq Computer Corporation, authored by
 * George France.
 */
unsigned short crc16_buf(unsigned char *buf, unsigned int length){
  unsigned short crc = 0;

  while(length-- > 0)
    crc = crc16_table[(crc >> 8) & 0xff] ^ (crc << 8) ^ *buf++;

  return crc;
}

#ifdef CRC32_INCLUDED
/* Perform a CRC32 computation over `buf'. This method was derived from
 * an algorithm (C) 1986 by Gary S. Brown.
 */
unsigned int crc32_buf(unsigned char *buf, unsigned int length){
  unsigned int crc = 0;

  while(length-- > 0)
    crc = crc32_table[(crc ^ *buf++) & 0xff] ^ (crc >> 8);

  return crc;
}
#endif

