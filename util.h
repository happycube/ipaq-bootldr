#ifndef __UTIL_H__
#define __UTIL_H__
#include "bootldr.h"
extern char HEX_TO_ASCII_TABLE[16];
extern void dwordtodecimal(char *buf, unsigned long x);
extern void binarytohex(char *buf, long x, int nbytes);
extern byte strtoul_err;
extern unsigned long strtoul(const char *str, char **endptr, int requestedbase);

#endif
