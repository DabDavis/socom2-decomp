/* sc: compiler=ee-gcc2.96; flags=-O2 -G8 */
/* Nellymoser's encoder, Voicing.c (OVL1 0x250F60-0x2513F0): the voicing decision -- the frame's
 * voicing estimate (0x250F60, its asserts Voicing.c's), its smoothing against the previous frame's
 * and the pitch's (0x251270, called from the frame coder 0x2492A0) and the clamped dB position of a
 * correlation between two bounds (0x251348, the estimate's). The encoder's objects link in the
 * alphabetical order of their files (PostFilt.c, PtchCand.c, RefineC0.c, Voicing.c, VoicLD.c), so
 * the two after the estimate are this object's. Written in the GCC lane's round 26a: 0x251270 and
 * 0x251348; the estimate 0x250F60 is not written. The constants are the vendor's truncated
 * conversions (0.8 as 0x3F4CCCCC: round 12a's decimal conversion), so they are hex floats.
 * Placeholders: the codec's names are not in SOCOM 1. Built as nelly.c is (ee-gcc 2.96, -O2 -G8). */

extern float log10f(float);

/* the voicing smoothed: a weak frame's estimate dropped, a strong one's raised (by 1/14 under a
 * short pitch) to at least its own, then pulled toward the previous frame's when the pitch held */
float func_00251270(float fCorr, float fVoic, float fPrev, float fPitch, float fPrevPitch)
{
	if (fCorr <= 0.01F)
		fVoic = 0.0F;
	if (fCorr >= 0.45F) {
		fCorr -= 0.45F;
		if (fCorr * (fPitch * 0.5F) < 1.0F)
			fCorr += 0x1.249248p-4F;
		if (fVoic < fCorr)
			fVoic = fCorr;
	}
	if (fVoic < fPrev && fPrev >= 0x1.249248p-4F) {
		if (fPitch < fPrevPitch * 1.25F && fPrevPitch * 0x1.999998p-1F < fPitch)
			fVoic = fVoic * 0x1.666666p-1F + fPrev * 0x1.333332p-2F;
		if (fVoic < 0x1.249248p-4F)
			fVoic += 0x1.249248p-4F;
	}
	return fVoic;
}

/* where -10 log10(1 - x) lies between fLo and fHi, as a fraction clamped to [0, 1] (x first kept
 * inside (0, 1)) */
float func_00251348(float fLo, float fHi, float fX)
{
	float fDb, fR;

	if (fX <= 0.0F)
		fX = 0x1.0c6f7ap-20F;
	if (fX >= 1.0F)
		fX = 0x1.ffffdep-1F;
	fDb = -10.0F * log10f(1.0F - fX);
	if (fDb < fLo)
		fR = 0.0F;
	else if (fHi < fDb)
		fR = 1.0F;
	else
		fR = (fDb - fLo) / (fHi - fLo);
	return fR;
}

#define NellyNull 0
extern void __assert(const char *file, int line, const char *expr);
#define assert(e) ((e) ? (void)0 : __assert(__FILE__, __LINE__, #e))

extern const float data_003EADF0[];	/* the harmonic's analysis window, a bin 32 steps */

/* the frame's voicing from its spectrum: per harmonic of the pitch, the energy left when the window's shape at the harmonic
 * is subtracted over its bins, against the band's energy; one less the mean ratio. The object after this one (VoicLD.c), kept
 * here by address. The band's end is the accumulator's expression (`(iAcc + iBnd + 0x4000) >> 15`: loop.c's giv, its start after
 * the entry test, the target's `move t2,v0`), the bins' pair each loop's own locals (their registers the target's). OPEN (GCC
 * round 44a: 46 words at the right size, every one a register's name): global-alloc's nH prefers a1 (.greg's `97 preferences: 5`,
 * from its fix: nH = iEnd has none), so k, which ranks above it, avoids a1 and takes a3 and the rest shift; the target's k has a1. */
float func_002513F0(const float *pfSW, int iEnd, float fPitch)
{
	const float *pfWin = data_003EADF0;
	float fSum = 0.0F;
	float fStep, fHRe, fHIm, fE, fD, fX;
	int iStep, nH, iBnd, iAcc, j, h, i0, k;
	const float *pf;

#line 110 "../../SaseEncVad/source/VoicLD.c"
	assert(pfSW != NellyNull);
	iAcc = 0;
	fStep = 512.0F / fPitch;
	iStep = (int)(fStep * 32768.0F + 0.5F);
	nH = (int)((float)iEnd / fStep);
	iBnd = iStep >> 1;
	j = (iBnd + 0x4000) >> 15;
	for (h = 0; h < nH; h++) {
		iAcc += iStep;
		k = j;
		i0 = k * 32 - ((iAcc + 512) >> 10);
		pf = pfSW + 2 * ((iAcc + 0x4000) >> 15);
		fHRe = pf[0];
		fHIm = pf[1];
		pf = pfSW + 2 * k;
		j = (iAcc + iBnd + 0x4000) >> 15;
		fE = 0x1.cd2b28p-54F;
		for (; i0 < -111; k++) {
			float fA = pf[0];
			float fB = pf[1];

			i0 += 32;
			pf += 2;
			fE += fA * fA + fB * fB;
		}
		fD = fE;
		for (; i0 < 112 && k < j; k++) {
			float fA = pf[0];
			float fB = pf[1];

			fX = pfWin[i0];
			i0 += 32;
			pf += 2;
			fE += fA * fA + fB * fB;
			fA -= fHRe * fX;
			fB -= fHIm * fX;
			fD += fA * fA + fB * fB;
		}
		for (; k < j; k++) {
			float fA = pf[0];
			float fB = pf[1];

			pf += 2;
			fX = fA * fA + fB * fB;
			fD += fX;
			fE += fX;
		}
		fSum += fD / fE;
	}
	return 1.0F - fSum / (float)nH;
}
