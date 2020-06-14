#ifdef CONFIG_FAKELIBC
#ifndef __FAKELIBC_H__
#define __FAKELIBC_H__

#ifndef __QNXNTO__
extern void *memcpy(char *dst, const char *src, long n);
extern void memset(void *dst, char c, unsigned long n);
extern int strlen(const char *s);
extern int isalpha(const char c);
extern int isalpha(const char c);
extern int isdigit(const char c);
extern char *strcat(char *dest, const char *src);
extern int strncmp(const char *s1, const char *s2, int len);
extern int strcmp(const char *s1, const char *s2);
extern char *strchr(const char *s, int c);
extern int isalnum(const char c);
#endif

extern int memmem(const char *haystack,int num1, const char *needle,int num2);
extern char *strstr(const char *haystack, const char *needle);

#endif
#endif
