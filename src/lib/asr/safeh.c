/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* safeh: the recogniser's safe handles (OVL2 0x50D8D0-0x50D968, SOCOM 1's object at 0x377FE0:
 * safeh_HandleCheck, safeh_HandlesEqual, s1:order), ScanSoft's speech recogniser, prebuilt with
 * ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops (README "The GCC front"). A handle
 * is a pointer and a check word, passed by value (the EABI puts the 8-byte struct in one
 * register; GCC spills it to the frame to read its halves). */

typedef struct SAFEH {
    void *p;
    int check;
} SAFEH;

extern int osspi_IsGoodReadPtr(void *p, int size);
extern int osspi_IsGoodWritePtr(void *p, int size);

int safeh_HandleCheck(SAFEH h, int check, int size)
{
    int r;

    if (h.check != check)
        return 0x80170189;
    if (h.p == 0)
        return 0x80170189;
    r = osspi_IsGoodReadPtr(h.p, size);
    if (r < 0)
        return r;
    return osspi_IsGoodWritePtr(h.p, size);
}

int safeh_HandlesEqual(SAFEH a, SAFEH b)
{
    if (a.check != b.check)
        return 0;
    return a.p == b.p;
}
