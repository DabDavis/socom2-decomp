/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 -fno-strict-aliasing */
/* The socket layer's allocator (MAIN 0x1C1CD0-0x1C20D8, before inet_sock.c's lock): two hooks
 * (0x1E5F70, 0x1E5F74) that default to newlib's malloc and free, and a first-fit heap the caller can
 * hand it (func_001C2028 installs it as the hooks): every block a 12-byte header {next, prev, size}
 * on one of two lists kept in address order -- the used list, each allocation linked in before the
 * used block that follows its free block, and the free list, each release linked in before the free
 * block after its run of used neighbours and merged with both free neighbours.
 *
 * ITS OWN OBJECT, BUILT WITHOUT STRICT ALIASING (GCC round 79; in inet_sock.c since round 20a): the
 * set-up 0x1C2028 keeps its nine stores in source order -- the globals' and the header's interleaved,
 * no store passing another -- which this compiler does only when the header's fields and the
 * globals may alias; the allocation reloads a block's size after the links are stored, the release
 * likewise. Under the socket layer's strict aliasing (ten of its functions lose without it) the
 * stores sort by register weight and the reloads are CSE'd away: every one of these three rows was
 * a register or a word off for 58 rounds.
 *
 * The allocation keeps the block's size from the search in a local (sz), used for the remainder
 * and, after an exact fit, for the next block; after a split the local is reloaded from the new free
 * block (the target's `lw v1,8(a1)` in both arms' slots). The next block's address reuses p, its
 * offset `sz + sizeof(BLK)` kept intact (fold otherwise reassociates it to (p + sz) + 12). The release
 * reads each unlink's other link into a local first (the slot loads), walks the used run on the
 * fields, and sums `+= sizeof(BLK) + x`, which keeps the adjacency test's (size + 12) for the add.
 *
 * OPEN (round 79): func_001C1D40 (right size, 28: sz and the remainder trade v1/a3 -- global-alloc's
 * order -- and the split's two link arms share one reload of p->size where the target's then-arm
 * keeps its own; the exact fit's arms load b->next into v0, b->prev into a0 on both paths -- the
 * target's beqz, this compile's beqzl), func_001C1E90 (right size, 6: in the first merge the target
 * stores the size through n and reads the next link through b after `b = n`, CSE here makes b the
 * class's canonical register for both; in the second the target addresses the neighbour through the
 * computed sum, the jump's equivalence -- `(t = ...) == n` gives it but then the next link loads
 * after the size store), func_001C2028 (right size, 23: the stores in the target's order, its head
 * `fail:` inside the first test as the target lays it; heap+size's addu and the %hi pseudos
 * allocated otherwise -- 24 orders of the free globals' stores, an `end` local: 22-27). */

typedef unsigned int u_int;
typedef unsigned int size_t;

extern void *malloc(size_t n);
extern void free(void *p);

/* the first-fit heap's block header: the payload follows it */
typedef struct _blk {
	struct _blk *next;	/* 0x0 */
	struct _blk *prev;	/* 0x4 */
	u_int size;		/* 0x8: the payload's */
} BLK;

extern void *(*data_001E5F70)(size_t n);	/* the allocator's hooks */
extern void (*data_001E5F74)(void *p);
extern BLK *data_001E5F78;			/* the used list */
extern BLK *data_001E5F7C;
extern BLK *data_001E5F80;			/* the free list */
extern BLK *data_001E5F84;
extern void *data_001E5F88;			/* the heap */
extern void *data_001E5F8C;

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
	u_int rest, sz;

	size = (size + 3) & ~3U;
	for (p = data_001E5F80; p; p = p->next) {
		sz = p->size;
		if (size < sz)
			break;
	}
	if (p == 0)
		return 0;

	rest = sz - sizeof(BLK);
	rest -= size;
	b = p;
	if (rest) {
		/* split: the rest stays free, in b's place */
		p = (BLK *)((char *)b + sizeof(BLK) + size);
		n = b->next;
		b->size = size;
		p->size = rest;
		p->next = n;
		if (n)
			n->prev = p;
		else
			data_001E5F84 = p;
		p->prev = b->prev;
		if (p->prev)
			p->prev->next = p;
		else
			data_001E5F80 = p;
		sz = p->size;
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
	p = (BLK *)((char *)p + (sz + sizeof(BLK)));
	if (data_001E5F78 == 0) {
		b->prev = 0;
		b->next = 0;
		data_001E5F7C = b;
		data_001E5F78 = b;
	} else if ((void *)p >= data_001E5F8C) {
		b->next = 0;
		b->prev = data_001E5F7C;
		data_001E5F7C->next = b;
		data_001E5F7C = b;
	} else {
		b->prev = p->prev;
		if (b->prev)
			b->prev->next = b;
		else
			data_001E5F78 = b;
		b->next = p;
		p->prev = b;
	}
	return (char *)b + sizeof(BLK);
}

void func_001C1E90(void *ptr)
{
	BLK *b, *p, *n, *t;
	u_int size;

	if (ptr == 0)
		return;
	b = (BLK *)((char *)ptr - sizeof(BLK));

	/* off the used list */
	t = b->prev;
	if (b->next)
		b->next->prev = t;
	else
		data_001E5F7C = t;
	t = b->next;
	if (b->prev)
		b->prev->next = t;
	else
		data_001E5F78 = t;

	/* the free block after b's run of used neighbours */
	size = b->size;
	p = b;
	while (p->next == (BLK *)((char *)p + (p->size + sizeof(BLK))))
		p = p->next;
	p = (BLK *)((char *)p + (p->size + sizeof(BLK)));
	if ((void *)p < data_001E5F8C) {
		b->prev = p->prev;
		if (b->prev)
			b->prev->next = b;
		else
			data_001E5F80 = b;
		b->next = p;
		p->prev = b;
	} else {
		b->next = 0;
		b->prev = data_001E5F84;
		if (b->prev)
			b->prev->next = b;
		else
			data_001E5F80 = b;
		data_001E5F84 = b;
	}

	/* merged with the free neighbours */
	n = b->prev;
	if (n && (BLK *)((char *)n + (n->size + sizeof(BLK))) == b) {
		t = n->next;
		b = n;
		n->size += sizeof(BLK) + size;
		b->next = t->next;
		if (b->next)
			b->next->prev = b;
		else
			data_001E5F84 = b;
	}
	n = b->next;
	if (n && (BLK *)((char *)b + (b->size + sizeof(BLK))) == n) {
		t = n->next;
		b->size += sizeof(BLK) + n->size;
		b->next = t;
		if (t)
			t->prev = b;
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
		if (size < mis) {
		fail:
			return -1;
		}
		heap = (char *)heap + mis;
		size -= mis;
	}
	if (size < sizeof(BLK))
		goto fail;
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
