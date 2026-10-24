#ifndef _SC_STDLIB_H
#define _SC_STDLIB_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
void *malloc(size_t n);
void *calloc(size_t n, size_t size);
void *realloc(void *p, size_t n);
void free(void *p);
int abs(int x);
long labs(long x);
int atoi(const char *s);
double atof(const char *s);
long strtol(const char *s, char **end, int base);
unsigned long strtoul(const char *s, char **end, int base);
int rand(void);
void srand(unsigned int seed);
void exit(int status);
void abort(void);
void qsort(void *base, size_t n, size_t size, int (*cmp)(const void *, const void *));
#ifdef __cplusplus
}
#endif
#endif
