/* <eetypes.h>: the EE SDK's integer types, in our own words. `long` is 64-bit under this
 * compiler; a 128-bit type is spelt with GCC's mode attribute, which MW accepts and mangles
 * as `1` (CVU::Code(u_long128 *) is Code__3CVUFP1). */
#ifndef _SC_EETYPES_H
#define _SC_EETYPES_H

typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;
typedef unsigned long u_long;
typedef int long128 __attribute__((mode(TI)));
typedef unsigned int u_long128 __attribute__((mode(TI)));

#endif
