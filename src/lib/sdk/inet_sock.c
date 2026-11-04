/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* The socket layer's core (MAIN 0x1C1CD0-0x1C4CC0, after BIND's inet_ntop.c): a socket API over
 * Sony's libinet RPC client (sceInetCreate/Open/Send/Recv/Close/Control/Name2Address/Address2Name/
 * Abort) and libnet (sceLibnetInitialize/Terminate), the -O0 BSD-name wrappers of inet_bsd.c its
 * callers. Not in SOCOM 1's demo (it links no network), so every row keeps its placeholder and
 * every name here is ours. Built, as the network library around it, with Sony's SDK toolchain at
 * -O2 -G0 (the drafts that matched first matched under 991111: dtls13010 compiles this C the same).
 *
 * What it holds, in the image's order (GCC round 20a):
 *  - the allocator: two hooks (0x1E5F70, 0x1E5F74) that default to newlib's malloc and free, and a
 *    first-fit heap the caller can hand it (func_001C2028 installs it as the hooks): every block a
 *    12-byte header {next, prev, size} on one of two lists kept in address order -- the used list,
 *    each allocation linked in before the used block that follows its free block, and the free list,
 *    each release linked in before the free block after its run of used neighbours and merged with
 *    both free neighbours;
 *  - the lock: one semaphore (0x1CE620), created at the first use;
 *  - each thread's context (0x68 bytes: its libnet client at +20, an error number at +12), found by
 *    the thread id on a list (0x1CE628) of at most 255, the oldest dropped first.
 *
 * The data are this object's own (.data and .bss); they keep the map's placeholders (data_<VA>)
 * until a data row names them. */

typedef unsigned int u_int;
typedef unsigned int size_t;

struct SemaParam {
	int currentCount;
	int maxCount;
	int initCount;
	int numWaitThreads;
	u_int attr;
	u_int option;
};

extern void *malloc(size_t n);
extern void free(void *p);
extern int CreateSema(struct SemaParam *p);
extern int WaitSema(int sema);
extern int SignalSema(int sema);
extern int GetThreadId(void);
extern int *__errno(void);
extern int sceLibnetInitialize(void *client, u_int bufsize, int a, int b);
extern int sceLibnetTerminate(void *client);

/* the first-fit heap's block header: the payload follows it */
typedef struct _blk {
	struct _blk *next;	/* 0x0 */
	struct _blk *prev;	/* 0x4 */
	u_int size;		/* 0x8: the payload's */
} BLK;

/* a thread's context */
typedef struct _ctx {
	struct _ctx *next;	/* 0x00 */
	struct _ctx *prev;	/* 0x04 */
	int tid;		/* 0x08 */
	int err;		/* 0x0C */
	int herr;		/* 0x10 */
	int client[16];		/* 0x14: libnet's client (sceLibnetInitialize) */
	void *buf;		/* 0x54: the client's buffer as allocated */
	void *abuf;		/* 0x58: aligned to 64 */
	int bufsize;		/* 0x5C */
	int rpcsize;		/* 0x60 */
	int prio;		/* 0x64 */
} CTX;

extern void *(*data_001E5F70)(size_t n);	/* the allocator's hooks */
extern void (*data_001E5F74)(void *p);
extern BLK *data_001E5F78;			/* the used list */
extern BLK *data_001E5F7C;
extern BLK *data_001E5F80;			/* the free list */
extern BLK *data_001E5F84;
extern void *data_001E5F88;			/* the heap */
extern void *data_001E5F8C;
extern int data_001CE620;			/* the lock */
extern int data_001CE624;			/* the contexts: how many */
extern CTX *data_001CE628;			/* the list */
extern CTX *data_001CE62C;

void *func_001C1CD0(size_t n)
{
	if (data_001E5F70)
		return data_001E5F70(n);
	return malloc(n);
}

void func_001C1D08(void *p)
{
	if (data_001E5F74 == 0) {
		free(p);
		return;
	}
	data_001E5F74(p);
}

