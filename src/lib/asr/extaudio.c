/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing */
/* The recogniser's external audio source (OVL2 0x4C6118-0x4C619C): ScanSoft's recogniser,
 * prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing (README "The GCC front"). Names are
 * SOCOM 1's (s1:align). The samples come from a callback the host registers. */

extern void *func_00243298(void *heap, int size);                  /* the heap's malloc */
extern int (*data_0064F7C8)(void *src, void *buf, int *count);     /* the host's reader */

int ExtAudioInit(void *heap, int **handle)
{
    int *p = (int *)func_00243298(heap, 4);

    *p = 0;
    *handle = p;
    return 0;
}

int ExtAudioGetSamples(void *src, void *buf, int count)
{
    data_0064F7C8(src, buf, &count);
    return count;
}
