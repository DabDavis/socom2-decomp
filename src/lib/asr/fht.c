/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* fht: the recogniser's real FFT through the fast Hartley transform (OVL2 0x4E71F8-0x4E7C78,
 * SOCOM 1's object at 0x354FD8: fht, realfft, both global there), ScanSoft's speech recogniser,
 * prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops (README "The GCC
 * front"). Names are SOCOM 1's (realfft called by ProcessFFTMod, fht by realfft, in both builds).
 * The file's name is ours. Written: realfft (round 32c); fht (0x8BC) is not. */

void fht(float *x, unsigned int n);

/* the real FFT of n samples in place: the Hartley transform, then each pair of bins i and n - i
 * (0 < i < n/2) turned into their half sum and half difference. Indexed by two counters, the
 * second from n - 1 down: loop.c's walkers then start after the entry test, the high one from
 * n - 1 scaled (pointers written as such start before it; x[n - i] starts from n scaled, -4 in the
 * accesses: 2 words) */
void realfft(unsigned int n, float *x)
{
    unsigned int i, j, half;
    float a, b;

    fht(x, n);
    half = n >> 1;
    for (i = 1, j = n - 1; i < half; i++, j--) {
        a = x[i];
        b = x[j];
        x[j] = (a - b) * 0.5f;
        x[i] = (a + b) * 0.5f;
    }
}
