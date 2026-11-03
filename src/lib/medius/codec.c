/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* codec.c: the voice library's codec wrapper (OVL2 0x6132B8-0x613FD0), SCEA's (its `$Header` is the
 * second of the voice library's .rodata, after audio.c's), prebuilt with ee-gcc 2.96 at -O2 -G0
 * (README "The GCC front"). SOCOM 1 has no voice library: the names are placeholders. A codec is
 * an allocated 0xA20-byte record made by 0x6132B8 for a codec type (33-63, a switch) and a kind
 * (1 an encoder, 2 a decoder): its type and kind, its frame sizes, the codec's own reset, destroy,
 * encode and decode entry points (the stub codecs' 8-byte functions at 0x64F580 for types 34-36,
 * this object's dummy for 33 and 63), the stream kind it was made for, the codec's context, and two
 * buffers -- 480 coded bytes at 0x40 and 2048 PCM bytes at 0x220 -- with their fill counts. In image
 * order; the frame calls 0x613808 and 0x613B80 are not written. */

extern int rt_memory_allocate_init_zero(void **ppMemory, unsigned long nSize);
extern int rt_memory_free(void **ppMemory);

typedef struct CODEC {
    int nType;                          /* 0x00 */
    int nKind;                          /* 0x04: 1 an encoder, 2 a decoder */
    int nFrameBytes;                    /* 0x08 */
    int nCodedBytes;                    /* 0x0C */
    int x10[4];
    int (*pfnReset)(void *pContext);    /* 0x20 */
    void (*pfnDestroy)(void *pContext); /* 0x24 */
    int (*pfnEncode)(void *pContext, void *pIn, void *pOut);   /* 0x28 */
    int (*pfnDecode)(void *pContext, void *pIn, void *pOut);   /* 0x2C */
    int x30;                            /* 0x30: PCM bytes buffered, cleared by a reset */
    int x34;                            /* 0x34: coded bytes buffered, cleared by a reset */
    int nStreamKind;                    /* 0x38 */
    void *pContext;                     /* 0x3C */
    unsigned char abCoded[480];         /* 0x40 */
    unsigned char abPcm[2048];          /* 0x220 */
} CODEC;                                /* 0xA20 */

static int gnCodecCalls = 0;            /* 0x651170 */

/* the stub codecs' entry points (after the server key, 0x64F580-0x64F5F8) */
extern void *func_0064F580(void);
extern void func_0064F588(void *pContext);
extern int func_0064F590(void *pContext);
extern int func_0064F598(void *pContext, void *pIn, void *pOut);
extern int func_0064F5A0(void *pContext, void *pIn, void *pOut);
extern void *func_0064F5A8(void);
extern int func_0064F5B0(void *pContext);
extern int func_0064F5B8(void *pContext, void *pIn, void *pOut);
extern int func_0064F5C0(void *pContext, void *pIn, void *pOut);
extern void func_0064F5C8(void *pContext);
extern void *func_0064F5D0(void);
extern int func_0064F5D8(void *pContext);
extern int func_0064F5E0(void *pContext, void *pIn, void *pOut);
extern int func_0064F5E8(void *pContext, void *pIn, void *pOut);
extern void func_0064F5F0(void *pContext);

void func_00613750(CODEC *p);
int func_00613F98(void);
void func_00613FB0(void *pContext);
int func_00613FB8(void *pContext);
int func_00613FC0(void *pContext, void *pIn, void *pOut);
int func_00613FC8(void *pContext, void *pIn, void *pOut);

CODEC *func_006132B8(int nStreamKind, int nType, int nKind)
{
    CODEC *pCodec = 0;

    rt_memory_allocate_init_zero((void **)&pCodec, sizeof(CODEC));
    if (pCodec == 0)
        return 0;
    pCodec->nType = nType;
    pCodec->nKind = nKind;
    pCodec->nStreamKind = nStreamKind;
    if (pCodec->nKind == 1) {
        switch (pCodec->nType) {
        case 34:
            pCodec->pContext = func_0064F580();
            pCodec->pfnReset = func_0064F590;
            pCodec->pfnDestroy = func_0064F588;
            pCodec->nFrameBytes = 320;
            pCodec->nCodedBytes = 33;
            pCodec->x10[0] = 320;
            pCodec->x10[1] = 320;
            pCodec->x10[2] = 320;
            pCodec->x10[3] = 320;
            pCodec->pfnEncode = func_0064F598;
            pCodec->pfnDecode = func_0064F5A0;
            break;
        case 35:
            pCodec->pContext = func_0064F5A8();
            pCodec->pfnReset = func_0064F5B0;
            pCodec->pfnDestroy = func_0064F5C8;
            pCodec->nFrameBytes = 360;
            pCodec->nCodedBytes = 7;
            pCodec->x10[0] = 360;
            pCodec->x10[1] = 360;
            pCodec->x10[2] = 360;
            pCodec->x10[3] = 360;
            pCodec->pfnEncode = func_0064F5B8;
            pCodec->pfnDecode = func_0064F5C0;
            break;
        case 36:
            pCodec->pContext = func_0064F5D0();
            pCodec->pfnReset = func_0064F5D8;
            pCodec->pfnDestroy = func_0064F5F0;
            pCodec->nFrameBytes = 320;
            pCodec->nCodedBytes = 8;
            pCodec->x10[0] = 320;
            pCodec->x10[1] = 320;
            pCodec->x10[2] = 320;
            pCodec->x10[3] = 320;
            pCodec->pfnEncode = func_0064F5E0;
            pCodec->pfnDecode = func_0064F5E8;
            break;
        case 33:
        case 63:
            pCodec->pContext = (void *)func_00613F98();
            pCodec->pfnReset = func_00613FB8;
            pCodec->pfnDestroy = func_00613FB0;
            pCodec->nFrameBytes = 0;
            pCodec->nCodedBytes = 0;
            pCodec->x10[0] = 0;
            pCodec->x10[1] = 0;
            pCodec->x10[2] = 0;
            pCodec->x10[3] = 0;
            pCodec->pfnEncode = func_00613FC0;
            pCodec->pfnDecode = func_00613FC8;
            break;
        }
    } else if (pCodec->nKind == 2) {
        switch (pCodec->nType) {
        case 34:
            pCodec->pContext = func_0064F580();
            pCodec->pfnReset = func_0064F590;
            pCodec->pfnDestroy = func_0064F588;
            pCodec->nFrameBytes = 320;
            pCodec->nCodedBytes = 33;
            pCodec->x10[0] = 320;
            pCodec->x10[1] = 320;
            pCodec->x10[2] = 320;
            pCodec->x10[3] = 320;
            pCodec->pfnEncode = func_0064F598;
            pCodec->pfnDecode = func_0064F5A0;
            break;
        case 35:
            pCodec->pContext = func_0064F5A8();
            pCodec->pfnReset = func_0064F5B0;
            pCodec->pfnDestroy = func_0064F5C8;
            pCodec->nFrameBytes = 360;
            pCodec->nCodedBytes = 7;
            pCodec->x10[0] = 360;
            pCodec->x10[1] = 360;
            pCodec->x10[2] = 360;
            pCodec->x10[3] = 360;
            pCodec->pfnEncode = func_0064F5B8;
            pCodec->pfnDecode = func_0064F5C0;
            break;
        case 36:
            pCodec->pContext = func_0064F5D0();
            pCodec->pfnReset = func_0064F5D8;
            pCodec->pfnDestroy = func_0064F5F0;
            pCodec->nFrameBytes = 320;
            pCodec->nCodedBytes = 8;
            pCodec->x10[0] = 320;
            pCodec->x10[1] = 320;
            pCodec->x10[2] = 320;
            pCodec->x10[3] = 320;
            pCodec->pfnEncode = func_0064F5E0;
            pCodec->pfnDecode = func_0064F5E8;
            break;
        case 33:
        case 63:
            pCodec->pContext = (void *)func_00613F98();
            pCodec->pfnReset = func_00613FB8;
            pCodec->pfnDestroy = func_00613FB0;
            pCodec->nFrameBytes = 0;
            pCodec->nCodedBytes = 0;
            pCodec->x10[0] = 0;
            pCodec->x10[1] = 0;
            pCodec->x10[2] = 0;
            pCodec->x10[3] = 0;
            pCodec->pfnEncode = func_00613FC0;
            pCodec->pfnDecode = func_00613FC8;
            break;
        }
    } else {
        func_00613750(pCodec);
        pCodec = 0;
    }
    if (pCodec != 0 && pCodec->pContext == 0) {
        func_00613750(pCodec);
        pCodec = 0;
    }
    return pCodec;
}

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

/* the dummy codec (types 33 and 63): its create counts the calls and returns the count as its
 * context, so the create's context test passes */
int func_00613F98(void)
{
    return ++gnCodecCalls;
}

void func_00613FB0(void *pContext)
{
}

int func_00613FB8(void *pContext)
{
}

int func_00613FC0(void *pContext, void *pIn, void *pOut)
{
    return 0;
}

int func_00613FC8(void *pContext, void *pIn, void *pOut)
{
    return 0;
}
