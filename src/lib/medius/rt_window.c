/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* rt_window.c: a table of 64 slots of 256 bytes (OVL2 0x642968-0x642DA8), in SCEA's RT library,
 * prebuilt with ee-gcc 2.96 at -O2 -G0 (README "The GCC front"): round 10b's "three-function table
 * object" between rt_upnp.c and SHA1.c. It has no $Header, no data and no SOCOM 1 counterpart, so the
 * file name (after rt_upnp.c in the alphabetical link order) and every name are ours. Its create and
 * destroy are rt_frag.c's (0x6451F0) over another layout: a table (0x4010 bytes, allocated zeroed)
 * holds four words, the first 1 when reset, and 64 slots whose first two words are cleared. The
 * object runs on past the three: 0x642BF8, the bytes held from the next sequence's slot on, is its own
 * (a leaf: the walk's `break` to one `return`, where returning from inside the loop gives a second
 * `jr ra`), and so are 0x642A58 between them -- a fragment stored by sequence in its slot, five
 * arguments -- and 0x642C48 after it, the read (round 23b: sha1.c's header had claimed it; it walks
 * this table's slots and state, and SHA1.c starts at 0x642DA8, its init). A jitter window: fragments
 * stored by sequence, read back in order once two are held. */

extern int rt_memory_allocate_init_zero(void **ppMemory, unsigned long nSize);
extern int rt_memory_free(void **ppMemory);
extern void *memcpy(void *d, const void *s, unsigned int n);

#define RT_WINDOW_SLOTS 64

/* the window's state (an enum: its own alias class -- the read loads it ahead of its store through
 * the caller's pointer, as the target does) */
typedef enum RT_WINDOW_STATE {
    RT_WINDOW_PLAYING = 0,              /* reading stops at the first empty slot */
    RT_WINDOW_FILLING = 1,              /* reset: nothing is read until two slots are held */
    RT_WINDOW_SKIPPING = 2              /* four or more held, or slots dropped: reads skip empty ones */
} RT_WINDOW_STATE;

typedef struct RT_WINDOW_SLOT {
    unsigned int nSeq;                  /* 0x00: the fragment's sequence number */
    unsigned int nLength;               /* 0x04: bytes held, 0 when empty */
    unsigned char aData[0xF8];          /* 0x08 */
} RT_WINDOW_SLOT;                       /* 0x100 */

typedef struct RT_WINDOW {
    RT_WINDOW_STATE eState;             /* 0x00: RT_WINDOW_FILLING when reset */
    unsigned int nUser;                 /* 0x04: the last store's word, handed back by every read */
    unsigned int nNext;                 /* 0x08: the next sequence number to read */
    unsigned int nCount;                /* 0x0C: slots held */
    RT_WINDOW_SLOT aSlots[RT_WINDOW_SLOTS]; /* 0x10 */
} RT_WINDOW;                            /* 0x4010 */

RT_WINDOW *func_00642968(void)
{
    RT_WINDOW *pWindow = 0;
    unsigned int i;

    rt_memory_allocate_init_zero((void **)&pWindow, sizeof(RT_WINDOW));
    if (pWindow != 0) {
        pWindow->nUser = 0;
        pWindow->eState = RT_WINDOW_FILLING;
        pWindow->nNext = 0;
        pWindow->nCount = 0;
        for (i = 0; i < RT_WINDOW_SLOTS; i++) {
            pWindow->aSlots[i].nSeq = 0;
            pWindow->aSlots[i].nLength = 0;
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

    pWindow->eState = RT_WINDOW_FILLING;
    pWindow->nNext = 0;
    pWindow->nCount = 0;
    for (i = 0; i < RT_WINDOW_SLOTS; i++) {
        pWindow->aSlots[i].nSeq = 0;
        pWindow->aSlots[i].nLength = 0;
    }
}

/* a fragment stored by its sequence number: one behind the window by less than its width is dropped,
 * one twice its width ahead or more too; otherwise a fragment outside the window moves it (the flush
 * walk after the move reads the moved window and never runs). A slot overwritten leaves the count.
 * Shapes (round 23b): each arm stores the move itself and the in-window case jumps past the walk --
 * one store after the arms lets CSE prove the walk dead (0x11C against 0x19C), and a store in each
 * arm falling into the walk's test still lets CSE follow the fall-through; the slots are indexed
 * through pWindow, never a slot pointer -- 2.96's expander sums the array's and the field's offsets
 * as the variable part and adds the 4-byte field offset to the base first, the target's `(pWindow +
 * 4) + (i * 256 + 16)`, where a pointer gives `pSlot + 4`. */
void func_00642A58(RT_WINDOW *pWindow, unsigned int nUser, unsigned int nSeq, void *pData,
                   unsigned int nLength)
{
    unsigned int nSlot = nSeq & (RT_WINDOW_SLOTS - 1);
    int nDiff;

    if (nLength < sizeof(pWindow->aSlots[0].aData) && pWindow != 0 && nLength != 0) {
        nDiff = nSeq - pWindow->nNext;
        pWindow->nUser = nUser;
        if (nDiff < 0) {
            if ((int)(pWindow->nNext - nSeq) < RT_WINDOW_SLOTS)
                return;
            pWindow->nNext = nSeq;
        } else if (nDiff < RT_WINDOW_SLOTS)
            goto store;
        else {
            if (nDiff >= 2 * RT_WINDOW_SLOTS)
                return;
            pWindow->nNext = nSeq;
        }
        while ((int)(nSeq - pWindow->nNext) >= RT_WINDOW_SLOTS) {
            if (pWindow->aSlots[pWindow->nNext & (RT_WINDOW_SLOTS - 1)].nLength != 0) {
                pWindow->aSlots[pWindow->nNext & (RT_WINDOW_SLOTS - 1)].nLength = 0;
                pWindow->eState = RT_WINDOW_SKIPPING;
                pWindow->nCount--;
            }
            pWindow->nNext++;
        }
store:
        if (pWindow->aSlots[nSlot].nLength != 0) {
            pWindow->aSlots[nSlot].nLength = 0;
            pWindow->nCount--;
        }
        pWindow->aSlots[nSlot].nSeq = nSeq;
        if (nLength != 0) {
            memcpy(pWindow->aSlots[nSlot].aData, pData, nLength);
            pWindow->aSlots[nSlot].nLength = nLength;
            pWindow->nCount++;
        }
        if (pWindow->eState == RT_WINDOW_FILLING && pWindow->nCount >= 2)
            pWindow->eState = RT_WINDOW_PLAYING;
        if (pWindow->nCount >= 4)
            pWindow->eState = RT_WINDOW_SKIPPING;
    }
}

/* the bytes held in the slots from the next sequence's slot on, up to the first empty one */
unsigned int func_00642BF8(RT_WINDOW *pWindow)
{
    unsigned int nTotal = 0;
    unsigned int nStart;
    unsigned int i;

    if (pWindow == 0)
        return 0;
    nStart = pWindow->nNext & (RT_WINDOW_SLOTS - 1);
    i = nStart;
    do {
        if (pWindow->aSlots[i].nLength == 0)
            break;
        nTotal += pWindow->aSlots[i].nLength;
        i = (i + 1) & (RT_WINDOW_SLOTS - 1);
    } while (i != nStart);
    return nTotal;
}

/* the read: the slots from the next sequence on copied out while they fit, each emptied and the next
 * sequence moved past it; an empty slot ends the read unless the window is skipping. Shapes (round
 * 23b): the loop is a `for` -- its body ends at the continue label, so expand_end_loop rotates the
 * leading exits (the size tests) to the bottom as the target has them; a `while` ending in `if (i ==
 * nStart) break;` makes that test the back edge instead (10 words short). The slot index and its start
 * are one chained assignment before the store through pUser: as two statements the store fills the
 * state test's delay slot, where the target has the start's copy. */
unsigned int func_00642C48(RT_WINDOW *pWindow, unsigned int *pUser, unsigned char *pBuffer,
                           unsigned int nSize)
{
    unsigned int nTotal = 0;
    unsigned int i, nStart;
    RT_WINDOW_SLOT *pSlot;
    unsigned int nLength;

    if (pWindow == 0)
        return 0;
    nStart = i = pWindow->nNext & (RT_WINDOW_SLOTS - 1);
    *pUser = pWindow->nUser;
    if (pWindow->eState == RT_WINDOW_FILLING)
        return 0;
    for (; nSize != 0;) {
        pSlot = &pWindow->aSlots[i];
        nLength = pSlot->nLength;
        if (nSize < nLength)
            break;
        if (nLength != 0) {
            memcpy(pBuffer + nTotal, pSlot->aData, nLength);
            nTotal += nLength;
            pSlot->nLength = 0;
            nSize -= nLength;
            pWindow->nCount--;
            pWindow->nNext = pSlot->nSeq + 1;
        } else if (pWindow->eState != RT_WINDOW_SKIPPING)
            return nTotal;
        i = (i + 1) & (RT_WINDOW_SLOTS - 1);
        if (i == nStart)
            break;
    }
    if (pWindow->eState == RT_WINDOW_SKIPPING && pWindow->nCount < 4)
        pWindow->eState = RT_WINDOW_PLAYING;
    return nTotal;
}
