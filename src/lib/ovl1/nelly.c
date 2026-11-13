/* sc: compiler=ee-gcc2.96; flags=-O2 -G8 */
/* Nellymoser's speech codec (SaseEncVad/SaseDec, "SC.01.01.00"), OVL1 0x248350-0x256FD0: the
 * voice chat's encoder and decoder, prebuilt with ee-gcc 2.96 at -O2 and the default -G 8 --
 * its small data and float literals sit in MAIN's $gp area (0x1D5300-0x1D55C0), so every
 * scalar global here is reached through $gp (README "The GCC front"). Placeholders: the
 * codec's own names are not in SOCOM 1. */

extern void *(*data_001D55A8)(int n, int size);  /* the host's allocator (calloc's arguments) */
extern float data_001D5444;
extern float data_001D5448;
extern float logf(float x);
extern int data_001D55A0;                      /* the codec's mode (0-2) */
extern int func_00252470(int a, int b, int n);
extern int func_00252098(int a, int b, int n);
extern int func_00251C80(int a, int b, int n);
extern void (*data_001D55B0)(void *p, int c, int n);    /* the host's memset */
extern unsigned short data_001D55C0;            /* nonzero while a frame is in flight */
extern void (*data_001D55AC)(void *);           /* the host's free callback */

int func_00256F88(void (*x)(void *p, int c, int n))
{
    data_001D55B0 = x;
    return 0;
}

int func_00256F48(void *(*alloc)(int n, int size))
{
    if (data_001D55C0 != 0)
        return 6220;
    data_001D55A8 = alloc;
    return 0;
}

typedef struct {
    int f0;
    int f4;
    float f8;
} CODEC_F;

int func_0024FF58(CODEC_F **pp, float x)
{
    CODEC_F *p;

    p = data_001D55A8(1, sizeof(CODEC_F));
    *pp = p;
    if (p == 0)
        return 1;
    p->f8 = x;
    p->f4 = 0;
    p->f0 = 0;
    return 0;
}

int func_00250BE0(float x)                       /* a level in dB, rounded: 8..160 through log */
{
    float t;

    if (!(x > 8.0f))
        x = 8.0f;
    t = (data_001D5444 - logf(x > 160.0f ? 160.0f : x)) * data_001D5448;
    if (t > 0.0f)
        return (int)(t + 0.5f);
    return (int)(t - 0.5f);
}

int func_00251C00(int a, int b, int n)          /* by the mode: one of two passes over n/2 */
{
    int half = n / 2;

    func_00252470(a, b, half);
    if (data_001D55A0 != 0)
        return func_00252098(b, a, half);
    return func_00251C80(b, a, half);
}

void func_0024FFB8(void **pblock)
{
    if (*pblock != 0) {
        data_001D55AC(*pblock);
        *pblock = 0;
    }
}

typedef struct {
    float afA[4];
    float afB[4];
    float f32, f36, f40, f44;
} Blk48;

/* a 48-byte block: four floats cleared, then its two 4-float arrays through the host's memset.
 * The floats are stored in the order tools/ccperm.py measured (one of 24; GCC round 19a): as ints
 * the stores may alias the memset pointer and its load stays after them (19 words) */
int func_00249EE0(Blk48 **pp)
{
    Blk48 *p;

    p = data_001D55A8(1, 48);
    *pp = p;
    if (p == 0)
        return 1;
    p->f44 = 0;
    p->f32 = 0;
    p->f36 = 0;
    p->f40 = 0;
    data_001D55B0(p->afA, 0, 16);
    data_001D55B0(p->afB, 0, 16);
    return 0;
}

/* a level's band: 261 above 96, 20 less for each 8 below it, down to 161 -- and nothing returned at
 * or below 40 (v0 still holds the 56 test's slot's 161 there). GCC round 74a: the chain one test
 * deeper than the levels it distinguishes, and no final return: the 40 test's jump is deleted after
 * flow's last pass, the 48 test left a dead `c.lt.s` before `jr ra` (open since round 19a: every
 * shape that ended in a return folded the 56 and 48 tests whole). */
int func_00251748(float fX)
{
    if (fX > 96.0F)
        return 261;
    if (fX > 88.0F)
        return 241;
    if (fX > 80.0F)
        return 221;
    if (fX > 72.0F)
        return 201;
    if (fX > 64.0F)
        return 181;
    if (fX > 56.0F)
        return 161;
    if (fX > 48.0F)
        return 161;
    if (fX > 40.0F)
        return 161;
}

int func_00249F68(void **pblock)
{
    if (*pblock != 0) {
        data_001D55AC(*pblock);
        *pblock = 0;
    }
    return 0;
}

/* the warp's table (GCC round 26a): 256 bins read back from the warped spectrum in fixed point,
 * the upper 169 by a quadratic in the bin, the lower 87 by a straight line, from the top down.
 * The upper index is a variable of its own (the target adds its 29 before the scaling; in one
 * expression combine folds it into the load's offset, 116(v0): 21 words) */
