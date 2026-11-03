/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* codec.c: the voice library's codec wrapper (OVL2 0x6132B8-0x613FD0), SCEA's (its `$Header` is the
 * second of the voice library's .rodata, after audio.c's), prebuilt with ee-gcc 2.96 at -O2 -G0
 * (README "The GCC front"). SOCOM 1 has no voice library: the names are placeholders. A codec is
 * an allocated 0xA20-byte record made by 0x6132B8 for a codec type (33-63, a switch) and a kind
 * (1 an encoder, 2 a decoder): its type and kind, its frame sizes, the codec's own reset and
 * destroy entry points (the dummy codec's 8-byte stubs at 0x64F580 among them), the stream kind it
 * was made for and the codec's context. Only the small functions are here, in image order: the
 * create 0x6132B8 and the frame calls 0x613808 and 0x613B80 are not written. */

extern int rt_memory_free(void **ppMemory);

typedef struct CODEC {
    int nType;                          /* 0x00 */
    int nKind;                          /* 0x04: 1 an encoder, 2 a decoder */
    int nFrameBytes;                    /* 0x08 */
    int nCodedBytes;                    /* 0x0C */
    int x10[4];
    int (*pfnReset)(void *pContext);    /* 0x20 */
    void (*pfnDestroy)(void *pContext); /* 0x24 */
    int x28[2];
    int x30;                            /* 0x30: cleared by a reset */
    int x34;                            /* 0x34: cleared by a reset */
    int nStreamKind;                    /* 0x38 */
    void *pContext;                     /* 0x3C */
} CODEC;

static int gnCodecCalls = 0;            /* 0x651170 */

void func_00613750(CODEC *p)
{
    if (p != 0) {
        if (p->pfnDestroy != 0)
            p->pfnDestroy(p->pContext);
        rt_memory_free((void **)&p);
    }
}

int func_00613790(CODEC *p)
{
    if (p == 0 || p->pContext == 0)
        return -1;
    if (p->pfnReset != 0)
        p->pfnReset(p->pContext);
    p->x30 = 0;
    p->x34 = 0;
    return 0;
}

int func_006137E8(CODEC *p)
{
    return p->nType;
}

int func_006137F0(CODEC *p)
{
    return p->nKind;
}

int func_006137F8(CODEC *p)
{
    return p->nCodedBytes;
}

int func_00613800(CODEC *p)
{
    return p->nFrameBytes;
}

void func_00613F98(void)
{
    gnCodecCalls++;
}

void func_00613FB0(void)
{
}

void func_00613FB8(void)
{
}

int func_00613FC0(void)
{
    return 0;
}

int func_00613FC8(void)
{
    return 0;
}
