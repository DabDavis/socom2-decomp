/* sc: compiler=ee-gcc2.96; flags=-O2 -G8 */
/* Nellymoser's encoder, libsnd.c (OVL1 0x24C5D0-0x24C8E8): the circular sample buffer -- its
 * create and release, a write and a centred read at an offset from the current position (each in
 * one or two copies through the host's memcpy callback, split where the ring wraps), and the
 * index wrap. Asserting as the codec does (file and line by `#line`, as in sase_encsc.c). The
 * release (0x24C648, round 12a's sase_misc.c) moved here in the GCC lane's round 18a, with the
 * create, the write, the read and the wrap (the create's and the wrap's drafts were byte-exact).
 *
 * The write and read read the block's three fields at their declarations, so the assert's
 * `pvCircMblk != NellyNull` is gone (GCC deletes a null test of a pointer already dereferenced)
 * and only the buffer's is tested. Placeholders: the codec's names are not in SOCOM 1. Built as
 * nelly.c is (ee-gcc 2.96, -O2 -G8). */

#define NellyNull 0
extern void __assert(const char *file, int line, const char *expr);
#define assert(e) ((e) ? (void)0 : __assert(__FILE__, __LINE__, #e))

extern void *(*data_001D55A8)(int n, int size);			/* the host's allocator */
extern void (*data_001D55AC)(void *);				/* the host's free callback */
extern void (*data_001D55B4)(void *dst, const void *src, int n);	/* the host's memcpy */

typedef struct {
	int iSize;
	int iPos;
	float *pBuf;
} CircMblk;

#define FREE(p) if ((p) != NellyNull) { data_001D55AC(p); (p) = NellyNull; }

int func_0024C8C0(int iOffset, int iPos, int iSize);

int func_0024C5D0(int iSize, CircMblk **pvCircMblk)
{
	CircMblk *p;

	p = data_001D55A8(1, sizeof(CircMblk));
	if (p == NellyNull)
		return 1;
	*pvCircMblk = p;
	p->iSize = iSize;
	p->iPos = 0;
	p->pBuf = data_001D55A8(iSize, sizeof(float));
	return 0;
}

void func_0024C648(CircMblk **pvCircMblk)
{
#line 119 "../../SaseEncVad/source/libsnd.c"
	assert(pvCircMblk != NellyNull);
	if (*pvCircMblk != NellyNull) {
		FREE((*pvCircMblk)->pBuf);
		FREE(*pvCircMblk);
	}
}

/* iLength samples in at iOffset past the current position, which then moves on by iLength */
void func_0024C6C0(CircMblk *pvCircMblk, int iOffset, int iLength, const float *pfInBuff)
{
	float *pBuf = pvCircMblk->pBuf;
	int iSize = pvCircMblk->iSize;
	int iPos = pvCircMblk->iPos;
	int iStart, iFirst;

#line 164 "../../SaseEncVad/source/libsnd.c"
	assert((pfInBuff != NellyNull) && (pvCircMblk != NellyNull));
	iStart = func_0024C8C0(iOffset, iPos, iSize);
	iFirst = iSize - iStart;
	if (iFirst >= iLength)
		data_001D55B4(pBuf + iStart, pfInBuff, iLength * sizeof(float));
	else {
		data_001D55B4(pBuf + iStart, pfInBuff, iFirst * sizeof(float));
		data_001D55B4(pBuf, pfInBuff + iFirst, (iLength - iFirst) * sizeof(float));
	}
	pvCircMblk->iPos = func_0024C8C0(iLength, iPos, iSize);
}

/* iLength samples out, centred iOffset behind the current position */
void func_0024C7D0(CircMblk *pvCircMblk, int iOffset, int iLength, float *pfOutBuff)
{
	int iSize = pvCircMblk->iSize;
	float *pBuf = pvCircMblk->pBuf;
	int iPos = pvCircMblk->iPos;
	int iStart, iFirst;

#line 250 "../../SaseEncVad/source/libsnd.c"
	assert(pvCircMblk != NellyNull && pfOutBuff != NellyNull);
	iOffset += (iLength + 1) >> 1;
	iStart = func_0024C8C0(-iOffset, iPos, iSize);
	iFirst = iSize - iStart;
	if (iFirst >= iLength)
		data_001D55B4(pfOutBuff, pBuf + iStart, iLength * sizeof(float));
	else {
		data_001D55B4(pfOutBuff, pBuf + iStart, iFirst * sizeof(float));
		data_001D55B4(pfOutBuff + iFirst, pBuf, (iLength - iFirst) * sizeof(float));
	}
}

/* the ring's index iOffset from iPos (iOffset may be negative) */
int func_0024C8C0(int iOffset, int iPos, int iSize)
{
	return (iSize + (iPos + iOffset % iSize)) % iSize;
}
