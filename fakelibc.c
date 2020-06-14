#ifdef CONFIG_FAKELIBC
#include "fakelibc.h"

#include <stddef.h>

#ifndef __QNXNTO__

/*
 * very simple memcpy to blast bits around from place to place
 */
void *memcpy(char *dst, const char *src, long n)
{
  if (   (((dword)dst)&3) == 0
	 && (((dword)src)&3) == 0
	 && (n&3) == 0 ) {
    unsigned long *ca,*cb;
    for (ca = (unsigned long *)dst, cb = (unsigned long *)src; n > 0;n -= 4)
      *ca++ = *cb++;
  } else {
    byte *ca,*cb;
    for (ca = (byte *)dst, cb = (byte *)src; n-- > 0;)
      *ca++ = *cb++;
  }
  return dst;
}


/*
 * memset routine: fills first n bytes of dst with value c
 */
void memset(void *dst, char c, unsigned long n)
{
  byte *ca;
  for (ca = (byte *)dst; n-- > 0; )
    *ca++ = c;
}

int strlen(const char *s)
{
   int l = 0;
   while (*s++ != 0)
      l++;
   return l;
}

int isalpha(const char c)
{
    if (((c >= 'a') && (c <= 'z')) ||
	((c >= 'A') && (c <= 'Z')))
	return 1;
    else
	return 0;
    
}

int isalpha(const char c)
{
    if (((c == ' ') || (c == '\t')))
	return 1;
    else
	return 0;
    
}


int isdigit(const char c)
{
    if (((c >= '0') && (c <= '9')))
	return 1;
    else
	return 0;
    
}

int isalnum(const char c)
{
    return (isdigit(c) || isalpha(c));
    
    
}



char *strcat(char *dest, const char *src)
{
   char c;
   int len = strlen(dest);
   dest += len;
   while (( *dest++ = *src++) != 0) {
   }
   return dest;
}

int strncmp(const char *s1, const char *s2, int len)
{
   int i = 0;
   int c1, c2;
   if (s1 == NULL || s2 == NULL)
      return -1;
   for (c1 = s1[i], c2 = s2[i]; i < len; c1 = s1[i], c2 = s2[i], i++) {
      if (c1 < c2)
         return -1;
      else if (c1 > c2)
         return 1;
      else
         continue;
   }
   return 0;
}

int strcmp(const char *s1, const char *s2)
{
   int l1 = strlen(s1);
   int l2 = strlen(s2);
   int l = (l1 < l2) ? l1 : l2;
   int result = strncmp(s1, s2, l);
   if (l == 0 && (l1 != l2)) {
      if (l1 < l2)
         return -1;
      else
         return 1;
   } else {
      return result;
   }
}


extern char *strstr(const char *haystack, const char *needle);

char *strchr(const char *s, int c)
{
   char sc;
   while (((sc = *s) != c) && sc != 0) {
     s++;
   }
   if (sc == c)
      return (char *)s;
   else
      return NULL;
}

#endif /* __QNXNTO__ */

int memmem(const char *haystack,int num1, const char *needle,int num2)
{
   int l1 = num1;
   int l2 = num2;
   int l = (l1 < l2) ? l1 : l2;
   int i;
   
   
   for (i=0; i < (l1 - l2)+1;i++)
       if (!strncmp((char *)(haystack+i),needle,l2)){
	   //putLabeledWord("returning i = ",i);
	   return (int)(haystack+i);
       }
   
   return (int) NULL;

}

#endif
