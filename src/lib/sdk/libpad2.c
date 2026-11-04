/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libpad2 (MAIN 0x1BD588-0x1BDE08): Sony's SDK controller library over libdbc's sockets -- not in
 * SOCOM 1's demo (it links libpad), so its rows keep their placeholders. A port table of sixteen
 * 820-byte records at 0x1E0E58 (data_001E0E58), each with a socket number at +12 and a DMA buffer
 * at +16 the DBC fills in two 128-byte halves, each half's frame counter at +124. GCC round 20a:
 * the start-up clear and the shutdown, the socket's deletion, its device number, a buffer's clear
 * (its stores in the one order of 720 tools/ccperm.py finds), the newer half of a buffer (cache
 * synced over 255 or 256 bytes) and the frame-changed test (guard clauses returning 0 first: the
 * combined `if (c && c != last)` lays the store first, 5 words). */

typedef unsigned char u_char;

extern int sceDbcDeleteSocket(int s);
extern int sceDbcGetDepNumber(int s);
extern void SyncDCache(void *start, void *end);
extern void *memset(void *d, int c, unsigned int n);

/* one half of a port's buffer, as the DBC writes it */
typedef struct {
	u_char b0, b1, b2, b3;	/* 0x00 */
	int f04;		/* 0x04 */
	u_char pad08[0x1C - 0x08];
	u_char f1c[32];		/* 0x1C: 0xFF when clear */
	u_char pad3c[0x7C - 0x3C];
	int count;		/* 0x7C: the frame it holds */
} PAD2BUF;

typedef struct {
	int state;		/* 0x000 */
	int open;		/* 0x004 */
	int f008;		/* 0x008 */
	int dep;		/* 0x00C: the socket, then its device number */
	PAD2BUF *buf;		/* 0x010 */
	u_char pad014[820 - 0x14];
} PAD2PORT;

extern PAD2PORT data_001E0E58[16];
extern int data_001CE568;

int func_001BD588(void)
{
	int i;

	data_001CE568 = 1;
	for (i = 0; i < 16; i++) {
		data_001E0E58[i].state = 0;
		data_001E0E58[i].open = 0;
		data_001E0E58[i].f008 = 0;
		data_001E0E58[i].dep = 0;
	}
	return 1;
}

int func_001BD738(int port);

int func_001BD5C8(void)
{
	int i, r;

	data_001CE568 = 0;
	for (i = 0; i < 16; i++) {
		if (data_001E0E58[i].state) {
			r = func_001BD738(i);
			if (r < 0)
				return r;
		}
	}
	return 1;
}

int func_001BD738(int port)
{
	int r = sceDbcDeleteSocket(port);

	if (r < 0)
		return r;
	data_001E0E58[port].state = 0;
	data_001E0E58[port].open = 0;
	data_001E0E58[port].dep = 0;
	return 1;
}

void func_001BDB58(int port)
{
	PAD2BUF *b = data_001E0E58[port].buf;
	int i;

	for (i = 0; i < 2; i++) {
		b->b0 = 0;
		b->count = 0;
		b->b1 = 0;
		b->b3 = 0;
		b->b2 = 0;
		b->f04 = 0;
		memset(b->f1c, 255, 32);
		b++;
	}
}

int func_001BDBD0(int port)
{
	int r = sceDbcGetDepNumber(port);

	if (r < 0)
		return r;
	data_001E0E58[port].dep = r;
	data_001E0E58[port].open = 1;
	return r;
}

PAD2BUF *func_001BDC28(int port)
{
	PAD2BUF *b[2];

	b[0] = data_001E0E58[port].buf;
	b[1] = b[0] + 1;
	SyncDCache(b[0], (u_char *)b[0] + 255);
	return b[b[0]->count < b[1]->count];
}

PAD2BUF *func_001BDC90(int port)
{
	PAD2BUF *b[2];

	b[0] = data_001E0E58[port].buf;
	b[1] = b[0] + 1;
	SyncDCache(b[0], (u_char *)b[0] + 256);
	return b[b[0]->count < b[1]->count];
}

int func_001BDCF8(int port)
{
	PAD2BUF *b = func_001BDC28(port);

	if (b->count == 0 || b->count == data_001E0E58[port].f008)
		return 0;
	data_001E0E58[port].f008 = b->count;
	return 1;
}
