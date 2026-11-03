/* What newlib's libc units here need of newlib's headers (the toolchain's newlib, ee-gcc 2.96):
 * the reentrancy block's fields at the offsets the image reads them at, the ctype table and its
 * macros. In our own words; the layout is newlib's struct _reent for the EE (long is 64-bit). */
#ifndef SC_NEWLIB_H
#define SC_NEWLIB_H

#define NULL 0
typedef unsigned int size_t;

struct __sFILE;

struct _reent {
	int _errno;                             /* 0x00 */
	struct __sFILE *_stdin, *_stdout, *_stderr;     /* 0x04, 0x08, 0x0C */
	char _pad10[0xA8 - 0x10];
	unsigned long long _rand_next;          /* 0xA8: _new._reent._rand_next */
};

extern struct _reent *_impure_ptr;
#define _REENT _impure_ptr

extern const char _ctype_[];
#define _U 01
#define _L 02
#define _N 04
#define _S 010
#define _P 020
#define _C 040
#define _X 0100
#define _B 0200
#define isupper(c) ((_ctype_ + 1)[(unsigned)(c)] & _U)
#define islower(c) ((_ctype_ + 1)[(unsigned)(c)] & _L)
#define tolower(c) __extension__ ({ int __x = (c); isupper(__x) ? (__x - 'A' + 'a') : __x; })
#define toupper(c) __extension__ ({ int __x = (c); islower(__x) ? (__x - 'a' + 'A') : __x; })

#endif
