/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libpad2 (MAIN 0x1BD588-0x1BDE08): Sony's SDK controller library over libdbc's sockets -- not in
 * SOCOM 1's demo (it links libpad), so its rows keep their placeholders. A port table of sixteen
 * 820-byte records at 0x1E0E58 (data_001E0E58), each with a socket number at +12 and a DMA buffer
 * at +16 the DBC fills in two 128-byte halves, each half's frame counter at +124. GCC round 20a:
 * the start-up clear and the shutdown, the socket's deletion, its device number, a buffer's clear
 * (its stores in the one order of 720 tools/ccperm.py finds), the newer half of a buffer (cache
 * synced over 255 or 256 bytes) and the frame-changed test (guard clauses returning 0 first: the
 * combined `if (c && c != last)` lays the store first, 5 words). GCC round 30a, the object whole (14
 * of 14): the socket's creation (zin_pad.cpp's call: a parameter block and a 64-byte-aligned buffer,
 * libpad2's own message), the button read, the profile read, the state, a button's value (playpss.cpp
 * and fts_controller.cpp read buttons 14, 26 and 29 through it) and the profile's parse into each
 * port's 40-entry table at +0x14. What they showed: the state's receive command is a union of a
 * u_long bitfield struct and the int passed (a DImode register: the masks' upper words kept, the
 * low word sign-extended into a1; `*(int *)&cmd` goes through ADDRESSOF and builds it in a1 from
 * another register, 39 words), its failure block inside the else before the shared `return state`
 * (as a label at the end it is laid out last); a guard's `return -1` first where the success value
 * is the branch-likely's (`if (b->f04) return b->b2;` lays -1 in the slot); the creation's guard
 * clause on the socket and the state stored before the buffer (1 of 4 orders); a button's record
 * copied from `q + 20`, q the table's base plus `(n * 16 + port * 820)` -- the field spelled any
 * other way folds the 20 into the symbol (4-10 words); the parse's declarations `i, bit, byte,
 * dbit` (i and dbit trade a2/a3 otherwise). */

typedef unsigned char u_char;
typedef unsigned long u_long;

/* the caller's socket parameters (zin_pad.cpp passes its m_socketType) and libdbc's own, which the
 * creation fills from them: the type word ORed with 1, a word of 1 inserted after it */
typedef struct {
	int f0, f4, f8, f12;
	char name[16];
} PAD2SOCKETPARAM;

typedef struct {
	int f0, f4, f8, f12, f16;
	char name[16];
} sceDbcSocketParam;

/* one button's place in the read data, as the profile gives it (40 per port) */
typedef struct {
	int present;	/* 0x0 */
	int size;	/* 0x4: 1 (a bit, active low) or 8 (a byte, an analog value) */
	int byte;	/* 0x8 */
	int bit;	/* 0xC */
} PAD2BTN;

extern int sceDbcCreateSocket(sceDbcSocketParam *p, void *buf0, void *buf1);
extern int sceDbcDeleteSocket(int s);
extern int sceDbcGetDepNumber(int s);
extern int sceDbcReceiveData(int s, int a1, int *size, char *buf);
extern void SyncDCache(void *start, void *end);
extern void *memset(void *d, int c, unsigned int n);
extern void *memcpy(void *d, const void *s, unsigned int n);
extern void scePrintf(const char *fmt, ...);

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
	u_char btn[40][16];	/* 0x014: PAD2BTN's, the profile's parse (0x1BDD58) */
	u_char pad294[820 - 0x294];
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
int func_001BD938(int port);
void func_001BDB58(int port);
int func_001BDBD0(int port);
PAD2BUF *func_001BDC28(int port);
PAD2BUF *func_001BDC90(int port);
int func_001BDCF8(int port);
int func_001BDD58(u_char *profile, PAD2BTN *btn);

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

/* a socket created over the DBC, its buffer the caller's (64-byte aligned: two 128-byte halves) */
int func_001BD630(PAD2SOCKETPARAM *param, void *addr)
{
	sceDbcSocketParam p;
	int s;

	if ((int)addr & 0x3f) {
		scePrintf("libpad2: buffer addr is not 64 byte align. %08x\n", addr);
		return -1;
	}
	if (param) {
		p.f0 = param->f0;
		p.f8 = param->f4;
		p.f12 = param->f8;
		p.f16 = param->f12;
		memcpy(p.name, param->name, 16);
	} else {
		p.f0 = 0;
		p.f8 = 0;
		p.f12 = 0;
		p.f16 = 0;
		p.name[0] = 0;
	}
	p.f4 = 1;
	p.f0 |= 1;
	s = sceDbcCreateSocket(&p, addr, (char *)addr + 128);
	if (s < 0)
		return s;
	data_001E0E58[s].state = 1;
	data_001E0E58[s].buf = addr;
	func_001BDB58(s);
	return s;
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

/* the buttons read: the newer half's data copied out, its profile parsed into the port's table;
 * the data's length, or -1 */
int func_001BD790(int port, u_char *data)
{
	PAD2BUF *b;

	if (data_001E0E58[port].state == 0)
		return -1;
	if (data_001E0E58[port].open == 0 && func_001BDBD0(port) < 0)
		return -1;
	b = func_001BDC28(port);
	if (b->b2 && b->f1c) {
		memcpy(data, b->f1c, b->b2);
		func_001BDD58(b->f1c + b->b2, (PAD2BTN *)data_001E0E58[port].btn);
	}
	if (b->f04 == 0)
		return -1;
	return b->b2;
}

/* the button profile: the state queried first, the profile after the data copied out and parsed;
 * its length, or -1 */
int func_001BD868(int port, u_char *profile)
{
	PAD2BUF *b;

	func_001BD938(port);
	if (data_001E0E58[port].open == 0 && func_001BDBD0(port) < 0)
		return -1;
	b = func_001BDC90(port);
	if (b->b3 && b->f1c) {
		memcpy(profile, b->f1c + b->b2, b->b3);
		func_001BDD58(b->f1c + b->b2, (PAD2BTN *)data_001E0E58[port].btn);
	}
	if (b->f04 == 0)
		return -1;
	return b->b3;
}

/* the DBC's receive command: a 14-bit size, a 2-bit type and two bytes (in the low word) */
typedef struct {
	u_long size : 14;
	u_long type : 2;
	u_long f16 : 8;
	u_long f24 : 8;
} PAD2CMD;

/* the port's state: the newer half's first byte when its frame changed, else one byte received
 * from the device; 0 (the port closed, its buffer cleared) when either fails */
int func_001BD938(int port)
{
	PAD2BUF *b;
	union { PAD2CMD c; int i; } cmd;
	int size;
	u_char state;

	if (data_001E0E58[port].open == 0 && func_001BDBD0(port) < 0)
		return 0;
	if (func_001BDCF8(port)) {
		b = func_001BDC90(port);
		if (b->f04 == 0 && func_001BDBD0(port) < 0)
			goto fail;
		state = b->b0;
	} else {
		cmd.c.size = 12;
		cmd.c.type = 2;
		cmd.c.f16 = 1;
		cmd.c.f24 = 1;
		if (sceDbcReceiveData(port, cmd.i, &size, (char *)&state) < 0) {
fail:
			data_001E0E58[port].open = 0;
			func_001BDB58(port);
			return 0;
		}
	}
	return state;
}

/* one button's value out of the read data: a bit (pressed: 1) or a byte, by the profile's table;
 * -1 when the profile has no such button */
int func_001BDA78(int port, u_char *data, int n)
{
	PAD2BTN b;
	u_char *q;
	u_char v;

	q = (u_char *)data_001E0E58 + (n * 16 + port * 820);
	memcpy(&b, q + 20, 16);
	if (b.present) {
		switch (b.size) {
		case 1:
			if ((data[b.byte] >> b.bit) & 1)
				return 0;
			return 1;
		case 8:
			v = data[b.byte] >> b.bit;
			if (b.bit) {
				b.byte++;
				v |= data[b.byte] << (b.size - b.bit);
			}
			return v;
		}
	}
	return -1;
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

/* the profile parsed: one bit a button, each present one placed in the read data -- the analog ones
 * (16-31, 35-38) a byte each, the rest a bit each, packed eight to a byte */
int func_001BDD58(u_char *profile, PAD2BTN *btn)
{
	int i, bit = 0, byte = 0, dbit = 0;

	for (i = 0; i < 40; i++) {
		if ((*profile >> bit) & 1) {
			btn->present = 1;
			btn->byte = byte;
			btn->bit = dbit;
			if ((i >= 16 && i < 32) || (i >= 35 && i < 39)) {
				btn->size = 8;
				byte++;
			} else {
				dbit++;
				btn->size = 1;
				if ((dbit & 7) == 0) {
					byte++;
					dbit = 0;
				}
			}
		} else {
			btn->present = 0;
			btn->byte = 0;
			btn->bit = 0;
		}
		bit++;
		if ((bit & 7) == 0) {
			profile++;
			bit = 0;
		}
		btn++;
	}
	return 1;
}
