/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* BIND's inet_ntop.c (inet_ntoa through it), MAIN 0x1C1858-0x1C1CD0: the network library at
 * 0x1C0948 (README "The GCC front"), built with Sony's SDK toolchain -- inet_ntop's returned calls
 * are jal-and-return, which 2.96 would make sibling calls. Written from BIND's text (Paul Vixie,
 * 1996): AF_INET 2, AF_INET6 28, EAFNOSUPPORT 106 and ENOSPC 28 as newlib's errno. Names by body:
 * inet_ntoa's own message, inet_ntop4's format; the two statics keep BIND's names. */

typedef unsigned char u_char;
typedef unsigned int u_int;
typedef unsigned int size_t;

#define NS_IN6ADDRSZ 16
#define NS_INT16SZ 2
#define AF_INET 2
#define AF_INET6 28
#define EAFNOSUPPORT 106
#define ENOSPC 28
#define errno (*__errno())

struct in_addr {
	u_int s_addr;
};

extern int *__errno(void);
extern int sprintf(char *s, const char *fmt, ...);
extern char *strcpy(char *d, const char *s);
extern size_t strlen(const char *s);
extern void *memset(void *d, int c, size_t n);

static const char *inet_ntop4(const u_char *src, char *dst, size_t size);
static const char *inet_ntop6(const u_char *src, char *dst, size_t size);
const char *inet_ntop(int af, const void *src, char *dst, size_t size);

char *inet_ntoa(struct in_addr in)
{
	static char ret[18];

	strcpy(ret, "[inet_ntoa error]");
	(void)inet_ntop(AF_INET, &in, ret, sizeof ret);
	return ret;
}

const char *inet_ntop(int af, const void *src, char *dst, size_t size)
{
	switch (af) {
	case AF_INET:
		return inet_ntop4(src, dst, size);
	case AF_INET6:
		return inet_ntop6(src, dst, size);
	default:
		errno = EAFNOSUPPORT;
		return 0;
	}
}

static const char *inet_ntop4(const u_char *src, char *dst, size_t size)
{
	static const char fmt[] = "%u.%u.%u.%u";
	char tmp[sizeof "255.255.255.255"];

	if (sprintf(tmp, fmt, src[0], src[1], src[2], src[3]) > size) {
		errno = ENOSPC;
		return 0;
	}
	strcpy(dst, tmp);
	return dst;
}

static const char *inet_ntop6(const u_char *src, char *dst, size_t size)
{
	char tmp[sizeof "ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255"], *tp;
	struct { int base, len; } best, cur;
	u_int words[NS_IN6ADDRSZ / NS_INT16SZ];
	int i;

	memset(words, '\0', sizeof words);
	for (i = 0; i < NS_IN6ADDRSZ; i++)
		words[i / 2] |= (src[i] << ((1 - (i % 2)) << 3));
	best.base = -1;
	cur.base = -1;
	for (i = 0; i < (NS_IN6ADDRSZ / NS_INT16SZ); i++) {
		if (words[i] == 0) {
			if (cur.base == -1)
				cur.base = i, cur.len = 1;
			else
				cur.len++;
		} else {
			if (cur.base != -1) {
				if (best.base == -1 || cur.len > best.len)
					best = cur;
				cur.base = -1;
			}
		}
	}
	if (cur.base != -1) {
		if (best.base == -1 || cur.len > best.len)
			best = cur;
	}
	if (best.base != -1 && best.len < 2)
		best.base = -1;

	tp = tmp;
	for (i = 0; i < (NS_IN6ADDRSZ / NS_INT16SZ); i++) {
		if (best.base != -1 && i >= best.base && i < (best.base + best.len)) {
			if (i == best.base)
				*tp++ = ':';
			continue;
		}
		if (i != 0)
			*tp++ = ':';
		if (i == 6 && best.base == 0 && (best.len == 6 || (best.len == 5 && words[5] == 0xffff))) {
			if (!inet_ntop4(src + 12, tp, sizeof tmp - (tp - tmp)))
				return 0;
			tp += strlen(tp);
			break;
		}
		tp += sprintf(tp, "%x", words[i]);
	}
	if (best.base != -1 && (best.base + best.len) == (NS_IN6ADDRSZ / NS_INT16SZ))
		*tp++ = ':';
	*tp++ = '\0';

	if ((size_t)(tp - tmp) > size) {
		errno = ENOSPC;
		return 0;
	}
	strcpy(dst, tmp);
	return dst;
}
