/* sc: compiler=ee-gcc2.96; flags=-O2 -G8 */
/* Nellymoser's encoder, EncSC.c's parameter setters (OVL1 0x249608-0x2499A8): each asserts its
 * block (the codec's own `assert(pEncMemBlk != NellyNull)`, file and line as its __assert call
 * passes them: the `#line`s give __FILE__ and __LINE__ the text the target's strings hold) and sets
 * one encoder parameter, marking it changed (the flag's store first: the target's registers). The
 * four flag setters read the parameter block before the assert, as a declaration's initialiser. Called one each by the encoder API's setters
 * (sase_encapi.c). Placeholders: the codec's names are not in SOCOM 1. Built as nelly.c is
 * (ee-gcc 2.96, -O2 -G8). */

#define NellyNull 0
extern void __assert(const char *file, int line, const char *expr);
#define assert(e) ((e) ? (void)0 : __assert(__FILE__, __LINE__, #e))

typedef struct {
	char pad0[28];
	float f28;
	short f32;
	short f34;
	char pad36[4];
	short f40;
	short f42;
	short f44;
	char pad46[2];
	float f48;
	float f52;
	float f56;
} EncParams;

typedef struct {
	char pad0[68];
	int f68;
	EncParams *params;
} EncMemBlk;

void func_00249608(EncMemBlk *pEncMemBlk, float x)
{
#line 876 "../../SaseEncVad/source/EncSC.c"
	assert(pEncMemBlk != NellyNull);
	pEncMemBlk->params->f40 = 1;
	pEncMemBlk->params->f28 = x;
}

void func_00249660(EncMemBlk *pEncMemBlk, float x)
{
#line 895 "../../SaseEncVad/source/EncSC.c"
	assert(pEncMemBlk != NellyNull);
	pEncMemBlk->params->f32 = x / 15.625f;
	pEncMemBlk->params->f40 = 1;
}

void func_002496D8(EncMemBlk *pEncMemBlk, float x)
{
#line 913 "../../SaseEncVad/source/EncSC.c"
	assert(pEncMemBlk != NellyNull);
	pEncMemBlk->params->f34 = x / 15.625f;
	pEncMemBlk->params->f40 = 1;
}

void func_00249750(EncMemBlk *pEncMemBlk, float x)
{
#line 932 "../../SaseEncVad/source/EncSC.c"
	assert(pEncMemBlk != NellyNull);
	pEncMemBlk->params->f44 = 1;
	pEncMemBlk->params->f56 = x;
}

void func_002497A8(EncMemBlk *pEncMemBlk, float x, float y)
{
#line 950 "../../SaseEncVad/source/EncSC.c"
	assert(pEncMemBlk != NellyNull);
	pEncMemBlk->params->f42 = 1;
	pEncMemBlk->params->f48 = x;
	pEncMemBlk->params->f52 = y;
}

void func_00249810(EncMemBlk *pEncMemBlk)
{
#line 960 "../../SaseEncVad/source/EncSC.c"
	assert(pEncMemBlk != NellyNull);
	pEncMemBlk->f68 = 1;
}

void func_00249858(EncMemBlk *pEncMemBlk)
{
#line 967 "../../SaseEncVad/source/EncSC.c"
	assert(pEncMemBlk != NellyNull);
	pEncMemBlk->f68 = 0;
}

void func_00249898(EncMemBlk *pEncMemBlk)
{
	EncParams *p = pEncMemBlk->params;

#line 976 "../../SaseEncVad/source/EncSC.c"
	assert(pEncMemBlk != NellyNull);
	p->f42 = 1;
}

void func_002498E0(EncMemBlk *pEncMemBlk)
{
	EncParams *p = pEncMemBlk->params;

#line 985 "../../SaseEncVad/source/EncSC.c"
	assert(pEncMemBlk != NellyNull);
	p->f42 = 0;
}

void func_00249920(EncMemBlk *pEncMemBlk)
{
	EncParams *p = pEncMemBlk->params;

#line 994 "../../SaseEncVad/source/EncSC.c"
	assert(pEncMemBlk != NellyNull);
	p->f44 = 1;
}

void func_00249968(EncMemBlk *pEncMemBlk)
{
	EncParams *p = pEncMemBlk->params;

#line 1003 "../../SaseEncVad/source/EncSC.c"
	assert(pEncMemBlk != NellyNull);
	p->f44 = 0;
}