void func_00251678(const float *pfIn, float *pfOut)
{
	unsigned int i, uPos, k;

	pfOut += 255;
	uPos = 0xA280;
	for (i = 255; i >= 87; i--) {
		uPos -= 162;
		k = ((((uPos >> 8) + 65) * i + 209) >> 8) + 29;
		*pfOut-- = pfIn[k];
	}
	uPos = 17789;
	for (i = 87; i != 0; i--) {
		uPos -= 203;
		*pfOut-- = pfIn[uPos >> 8];
	}
}

/* Not written (GCC round 26a, measured): the fast log2 0x2517D8 (exp2 0x251850: written below, OPEN) after the warp
 * (the float's exponent and mantissa split and rebuilt by bits). The target reads the bits with
 * mfc1 and widens them with an `and` of 0xFFFFFFFF -- an unsigned 32-bit field extracted from a
 * 64-bit word, as the codec's `union { float f; long l; }` (its own asserts name `union_x.l`) is
 * on the EE, where long is 64 bits -- and passes the rebuilt mantissa and the exponent through
 * one stack word (sw, lwc1; cvt.s.w from memory). Eighteen spellings (pointer casts, unions of
 * float and int or long, volatile, BLKmode unions: build/scratch/gcc/v_flog*.c) reach neither: a
 * register union ORs in the undefined upper word, a pointer cast stays in registers (ADDRESSOF).
 * GCC round 39a, the log2 (build/scratch/gcc/r39/v_log2*.py, 23 shapes; try.py's distance): each half
 * reproduced apart, not yet together -- the input's `mfc1; and 0xFFFFFFFF` is a float/long union in
 * a REGISTER cleared first (`x.l = 0; x.f = fX;`: store_bit_field's OR with the upper word folds
 * away, the mask is the zero-extension), and the output's stack word (sw, lwc1; sw, lwc1, cvt.s.w)
 * a float/int union kept in the frame only once its FLOAT member is written (`union_x.l =
 * union_x.l * 0.5F - 1.0F`: written only as an int it moves through mtc1). Left: each DImode value
 * stored to the int member is truncated here (dsll32/dsra32) where the target stores the 64-bit
 * register as it is -- d=19 of 29 words at best ((int) casts, an int member of the input union, a
 * 32-bit copy of the bits: right size, d=21). */

float func_00251700(float x)                    /* a frequency warp (the .lit4 constants) */
{
    if (x > 97.0f)
        return x * (x * -0.0016021727351471782f + 1.40234375f) + 1.7890625f;
    return x * 1.26953125f;
}

/* the encoder's fast exp2 (GCC round 40a, written; OPEN, d=33 of 38 words, 0x94 for 0x98 --
 * build/scratch/gcc/r40/v_exp2*.py, 30 shapes): 2^fX from the integer part i (fX less 0.9999F, the
 * .lit4 constant at 0x1D530C, when fX <= 0, then truncated and one added) and a linear mantissa
 * rebuilt by bits. Measured: the input's `mfc1; and 0xFFFFFFFF` is round 39a's rule (a float/long
 * union cleared first); a long result read back through a pointer is truncated before its mtc1
 * (dsll32/dsra32), a union's is not; `i` set in BOTH arms keeps the target's `(i + 1) + 126` unfolded
 * (one assignment after the test lets CSE fold it to i + 127) -- but here each arm keeps its own
 * conversion, where the target converts once after the merge. Not reproduced: that single
 * conversion; the target's 16-byte frame no instruction touches (an address-taken local's slot,
 * ADDRESSOF's footprint: `*(u_int *)&x` makes the frame but keeps x in memory); its zero loaded as
 * `li at,0; mtc1` (a CONST_DOUBLE 0.0 that is not CONST0_RTX -- `mtc1 zero` for every 0.0F, (float)0,
 * 0 or the cleared union's float tried). Round 41a (build/scratch/gcc/r41/v_exp2*.py, 17 shapes): the
 * target's one conversion reads as the two arms' common tail CROSS-JUMPED after reload (both arms'
 * values in $f0, fX copied there ahead of the test, `cvt.w.s; mfc1; addiu 1` once); here the jump
 * optimiser hoists the then-arm's conversion above the test instead (`cvt.w.s; mfc1` before the
 * c.lt.s, its +1 in the slot) in every arm order, the ternary, a goto and a float copy of fX feeding
 * both arms (d=33); one conversion after the merge through a float t folds `(i + 1) + 126` to `+ 127`
 * and turns the test into c.le.s (d=37). Round 79 (build/scratch/gcc2-r79/vexp2.py): the 16-byte frame
 * is an addressable local in a dead arm -- `if (0) { long t[2]; ... }` anywhere in the body gives it
 * (the right size, d=32) and changes no instruction: `if (0)` is gone before RTL, so it leaves no
 * join; the arm the target lost was dead only after expansion. */
float func_00251850(float fX)
{
    union { float f; long l; } m;
    union { float f; long l; } r;
    int i;

    if (fX > 0.0F)
        i = (int)fX + 1;
    else
        i = (int)(fX - 0.9999F) + 1;
    m.l = 0;
    m.f = (fX - (float)i) * 0.5F + 1.0F;
    r.l = (m.l & 0x7FFFFF) | (((long)(i + 126) << 23) & 0x7F800000);
    return r.f;
}
