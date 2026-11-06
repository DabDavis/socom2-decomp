/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* rt_mt_platform.c: the random numbers' platform seed (OVL2 0x639058-0x6390F0), SCEA's RT library,
 * prebuilt with ee-gcc 2.96 at -O2 -G0 (README "The GCC front"): its own object after rt_mt.c's, as
 * SOCOM 1's setNewSeed_platform (0x411280, the same 0x98 bytes) is after its MT object. The seed array
 * handed to seedMTUsingArray is 64 words: the caller's seed, the RT time stamp, the time string, and
 * the four EE timers' counts (T0-T3_COUNT, 0x10000000 + 0x800 n). The seed is a `long` (setNewSeed
 * zero-extends its 32-bit seed into it), narrowed for its slot (`dsll32; dsra32`); each count is read
 * as a signed word into a `long` and narrowed back the same way (`lw; dsll32; dsra32; sw`: a count
 * stored straight from the volatile read folds the conversions away, 0x74 bytes).
 *
 * OPEN (d=19, every instruction the target's but their schedule): the target's first count has its
 * `dsll32` and `dsra32` apart (an `ori` between them) -- two insns there, where this compile's
 * narrowing is truncdisi2, one insn of two instructions -- and issues the call's `li a1,64` before the
 * second count's load, so the second count's address takes a2 and the fourth t0 (here a1 and a2).
 * Tried: four temporaries, all four read first, pointer locals, a loop, inline readers returning
 * long/unsigned long/int, volatile and char-offset stores, -fno-strict-aliasing, -O1, -O3,
 * -fno-schedule-insns (each d=19 or worse); no row in 0x60FB50-0x668000 but this one keeps a dsll32/dsra32
 * pair apart (build/scratch/gcc-r22b/split32.py). */

typedef struct RT_TIME { unsigned int secs; unsigned int usecs; } RT_TIME;

extern int rt_time_platform_get_time_stamp(RT_TIME *t);
extern int rt_time_platform_get_time_string(char *str);
extern void seedMTUsingArray(unsigned int *pArray, unsigned int nLength);

#define RT_EE_COUNT(n)  (*(volatile int *)(0x10000000 + (n) * 0x800))

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
