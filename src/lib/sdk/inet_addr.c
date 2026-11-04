/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* The socket library's address routines (MAIN 0x1C1308-0x1C1854), BSD's own text: htonl, htons,
 * inet_addr and inet_aton (FreeBSD's libc/net/inet_addr.c of 1996-2002 -- strtoul per part, errno
 * ERANGE and an empty part fail, a.b, a.b.c and a.b.c.d assembled with their range checks),
 * inet_lnaof, inet_makeaddr, inet_netof and inet_network (4.4BSD's). The socket layer's -O0
 * wrappers (inet_bsd.c) call each one; BIND's inet_ntop.c follows. Built with the SDK's toolchain
 * (dtls13010) at -O2 -G0 like the socket calls after it, with 32-bit types throughout: this
 * compiler's long is 64 bits, and the code's u_long-typed BSD text (parts[], IN_CLASSA's cast)
 * would be doublewords -- the target's are words, so the library says u_int32. inet_aton,
 * inet_addr, inet_lnaof, inet_makeaddr, inet_netof byte-exact as written under dtls13010 only
 * (990721/991111/-01/a miss inet_aton by 107-129 words; 2.95.x and 2.96 miss all of the class
 * functions). The ctype tests are the library's own inlines (no table, no call). Rows keep their
 * placeholders: no symbol names this library (it is not in the SOCOM 1 demo).
 *
 * OPEN: htonl and htons -- the target computes each in strict source order through two registers
 *   (`sll; andi; sll; or; srl; andi; or; srl; jr; or`); -O2 schedules the pieces apart (7-10 words),
 *   -O1 and -O2 without both schedulers come within two (the last shift lands in a0, not v1); no
 *   spelling tried gives the target's (build/scratch/gcc/v_htonl.c: five forms, five flag sets).
 *   GCC round 30a: htonl is byte-exact through ONE reused temporary (`r = x << 24; t = x & 0xff00;
 *   t <<= 8; r |= t; t = x >> 8; t &= 0xff00; r |= t; t = x >> 24; r |= t;` -- the temporary is
 *   v1 throughout, the last shift's too) but only with both schedulers off (`-fno-schedule-insns
 *   -fno-schedule-insns2`, or -O1): -O2 hoists the andi above the first sll, 2 words, and no flag
 *   of this unit's other functions allows that. htons's parameter is no u_short (the target never
 *   zero-extends a0: an int or u_int), and every C spelling puts its second term in the dying a0
 *   where the target's is v1 (3 words without the schedulers, 5 with). Both read as code no
 *   scheduler touched -- a separate object (-O1, or BSD's MIPS htonl.S/htons.S style assembly, two
 *   registers, the delay slot GNU as's) -- which the demo cannot tell: this library is not in it
 *   (build/scratch/gcc/v_htonl.py, v_htons.py).
 * OPEN: inet_network -- right size, one instruction placed apart: the hex digit's sum goes through
 *   v0 and a move into val's t0 (the decimal digit's goes straight). Its isxdigit tests an
 *   UNINITIALISED variable for the digit case -- the target computes `(u_char)(x - '0') < 10` of a
 *   register nothing sets (t5), hoisted out of the loop, which only an uninitialised local in the
 *   inline reproduces (the size is right only so). */

typedef unsigned int u_int;
typedef unsigned short u_short;
typedef unsigned int in_addr_t;
struct in_addr { in_addr_t s_addr; };
#define NULL 0
#define ERANGE 34
extern int *__errno(void);
#define errno (*__errno())
extern unsigned long strtoul(const char *, char **, int);

#define IN_CLASSA(i)		(((int)(i) & 0x80000000) == 0)
#define	IN_CLASSA_NET		0xff000000
#define	IN_CLASSA_NSHIFT	24
#define	IN_CLASSA_HOST		0x00ffffff
#define IN_CLASSB(i)		(((int)(i) & 0xc0000000) == 0x80000000)
#define	IN_CLASSB_NET		0xffff0000
#define	IN_CLASSB_NSHIFT	16
#define	IN_CLASSB_HOST		0x0000ffff
#define	IN_CLASSC_NET		0xffffff00
#define	IN_CLASSC_NSHIFT	8
#define	IN_CLASSC_HOST		0x000000ff
#define INADDR_NONE		0xffffffff

static __inline__ int isdigit(unsigned char c) { return (unsigned)(c - '0') < 10; }
static __inline__ int islower(unsigned char c) { return (unsigned)(c - 'a') < 26; }
static __inline__ int isxdigit(unsigned char c)
{
	unsigned char d;	/* sic: never set (see inet_network's note) */

	return (unsigned char)(d - '0') < 10 || (unsigned)(c - 'a') < 6 || (unsigned)(c - 'A') < 6;
}
static __inline__ int isspace(unsigned char c) { return (unsigned)(c - '\t') < 5 || c == ' '; }

u_int func_001C1308(u_int x)
{
	return (x << 24) | ((x & 0xff00) << 8) | ((x >> 8) & 0xff00) | (x >> 24);
}

u_short func_001C1330(u_short x)
{
	return ((x << 8) & 0xff00) | ((x >> 8) & 0xff);
}

int func_001C1378(const char *cp, struct in_addr *addr);

u_int func_001C1348(const char *cp)
{
	struct in_addr val;

	if (func_001C1378(cp, &val))
		return (val.s_addr);
	return (INADDR_NONE);
}

int func_001C1378(const char *cp, struct in_addr *addr)
{
	u_int parts[4];
	in_addr_t val;
	char *c;
	char *endptr;
	int gotend, n;

	c = (char *)cp;
	n = 0;
	gotend = 0;
	while (!gotend) {
		errno = 0;
		val = strtoul(c, &endptr, 0);

		if (errno == ERANGE)
			return (0);
		if (endptr == c)
			return (0);
		parts[n] = val;
		c = endptr;

		switch (*c) {
		case '.' :
			if (n == 3)
				return (0);
			n++;
			c++;
			break;

		case '\0':
			gotend = 1;
			break;

		default:
			if (isspace((unsigned char)*c)) {
				gotend = 1;
				break;
			} else
				return (0);
		}
	}

	switch (n) {
	case 0:
		break;
	case 1:
		if (val > 0xffffff || parts[0] > 0xff)
			return (0);
		val |= parts[0] << 24;
		break;

	case 2:
		if (val > 0xffff || parts[0] > 0xff || parts[1] > 0xff)
			return (0);
		val |= (parts[0] << 24) | (parts[1] << 16);
		break;

	case 3:
		if (val > 0xff || parts[0] > 0xff || parts[1] > 0xff ||
		    parts[2] > 0xff)
			return (0);
		val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
		break;
	}

	if (addr != NULL)
		addr->s_addr = func_001C1308(val);
	return (1);
}

u_int func_001C1590(struct in_addr in)
{
	register u_int i = func_001C1308(in.s_addr);

	if (IN_CLASSA(i))
		return ((i)&IN_CLASSA_HOST);
	else if (IN_CLASSB(i))
		return ((i)&IN_CLASSB_HOST);
	else
		return ((i)&IN_CLASSC_HOST);
}

struct in_addr func_001C15E0(u_int net, u_int host)
{
	u_int addr;

	if (net < 128)
		addr = (net << IN_CLASSA_NSHIFT) | (host & IN_CLASSA_HOST);
	else if (net < 65536)
		addr = (net << IN_CLASSB_NSHIFT) | (host & IN_CLASSB_HOST);
	else if (net < 16777216)
		addr = (net << IN_CLASSC_NSHIFT) | (host & IN_CLASSC_HOST);
	else
		addr = net | host;
	addr = func_001C1308(addr);
	return (*(struct in_addr *)&addr);
}

u_int func_001C1658(struct in_addr in)
{
	register u_int i = func_001C1308(in.s_addr);

	if (IN_CLASSA(i))
		return (((i)&IN_CLASSA_NET) >> IN_CLASSA_NSHIFT);
	else if (IN_CLASSB(i))
		return (((i)&IN_CLASSB_NET) >> IN_CLASSB_NSHIFT);
	else
		return (((i)&IN_CLASSC_NET) >> IN_CLASSC_NSHIFT);
}

u_int func_001C1698(const char *cp)
{
	register u_int val, base, n, i;
	register char c;
	u_int parts[4], *pp = parts;

again:
	val = 0; base = 10;
	if (*cp == '0')
		base = 8, cp++;
	if (*cp == 'x' || *cp == 'X')
		base = 16, cp++;
	while ((c = *cp)) {
		if (isdigit(c)) {
			val = (val * base) + (c - '0');
			cp++;
			continue;
		}
		if (base == 16 && isxdigit(c)) {
			val = (val << 4) + (c + 10 - (islower(c) ? 'a' : 'A'));
			cp++;
			continue;
		}
		break;
	}
	if (*cp == '.') {
		if (pp >= parts + 3)
			return (INADDR_NONE);
		*pp++ = val, cp++;
		goto again;
	}
	if (*cp && !isspace(*cp))
		return (INADDR_NONE);
	*pp++ = val;
	n = pp - parts;
	for (val = 0, i = 0; i < n; i++) {
		val <<= 8;
		val |= parts[i] & 0xff;
	}
	return (val);
}
