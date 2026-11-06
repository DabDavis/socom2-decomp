/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* rt_mt_platform.c: the random numbers' platform seed (OVL2 0x639058-0x6390F0), SCEA's RT library,
 * prebuilt with ee-gcc 2.96 at -O2 -G0 (README "The GCC front"): its own object after rt_mt.c's, as
 * SOCOM 1's setNewSeed_platform (0x411280, the same 0x98 bytes) is after its MT object. The seed array
 * handed to seedMTUsingArray is 64 words: the caller's seed, the RT time stamp, the time string, and
 * the four EE timers' counts (T0-T3_COUNT, 0x10000000 + 0x800 n). The seed is a `long` (setNewSeed
 * zero-extends its 32-bit seed into it), narrowed for its slot (`dsll32; dsra32`); each count is read
 * into a `long` and narrowed back the same way (`lw; dsll32; dsra32; sw`: a count stored straight from
 * the volatile read folds the conversions away, 0x74 bytes).
 *
 * Round 23b closed round 22b's open (d=19, every instruction the target's but their schedule: the
 * first count's dsll32/dsra32 two insns apart, the call's `li a1,64` before the second count's load):
 * the counts are read as UNSIGNED words, as Sony's eeregs.h declares T0_COUNT (`volatile u_int *`).
 * The store into the long is then a zero extension, which 2.96's MIPS expander makes as two shifts,
 * and combine leaves the narrowing as two insns sched2 can pull apart; read as `int`, the extension
 * is a move and the narrowing one truncdisi2 insn (`dsll32; dsra32` fused), which no schedule splits. */

typedef struct RT_TIME { unsigned int secs; unsigned int usecs; } RT_TIME;

extern int rt_time_platform_get_time_stamp(RT_TIME *t);
extern int rt_time_platform_get_time_string(char *str);
extern void seedMTUsingArray(unsigned int *pArray, unsigned int nLength);

#define RT_EE_COUNT(n)  (*(volatile unsigned int *)(0x10000000 + (n) * 0x800))

void setNewSeed_platform(unsigned long seed)
{
    unsigned int aSeed[64];
    long t;

    aSeed[0] = seed;
    rt_time_platform_get_time_stamp((RT_TIME *)&aSeed[1]);
    rt_time_platform_get_time_string((char *)&aSeed[3]);
    t = RT_EE_COUNT(0);
    aSeed[6] = t;
    t = RT_EE_COUNT(1);
    aSeed[7] = t;
    t = RT_EE_COUNT(2);
    aSeed[8] = t;
    t = RT_EE_COUNT(3);
    aSeed[9] = t;
    seedMTUsingArray(aSeed, 64);
}
