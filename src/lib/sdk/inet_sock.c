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
 * until a data row names them. Shapes the target showed: the new free block's neighbours are tested
 * through the copies just stored into it (`p->next = b->next; if (p->next) ...`: testing b->next
 * reloads it past the stores, 2 words); the lock's semaphore is created with only its counts and
 * option set; libinet's errors map through one switch in libinet's code order (the case tree's
 * leaves laid out in source order); libnet's buffer size is masked with an unsigned ~63U (lui/ori,
 * held across the call) where the aligned pointer's mask is a plain -64.
 *
 *  - the BSD calls (GCC round 21a): socket 0x1C2B90, bind 0x1C2CD8, connect 0x1C2DD0, listen 0x1C3120
 *    (a libinet listener per backlog slot, 0x1C2F38), send/recv 0x1C3660/0x1C3810, sendto/recvfrom
 *    0x1C39B0/0x1C3B20, close 0x1C3CE8, accept 0x1C32A8 (OPEN), gethostbyaddr 0x1C4A20 (OPEN), the
 *    socket's abort 0x1C28F8, poll 0x1C3E60, getsockopt 0x1C4140, setsockopt 0x1C43D0 (GCC round 45a) and around them the thread's buffer resize 0x1C2648, its
 *    context's drop 0x1C2A98, the send side's lazy open 0x1C35A0, an option setter 0x1C4088, the
 *    abort 0x1C4C20; each takes the thread's context with the lock,
 *    releases the lock around every libinet call (func_001C2140 ... func_001C20D8) and returns
 *    through func_001C2310 with an errno.
 *
 * Shapes the target showed, round 21a: EVERY UNLINK READS THE OTHER LINK INTO A LOCAL FIRST --
 * `{ CTX *n = c->next; if (c->prev) c->prev->next = n; else head = n; }` and the second test on the
 * fields themselves: the target's non-annulled slot loads the local before the branch and the
 * reloads after the store come in the target's order (written on the fields, the context's three
 * unlinks reloaded prev before next, the socket's beqzl'd its first arm: 2-24 words; ten spellings
 * of the fields alike, build/scratch/gcc/gen_unlink*.py, gen_close.py). libnet's buffer mask is a
 * local `mask = ~63U` assigned before the buffer's allocation (its lui/ori in the freed tid's s1,
 * held across the calls) and the aligned pointer an int's (`li -64`: as u_int it is the mask's
 * constant, which CSE then reuses). socket()'s case 2 is written first (its body follows the case
 * tree, so the tree's last test is `beq 3 -> the allocation; b default`), its record filled in field
 * order (one of three orders of 120). The send side's open nests its body under `if (s->cid ==
 * 0)`, its final `return 0` laid out last, and so does the listener slot's (`if (r < 0 && r != -500)
 * return func_001C24D0(r); return 0;`: its error call jumps over the epilogue's first word); the
 * slot's new tail is tested through the copy just stored (`t->prev = s->ctail; if (t->prev)`), its
 * -1 options in field order (one of 24). THE DATA CALLS RETURN A LONG: each `return
 * func_001C2310(c, X)` widens the int with a register move (extendsidi2's `move v0,v0`, which
 * survives as no no-op move would), so the error paths' tails are two insns and cross-jump into one
 * call site as the target's (as int: each its own call, 86 words); their piece size is an if/else
 * (`n = len < ... ? ... : len` folds to a signed MIN: `slt` for the target's `sltu`).
 *
 * getsockname/getpeername func_001C46D0 (GCC round 78; open since 21a, 4 words): the port's
 * conversion and store are written in EACH arm -- one call site in the target is jump2's cross-jump
 * of the two arms' tails, so the 16 for *namelen sits in the join's own block, where sched1 cannot
 * hoist it above the sh and local-alloc hands it the call's dead v0.
 *
 * OPEN: func_001C1D40, the first-fit allocation (0x14C against 0x150: the target computes the
 *   remainder as (size' - 12) - size from the loop's own load of the block's size, and after a split
 *   reloads the new block's size where the exact fit keeps the loop's -- nine spellings of the
 *   remainder over four loop forms are all a word short; build/scratch/gcc/gen_malloc.py);
 *   func_001C1E90, the release (0x1B0 against 0x198: the run of used neighbours is walked with the
 *   size held from the start, the merges reuse the adjacency sums -- the walk's shape is not yet the
 *   target's); func_001C2028, the heap's set-up (right size, 34: the first `return -1` is the one
 *   the second test jumps back to, and nine stores in another order -- not yet permuted);
 *   func_001C32A8, accept (10 words, right size: the peer address's pointer and libnet's client
 *   address trade s5/s6 -- global-alloc's priorities; four spellings alike, gen_accept.py; the
 *   lists' pushes and unlinks, the gotos' layout and every store as the target's);
 *    *   func_001C48A0, gethostbyname (0x178 against 0x180: the target keeps each error path's
 *   `func_001C2310(c, X); return 0;` apart -- three calls, each `b epilogue; move v0,zero` -- where
 *   this compile cross-jumps them into one call; the h_errno chain's value through one store as
 *   the target's); func_001C4A20, gethostbyaddr (0x1EC against 0x1FC: the same question -- the
 *   target cross-jumps the type, length and h_errno paths into the lock failure's call and keeps
 *   the errno path's call apart, this compile merges all four; its h_errno 2 and 4 stored from the
 *   checked type and length, as the target's). */

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
extern int DeleteSema(int sema);
extern void *memset(void *p, int c, size_t n);
extern void *memcpy(void *d, const void *s, size_t n);
extern int sceInetCreate(void *cd, void *buf, void *param);
extern int sceInetOpen(void *cd, void *buf, int s, int ms);
extern int sceInetAbort(void *cd, void *buf, int s, int flags);
extern int sceInetClose(void *cd, void *buf, int s, int ms);
extern int sceInetAddress2Name(void *cd, void *buf, int flags, char *name, int len, void *paddr, int ms,
			       int nretry);
extern void bcopy(const void *src, void *dst, size_t n);
extern int sceInetSend(void *cd, void *buf, int s, void *ptr, int count, int *pflags, int ms);
extern int sceInetRecv(void *cd, void *buf, int s, void *ptr, int count, int *pflags, int ms);
extern int sceInetSendTo(void *cd, void *buf, int s, void *ptr, int count, int *pflags, void *paddr,
			 int port, int ms);
extern int sceInetRecvFrom(void *cd, void *buf, int s, void *ptr, int count, int *pflags, void *paddr,
			   int *pport, int ms);
extern int sceInetControl(void *cd, void *buf, int s, int code, void *ptr, int len);
extern int sceInetName2Address(void *cd, void *buf, int flags, void *paddr, const char *name, int ms,
			       int nretry);

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

/* libinet's address and connection parameters (libinet.h's sceInetAddress and sceInetParam) */
typedef struct {
	int reserved;
	unsigned char data[12];	/* +4: the IPv4 address, network order */
} sceInetAddress_t;

typedef struct {
	int type;			/* +0x00 */
	int local_port;			/* +0x04: -1 any */
	sceInetAddress_t remote_addr;	/* +0x08 */
	int remote_port;		/* +0x18 */
	int reserved[9];
} sceInetParam_t;

/* a BSD socket address (4.4BSD's, with its length byte) */
struct sockaddr_in {
	unsigned char sin_len;
	unsigned char sin_family;
	unsigned short sin_port;
	u_int sin_addr;
	char sin_zero[8];
};

/* a socket's record: on the list at 0x1CE630, by its descriptor */
typedef struct _sock {
	struct _sock *next;	/* 0x00 */
	struct _sock *prev;	/* 0x04 */
	int id;			/* 0x08: the descriptor */
	int cid;		/* 0x0C: libinet's connection */
	sceInetParam_t param;	/* 0x10 */
	struct _sock *conn;	/* 0x50: its connections */
	struct _sock *ctail;	/* 0x54: the last of them */
	struct _sock *pend;	/* 0x58: accepted, not yet open */
	struct _sock *ptail;	/* 0x5C */
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
	u_int mask;

	tid = GetThreadId();
	if (func_001C20D8() < 0)
		return 0;
	for (c = data_001CE628; c; c = c->next)
		if (tid == c->tid)
			goto found;
	if (++data_001CE624 >= 256) {
		c = data_001CE62C;
		{
			CTX *n = c->next;

			if (c->prev)
				c->prev->next = n;
			else
				data_001CE628 = n;
		}
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
	mask = ~63U;
	c->buf = func_001C1CD0((c->bufsize + 99) | 63);
	if (c->buf == 0) {
		func_001C1D08(c);
		func_001C2140();
		return 0;
	}
	c->abuf = (void *)(((int)c->buf + 63) & ~63);
	sceLibnetInitialize(c->client, (c->bufsize + 99) & mask, c->rpcsize, c->prio);
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

/* the calling thread's libnet buffer resized: its client restarted over a new buffer (the context
 * dropped when there is none) */
int func_001C2648(int bufsize, int rpcsize, int prio)
{
	CTX *c;
	u_int mask;

	c = func_001C2160(0);
	if (c == 0)
		return -1;
	mask = ~63U;
	sceLibnetTerminate(c->client);
	func_001C1D08(c->buf);
	c->rpcsize = rpcsize;
	c->prio = prio;
	c->bufsize = bufsize;
	c->buf = func_001C1CD0((bufsize + 99) | 63);
	if (c->buf == 0) {
		{
			CTX *n = c->next;

			if (c->prev)
				c->prev->next = n;
			else
				data_001CE628 = n;
		}
		if (c->next)
			c->next->prev = c->prev;
		else
			data_001CE62C = c->prev;
		func_001C1D08(c);
		return func_001C2310(c, 12);
	}
	c->abuf = (void *)(((int)c->buf + 63) & ~63);
	sceLibnetInitialize(c->client, (c->bufsize + 99) & mask, c->rpcsize, c->prio);
	return func_001C2310(c, 0);
}

/* a thread's context dropped (the calling thread's for 0); the lock deleted with the last */
int func_001C2A98(int tid)
{
	CTX *c;

	if (tid == 0)
		tid = GetThreadId();
	if (func_001C20D8() < 0)
		return -1;
	for (c = data_001CE628; c; c = c->next)
		if (tid == c->tid)
			break;
	if (c) {
		{
			CTX *n = c->next;

			if (c->prev)
				c->prev->next = n;
			else
				data_001CE628 = n;
		}
		if (c->next)
			c->next->prev = c->prev;
		else
			data_001CE62C = c->prev;
		sceLibnetTerminate(c->client);
		func_001C1D08(c->buf);
		func_001C1D08(c);
		if (--data_001CE624 <= 0) {
			DeleteSema(data_001CE620);
			data_001CE620 = -1;
			return 0;
		}
	}
	func_001C2140();
	return 0;
}

extern SOCK *data_001CE634;

/* socket(): a record on the list under the lowest free descriptor */
int func_001C2B90(int domain, int type, int protocol)
{
	CTX *c;
	SOCK *s;
	int id;

	c = func_001C2160(0);
	if (c == 0)
		return -1;
	if (domain != 2)
		return func_001C2310(c, 96);
	id = func_001C25D0();
	switch (type) {
	case 2:
		type = 0;
		break;
	case 1:
	case 3:
		break;
	default:
		return func_001C2310(c, 107);
	}
	s = func_001C1CD0(sizeof(SOCK));
	if (s == 0)
		return func_001C2310(c, 12);
	memset(s, 0, sizeof(SOCK));
	s->id = id;
	s->param.type = type;
	s->opt60 = -1;
	s->opt64 = -1;
	s->opt68 = -1;
	s->next = data_001CE630;
	if (data_001CE630)
		data_001CE630->prev = s;
	else
		data_001CE634 = s;
	data_001CE630 = s;
	s->prev = 0;
	func_001C2310(c, 0);
	return id;
}

extern int func_001C2488(int port);
extern void func_001C2410(sceInetAddress_t *a, u_int *in);

/* bind(): the local port, refused when another socket holds it */
int func_001C2CD8(int id, struct sockaddr_in *addr, int len)
{
	CTX *c;
	SOCK *s, *t;
	int port;

	c = func_001C2160(0);
	if (c == 0)
		return -1;
	s = func_001C23D8(id);
	if (s == 0)
		return func_001C2310(c, 9);
	if (len != 16)
		return func_001C2310(c, 22);
	if (addr->sin_family != 2)
		return func_001C2310(c, 106);
	port = func_001C2488(addr->sin_port);
	for (t = data_001CE630; t; t = t->next)
		if (port == t->param.local_port)
			return func_001C2310(c, 112);
	s->param.local_port = port;
	return func_001C2310(c, 0);
}

/* connect(): libinet's connection created and opened, the lock released around each call */
int func_001C2DD0(int id, struct sockaddr_in *addr, int len)
{
	CTX *c;
	SOCK *s;
	int r;

	c = func_001C2160(0);
	if (c == 0)
		return -1;
	s = func_001C23D8(id);
	if (s == 0)
		return func_001C2310(c, 9);
	if (len != 16)
		return func_001C2310(c, 22);
	if (addr->sin_family != 2)
		return func_001C2310(c, 106);
	func_001C2410(&s->param.remote_addr, &addr->sin_addr);
	s->param.remote_port = func_001C2488(addr->sin_port);
	if (s->param.local_port == 0)
		s->param.local_port = -1;
	func_001C2140();
	r = sceInetCreate(c->client, c->abuf, &s->param);
	if (func_001C20D8() < 0)
		return func_001C2310(c, -1);
	if (r <= 0)
		return func_001C2310(c, func_001C24D0(r));
	s->cid = r;
	func_001C2140();
	r = sceInetOpen(c->client, c->abuf, s->cid, -1);
	if (func_001C20D8() < 0)
		return func_001C2310(c, -1);
	if (r < 0)
		return func_001C2310(c, func_001C24D0(r));
	return func_001C2310(c, 0);
}

extern int func_001C2F38(CTX *c, SOCK *s);

/* libinet's connection information (sceInetControl's GET_INFO) */
typedef struct {
	int cid;
	int proto;
	int recv_queue_length;
	int send_queue_length;
	sceInetAddress_t local_adr;	/* 0x10 */
	int local_port;			/* 0x20 */
	sceInetAddress_t remote_adr;
	int remote_port;
	int state;
	int reserved[4];
} sceInetInfo_t;

/* listen(): a listening connection per backlog slot; the port the first one got kept */
int func_001C3120(int id, int backlog)
{
	CTX *c;
	SOCK *s;
	int i, r;
	sceInetInfo_t info;

	c = func_001C2160(0);
	if (c == 0)
		return -1;
	s = func_001C23D8(id);
	if (s == 0)
		return func_001C2310(c, 9);
	if (s->param.type != 1)
		return func_001C2310(c, 95);
	s->param.type = 2;
	if (s->param.local_port == 0)
		s->param.local_port = -1;
	for (i = 0; i < backlog; i++) {
		r = func_001C2F38(c, s);
		if (r)
			return func_001C2310(c, r);
		if (i == 0 && s->param.local_port == -1) {
			func_001C2140();
			r = sceInetControl(c->client, c->abuf, s->conn->cid, 1, &info, sizeof(info));
			if (func_001C20D8() < 0)
				return func_001C2310(c, -1);
			if (r < 0)
				return func_001C2310(c, func_001C24D0(r));
			s->param.local_port = info.local_port;
		}
	}
	return func_001C2310(c, 0);
}

struct hostent {
	char *h_name;
	char **h_aliases;
	int h_addrtype;
	int h_length;
	char **h_addr_list;
};

extern struct hostent data_001E6098;
extern char *data_001CE640[];
extern char *data_001CE648[];
extern u_int data_001E5F90;
extern void func_001C2458(u_int *in, sceInetAddress_t *a);

/* gethostbyname(): libinet's resolver into one static hostent */
struct hostent *func_001C48A0(const char *name)
{
	struct hostent *h = &data_001E6098;
	CTX *c;
	int r;
	sceInetAddress_t addr;

	c = func_001C2160(0);
	if (c == 0)
		return 0;
	c->herr = 0;
	func_001C2140();
	r = sceInetName2Address(c->client, c->abuf, 65, &addr, name, 0, 0);
	if (func_001C20D8() < 0) {
		c->herr = -1;
		func_001C2310(c, -1);
		return 0;
	}
	if (r < 0) {
		if (r == -500 || r == -515)
			c->herr = 1;
		else if (r == -516)
			c->herr = 2;
		else if (r == -517)
			c->herr = 3;
		else if (r == -518)
			c->herr = 4;
		else {
			c->herr = -1;
			func_001C2310(c, func_001C24D0(r));
			return 0;
		}
		func_001C2310(c, 0);
		return 0;
	}
	h->h_name = 0;
	h->h_aliases = data_001CE640;
	h->h_addrtype = 2;
	h->h_length = 4;
	func_001C2458(&data_001E5F90, &addr);
	c->herr = 0;
	data_001CE648[0] = (char *)&data_001E5F90;
	h->h_addr_list = data_001CE648;
	func_001C2310(c, 0);
	return h;
}

/* every pending libinet call of the thread aborted */
int func_001C4C20(int unused)
{
	CTX *c;

	c = func_001C2160(0);
	if (c == 0)
		return -1;
	func_001C2140();
	sceInetAbort(c->client, c->abuf, 0, 0x52534C56);
	if (func_001C20D8() < 0)
		return func_001C2310(c, -1);
	return func_001C2310(c, 0);
}

/* a datagram or raw socket's libinet connection made at its first send (128 for a connection socket
 * never connected) */
int func_001C35A0(CTX *c, SOCK *s)
{
	int r;

	if (s->cid == 0) {
		if (s->param.type != 0 && s->param.type != 3)
			return 128;
		memset(&s->param.remote_addr, 0, sizeof(s->param.remote_addr));
		s->param.remote_port = 0;
		if (s->param.local_port == 0)
			s->param.local_port = -1;
		r = sceInetCreate(c->client, c->abuf, &s->param);
		if (r <= 0)
			return func_001C24D0(r);
		s->cid = r;
		r = sceInetOpen(c->client, c->abuf, r, -1);
		if (r < 0)
			return func_001C24D0(r);
	}
	return 0;
}

/* close(): the socket's connection closed, then each pending one, and the record freed */
int func_001C3CE8(int id)
{
	CTX *c;
	SOCK *s, *t;
	int r;

	c = func_001C2160(0);
	if (c == 0)
		return -1;
	s = func_001C23D8(id);
	if (s == 0)
		return func_001C2310(c, 9);
	r = 0;
	if (s->cid) {
		func_001C2140();
		r = sceInetClose(c->client, c->abuf, s->cid, s->opt68);
		if (func_001C20D8() < 0)
			return func_001C2310(c, -1);
	}
	{
		SOCK *p = s->prev;

		if (s->next)
			s->next->prev = p;
		else
			data_001CE634 = p;
	}
	if (s->prev)
		s->prev->next = s->next;
	else
		data_001CE630 = s->next;
	while ((t = s->conn) != 0) {
		s->conn = t->next;
		if (t->cid) {
			func_001C2140();
			sceInetClose(c->client, c->abuf, t->cid, 0);
			if (func_001C20D8() < 0)
				return func_001C2310(c, -1);
		}
		func_001C1D08(t);
	}
	func_001C1D08(s);
	if (r < 0)
		return func_001C2310(c, func_001C24D0(r));
	return func_001C2310(c, 0);
}

/* an option of the socket's connection set through libinet (code 16, one word) */
int func_001C4088(int id, int val)
{
	CTX *c;
	SOCK *s;
	int r;

	c = func_001C2160(0);
	if (c == 0)
		return -1;
	s = func_001C23D8(id);
	if (s == 0)
		return func_001C2310(c, 9);
	func_001C2140();
	r = sceInetControl(c->client, c->abuf, s->cid, 16, &val, 4);
	if (func_001C20D8() < 0)
		return func_001C2310(c, -1);
	if (r < 0)
		return func_001C2310(c, func_001C24D0(r));
	return func_001C2310(c, 0);
}

/* poll() over libinet's poll (0x245CD0, OVL1: RPC 20): each descriptor swapped for its connection's id
 * (the socket's first pending connection for a listening TCP socket, else the socket's own, opened
 * on the way; 0 for what has none), the ids put back afterwards */
extern int func_00245CD0(void *cd, void *buf, void *fds, int nfds, int ms);

struct pollfd_ {
	int fd;
	short events;
	short revents;
};

int func_001C3E60(struct pollfd_ *fds, int nfds, int timeout)
{
	CTX *c;
	SOCK *s;
	struct pollfd_ *p;
	int *saved;
	int i, r;

	c = func_001C2160(0);
	if (c == 0)
		return -1;
	if (fds == 0 || nfds <= 0)
		return func_001C2310(c, 22);
	saved = func_001C1CD0(nfds * 4);
	if (saved == 0)
		return func_001C2310(c, 12);
	for (i = 0; i < nfds; i++)
		saved[i] = fds[i].fd;
	p = fds;
	for (i = 0; i < nfds; i++) {
		s = func_001C23D8(p->fd);
		if (s == 0)
			p->fd = 0;
		else if (s->param.type == 2 && s->conn != 0)
			p->fd = s->conn->cid;
		else if (func_001C35A0(c, s) != 0)
			p->fd = 0;
		else
			p->fd = s->cid;
		p++;
	}
	func_001C2140();
	r = func_00245CD0(c->client, c->abuf, fds, nfds, timeout);
	if (func_001C20D8() < 0) {
		func_001C1D08(saved);
		return func_001C2310(c, -1);
	}
	for (i = 0; i < nfds; i++)
		fds[i].fd = saved[i];
	func_001C1D08(saved);
	if (r < 0)
		return func_001C2310(c, func_001C24D0(r));
	func_001C2310(c, 0);
	return r;
}

/* getsockopt(): TCP's one option (level 6, name 1: bit 0 of libinet's flags), IP's three (level 0: 9 the
 * multicast interface, 10 and 11 two words), each through libinet's control call */
int func_001C4140(int id, int level, int name, void *val, u_int *len)
{
	sceInetAddress_t addr;
	u_int flags;
	CTX *c;
	SOCK *s;
	int r = -1;
	u_int size;

	c = func_001C2160(0);
	if (c == 0)
		return -1;
	s = func_001C23D8(id);
	if (s == 0)
		return func_001C2310(c, 9);
	if (level == 6) {
		if (s->cid == 0)
			return func_001C2310(c, 9);
		if ((s->param.type != 1 && s->param.type != 2) || name != 1)
			return func_001C2310(c, 109);
		size = 4;
	} else if (level == 0) {
		if (s->param.type != 0)
			return func_001C2310(c, 109);
		if (name == 9)
			size = 4;
		else if (name == 10)
			size = 4;
		else if (name == 11)
			size = 4;
		else
			return func_001C2310(c, 109);
		r = func_001C35A0(c, s);
		if (r != 0)
			return func_001C2310(c, r);
	} else
		return func_001C2310(c, 109);
	if (val == 0 || len == 0 || *len < 4)
		return func_001C2310(c, 22);
	func_001C2140();
	if (level == 6 && name == 1) {
		r = sceInetControl(c->client, c->abuf, s->cid, 2, &flags, 4);
		*(int *)val = flags & 1;
	} else if (level == 0) {
		if (name == 9) {
			r = sceInetControl(c->client, c->abuf, s->cid, 5, &addr, sizeof(addr));
			func_001C2458(val, &addr);
		} else if (name == 10)
			r = sceInetControl(c->client, c->abuf, s->cid, 7, val, 4);
		else if (name == 11)
			r = sceInetControl(c->client, c->abuf, s->cid, 9, val, 4);
	}
	if (func_001C20D8() < 0)
		return func_001C2310(c, -1);
	if (r < 0)
		return func_001C2310(c, func_001C24D0(r));
	*len = size;
	return func_001C2310(c, 0);
}

/* setsockopt(): the same options, and IP's two group memberships (12, 13: two addresses, 8 bytes) */
int func_001C43D0(int id, int level, int name, void *val, u_int len)
{
	sceInetAddress_t addr;
	sceInetAddress_t grp[2];
	sceInetAddress_t grp2[2];
	int flag;
	CTX *c;
	SOCK *s;
	int r = -1;
	u_int size;

	c = func_001C2160(0);
	if (c == 0)
		return -1;
	s = func_001C23D8(id);
	if (s == 0)
		return func_001C2310(c, 9);
	if (level == 6) {
		if (s->cid == 0)
			return func_001C2310(c, 109);
		if ((s->param.type != 1 && s->param.type != 2) || name != 1)
			return func_001C2310(c, 109);
		size = 4;
	} else if (level == 0) {
		if (s->param.type != 0)
			return func_001C2310(c, 109);
		if (name == 9)
			size = 4;
		else if (name == 10)
			size = 4;
		else if (name == 11)
			size = 4;
		else if (name == 12)
			size = 8;
		else if (name == 13)
			size = 8;
		else
			return func_001C2310(c, 109);
		r = func_001C35A0(c, s);
		if (r != 0)
			return func_001C2310(c, r);
	} else
		return func_001C2310(c, 109);
	if (val == 0 || len < size)
		return func_001C2310(c, 22);
	func_001C2140();
	if (level == 6 && name == 1) {
		flag = *(int *)val != 0;
		r = sceInetControl(c->client, c->abuf, s->cid, 3, &flag, 4);
	} else if (level == 0) {
		if (name == 9) {
			func_001C2410(&addr, val);
			r = sceInetControl(c->client, c->abuf, s->cid, 4, &addr, sizeof(addr));
		} else if (name == 10)
			r = sceInetControl(c->client, c->abuf, s->cid, 6, val, size);
		else if (name == 11)
			r = sceInetControl(c->client, c->abuf, s->cid, 8, val, size);
		else if (name == 12) {
			func_001C2410(&grp[0], val);
			func_001C2410(&grp[1], (u_int *)val + 1);
			r = sceInetControl(c->client, c->abuf, s->cid, 10, grp, sizeof(grp));
		} else if (name == 13) {
			func_001C2410(&grp2[0], val);
			func_001C2410(&grp2[1], (u_int *)val + 1);
			r = sceInetControl(c->client, c->abuf, s->cid, 11, grp2, sizeof(grp2));
		}
	}
	if (func_001C20D8() < 0)
		return func_001C2310(c, -1);
	if (r < 0)
		return func_001C2310(c, func_001C24D0(r));
	return func_001C2310(c, 0);
}

/* send(): the data in pieces of at most the thread's buffer, until one is short; nothing sent at all
 * with a timeout set is EAGAIN */
long func_001C3660(int id, char *buf, u_int len, int flags)
{
	CTX *c;
	SOCK *s;
	int r, n, total = 0;
	int fl;

	c = func_001C2160(0);
	if (c == 0)
		return -1;
	s = func_001C23D8(id);
	if (s == 0)
		return func_001C2310(c, 9);
	if ((s->param.type == 0 || s->param.type == 3) && c->bufsize < len)
		return func_001C2310(c, 22);
	r = func_001C35A0(c, s);
	if (r)
		return func_001C2310(c, r);
	if (flags & 1)
		fl = 2;
	else
		fl = 0;
	while (len) {
		if (c->bufsize < len)
			n = c->bufsize;
		else
			n = len;
		func_001C2140();
		r = sceInetSend(c->client, c->abuf, s->cid, buf, n, &fl, s->opt64);
		if (func_001C20D8() < 0)
			return func_001C2310(c, -1);
		if (r < 0)
			return func_001C2310(c, func_001C24D0(r));
		buf += r;
		len -= r;
		total += r;
		if (r < n)
			break;
	}
	if (total == 0 && s->opt64 >= 0)
		return func_001C2310(c, 11);
	func_001C2310(c, 0);
	return total;
}

/* recv(): as send, the receive timeout the socket's own; EAGAIN unless the peer finished */
long func_001C3810(int id, char *buf, u_int len, int flags)
{
	CTX *c;
	SOCK *s;
	int r, n, total = 0;
	int fl;

	c = func_001C2160(0);
	if (c == 0)
		return -1;
	s = func_001C23D8(id);
	if (s == 0)
		return func_001C2310(c, 9);
	if ((s->param.type == 0 || s->param.type == 3) && c->bufsize < len)
		return func_001C2310(c, 22);
	r = func_001C35A0(c, s);
	if (r)
		return func_001C2310(c, r);
	fl = 0;
	while (len) {
		if (c->bufsize < len)
			n = c->bufsize;
		else
			n = len;
		func_001C2140();
		r = sceInetRecv(c->client, c->abuf, s->cid, buf, n, &fl, s->opt60);
		if (func_001C20D8() < 0)
			return func_001C2310(c, -1);
		if (r < 0)
			return func_001C2310(c, func_001C24D0(r));
		buf += r;
		len -= r;
		total += r;
		if (r < n)
			break;
	}
	if (total == 0 && s->opt60 >= 0 && !(fl & 4))
		return func_001C2310(c, 11);
	func_001C2310(c, 0);
	return total;
}

extern int func_001C24B0(int port);

/* sendto(): a datagram or raw socket's one datagram, at most the thread's buffer */
long func_001C39B0(int id, char *buf, u_int len, int flags, struct sockaddr_in *to, int tolen)
{
	CTX *c;
	SOCK *s;
	int r, port;
	int fl;
	sceInetAddress_t addr;

	c = func_001C2160(0);
	if (c == 0)
		return -1;
	s = func_001C23D8(id);
	if (s == 0)
		return func_001C2310(c, 9);
	if (s->param.type != 0 && s->param.type != 3)
		return func_001C2310(c, 95);
	if (c->bufsize < len)
		return func_001C2310(c, 22);
	r = func_001C35A0(c, s);
	if (r)
		return func_001C2310(c, r);
	fl = 0;
	func_001C2410(&addr, &to->sin_addr);
	port = func_001C2488(to->sin_port);
	func_001C2140();
	r = sceInetSendTo(c->client, c->abuf, s->cid, buf, len, &fl, &addr, port, s->opt64);
	if (func_001C20D8() < 0)
		return func_001C2310(c, -1);
	if (r < 0)
		return func_001C2310(c, func_001C24D0(r));
	func_001C2310(c, 0);
	return r;
}

/* recvfrom(): one datagram, its sender written back */
long func_001C3B20(int id, char *buf, u_int len, int flags, struct sockaddr_in *from, int *fromlen)
{
	CTX *c;
	SOCK *s;
	int r, port;
	int fl;
	sceInetAddress_t addr;

	c = func_001C2160(0);
	if (c == 0)
		return -1;
	if (*fromlen < sizeof(struct sockaddr_in))
		return func_001C2310(c, 22);
	s = func_001C23D8(id);
	if (s == 0)
		return func_001C2310(c, 9);
	if (s->param.type != 0 && s->param.type != 3)
		return func_001C2310(c, 95);
	if (c->bufsize < len)
		return func_001C2310(c, 22);
	r = func_001C35A0(c, s);
	if (r)
		return func_001C2310(c, r);
	fl = 0;
	func_001C2410(&addr, &from->sin_addr);
	port = func_001C2488(from->sin_port);
	func_001C2140();
	r = sceInetRecvFrom(c->client, c->abuf, s->cid, buf, len, &fl, &addr, &port, s->opt60);
	if (func_001C20D8() < 0)
		return func_001C2310(c, -1);
	if (r < 0)
		return func_001C2310(c, func_001C24D0(r));
	from->sin_family = 2;
	func_001C2458(&from->sin_addr, &addr);
	from->sin_port = func_001C24B0(port);
	*fromlen = sizeof(struct sockaddr_in);
	func_001C2310(c, 0);
	return r;
}

/* a listening socket's next slot: a record for the connection to come, on the socket's list, its
 * libinet listener created and opened (-500, nothing to accept yet, is no error) */
int func_001C2F38(CTX *c, SOCK *s)
{
	SOCK *t;
	int id, r;

	id = func_001C25D0();
	t = func_001C1CD0(sizeof(SOCK));
	if (t == 0)
		return 12;
	memset(t, 0, sizeof(SOCK));
	t->id = id;
	memcpy(&t->param, &s->param, sizeof(t->param));
	t->opt60 = -1;
	t->opt64 = -1;
	t->opt68 = -1;
	t->prev = s->ctail;
	if (t->prev)
		t->prev->next = t;
	else
		s->conn = t;
	t->next = 0;
	s->ctail = t;
	func_001C2140();
	r = sceInetCreate(c->client, c->abuf, &t->param);
	if (func_001C20D8() < 0)
		return -1;
	if (r <= 0) {
		{
			SOCK *p = t->prev;

			if (t->next)
				t->next->prev = p;
			else
				s->ctail = p;
		}
		if (t->prev)
			t->prev->next = t->next;
		else
			s->conn = t->next;
		func_001C1D08(t);
		return func_001C24D0(r);
	}
	t->cid = r;
	func_001C2140();
	r = sceInetOpen(c->client, c->abuf, t->cid, 0);
	if (func_001C20D8() < 0)
		return -1;
	if (r < 0 && r != -500)
		return func_001C24D0(r);
	return 0;
}

/* getsockname() and getpeername() (peer): from libinet's connection information; a listening
 * socket answers through its first slot */
int func_001C46D0(int id, struct sockaddr_in *name, int *namelen, int peer)
{
	CTX *c;
	SOCK *s;
	int r;
	sceInetInfo_t info;

	c = func_001C2160(0);
	if (c == 0)
		return -1;
	if (*namelen < sizeof(struct sockaddr_in))
		return func_001C2310(c, 22);
	s = func_001C23D8(id);
	if (s == 0)
		return func_001C2310(c, 9);
	if (s->cid == 0) {
		if (peer)
			return func_001C2310(c, 22);
		if (s->param.type != 2 || s->conn == 0 || s->conn->cid == 0)
			return func_001C2310(c, 9);
		s = s->conn;
	}
	func_001C2140();
	r = sceInetControl(c->client, c->abuf, s->cid, 1, &info, sizeof(info));
	if (func_001C20D8() < 0)
		return func_001C2310(c, -1);
	if (r < 0)
		return func_001C2310(c, func_001C24D0(r));
	name->sin_family = 2;
	if (peer) {
		func_001C2458(&name->sin_addr, &info.remote_adr);
		name->sin_port = func_001C24B0(info.remote_port);
	} else {
		func_001C2458(&name->sin_addr, &info.local_adr);
		name->sin_port = func_001C24B0(info.local_port);
	}
	*namelen = sizeof(struct sockaddr_in);
	return func_001C2310(c, 0);
}

/* accept(): the first slot taken off the listening socket's list and opened (-507 is no error),
 * a new slot made in its place, the peer's address read back; a failure unlinks it again */
int func_001C32A8(int id, struct sockaddr_in *addr, int *addrlen)
{
	CTX *c;
	SOCK *s, *t;
	int r;
	sceInetInfo_t info;

	c = func_001C2160(0);
	if (c == 0)
		return -1;
	if (*addrlen < sizeof(struct sockaddr_in))
		return func_001C2310(c, 22);
	s = func_001C23D8(id);
	if (s == 0)
		return func_001C2310(c, 9);
	if (s->param.type != 2)
		return func_001C2310(c, 95);
	t = s->conn;
	if (t == 0)
		return func_001C2310(c, -1);
	s->conn = t->next;
	if (s->conn)
		t->next->prev = 0;
	else
		s->ctail = 0;
	t->next = s->pend;
	if (t->next)
		t->next->prev = t;
	else
		s->ptail = t;
	t->prev = 0;
	s->pend = t;
	func_001C2140();
	r = sceInetOpen(c->client, c->abuf, t->cid, -1);
	if (func_001C20D8() < 0) {
		r = -1;
		goto unpend;
	}
	if (r < 0 && r != -507) {
		r = func_001C24D0(r);
		goto unpend;
	}
	{
		SOCK *n = t->next;

		if (t->prev)
			t->prev->next = n;
		else
			s->pend = n;
	}
	if (t->next)
		t->next->prev = t->prev;
	else
		s->ptail = t->prev;
	t->next = data_001CE630;
	if (data_001CE630)
		data_001CE630->prev = t;
	else
		data_001CE634 = t;
	t->prev = 0;
	data_001CE630 = t;
	r = func_001C2F38(c, s);
	if (r)
		goto unlist;
	func_001C2140();
	r = sceInetControl(c->client, c->abuf, t->cid, 1, &info, sizeof(info));
	if (func_001C20D8() < 0) {
		r = -1;
		goto unlist;
	}
	if (r < 0) {
		r = func_001C24D0(r);
		goto unlist;
	}
	addr->sin_family = 2;
	func_001C2458(&addr->sin_addr, &info.remote_adr);
	addr->sin_port = func_001C24B0(info.remote_port);
	*addrlen = sizeof(struct sockaddr_in);
	func_001C2310(c, 0);
	return t->id;
unpend:
	{
		SOCK *n = t->next;

		if (t->prev)
			t->prev->next = n;
		else
			s->pend = n;
	}
	if (t->next)
		t->next->prev = t->prev;
	else
		s->ptail = t->prev;
	goto close;
unlist:
	{
		SOCK *n = t->next;

		if (t->prev)
			t->prev->next = n;
		else
			data_001CE630 = n;
	}
	if (t->next)
		t->next->prev = t->prev;
	else
		data_001CE634 = t->prev;
close:
	sceInetClose(c->client, c->abuf, t->cid, 0);
	func_001C1D08(t);
	return func_001C2310(c, r);
}

/* every libinet call pending on a socket aborted: its own connection's, its slots' and its
 * accepted connections' */
int func_001C28F8(int id, int flags)
{
	CTX *c;
	SOCK *s, *t;
	int r;

	c = func_001C2160(0);
	if (c == 0)
		return -1;
	s = func_001C23D8(id);
	if (s == 0)
		return func_001C2310(c, 9);
	if (s->cid) {
		func_001C2140();
		r = sceInetAbort(c->client, c->abuf, s->cid, flags);
		if (func_001C20D8() < 0)
			return func_001C2310(c, -1);
		if (r < 0)
			return func_001C2310(c, func_001C24D0(r));
	}
	for (t = s->conn; t; t = t->next) {
		if (t->cid) {
			func_001C2140();
			sceInetAbort(c->client, c->abuf, t->cid, flags);
			if (func_001C20D8() < 0)
				return func_001C2310(c, -1);
		}
	}
	for (t = s->pend; t; t = t->next) {
		if (t->cid) {
			func_001C2140();
			sceInetAbort(c->client, c->abuf, t->cid, flags);
			if (func_001C20D8() < 0)
				return func_001C2310(c, -1);
		}
	}
	func_001C2310(c, 0);
	return 0;
}

extern char data_001E5F98[];

/* gethostbyaddr(): libinet's reverse lookup into the same static hostent */
struct hostent *func_001C4A20(u_int *addr, int len, int type)
{
	struct hostent *h = &data_001E6098;
	CTX *c;
	int r;
	sceInetAddress_t a;

	c = func_001C2160(0);
	if (c == 0)
		return 0;
	if (type != 2) {
		c->herr = -1;
		func_001C2310(c, 106);
		return 0;
	}
	if (len != 4) {
		c->herr = -1;
		func_001C2310(c, 22);
		return 0;
	}
	c->herr = 0;
	func_001C2140();
	func_001C2410(&a, addr);
	r = sceInetAddress2Name(c->client, c->abuf, 64, data_001E5F98, 256, &a, 0, 0);
	if (func_001C20D8() < 0) {
		c->herr = -1;
		func_001C2310(c, -1);
		return 0;
	}
	if (r < 0) {
		if (r == -500 || r == -515)
			c->herr = 1;
		else if (r == -516)
			c->herr = 2;
		else if (r == -517)
			c->herr = 3;
		else if (r == -518)
			c->herr = 4;
		else {
			c->herr = -1;
			func_001C2310(c, func_001C24D0(r));
			return 0;
		}
		func_001C2310(c, 0);
		return 0;
	}
	h->h_name = data_001E5F98;
	h->h_aliases = data_001CE640;
	h->h_addrtype = type;
	h->h_length = len;
	bcopy(addr, &data_001E5F90, 4);
	c->herr = 0;
	data_001CE648[0] = (char *)&data_001E5F90;
	h->h_addr_list = data_001CE648;
	func_001C2310(c, 0);
	return h;
}
