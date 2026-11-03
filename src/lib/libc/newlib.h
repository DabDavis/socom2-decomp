/* What newlib's libc units here need of newlib's headers (the toolchain's newlib, ee-gcc 2.96):
 * the reentrancy block's fields at the offsets the image reads them at, the ctype table and its
 * macros. In our own words; the layout is newlib's struct _reent for the EE (long is 64-bit). */
#ifndef SC_NEWLIB_H
#define SC_NEWLIB_H

#define NULL 0
typedef unsigned int size_t;

/* newlib's stdio (sys/reent.h's struct __sFILE; _fpos_t and _off_t are long, 64-bit here) */
typedef long _fpos_t;
typedef long _off_t;
struct __sbuf {
	unsigned char *_base;
	int _size;
};
typedef struct __sFILE {
	unsigned char *_p;                      /* 0x00: current position in (some) buffer */
	int _r;                                 /* 0x04: read space left for getc() */
	int _w;                                 /* 0x08: write space left for putc() */
	short _flags;                           /* 0x0C */
	short _file;                            /* 0x0E: fileno, if Unix descriptor, else -1 */
	struct __sbuf _bf;                      /* 0x10: the buffer */
	int _lbfsize;                           /* 0x18: 0 or -_bf._size, for inline putc */
	void *_cookie;                          /* 0x1C */
	int (*_read)(void *_cookie, char *_buf, int _n);
	int (*_write)(void *_cookie, const char *_buf, int _n);
	_fpos_t (*_seek)(void *_cookie, _fpos_t _offset, int _whence);
	int (*_close)(void *_cookie);
	struct __sbuf _ub;                      /* 0x30: ungetc buffer */
	unsigned char *_up;
	int _ur;
	unsigned char _ubuf[3];
	unsigned char _nbuf[1];
	struct __sbuf _lb;                      /* 0x44 */
	int _blksize;
	int _offset;                            /* 0x50: current lseek offset */
	struct _reent *_data;                   /* 0x54 */
} FILE;

struct _glue {
	struct _glue *_next;
	int _niobs;
	struct __sFILE *_iobs;
};

struct _atexit {
	struct _atexit *_next;                  /* next in list */
	int _ind;                               /* next index in this table */
	void (*_fns[32])(void);                 /* the table itself */
};

struct _Bigint {
	struct _Bigint *_next;
	int _k, _maxwds, _sign, _wds;
	unsigned int _x[1];                     /* __ULong: long is 64-bit here, the digits are words */
};

/* newlib 1.9's struct _reent (sys/reent.h), the offsets the image reads */
struct _reent {
	int _errno;                             /* 0x00 */
	struct __sFILE *_stdin, *_stdout, *_stderr;     /* 0x04, 0x08, 0x0C */
	int _inc;                               /* 0x10 */
	char _emergency[25];                    /* 0x14 */
	int _current_category;                  /* 0x30 */
	const char *_current_locale;            /* 0x34 */
	int __sdidinit;                         /* 0x38: 1 means stdio has been init'd */
	void (*__cleanup)(struct _reent *);     /* 0x3C */
	struct _Bigint *_result;                /* 0x40: used by mprec routines */
	int _result_k;                          /* 0x44 */
	struct _Bigint *_p5s;                   /* 0x48 */
	struct _Bigint **_freelist;             /* 0x4C */
	int _cvtlen;                            /* 0x50: should be size_t */
	char *_cvtbuf;                          /* 0x54 */
	char _pad58[0xA8 - 0x58];               /* 0x58: _new._reent's strtok/asctime/localtime state */
	unsigned long long _rand_next;          /* 0xA8: _new._reent._rand_next */
	char _padB0[0x148 - 0xB0];              /* the rest of the _new union (_unused's 240 bytes) */
	struct _atexit *_atexit;                /* 0x148: points to head of LIFO stack */
	struct _atexit _atexit0;                /* 0x14C: one guaranteed table, required by ANSI */
	void (**_sig_func)(int);                /* 0x1D4: signal info */
	struct _glue __sglue;                   /* 0x1D8: root of glue chain */
	struct __sFILE __sf[3];                 /* 0x1E4: first three file descriptors */
};


#define	__SLBF	0x0001		/* line buffered */
#define	__SNBF	0x0002		/* unbuffered */
#define	__SRD	0x0004		/* OK to read */
#define	__SWR	0x0008		/* OK to write */
#define	__SRW	0x0010		/* open for reading & writing */
#define	__SEOF	0x0020		/* found EOF */
#define	__SERR	0x0040		/* found error */
#define	__SMBF	0x0080		/* _buf is from malloc */
#define	__SAPP	0x0100		/* fdopen()ed in append mode - so must  write to end */
#define	__SSTR	0x0200		/* this is an sprintf/snprintf string */
#define	__SOPT	0x0400		/* do fseek() optimisation */
#define	__SNPT	0x0800		/* do not do fseek() optimisation */
#define	__SOFF	0x1000		/* set iff _offset is in fact correct */
#define	__SMOD	0x2000		/* true => fgetline modified _p text */

#define	EOF	(-1)
#define	BUFSIZ	1024
#define	INT_MAX	2147483647

#define	_stdout_r(x)	((x)->_stdout)

#define CHECK_INIT(fp) \
  do						\
    {						\
      if ((fp)->_data == 0)			\
	(fp)->_data = _REENT;			\
      if (!(fp)->_data->__sdidinit)		\
	__sinit ((fp)->_data);			\
    }						\
  while (0)

struct __siov {
	const void *iov_base;
	size_t iov_len;
};
struct __suio {
	struct __siov *uio_iov;
	int uio_iovcnt;
	int uio_resid;
};

/* stdio/local.h's */
#define	cantwrite(fp) \
  ((((fp)->_flags & __SWR) == 0 || (fp)->_bf._base == NULL) && \
   __swsetup(fp))
#define	HASUB(fp) ((fp)->_ub._base != NULL)
#define	FREEUB(fp) { \
	if ((fp)->_ub._base != (fp)->_ubuf) \
		_free_r(fp->_data, (char *)(fp)->_ub._base); \
	(fp)->_ub._base = NULL; \
}
#define	HASLB(fp) ((fp)->_lb._base != NULL)
#define	FREELB(fp) { _free_r(fp->_data, (char *)(fp)->_lb._base); (fp)->_lb._base = NULL; }

extern void *_malloc_r(struct _reent *, size_t);
extern void _free_r(struct _reent *, void *);
extern void *_realloc_r(struct _reent *, void *, size_t);
extern void __sinit(struct _reent *);
extern int __srefill(FILE *);
extern int __swsetup(FILE *);
extern void __smakebuf(FILE *);
extern int fflush(FILE *);
extern int _fwalk(struct _reent *, int (*)(FILE *));
extern void *memset(void *, int, size_t);
extern void *memcpy(void *, const void *, size_t);
extern void *memchr(const void *, int, size_t);

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
