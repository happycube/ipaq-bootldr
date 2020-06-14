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
 *
 *
 * History:
 * ^^^^^^^^
 * 12 March, 2001 - created by separating from jffs.c. (jd)
 *
 */

#ifndef CONFIG_ACCEPT_GPL
#error This file covered by GPL but CONFIG_ACCEPT_GPL undefined.
#endif

#include <string.h>

#include "md5.h"

/* MD5 Message Digest Algorithm - IETF Request for Comments: 1321
 *
 * We provide a (hopefully) conforming implementation here to verify
 * the correctness of reconstructed JFFS files and downloaded kernel
 * images.
 */

/* Auxiliary function "F" from RFC 1321 section 3.4: */
static inline unsigned int md5_F(unsigned int X, unsigned int Y,
				 unsigned int Z){
  return (X & Y) + (~X & Z);
}

/* Auxiliary function "G" from RFC 1321 section 3.4: */
static inline unsigned int md5_G(unsigned int X, unsigned int Y,
				 unsigned int Z){
  return (X & Z) + (Y & ~Z);
}

/* Auxiliary function "H" from RFC 1321 section 3.4: */
static inline unsigned int md5_H(unsigned int X, unsigned int Y,
				 unsigned int Z){
  return X ^ Y ^ Z;
}

/* Auxiliary function "I" from RFC 1321 section 3.4: */
static inline unsigned int md5_I(unsigned int X, unsigned int Y,
				 unsigned int Z){
  return Y ^ (X | ~Z);
}

/* Rotate ("<<<") operation from RFC 1321 section 3.4: */
static inline unsigned int md5_rotate(unsigned int a, int r){
  return (a << r) | (a >> ((sizeof(a) * 8) - r));
}

/* Compute the number of padding and append-length bytes necessary 
 * for the given `length'. (See RFC 1321 sections 3.1, 3.2)
 */
unsigned int md5_extend(unsigned int length){
  unsigned int ext;
  
  ext = (MD5_LENGTH_MODULUS - (length % MD5_LENGTH_MODULUS)) % 
    MD5_LENGTH_MODULUS;

  if(ext < MD5_PREPAD_LEN_LENGTH)
    ext += MD5_LENGTH_MODULUS;
  
  return ext;
}


/* Run the MD5 algorithm over each block in the file. (Sections 3.3 and
 * 3.4 of the RFC.)
 */
