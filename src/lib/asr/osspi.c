/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* osspi: the recogniser's operating-system interface (OVL2 0x50D968-0x50D9B8, SOCOM 1's object at
 * 0x378078: the same four functions in the same order, s1:order), ScanSoft's speech recogniser,
 * prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops (README "The GCC
 * front"). The PS2 has no memory protection: a pointer is good when it is not null. */

int osspi_SpiGetVersion(int *version)
{
    if (version == 0)
        return 0x80060183;
    *version = 0x01010000;
    return 0;
}

int osspi_IsGoodReadPtr(void *p, int size)
{
    return p ? 0 : 0x80060183;
}

int osspi_IsGoodWritePtr(void *p, int size)
{
    return p ? 0 : 0x80060183;
}

int osspi_IsGoodStringPtr(char *s, int size)
{
    return s ? 0 : 0x80060183;
}
