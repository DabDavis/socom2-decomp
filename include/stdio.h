#ifndef _SC_STDIO_H
#define _SC_STDIO_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _FILE FILE;
#define EOF (-1)
int printf(const char *fmt, ...);
int sprintf(char *dst, const char *fmt, ...);
int puts(const char *s);
#ifdef __cplusplus
}
#endif
#endif
