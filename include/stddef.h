/* Minimal libc headers for the matching build, in our own words. Zipper built
 * against Metrowerks' MSL; only the declarations matter to codegen, so these
 * stay standard prototypes and grow as the source needs them. */
#ifndef _SC_STDDEF_H
#define _SC_STDDEF_H
typedef unsigned int size_t;
typedef int ptrdiff_t;
#ifndef NULL
#define NULL 0
#endif
#define offsetof(type, member) ((size_t)&(((type *)0)->member))
#endif
