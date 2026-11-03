/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* window: the recogniser's windowing block (OVL2 0x4FC620-0x4FCEA8, SOCOM 1's object at 0x3684C0:
 * WindowFlush, ProcessWindow, InitWindow, ConstructWindow, ControlWindow, all but the constructor
 * LOCAL there; SOCOM 1's WindowFlush has no row here, inlined into ProcessWindow), ScanSoft's
 * speech recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops
 * (README "The GCC front"). Names are SOCOM 1's (the process and init rows by the constructor's
 * slots). Written so far: the constructor and the control; the process and init functions are
 * declared, not yet defined. */
#include "tosblk.h"

typedef struct WINDOW {
    TOSBLOCK b;
    void *buf;                          /* +40: the window's samples */
    char pad44[20];
    int pos;                            /* +64 */
    unsigned char started;              /* +68 */
    unsigned char active;               /* +69 */
    char pad70[4];
    unsigned char flush;                /* +74: a flush asked for before the start */
    char pad75;
    void *coefs;                        /* +76 */
} WINDOW;                               /* 0x50 */

extern void ProcessWindow(WINDOW *w, unsigned char **in, int n);
extern int InitWindow(WINDOW *w);
static int ControlWindow(WINDOW *w, unsigned char cmd);

TOSBLOCK *ConstructWindow(TOS *tos, unsigned char id)
{
    return tosBaseBlockConstruct(tos, id, 1, 1, (TOSFUNC)ProcessWindow, (TOSFUNC)InitWindow,
                                 (TOSFUNC)ControlWindow, sizeof(WINDOW));
}

static int ControlWindow(WINDOW *w, unsigned char cmd)
{
    switch (cmd) {
    case 1:
        w->started = 1;
        w->active = 1;
        w->pos = 0;
        break;
    case 2:
        if (w->active == 0)
            w->flush = 1;
        break;
    case TOS_DESTROY:
        heap_Free(w->b.tos->heap, w->buf);
        heap_Free(w->b.tos->heap, w->coefs);
        tosBaseBlockDestruct(&w->b);
        return 1;
    default:
        return 0;
    }
    return 1;
}
