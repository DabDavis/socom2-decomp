/* sc: compiler=ee-gcc2.96; flags=-O2 -G8 */
/* Nellymoser's speech codec (SaseEncVad/SaseDec, "SC.01.01.00"), OVL1 0x248350-0x256FD0: the
 * voice chat's encoder and decoder, prebuilt with ee-gcc 2.96 at -O2 and the default -G 8 --
 * its small data and float literals sit in MAIN's $gp area (0x1D5300-0x1D55C0), so every
 * scalar global here is reached through $gp (README "The GCC front"). Placeholders: the
 * codec's own names are not in SOCOM 1. */

extern int data_001D55A8;                       /* the decoder's packet size */
extern int data_001D55B0;
extern unsigned short data_001D55C0;            /* nonzero while a frame is in flight */
extern void (*data_001D55AC)(void *);           /* the host's free callback */

int func_00256F88(int x)
{
    data_001D55B0 = x;
    return 0;
}

int func_00256F48(int size)
{
    if (data_001D55C0 != 0)
        return 6220;
    data_001D55A8 = size;
    return 0;
}

void func_0024FFB8(void **pblock)
{
    if (*pblock != 0) {
        data_001D55AC(*pblock);
        *pblock = 0;
    }
}

int func_00249F68(void **pblock)
{
    if (*pblock != 0) {
        data_001D55AC(*pblock);
        *pblock = 0;
    }
    return 0;
}

float func_00251700(float x)                    /* a frequency warp (the .lit4 constants) */
{
    if (x > 97.0f)
        return x * (x * -0.0016021727351471782f + 1.40234375f) + 1.7890625f;
    return x * 1.26953125f;
}
