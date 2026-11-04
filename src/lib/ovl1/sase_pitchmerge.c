/* sc: compiler=ee-gcc2.96; flags=-O2 -G8 */
/* Nellymoser's encoder, the pitch decision's object after libvect.c (OVL1 0x24F010-0x24F518): the
 * frame's decision 0x24F010 (called by the frame analysis 0x248FB0) over its helpers -- the two
 * candidates' merge 0x24F468, 0x24F108, the voicing ratio 0x24F170 and 0x24F250. No assert names
 * the object's file (it links between libvect.c and the encoder API's), so the unit is named by
 * what it does, as sase_vad.c is. Written in the GCC lane's round 26a: 0x24F170, 0x24F468 (the
 * decoder's 0x254458 is the same text, at -G0). The constants are the vendor's truncated
 * conversions, as hex floats (round 12a). Placeholders: the codec's names are not in SOCOM 1.
 * Built as nelly.c is (ee-gcc 2.96, -O2 -G8). */

extern float fabsf(float);

/* the voicing ratio: the smaller estimate over the previous maximum when weakly voiced, the larger
 * over the current one when not (each maximum floored at 0.1); close estimates averaged; clamped
 * to [0, 1], the current maximum kept for the next frame */
float func_0024F170(float *pfPrev, float fA, float fB, float fCur, float fV)
{
	float fR;

	if (*pfPrev < 0x1.999998p-4F)
		*pfPrev = 0x1.999998p-4F;
	if (fCur < 0x1.999998p-4F)
		fCur = 0x1.999998p-4F;
	if (fabsf(fB - fA) < 0x1.333332p-3F)
		fR = (fB + fA) * 0.5F;
	else if (fV < 0x1.666666p-2F) {
		if (fA < fB)
			fR = fA * fV / *pfPrev;
		else
			fR = fB * fV / fCur;
	} else {
		if (fA < fB)
			fR = fB * fV / fCur;
		else
			fR = fA * fV / *pfPrev;
	}
	if (fR < 0.0F)
		fR = 0.0F;
	if (1.0F < fR)
		fR = 1.0F;
	*pfPrev = fCur;
	return fR;
}

/* the two pitch candidates merged: within 25 % of each other averaged, else the better-voiced
 * one, and a low pitch whose voicing is weak gives way to the higher candidate (one exit) */
float func_0024F468(float fP1, float fP2, float fV1, float fV2)
{
	float fP, fV;

	if (fP2 <= fP1 * 1.25F && fP1 * 0x1.999998p-1F <= fP2)
		fP = (fP1 + fP2) * 0.5F;
	else {
		if (fV2 < fV1) {
			fV = fV1;
			fP = fP1;
		} else {
			fV = fV2;
			fP = fP2;
		}
		if (fP < 30.0F && fV * fP * 0.5F < 1.0F)
			fP = (fP1 < fP2) ? fP2 : fP1;
	}
	return fP;
}
