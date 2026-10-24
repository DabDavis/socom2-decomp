#ifndef _SC_STDIO_H
#define _SC_STDIO_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _FILE FILE;
#define EOF (-1)
/* newlib's: the standard streams hang off the reentrancy structure (MSL's errors print to
 * stderr through it -- _impure_ptr->_stderr, at 12) */
struct _reent {
    int _errno;
    FILE *_stdin;
    FILE *_stdout;
    FILE *_stderr;
};
extern struct _reent *_impure_ptr;
#define stderr (_impure_ptr->_stderr)
int fprintf(FILE *f, const char *fmt, ...);
int printf(const char *fmt, ...);
int sprintf(char *dst, const char *fmt, ...);
int puts(const char *s);
#ifdef __cplusplus
}
#endif
#endif
