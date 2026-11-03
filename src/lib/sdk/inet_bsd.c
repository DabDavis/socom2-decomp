/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O0 -G0 */
/* The BSD socket names over the socket library (MAIN 0x1C0948-0x1C1304): 33 one-call wrappers,
 * each passing its arguments straight to the library function at 0x1C1308-0x1C4C98 (the
 * byte swaps, the address parsers, BIND's inet_ntoa, and the socket calls over libinet's
 * sceInetCreate/Open/Send/Recv/Control...). Built UNOPTIMISED with the SDK's own toolchain:
 * -O0's frame pointer, every argument spilled to the frame and reloaded, the result through
 * v1 to v0 -- byte-exact only under ee-gcc 2.9-991111-01-dtls13010 (its GNU as fills the
 * call's delay slot: 2.9-991111, -01, a and 990721 12 words off, 2.95.x 14, 2.96 11, 3.2 11).
 * Not in the SOCOM 1 demo, which has no socket layer; the rows keep their placeholders (the
 * two byte swaps each have two wrappers -- hton and ntoh -- that nothing here tells apart).
 * The receive wrappers set the socket's receive timeout to 600000 first. */

struct in_addr {
	unsigned int s_addr;
};

/* the library's own address type: a wrapper copies the BSD one into it (and back) through a
 * local -- -O0 keeps the copy, which is the target's extra store and load */
typedef struct sceInsockInAddr {
	unsigned int s_addr;
} sceInsockInAddr_t;

extern int func_001C2348(void);
extern int func_001C2390(void);
extern int func_001C28F8(int, int);
extern int func_001C4C20(int);
extern int func_001C4C98(int, int, int);
extern int func_001C2A98(int);
extern int func_001C4A20(int, int, int);
extern int func_001C48A0(int);
extern int func_001C1348(int);
extern int func_001C1378(int, int);
extern int func_001C1590(sceInsockInAddr_t);
extern sceInsockInAddr_t func_001C15E0(int, int);
extern int func_001C1658(sceInsockInAddr_t);
extern int func_001C1698(int);
extern char *inet_ntoa(sceInsockInAddr_t);
extern unsigned int func_001C1308(unsigned int);
extern unsigned short func_001C1330(unsigned short);
extern int func_001C32A8(int, int, int);
extern int func_001C2CD8(int, int, int);
extern int func_001C2DD0(int, int, int);
extern int func_001C4880(int, int, int);
extern int func_001C4860(int, int, int);
extern int func_001C4140(int, int, int, int, int);
extern int func_001C3120(int, int);
extern int func_001C2778(int, int);
extern int func_001C3810(int, int, int, int);
extern int func_001C3B20(int, int, int, int, int, int);
extern int func_001C3660(int, int, int, int);
extern int func_001C39B0(int, int, int, int, int, int);
extern int func_001C43D0(int, int, int, int, int);
extern int func_001C3CE8(int, int);
extern int func_001C2B90(int, int, int);

int func_001C0948(void)
{
	return func_001C2348();
}

int func_001C0980(void)
{
	return func_001C2390();
}

int func_001C09B8(int a)
{
	return func_001C28F8(a, 0);
}

int func_001C09F8(int a)
{
	func_001C4C20(0);
	func_001C4C98(0, 0, 0);
	return 0;
}

int func_001C0A40(int a)
{
	return 0;
}

int func_001C0A68(int a)
{
	return func_001C2A98(a);
}

int func_001C0AA8(int a, int b, int c)
{
	return func_001C4A20(a, b, c);
}

int func_001C0AF8(int a)
{
	return func_001C48A0(a);
}

int func_001C0B38(int a)
{
	return func_001C1348(a);
}

int func_001C0B78(int a, int b)
{
	return func_001C1378(a, b);
}

int func_001C0BC0(struct in_addr in)
{
	sceInsockInAddr_t t;

	t = *(sceInsockInAddr_t *)&in;
	return func_001C1590(t);
}

struct in_addr func_001C0C08(int net, int host)
{
	struct in_addr r;
	sceInsockInAddr_t t;

	t = func_001C15E0(net, host);
	r = *(struct in_addr *)&t;
	return r;
}

int func_001C0C58(struct in_addr in)
{
	sceInsockInAddr_t t;

	t = *(sceInsockInAddr_t *)&in;
	return func_001C1658(t);
}

int func_001C0CA0(int a)
{
	return func_001C1698(a);
}

char *func_001C0CE0(struct in_addr in)
{
	sceInsockInAddr_t t;

	t = *(sceInsockInAddr_t *)&in;
	return inet_ntoa(t);
}

unsigned int func_001C0D28(unsigned int x)
{
	return func_001C1308(x);
}

unsigned short func_001C0D68(unsigned short x)
{
	return func_001C1330(x);
}

unsigned int func_001C0DB0(unsigned int x)
{
	return func_001C1308(x);
}

unsigned short func_001C0DF0(unsigned short x)
{
	return func_001C1330(x);
}

int func_001C0E38(int s, int addr, int len)
{
	return func_001C32A8(s, addr, len);
}

int func_001C0E88(int s, int addr, int len)
{
	return func_001C2CD8(s, addr, len);
}

int func_001C0ED8(int s, int addr, int len)
{
	return func_001C2DD0(s, addr, len);
}

int func_001C0F28(int s, int addr, int len)
{
	return func_001C4880(s, addr, len);
}

int func_001C0F78(int s, int addr, int len)
{
	return func_001C4860(s, addr, len);
}

int func_001C0FC8(int s, int level, int name, int val, int len)
{
	return func_001C4140(s, level, name, val, len);
}

int func_001C1028(int s, int backlog)
{
	return func_001C3120(s, backlog);
}

int func_001C1070(int s, int buf, int len, int flags)
{
	func_001C2778(s, 600000);
	return func_001C3810(s, buf, len, flags);
}

int func_001C10D8(int s, int buf, int len, int flags, int from, int fromlen)
{
	func_001C2778(s, 600000);
	return func_001C3B20(s, buf, len, flags, from, fromlen);
}

int func_001C1150(int s, int buf, int len, int flags)
{
	return func_001C3660(s, buf, len, flags);
}

int func_001C11A8(int s, int buf, int len, int flags, int to, int tolen)
{
	return func_001C39B0(s, buf, len, flags, to, tolen);
}

int func_001C1210(int s, int level, int name, int val, int len)
{
	return func_001C43D0(s, level, name, val, len);
}

int func_001C1270(int s, int how)
{
	return func_001C3CE8(s, how);
}

int func_001C12B8(int domain, int type, int protocol)
{
	return func_001C2B90(domain, type, protocol);
}
