/* MD5 support for bootldr
 * ^^^^^^^^^^^^^^^^^^^^^^^
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
 */

#ifndef CONFIG_ACCEPT_GPL
#error This file covered by GPL but CONFIG_ACCEPT_GPL undefined.
#endif

#if !defined(_MD5_H)
#define _MD5_H

#define MD5_SUM_WORDS		(4)

#define MD5_LENGTH_MODULUS	(512 / 8)
#define MD5_PREPAD_LEN_LENGTH	(64 / 8)

#define MD5_BLOCK_WORDS		(16)

#define MD5_WORD_A_INIT		(0x67452301)
#define MD5_WORD_B_INIT		(0xefcdab89)
#define MD5_WORD_C_INIT		(0x98badcfe)
#define MD5_WORD_D_INIT		(0x10325476)

/* The 64-element "T" table, constructed using the following:
 *
 * #include <math.h>
 * #include <stdio.h>
 *
 * int main(void){
 *   int i;
 *   for(i = 1; i <= 64; ++i)
 *     printf("  0x%08x, \\\n", (unsigned int )(4294967296.0 * fabs(sin(i))));
 * }
 */

#define MD5_T_INIT \
{ \
  0xd76aa478, \
  0xe8c7b756, \
  0x242070db, \
  0xc1bdceee, \
  0xf57c0faf, \
  0x4787c62a, \
  0xa8304613, \
  0xfd469501, \
  0x698098d8, \
  0x8b44f7af, \
  0xffff5bb1, \
  0x895cd7be, \
  0x6b901122, \
  0xfd987193, \
  0xa679438e, \
  0x49b40821, \
  0xf61e2562, \
  0xc040b340, \
  0x265e5a51, \
  0xe9b6c7aa, \
  0xd62f105d, \
  0x02441453, \
  0xd8a1e681, \
  0xe7d3fbc8, \
  0x21e1cde6, \
  0xc33707d6, \
  0xf4d50d87, \
  0x455a14ed, \
  0xa9e3e905, \
  0xfcefa3f8, \
  0x676f02d9, \
  0x8d2a4c8a, \
  0xfffa3942, \
  0x8771f681, \
  0x6d9d6122, \
  0xfde5380c, \
  0xa4beea44, \
  0x4bdecfa9, \
  0xf6bb4b60, \
  0xbebfbc70, \
  0x289b7ec6, \
  0xeaa127fa, \
  0xd4ef3085, \
  0x04881d05, \
  0xd9d4d039, \
  0xe6db99e5, \
  0x1fa27cf8, \
  0xc4ac5665, \
  0xf4292244, \
  0x432aff97, \
  0xab9423a7, \
  0xfc93a039, \
  0x655b59c3, \
  0x8f0ccc92, \
  0xffeff47d, \
  0x85845dd1, \
  0x6fa87e4f, \
  0xfe2ce6e0, \
  0xa3014314, \
  0x4e0811a1, \
  0xf7537e82, \
  0xbd3af235, \
  0x2ad7d2bb, \
  0xeb86d391, \
}

extern unsigned int md5_extend(unsigned int length);
extern void         md5_sum(unsigned char *data, unsigned int length,
			    unsigned int *sum);
extern void         md5_display(unsigned int *sum);

#endif  /* !defined(_MD5_H) */
