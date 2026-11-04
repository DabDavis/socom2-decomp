/* sc: compiler=ee-gcc2.96; flags=-O2 -G8 */
/* Nellymoser's encoder, LDPDA.c (OVL1 0x249FA8-0x24AB48): the pitch detector's small entry points --
 * its pitch table (89 lags in sixteenths, a .rodata table compared whole), its 8-byte block made,
 * freed and set. The asserts carry the codec's own expressions, file and line by `#line` (as in
 * sase_encsc.c). GCC round 18a: the band edge 0x24A908 (its count decremented in place, the
 * total scaled in its own variable) and the voicing candidate 0x24A9F0 (its thresholds a static
 * const table, compared whole; `if (c) r = last; else r = index;` is if-converted after the
 * compare -- the target's `move` in the branch's slot -- where `r = index; if (c) r = last;`
 * assigns first and re-forms the address from it, 2 words long). The analysis functions
 * (0x249FA8-0x24A5A8) are not written.
 * Placeholders: the codec's names are not in SOCOM 1. Built as nelly.c is (ee-gcc 2.96, -O2 -G8). */

#define NellyNull 0
extern void __assert(const char *file, int line, const char *expr);
#define assert(e) ((e) ? (void)0 : __assert(__FILE__, __LINE__, #e))

typedef struct {
	float f0;
	float f4;
} LdPdaMblk;

extern void *(*data_001D55A8)(int n, int size);  /* the host's allocator (calloc's arguments) */
extern void (*data_001D55AC)(void *);           /* the host's free callback */

static const float fPitchTable[89] = {
	8.0f, 8.3125f, 8.625f, 9.0f, 9.375f, 9.8125f, 10.25f, 10.75f, 11.25f, 11.875f,
	12.5625f, 13.3125f, 13.6875f, 14.125f, 14.625f, 15.125f, 15.625f, 16.1875f, 16.8125f, 17.5f,
	18.1875f, 19.0f, 19.8125f, 20.4375f, 21.0625f, 21.75f, 22.5f, 23.25f, 24.125f, 25.0f,
	26.0f, 26.75f, 27.625f, 28.5f, 29.4375f, 30.4375f, 31.5f, 32.625f, 33.625f, 34.6875f,
	35.8125f, 37.0f, 38.25f, 39.625f, 40.8125f, 42.125f, 43.5f, 44.9375f, 46.5625f, 47.9375f,
	49.5f, 51.125f, 52.875f, 54.4375f, 56.1875f, 58.0f, 59.9375f, 61.8125f, 63.75f, 65.875f,
	67.875f, 70.0f, 72.3125f, 74.5f, 76.8125f, 79.3125f, 81.75f, 84.3125f, 86.875f, 89.5625f,
	92.375f, 95.25f, 98.25f, 101.1875f, 104.375f, 107.5f, 110.875f, 114.1875f, 117.75f, 121.3125f,
	125.125f, 128.9375f, 133.0f, 137.0625f, 141.25f, 145.625f, 150.0625f, 154.625f, 159.4375f,
};

void func_0024A7C0(const float **pfPitchTable, int *piSize)
{
#line 244 "../../SaseEncVad/source/LDPDA.c"
	assert(pfPitchTable != NellyNull);
	assert(piSize != NellyNull);
	*piSize = 89;
	*pfPitchTable = fPitchTable;
}

int func_0024A840(LdPdaMblk **hLD_PDA_mblk)
{
	LdPdaMblk *p;

#line 276 "../../SaseEncVad/source/LDPDA.c"
	assert(hLD_PDA_mblk != NellyNull);
	p = data_001D55A8(1, sizeof(LdPdaMblk));
	*hLD_PDA_mblk = p;
	if (p == NellyNull)
		return 1;
	p->f0 = 100.0f;
	p->f4 = 0;
	return 0;
}

int func_0024A8B0(LdPdaMblk **hLD_PDA_mblk)
{
#line 313 "../../SaseEncVad/source/LDPDA.c"
	assert(hLD_PDA_mblk != NellyNull);
	if (*hLD_PDA_mblk != NellyNull) {
		data_001D55AC(*hLD_PDA_mblk);
		*hLD_PDA_mblk = NellyNull;
	}
	return 0;
}

void func_0024AAE8(LdPdaMblk *hLD_PDA_mblk, float x, float y)
{
#line 480 "../../SaseEncVad/source/LDPDA.c"
	assert(hLD_PDA_mblk != NellyNull);
	hLD_PDA_mblk->f0 = x;
	hLD_PDA_mblk->f4 = y;
}

/* the band edge: from the top, the bin where the power summed downward first reaches 5 % of
 * the whole, two bins up, at least 76 */
int func_0024A908(const float *pfPower, int iLength)
{
	float fSum, fAcc;
	int i;

#line 357 "../../SaseEncVad/source/LDPDA.c"
	assert(pfPower != NellyNull);
	iLength--;
	fSum = 0.0F;
	for (i = iLength; i >= 0; i--)
		fSum += pfPower[i];
	fSum *= 0x1.999998p-5F;				/* 0.05, one ulp short */
	fAcc = 0.0F;
	for (i = iLength; i >= 0; i--) {
		fAcc += pfPower[i];
		if (fAcc >= fSum)
			break;
	}
	i += 2;
	if (i < 76)
		i = 76;
	return i;
}

/* the voicing candidate: iIndex kept when its value holds 0.75 of the first, the last bin taken
 * over it when that holds its own share (1.1 of the candidate's, 0.88 of the first's) */
static const float afPvThres[3] = { 0x1.8p-1F, 0x1.199998p+0F, 0x1.c28f5cp-1F };

int func_0024A9F0(const float *pfPv, int iLength, int iIndex)
{
	int iLast = iLength - 1;
	int iRet;

#line 425 "../../SaseEncVad/source/LDPDA.c"
	assert(pfPv != NellyNull);
	iRet = 0;
	if (iIndex < iLast) {
		if (iIndex == 0 || pfPv[iIndex] >= afPvThres[0] * pfPv[0]) {
			if (pfPv[iLast] >= afPvThres[1] * pfPv[iIndex])
				iRet = iLast;
			else
				iRet = iIndex;
		}
	} else if (pfPv[iLast] >= afPvThres[2] * pfPv[0])
		iRet = iLast;
	return iRet;
}