void *func_001C1D40(size_t size)
{
	BLK *p, *b, *n;
	u_int rest;

	size = (size + 3) & ~3U;
	for (p = data_001E5F80; p; p = p->next)
		if (size < p->size)
			break;
	if (p == 0)
		return 0;

	b = p;
	rest = p->size - sizeof(BLK) - size;
	if (rest) {
		/* split: the rest stays free, in b's place */
		p = (BLK *)((char *)b + sizeof(BLK) + size);
		b->size = size;
		p->size = rest;
		p->next = b->next;
		if (p->next)
			p->next->prev = p;
		else
			data_001E5F84 = p;
		p->prev = b->prev;
		if (p->prev)
			p->prev->next = p;
		else
			data_001E5F80 = p;
	} else {
		if (b->next)
			b->next->prev = b->prev;
		else
			data_001E5F84 = b->prev;
		if (b->prev)
			b->prev->next = b->next;
		else
			data_001E5F80 = b->next;
	}

	/* the used list in address order: before the block that follows the free one */
	n = (BLK *)((char *)p + p->size + sizeof(BLK));
	if (data_001E5F78 == 0) {
		b->prev = 0;
		b->next = 0;
		data_001E5F7C = b;
		data_001E5F78 = b;
	} else if ((void *)n >= data_001E5F8C) {
		b->next = 0;
		b->prev = data_001E5F7C;
		data_001E5F7C->next = b;
		data_001E5F7C = b;
	} else {
		b->prev = n->prev;
		if (n->prev)
			n->prev->next = b;
		else
			data_001E5F78 = b;
		b->next = n;
		n->prev = b;
	}
	return (char *)b + sizeof(BLK);
}

void func_001C1E90(void *ptr)
{
	BLK *b, *p, *n;
	u_int size;

	if (ptr == 0)
		return;
	b = (BLK *)((char *)ptr - sizeof(BLK));

	/* off the used list */
	if (b->next)
		b->next->prev = b->prev;
	else
		data_001E5F7C = b->prev;
	if (b->prev)
		b->prev->next = b->next;
	else
		data_001E5F78 = b->next;

	/* the free block after b's run of used neighbours */
	size = b->size;
	p = b;
	while (p->next == (BLK *)((char *)p + p->size + sizeof(BLK)))
		p = p->next;
	n = (BLK *)((char *)p + p->size + sizeof(BLK));
	if ((void *)n < data_001E5F8C) {
		b->prev = n->prev;
		if (n->prev)
			n->prev->next = b;
		else
			data_001E5F80 = b;
		b->next = n;
		n->prev = b;
	} else {
		b->next = 0;
		b->prev = data_001E5F84;
		if (data_001E5F84)
			data_001E5F84->next = b;
		else
			data_001E5F80 = b;
		data_001E5F84 = b;
	}

	/* merged with the free neighbours */
	p = b->prev;
	if (p && (BLK *)((char *)p + p->size + sizeof(BLK)) == b) {
		p->size = p->size + sizeof(BLK) + size;
		p->next = p->next->next;
		if (p->next)
			p->next->prev = p;
		else
			data_001E5F84 = p;
		b = p;
	}
	n = b->next;
	if (n && (BLK *)((char *)b + b->size + sizeof(BLK)) == n) {
		b->size = b->size + sizeof(BLK) + n->size;
		b->next = n->next;
		if (n->next)
			n->next->prev = b;
		else
			data_001E5F84 = b;
	}
}

int func_001C2028(void *heap, u_int size)
{
	u_int mis;
	BLK *b;

	mis = (u_int)heap & 3;
	if (mis) {
		if (size < mis)
			return -1;
		size -= mis;
		heap = (char *)heap + mis;
	}
	if (size < sizeof(BLK))
		return -1;
	b = heap;
	data_001E5F80 = b;
	data_001E5F84 = b;
	b->prev = 0;
	b->next = 0;
	data_001E5F8C = (char *)heap + size;
	data_001E5F70 = func_001C1D40;
	data_001E5F74 = func_001C1E90;
	b->size = size - sizeof(BLK);
	data_001E5F88 = heap;
	return 0;
}

int func_001C20D8(void)
{
	struct SemaParam sp;

	if (data_001CE620 < 0) {
		sp.initCount = 1;
		sp.maxCount = 1;
		sp.option = 0;
		data_001CE620 = CreateSema(&sp);
		if (data_001CE620 < 0)
			return -1;
	}
	return WaitSema(data_001CE620) < 0 ? -1 : 0;
}

/* a socket's record: on the list at 0x1CE630, by its descriptor */
typedef struct _sock {
	struct _sock *next;	/* 0x00 */
	struct _sock *prev;	/* 0x04 */
	int id;			/* 0x08: the descriptor */
	int pad0c[17];
	struct _sock *conn;	/* 0x50: its connections */
	int pad54;
	struct _sock *pend;	/* 0x58 */
	int pad5c;
	int opt60;		/* 0x60 */
	int opt64;		/* 0x64 */
	int opt68;		/* 0x68 */
} SOCK;

extern SOCK *data_001CE630;
extern int data_001CE638;		/* the error numbers without a context */
extern int data_001CE63C;

