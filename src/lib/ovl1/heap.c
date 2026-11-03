/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* The speech recogniser's heap (heap_*), OVL1 0x2430F0-0x243840: ScanSoft's, the demo's object
 * 0x378348 (C, nine functions, there inside the recogniser), prebuilt with ee-gcc 2.96 at -O2 -G0
 * and, as the recogniser itself, -funroll-loops (README "The GCC front": SearchFor's loop is unrolled
 * by four, which no other flag set gives). A heap is malloc's with a table of the blocks it gave out
 * when opened with a capacity (their sizes, the live and the high-water counts and bytes); without
 * one -- or with no heap -- every call is the C library's. Results are the library's codes
 * (0x800B0183: a null argument, 0x800B0186: out of memory). Names are SOCOM 1's (s1:exact/callee/
 * order); the realloc/alloc/calloc rows by their callees and the demo's order; the table's fields are
 * named by their use here. The functions in the image's order. heap_Realloc's null-block path is
 * heap_Alloc's body inlined (its result reaches v0 through the inlined call's own register); the
 * helper is ours, heap_Alloc itself written out (through the helper: 12 words and the size). */

typedef unsigned int u_int;

typedef struct HEAP {
	u_int count;                    /* 0x00: the blocks the table holds */
	void **items;                   /* 0x04 */
	u_int *sizes;                   /* 0x08 */
	u_int maxcount;                 /* 0x0C: the most blocks at once */
	u_int cursize;                  /* 0x10: the bytes the blocks hold */
	u_int maxsize;                  /* 0x14: the most bytes at once */
	u_int capacity;                 /* 0x18: the table's slots; 0: no table */
} HEAP;

extern void *malloc(u_int size);
extern void *calloc(u_int n, u_int size);
extern void *realloc(void *p, u_int size);
extern void free(void *p);
extern void *memset(void *d, int c, u_int n);

int SearchFor(HEAP *heap, void *item);

static __inline__ void *HeapAlloc(HEAP *heap, u_int size)
{
	void *p;

	p = malloc(size);
	if (heap->capacity != 0 && SearchFor(heap, p) == -1) {
		if (heap->count >= heap->capacity)
			return 0;
		heap->items[heap->count] = p;
		heap->sizes[heap->count] = size;
		heap->count++;
		heap->cursize += size;
		if (heap->maxcount < heap->count)
			heap->maxcount = heap->count;
		if (heap->cursize > heap->maxsize)
			heap->maxsize = heap->cursize;
	}
	return p;
}

void *heap_Realloc(HEAP *heap, void *p, u_int size)
{
	void *q;
	int i;
	u_int old;

	if (heap == 0)
		return realloc(p, size);
	if (p == 0)
		return HeapAlloc(heap, size);
	q = realloc(p, size);
	if (heap->capacity != 0 && q != 0) {
		i = SearchFor(heap, p);
		if (i != -1) {
			old = heap->sizes[i];
			heap->items[i] = q;
			heap->sizes[i] = size;
			heap->cursize = heap->cursize - old + size;
			if (heap->cursize > heap->maxsize)
				heap->maxsize = heap->cursize;
		}
	}
	return q;
}

void *heap_Alloc(HEAP *heap, u_int size)
{
	void *p;

	if (heap == 0)
		return malloc(size);
	p = malloc(size);
	if (heap->capacity != 0 && SearchFor(heap, p) == -1) {
		if (heap->count >= heap->capacity)
			return 0;
		heap->items[heap->count] = p;
		heap->sizes[heap->count] = size;
		heap->count++;
		heap->cursize += size;
		if (heap->maxcount < heap->count)
			heap->maxcount = heap->count;
		if (heap->cursize > heap->maxsize)
			heap->maxsize = heap->cursize;
	}
	return p;
}

void *heap_Calloc(HEAP *heap, u_int n, u_int size)
{
	void *p;

	if (heap == 0)
		return calloc(n, size);
	p = calloc(n, size);
	if (heap->capacity != 0 && SearchFor(heap, p) == -1) {
		if (heap->count >= heap->capacity)
			return 0;
		heap->items[heap->count] = p;
		heap->sizes[heap->count] = size * n;
		heap->count++;
		heap->cursize += size * n;
		if (heap->maxcount < heap->count)
			heap->maxcount = heap->count;
		if (heap->cursize > heap->maxsize)
			heap->maxsize = heap->cursize;
	}
	return p;
}

void heap_Free(HEAP *heap, void *p)
{
	int i;

	if (heap == 0) {
		free(p);
		return;
	}
	if (heap->capacity != 0) {
		i = SearchFor(heap, p);
		if (i == -1)
			return;
		heap->cursize -= heap->sizes[i];
		heap->count--;
		for (; i < heap->count; i++) {
			heap->items[i] = heap->items[i + 1];
			heap->sizes[i] = heap->sizes[i + 1];
		}
	}
	free(p);
}

int heap_GetMemInfo(HEAP *heap, u_int *count, u_int *cursize, u_int *maxcount, u_int *maxsize)
{
	if (heap != 0) {
		if (count != 0)
			*count = heap->count;
		if (cursize != 0)
			*cursize = heap->cursize;
		if (maxcount != 0)
			*maxcount = heap->maxcount;
		if (maxsize != 0)
			*maxsize = heap->maxsize;
	} else {
		if (count != 0)
			*count = 0;
		if (cursize != 0)
			*cursize = 0;
		if (maxcount != 0)
			*maxcount = 0;
		if (maxsize != 0)
			*maxsize = 0;
	}
	return 0;
}

int heap_GetVersion(u_int *version)
{
	if (version == 0)
		return 0x800B0183;
	*version = 0x01000001;
	return 0;
}

int heap_Open(HEAP **pheap, u_int capacity)
{
	HEAP *heap;

	heap = malloc(sizeof(HEAP));
	if (heap == 0) {
		*pheap = 0;
		return 0x800B0186;
	}
	memset(heap, 0, sizeof(HEAP));
	heap->capacity = capacity;
	if (capacity != 0) {
		heap->items = malloc(capacity * 4);
		if (heap->items == 0) {
			heap->items = 0;
			return 0x800B0186;
		}
		heap->sizes = malloc(heap->capacity * 4);
		if (heap->sizes == 0) {
			heap->sizes = 0;
			return 0x800B0186;
		}
	}
	*pheap = heap;
	return 0;
}

int heap_Close(HEAP *heap)
{
	if (heap != 0) {
		if (heap->sizes != 0)
			free(heap->sizes);
		if (heap->items != 0)
			free(heap->items);
		free(heap);
	}
	return 0;
}

int SearchFor(HEAP *heap, void *item)
{
	u_int i;

	if (heap != 0 && item != 0) {
		for (i = 0; i < heap->count; i++)
			if (item == heap->items[i])
				return i;
	}
	return -1;
}
