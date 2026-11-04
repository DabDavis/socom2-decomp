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