void func_001C2140(void)
{
	if (data_001CE620 > 0)
		SignalSema(data_001CE620);
}

/* the calling thread's context, the lock held (the caller's func_001C2310 releases it); made when
 * the thread has none, the oldest dropped past 255; its error number cleared unless asked to keep */
CTX *func_001C2160(int keep)
{
	int tid;
	CTX *c;

	tid = GetThreadId();
	if (func_001C20D8() < 0)
		return 0;
	for (c = data_001CE628; c; c = c->next)
		if (tid == c->tid)
			goto found;
	if (++data_001CE624 >= 256) {
		c = data_001CE62C;
		if (c->prev)
			c->prev->next = c->next;
		else
			data_001CE628 = c->next;
		if (c->next)
			c->next->prev = c->prev;
		else
			data_001CE62C = c->prev;
		sceLibnetTerminate(c->client);
		func_001C1D08(c->buf);
		func_001C1D08(c);
		data_001CE624--;
	}
	c = func_001C1CD0(sizeof(CTX));
	if (c == 0)
		goto found;
	c->tid = tid;
	c->bufsize = 2048;
	c->rpcsize = 8192;
	c->prio = 32;
	c->buf = func_001C1CD0(2048 + 127);
	if (c->buf == 0) {
		func_001C1D08(c);
		func_001C2140();
		return 0;
	}
	c->abuf = (void *)(((u_int)c->buf + 63) & ~63);
	sceLibnetInitialize(c->client, (c->bufsize + 99) & ~63, c->rpcsize, c->prio);
	c->next = data_001CE628;
	if (data_001CE628)
		data_001CE628->prev = c;
	else
		data_001CE62C = c;
	data_001CE628 = c;
	c->prev = 0;
found:
	if (!keep)
		c->err = 0;
	return c;
}

int func_001C2310(CTX *c, int err)
{
	c->err = err;
	*__errno() = err;
	func_001C2140();
	return err == 0 ? 0 : -1;
}

int *func_001C2348(void)
{
	CTX *c = func_001C2160(1);

	if (c == 0)
		return &data_001CE638;
	func_001C2140();
	return &c->err;
}

int *func_001C2390(void)
{
	CTX *c = func_001C2160(1);

	if (c == 0)
		return &data_001CE63C;
	func_001C2140();
	return &c->herr;
}

SOCK *func_001C23D8(int id)
{
	SOCK *s;

	for (s = data_001CE630; s; s = s->next)
		if (id == s->id)
			return s;
	return 0;
}

/* libinet's error codes as errno values */
int func_001C24D0(int err)
{
	switch (err) {
	case 0:
		return 0;
	case -500:
		return 116;
	case -501:
		return 113;
	case -502:
		return 16;
	case -503:
		return 115;
	case -504:
		return 12;
	case -505:
	case -506:
		return 125;
	case -507:
		return 127;
	case -508:
		return 128;
	case -509:
		return 110;
	case -510:
		return 104;
	case -511:
		return 111;
	case -512:
	case -513:
		return 22;
	case -514:
		return 118;
	}
	return -1;
}

/* the lowest descriptor no socket and no connection of one holds */
int func_001C25D0(void)
{
	int id = 0;
	SOCK *s, *c;

	for (;;) {
		for (s = data_001CE630; s; s = s->next) {
			if (id == s->id)
				break;
			for (c = s->conn; c; c = c->next)
				if (id == c->id)
					break;
			if (c)
				break;
			for (c = s->pend; c; c = c->next)
				if (id == c->id)
					break;
			if (c)
				break;
		}
		if (s == 0)
			break;
		id++;
	}
	return id;
}

int func_001C2778(int id, int v)
{
	CTX *c;
	SOCK *s;

	c = func_001C2160(0);
	if (c == 0)
		return -1;
	s = func_001C23D8(id);
	if (s == 0)
		return func_001C2310(c, 9);
	s->opt60 = v;
	func_001C2310(c, 0);
	return 0;
}

int func_001C27F8(int id, int v)
{
	CTX *c;
	SOCK *s;

	c = func_001C2160(0);
	if (c == 0)
		return -1;
	s = func_001C23D8(id);
	if (s == 0)
		return func_001C2310(c, 9);
	s->opt64 = v;
	func_001C2310(c, 0);
	return 0;
}

int func_001C2878(int id, int v)
{
	CTX *c;
	SOCK *s;

	c = func_001C2160(0);
	if (c == 0)
		return -1;
	s = func_001C23D8(id);
	if (s == 0)
		return func_001C2310(c, 9);
	s->opt68 = v;
	func_001C2310(c, 0);
	return 0;
}
