/* sc: compiler=ee-gcc2.96; flags=-O2 -G8 */
/* Nellymoser's encoder, Coder.c (OVL1 0x2488A0-0x248AD0): the frame coder 0x2488A0 (its asserts
 * Coder.c's) and after it the harmonic comb search 0x248A18 -- no assert names that one's file, so
 * it is kept here by address (the objects link alphabetically: CodePv.c, Coder.c, DecVcFry.c).
 * Written in the GCC lane's round 26a: 0x248A18. Placeholders: the codec's names are not in
 * SOCOM 1. Built as nelly.c is (ee-gcc 2.96, -O2 -G8: the cosine table's pointer is a $gp word). */

extern float *data_001D5598;		/* the cosine table, 256 points a turn (0x3EA7E8) */

/* the comb's best fundamental between fFreq and fEnd: at each step the amplitudes summed over the
 * harmonics' phases (the step in 1/4096 turns, the phase's top byte the table's index). The scale
 * is a local initialised at its declaration -- its lui/mtc1 before the loop's entry test, as the
 * target's (a literal is hoisted into the preheader after it: 4 words); the 0.5 stays a literal. */
void func_00248A18(float *pfMax, int n, const float *pfAmp, const int *piHarm, float *pfMaxFreq,
		   float fStep, float fFreq, float fEnd, float fBias)
{
	int i, iStep;
	float fSum;
	float fScale = 4096.0F;

	for (; fFreq <= fEnd; fFreq += fStep) {
		iStep = (int)(fFreq * fScale + 0.5F);
		fSum = fBias;
		for (i = 0; i < n; i++)
			fSum += pfAmp[i] * data_001D5598[(unsigned int)(iStep * piHarm[i] + 0x800000) >> 24];
		if (*pfMax < fSum) {
			*pfMax = fSum;
			*pfMaxFreq = fFreq;
		}
	}
}
