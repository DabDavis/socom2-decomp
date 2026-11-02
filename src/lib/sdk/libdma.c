/* sc: compiler=ee-gcc2.9-991111; flags=-O2 -G0 */
/* libdma (PsIIlibdma 2700): Sony's SDK 2.7 DMA library, MAIN 0x190BC8-0x191410, prebuilt with
 * ee-gcc 2.9-ee-991111 at -O2 -G0 (README "The GCC front": memclr's short-loop nops are that
 * assembler's, and 2.96's register choices miss sceDmaGetChan). Names are SOCOM 1's (s1:exact). */

typedef unsigned int u_int;
typedef struct sceDmaChan sceDmaChan;

extern sceDmaChan *dch[10];             /* the ten channels' register blocks (data.txt) */

#define D_STADR     ((u_int *)0x1000E060)    /* not volatile: the store fills the return's delay slot */

void *CheckAddress(void *addr)
{
    u_int a = (u_int)addr;

    if ((a >> 28) == 7) {               /* the scratchpad: a DMA tag's SPR bit */
        a &= 0x0FFFFFFF;                /* two statements: one expression computes in v0 */
        a |= 0x80000000;
    }
    return (void *)a;
}

void memclr(void *p, int size)
{
    char *q = p;
    while (size--)
        *q++ = 0;
}

sceDmaChan *sceDmaGetChan(int id)
{
    return ((u_int)id < 10) ? dch[id] : 0;
}

void *sceDmaPutStallAddr(void *addr)
{
    void *old = (void *)*D_STADR;
    *D_STADR = (u_int)addr;
    return old;
}

int sceDmaRestart(sceDmaChan *d, u_int chcr)
{
    u_int old = *(volatile u_int *)d;
    *(volatile u_int *)d = chcr;
    return (old >> 8) & 1;
}
