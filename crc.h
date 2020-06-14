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
 */

#ifndef CONFIG_ACCEPT_GPL
#error This file covered by GPL but CONFIG_ACCEPT_GPL undefined.
#endif

#if !defined(_CRC_H)
#define _CRC_H

/* Constants used in generating the CRC tables: */

#define CRC_TABLE_SIZE   (256)

#define CRC16_POLYNOMIAL (0x1021)
#define CRC32_POLYNOMIAL (0xedb88320)

extern int crc16_init(void);
extern int crc32_init(void);

extern unsigned short crc16_buf(unsigned char *buf, unsigned int length);
extern unsigned int   crc32_buf(unsigned char *buf, unsigned int length);

#endif  /* !defined(_CRC_H) */
