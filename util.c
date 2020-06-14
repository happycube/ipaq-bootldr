#include "util.h"
#include "serial.h"

/******************* UTIL FUNCTIONS ********************/
extern char HEX_TO_ASCII_TABLE[16];

void dwordtodecimal(char *buf, unsigned long x)
{
  int i = 0;
  int j = 0;
  char localbuf[16];

  if (x != 0) {
    while (x > 0) {
      unsigned long rem = x % 10;
      localbuf[i++] = HEX_TO_ASCII_TABLE[rem];
      x /= 10;
    }
    /* now reverse the characters into buf */
    while (i > 0) {
      i--;
      buf[j++] = localbuf[i];
    }
    buf[j] = '\0';
  } else {
    buf[0] = '0';
    buf[1] = '\0';
  }
}

void binarytohex(char *buf, long x, int nbytes)
{
  int i;
  int s = 4*(2*nbytes - 1);
  if (HEX_TO_ASCII_TABLE[0] != '0')
     putstr("HEX_TO_ASCII_TABLE corrupted\n");
  for (i = 0; i < 2*nbytes; i++) {
    buf[i] = HEX_TO_ASCII_TABLE[(x >> s) & 0xf];
    s -= 4;
  }
  buf[2*nbytes] = 0;
}

byte strtoul_err;
unsigned long strtoul(const char *str, char **endptr, int requestedbase)
{
   unsigned long num = 0;
   char c;
   byte digit;
   int base = 10;
   int nchars = 0;
   int leadingZero = 0;

   strtoul_err = 0;

   while ((c = *str) != 0) {
      if (nchars == 0 && c == '0') {
         leadingZero = 1;
         if (0) putLabeledWord("strtoul: leadingZero nchars=", nchars);
         goto step;
      } else if (leadingZero && nchars == 1) {
         if (c == 'x') {
            base = 16;
            if (0) putLabeledWord("strtoul: base16 nchars=", nchars);
            goto step;
         } else if (c == 'o') {
            base = 8;
            if (0) putLabeledWord("strtoul: base8 nchars=", nchars);
            goto step;
         }
      }
      if (0) putLabeledWord("strtoul: c=", c);
      if (c >= '0' && c <= '9') {
         digit = c - '0';
      } else if (c >= 'a' && c <= 'z') {
         digit = c - 'a' + 10;
      } else if (c >= 'A' && c <= 'Z') {
         digit = c - 'A' + 10;
      } else {
         strtoul_err = 3;
         return 0;
      }
      if (digit >= base) {
         strtoul_err = 4;
         return 0;
      }
      num *= base;
      num += digit;
   step:
      str++;
      nchars++;
   }
   return num;
}
/******************* END UTIL FUNCTIONS ********************/
