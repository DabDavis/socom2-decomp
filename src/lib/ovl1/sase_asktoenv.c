/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* Nellymoser's decoder, AskToEnvInline.c (OVL1 0x253510-0x2539C0): the decoder's spectral envelope
 * from the received reflection coefficients -- the envelope itself (0x253510, its asserts this
 * file's), its helpers 0x253760, the step-up recursion (0x253888: libtran.c's 0x24EA68 at the
 * decoder's fixed order 12, the odd-order swap folded away, no assert) and the envelope's
 * resampling by a pitch ratio (0x253920, 65 points); and after them the decoder's bit reader
 * (0x2539C0, called by the frame unpackers 0x253DF8 and 0x254020) -- no assert names its file, so
 * it may be the next object's: it is kept here until one does. Written in the GCC lane's round 26a:
 * 0x253888, 0x253920, 0x2539C0. Placeholders: the codec's names are not in SOCOM 1. Built as
 * sase_decapi.c is (ee-gcc 2.96, -O2 -G0: the decoder reaches nothing through $gp). */

#define DEC_LPC_ORDER 12
#define ENV_POINTS 65

/* the predictor polynomial from the reflection coefficients, by the step-up recursion */
void func_00253888(const float *pfk, float *pfA)
{
	float afTmp[DEC_LPC_ORDER + 1];
	float *pfPrev, *pfT;
	float fK;
	int i, j;

	pfA[0] = 1.0F;
	pfPrev = afTmp;
	for (i = 0; i < DEC_LPC_ORDER; i++) {
		pfT = pfA;
		pfA = pfPrev;
		pfPrev = pfT;
		fK = pfk[i];
		pfA[i + 1] = -fK;
		for (j = 1; j <= i; j++)
			pfA[j] = pfPrev[j] - fK * pfPrev[i + 1 - j];
	}
}

/* the envelope's 65 points resampled by fRatio through the scratch array, the last point held.
 * The point is stored once before the range test and again in each arm -- the vendor's redundant
 * store is the target's `swc1` in the test's delay slot and the in-range arm's reload of pfEnv[k]
 * (the store may alias it); a plain if/else, a conditional expression, a clamped index or a
 * temporary are 23-38 words off (build/scratch/gcc/v_resamp.py, GCC round 26a). */
void func_00253920(float *pfEnv, float *pfTmp, float fRatio)
{
	int i, k;

	if (fRatio == 1.0F)
		return;
	for (i = 0; i < ENV_POINTS; i++) {
		k = (int)((float)i * fRatio);
		pfTmp[i] = pfEnv[k];
		if (k < ENV_POINTS)
			pfTmp[i] = pfEnv[k];
		else
			pfTmp[i] = pfEnv[ENV_POINTS - 1];
	}
	for (i = 0; i < ENV_POINTS; i++)
		pfEnv[i] = pfTmp[i];
}

/* the decoder's bit reader: the packet ring's base and length, its byte position and the bit
 * offset in that byte (both kept by the caller) */
typedef struct {
	unsigned char *pucBuf;		/* 0x00 */
	unsigned short usLen;		/* 0x04: bytes in the ring */
	unsigned short *pusPos;		/* 0x08 */
	unsigned char *pucBit;		/* 0x0C */
} BIT_READER;

/* iBits (up to 8) bits into *pucOut, low bits first, across a byte boundary and the ring's end.
 * The reader's pointers and length are locals (every store through pucOut, a char pointer, would
 * reload them; the target re-reads only the base), and the second byte's shift is a variable of
 * its own: through iBit, iBit outranks pusPos in global-alloc and the two trade a3/t0 (15 words,
 * every declaration order alike). */
void func_002539C0(BIT_READER *p, unsigned char *pucOut, int iBits)
{
	unsigned char *pucBit = p->pucBit;
	unsigned short *pusPos = p->pusPos;
	int iMask = (1 << iBits) - 1;
	int iBit = *pucBit;
	unsigned char *puc = p->pucBuf + *pusPos;
	int iEnd = iBit + iBits;
	unsigned short usLen = p->usLen;
	int iShift;

	if (iEnd >> 3) {
		*pucOut = *puc++ >> iBit;
		iShift = 8 - iBit;
		iMask >>= iShift;
		if (++*pusPos >= usLen) {
			*pusPos = 0;
			puc = p->pucBuf;
		}
		*pucOut |= (*puc & iMask) << iShift;
		iEnd -= 8;
	} else {
		*pucOut = (*puc >> iBit) & iMask;
		if (iEnd == 8) {
			iEnd = 0;
			if (++*pusPos >= usLen)
				*pusPos = 0;
		}
	}
	*pucBit = iEnd;
}
