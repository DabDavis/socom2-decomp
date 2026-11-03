/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* Nellymoser's decoder, DecSC.c's parameter setters (OVL1 0x254318-0x254458): each asserts its
 * block (`assert(pMemBlkD != NellyNull)`, file and line by `#line` as in sase_encsc.c) and stores
 * one float. Called by the decoder API's setters (sase_decapi.c). The decoder's objects were built
 * at -G0 (sase_decapi.c reaches the host's callbacks by lui/lw where the encoder's use $gp); these
 * four touch no global. Placeholders: the codec's names are not in SOCOM 1. */

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
