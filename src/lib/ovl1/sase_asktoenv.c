/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* Nellymoser's decoder, AskToEnvInline.c (OVL1 0x253510-0x2539C0): the decoder's spectral envelope
 * from the received reflection coefficients -- the envelope itself (0x253510, its asserts this
 * file's), its helpers 0x253760, the step-up recursion (0x253888: libtran.c's 0x24EA68 at the
 * decoder's fixed order 12, the odd-order swap folded away, no assert) and the envelope's
 * resampling by a pitch ratio (0x253920, 65 points). Written in the GCC lane's round 26a: 0x253888,
 * 0x253920. Placeholders: the codec's names are not in SOCOM 1. Built as sase_decapi.c is (ee-gcc
 * 2.96, -O2 -G0: the decoder reaches nothing through $gp). */

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
