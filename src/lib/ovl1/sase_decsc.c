/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* Nellymoser's decoder, DecSC.c's parameter setters (OVL1 0x254318-0x254458): each asserts its
 * block (`assert(pMemBlkD != NellyNull)`, file and line by `#line` as in sase_encsc.c) and stores
 * one float. Called by the decoder API's setters (sase_decapi.c). The decoder's objects were built
 * at -G0 (sase_decapi.c reaches the host's callbacks by lui/lw where the encoder's use $gp); these
 * four touch no global. After them, the pitch candidates' merge 0x254458 (GCC round 26a: one exit,
 * the result in the first parameter's register; no assert names its file, so it is kept here).
 * Placeholders: the codec's names are not in SOCOM 1. */

#define NellyNull 0
extern void __assert(const char *file, int line, const char *expr);
#define assert(e) ((e) ? (void)0 : __assert(__FILE__, __LINE__, #e))

typedef struct {
	char pad0[28];
	float f28;
	float f32;
	float f36;
	float f40;
} DecMemBlk;

void func_00254318(DecMemBlk *pMemBlkD, float x)
{
#line 526 "../../SaseDec/source/DecSC.c"
	assert(pMemBlkD != NellyNull);
	pMemBlkD->f28 = x;
}

void func_00254368(DecMemBlk *pMemBlkD, float x)
{
#line 540 "../../SaseDec/source/DecSC.c"
	assert(pMemBlkD != NellyNull);
	pMemBlkD->f32 = x;
}

void func_002543B8(DecMemBlk *pMemBlkD, float x)
{
#line 554 "../../SaseDec/source/DecSC.c"
	assert(pMemBlkD != NellyNull);
	pMemBlkD->f36 = x;
}

void func_00254408(DecMemBlk *pMemBlkD, float x)
{
#line 568 "../../SaseDec/source/DecSC.c"
	assert(pMemBlkD != NellyNull);
	pMemBlkD->f40 = x;
}

/* the two pitch candidates merged (GCC round 26a; after the setters, no assert naming its file):
 * within 25 % of each other they are averaged, else the better-voiced one is taken -- and a low
 * pitch whose voicing is weak gives way to the higher candidate. 0.8 is the vendor's truncated
 * conversion (0x3F4CCCCC), a hex float. */
float func_00254458(float fP1, float fP2, float fV1, float fV2)
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