static void md5_rounds(unsigned char *data, unsigned int length,
		       unsigned int *buf){

  unsigned int A, B, C, D, AA, BB, CC, DD, X[MD5_BLOCK_WORDS];
  unsigned int T[] = MD5_T_INIT;
  unsigned int *M = (unsigned int *)data;
  unsigned int words;
  int i, j;

  A = MD5_WORD_A_INIT;
  B = MD5_WORD_B_INIT;
  C = MD5_WORD_C_INIT;
  D = MD5_WORD_D_INIT;
  
  words = (length + md5_extend(length)) / sizeof(unsigned int);

  for(i = 0; i < (words / MD5_BLOCK_WORDS); ++i){

    for(j = 0; j < MD5_BLOCK_WORDS; ++j)
      X[j] = M[(i * MD5_BLOCK_WORDS) + j];

    AA = A;
    BB = B;
    CC = C;
    DD = D;

#define MD5_ROUND1(a, b, c, d, k, s, i) \
((a) = (b) + md5_rotate((a) + \
 md5_F((b), (c), (d)) + X[(k)] + T[(i) - 1], (s)))

    MD5_ROUND1(A, B, C, D,  0,  7,  1);
    MD5_ROUND1(D, A, B, C,  1, 12,  2);
    MD5_ROUND1(C, D, A, B,  2, 17,  3);
    MD5_ROUND1(B, C, D, A,  3, 22,  4);

    MD5_ROUND1(A, B, C, D,  4,  7,  5);
    MD5_ROUND1(D, A, B, C,  5, 12,  6);
    MD5_ROUND1(C, D, A, B,  6, 17,  7);
    MD5_ROUND1(B, C, D, A,  7, 22,  8);

    MD5_ROUND1(A, B, C, D,  8,  7,  9);
    MD5_ROUND1(D, A, B, C,  9, 12, 10);
    MD5_ROUND1(C, D, A, B, 10, 17, 11);
    MD5_ROUND1(B, C, D, A, 11, 22, 12);

    MD5_ROUND1(A, B, C, D, 12,  7, 13);
    MD5_ROUND1(D, A, B, C, 13, 12, 14);
    MD5_ROUND1(C, D, A, B, 14, 17, 15);
    MD5_ROUND1(B, C, D, A, 15, 22, 16);

#define MD5_ROUND2(a, b, c, d, k, s, i) \
((a) = (b) + md5_rotate((a) + \
 md5_G((b), (c), (d)) + X[(k)] + T[(i) - 1], (s)))

    MD5_ROUND2(A, B, C, D,  1,  5, 17);
    MD5_ROUND2(D, A, B, C,  6,  9, 18);
    MD5_ROUND2(C, D, A, B, 11, 14, 19);
    MD5_ROUND2(B, C, D, A,  0, 20, 20);

    MD5_ROUND2(A, B, C, D,  5,  5, 21);
    MD5_ROUND2(D, A, B, C, 10,  9, 22);
    MD5_ROUND2(C, D, A, B, 15, 14, 23);
    MD5_ROUND2(B, C, D, A,  4, 20, 24);

    MD5_ROUND2(A, B, C, D,  9,  5, 25);
    MD5_ROUND2(D, A, B, C, 14,  9, 26);
    MD5_ROUND2(C, D, A, B,  3, 14, 27);
    MD5_ROUND2(B, C, D, A,  8, 20, 28);

    MD5_ROUND2(A, B, C, D, 13,  5, 29);
    MD5_ROUND2(D, A, B, C,  2,  9, 30);
    MD5_ROUND2(C, D, A, B,  7, 14, 31);
    MD5_ROUND2(B, C, D, A, 12, 20, 32);

#define MD5_ROUND3(a, b, c, d, k, s, i) \
((a) = (b) + md5_rotate((a) + \
 md5_H((b), (c), (d)) + X[(k)] + T[(i) - 1], (s)))

    MD5_ROUND3(A, B, C, D,  5,  4, 33);
    MD5_ROUND3(D, A, B, C,  8, 11, 34);
    MD5_ROUND3(C, D, A, B, 11, 16, 35);
    MD5_ROUND3(B, C, D, A, 14, 23, 36);

    MD5_ROUND3(A, B, C, D,  1,  4, 37);
    MD5_ROUND3(D, A, B, C,  4, 11, 38);
    MD5_ROUND3(C, D, A, B,  7, 16, 39);
    MD5_ROUND3(B, C, D, A, 10, 23, 40);

    MD5_ROUND3(A, B, C, D, 13,  4, 41);
    MD5_ROUND3(D, A, B, C,  0, 11, 42);
    MD5_ROUND3(C, D, A, B,  3, 16, 43);
    MD5_ROUND3(B, C, D, A,  6, 23, 44);

    MD5_ROUND3(A, B, C, D,  9,  4, 45);
    MD5_ROUND3(D, A, B, C, 12, 11, 46);
    MD5_ROUND3(C, D, A, B, 15, 16, 47);
    MD5_ROUND3(B, C, D, A,  2, 23, 48);

#define MD5_ROUND4(a, b, c, d, k, s, i) \
((a) = (b) + md5_rotate((a) + \
 md5_I((b), (c), (d)) + X[(k)] + T[(i) - 1], (s)))

    MD5_ROUND4(A, B, C, D,  0,  6, 49);
    MD5_ROUND4(D, A, B, C,  7, 10, 50);
    MD5_ROUND4(C, D, A, B, 14, 15, 51);
    MD5_ROUND4(B, C, D, A,  5, 21, 52);

    MD5_ROUND4(A, B, C, D, 12,  6, 53);
    MD5_ROUND4(D, A, B, C,  3, 10, 54);
    MD5_ROUND4(C, D, A, B, 10, 15, 55);
    MD5_ROUND4(B, C, D, A,  1, 21, 56);

    MD5_ROUND4(A, B, C, D,  8,  6, 57);
    MD5_ROUND4(D, A, B, C, 15, 10, 58);
    MD5_ROUND4(C, D, A, B,  6, 15, 59);
    MD5_ROUND4(B, C, D, A, 13, 21, 60);

    MD5_ROUND4(A, B, C, D,  4,  6, 61);
    MD5_ROUND4(D, A, B, C, 11, 10, 62);
    MD5_ROUND4(C, D, A, B,  2, 15, 63);
    MD5_ROUND4(B, C, D, A,  9, 21, 64);

    A += AA;
    B += BB;
    C += CC;
    D += DD;

  }

  buf[0] = A;
  buf[1] = B;
  buf[2] = C;
  buf[3] = D;
}


/* Initialize the padded postamble to the reconstructed file. (Sections
 * 3.1 and 3.2 of the RFC.)
 */
static void md5_prep(unsigned char *data, unsigned int length){
  unsigned int extend;

  extend = md5_extend(length);
  
  memset(data + length, 0, extend);

  data[length] = 0x80;

  *((unsigned int *)(data + length + 
		     extend - (2 * sizeof(unsigned int)))) = length << 3;
  *((unsigned int *)(data + length + 
		     extend - (1 * sizeof(unsigned int)))) = length >> 29;
}


/* Modify the postamble section of the buffer pointed to by `data' having
 * `length' bytes (with appropriate padding sized by md5_extend()), then
 * return the MD5 sum in a four-integer array pointed to by `sum':
 */
void md5_sum(unsigned char *data, unsigned int length, unsigned int *sum){

  md5_prep(data, length);

  md5_rounds(data, length, sum);

}

/* Display the result of an MD5 computation (section 3.5 of the RFC): */
void md5_display(unsigned int *sum){
  int i, j;

  for(i = 0; i < MD5_SUM_WORDS; ++i)
    for(j = 0; j < sizeof(unsigned int); ++j)
      putHexInt8((sum[i] >> (j * 8)) & 0xff);
}
