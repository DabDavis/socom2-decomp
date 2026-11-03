/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* rt_window.c: a table of 64 slots of 256 bytes (OVL2 0x642968-0x642A58), in SCEA's RT library,
 * prebuilt with ee-gcc 2.96 at -O2 -G0 (README "The GCC front"): round 10b's "three-function table
 * object" between rt_upnp.c and SHA1.c. It has no $Header, no data and no SOCOM 1 counterpart, so the
 * file name (after rt_upnp.c in the alphabetical link order) and every name are ours. Its create and
 * destroy are rt_frag.c's (0x6451F0) over another layout: a table (0x4010 bytes, allocated zeroed)
 * holds four words, the first 1 when reset, and 64 slots whose first two words are cleared. */

extern int rt_memory_allocate_init_zero(void **ppMemory, unsigned long nSize);
extern int rt_memory_free(void **ppMemory);

#define RT_WINDOW_SLOTS 64

typedef struct RT_WINDOW_SLOT {
    unsigned int x00;                   /* 0x00 */
    unsigned int x04;                   /* 0x04 */
    char data[0xF8];                    /* 0x08 */
} RT_WINDOW_SLOT;                       /* 0x100 */

typedef struct RT_WINDOW {
    unsigned int x00;                   /* 0x00: 1 when reset */
    unsigned int x04;                   /* 0x04 */
    unsigned int x08;                   /* 0x08 */
    unsigned int x0C;                   /* 0x0C */
    RT_WINDOW_SLOT aSlots[RT_WINDOW_SLOTS]; /* 0x10 */
} RT_WINDOW;                            /* 0x4010 */

RT_WINDOW *func_00642968(void)
{
    RT_WINDOW *pWindow = 0;
    unsigned int i;

    rt_memory_allocate_init_zero((void **)&pWindow, sizeof(RT_WINDOW));
    if (pWindow != 0) {
        pWindow->x04 = 0;
        pWindow->x00 = 1;
        pWindow->x08 = 0;
        pWindow->x0C = 0;
        for (i = 0; i < RT_WINDOW_SLOTS; i++) {
            pWindow->aSlots[i].x00 = 0;
            pWindow->aSlots[i].x04 = 0;
        }
    }
    return pWindow;
}

void func_006429E8(RT_WINDOW *pWindow)
{
    if (pWindow != 0)
        rt_memory_free((void **)&pWindow);
}

void func_00642A10(RT_WINDOW *pWindow)
{
    unsigned int i;

    pWindow->x00 = 1;
    pWindow->x08 = 0;
    pWindow->x0C = 0;
    for (i = 0; i < RT_WINDOW_SLOTS; i++) {
        pWindow->aSlots[i].x00 = 0;
        pWindow->aSlots[i].x04 = 0;
    }
}
