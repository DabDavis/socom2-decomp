/* sc: compiler=ee-gcc2.96; flags=-O2 -G8 */
/* Nellymoser's encoder, PackSC.c (OVL1 0x24FAE8-0x24FBE8): a frame's 17 quantiser indices packed
 * into the packet, each at its own bit width (a static const table of shorts, compared whole),
 * through BitPackC.c's packer (sase_bitpackc.c) over a packer block on the stack. Asserting as
 * the codec does (file and line by `#line`, as in sase_encsc.c). Written in the GCC lane's round
 * 18a; the block's stores in one of the three orders of 720 that give the target's schedule and
 * registers (tools/ccperm.py), the plainest. The packer's prototype here takes an unsigned char
 * (the caller's lbu of each int index) where its definition takes an int. Placeholders: the codec's names are not in SOCOM 1. Built as nelly.c is (ee-gcc 2.96,
 * -O2 -G8). */

#define NellyNull 0
extern void __assert(const char *file, int line, const char *expr);
#define assert(e) ((e) ? (void)0 : __assert(__FILE__, __LINE__, #e))

typedef struct {
	unsigned char *pcBuffer;	/* 0x00 */
	unsigned short usBufSize;	/* 0x04 */
	short sPad;
	int i8, iC;
	unsigned short *pusBytePos;	/* 0x10 */
	unsigned char *pcBitPos;	/* 0x14 */
} BitPackMblk;

extern void func_002484A0(BitPackMblk *hBitPack, unsigned char cData, short cDataWidth);

static const short asBitWidth[17] = { 6, 4, 8, 5, 5, 5, 4, 5, 3, 5, 2, 5, 0, 5, 0, 1, 1 };

void func_0024FAE8(int *piQuanIndex, unsigned char *pchPacketBuffer, unsigned short *psByteOffset,
		   unsigned char *pchBitOffset, int iBufSize, int iFrame)
{
	BitPackMblk bp;
	int i;

#line 99 "../../SaseEncVad/source/PackSC.c"
	assert(piQuanIndex != NellyNull && pchPacketBuffer != NellyNull);
	assert(psByteOffset != NellyNull && pchBitOffset != NellyNull);
	bp.usBufSize = iBufSize;
	bp.pusBytePos = psByteOffset;
	bp.pcBitPos = pchBitOffset;
	bp.pcBuffer = pchPacketBuffer;
	piQuanIndex[16] = (unsigned short)iFrame;
	piQuanIndex[15] = 0;
	for (i = 0; i < 17; i++)
		func_002484A0(&bp, piQuanIndex[i], asBitWidth[i]);
}

extern void func_0024ECF8(const float *pfX, int iLength, int *piMaxIndex, float *pfXmax);
extern float func_002517D8(float fX);	/* the encoder's fast log2 (shared) */
extern float *data_001D5598;		/* the cosine table, 256 points a turn */

/* PostFilt.c, kept here by address: the spectrum's envelope pulled toward the first reflection coefficient's own: five
 * levels of its one-pole response, the gap to them at each bin halved, clamped and faded in over the first band, the maximum
 * then restored */
void func_0024FBE8(float *pfLogEnv, const float *pfKS, float fGain)
{
	float afLevel[8];
	int iIdx;
	float fMax;
	int iIdx2;
	float fMax2;
	float fP, fG, fA, fB, fD, fL, fX;
	int i, j, k, n;
	const float *pf;
	float *pfL, *pfW;

#line 131 "../../SaseEncVad/source/PostFilt.c"
	assert((pfLogEnv != NellyNull) && (pfKS != NellyNull));
	func_0024ECF8(pfLogEnv, 64, &iIdx, &fMax);
	fP = 1.0F;
	pf = pfKS + 1;
	for (i = 10; i >= 0; i--) {
		fX = *pf++;
		fP *= 1.0F - fX * fX;
	}
	fG = fGain - func_002517D8(fP) * 0.5F;
	fA = pfKS[0] * pfKS[0] + 1.0F;
	fB = pfKS[0] * -2.0F;
	n = 0;
	pfL = afLevel;
	for (i = 4; i >= 0; i--) {
		*pfL++ = fG - func_002517D8(fA + fB * data_001D5598[n]) * 0.5F;
		n += 32;
	}
	for (j = 1; j < 64; j++) {
		k = j >> 4;
		fL = afLevel[k] + (float)(j & 15) * 0.0625F * (afLevel[k + 1] - afLevel[k]);
		fD = (pfLogEnv[j] - fL) * 0.5F;
		fD = (0x1.0d58e4p-2F < fD) ? 0x1.0d58e4p-2F : fD;
		fD = (-1.0F < fD) ? fD : -1.0F;
		if (j < 16)
			fD *= (float)j * 0.0625F;
		pfLogEnv[j] += fD;
	}
	func_0024ECF8(pfLogEnv, 64, &iIdx2, &fMax2);
	fD = fMax - fMax2;
	pfW = pfLogEnv;
	for (j = 63; j >= 0; j--)
		*pfW++ += fD;
}
