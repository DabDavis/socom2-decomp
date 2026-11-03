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
extern int data_001D55B0;
extern unsigned short data_001D55C0;            /* nonzero while a frame is in flight */
extern void (*data_001D55AC)(void *);           /* the host's free callback */

int func_00256F88(int x)
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
