/* sc: compiler=ee-gcc2.96; flags=-O2 -G8 */
/* Nellymoser's shared FFT, fft4CFG.c and after (OVL1 0x251C80-0x2525C0): the transforms the FFT
 * interface (sase_fftif.c) dispatches to by the codec's mode -- the radix-4 transform 0x251C80
 * (its asserts fft4CFG.c's), the others 0x252098 and 0x252338, mode 2's set-up 0x252150 -- and
 * the bit-reversal permutation of a split complex vector (0x252290). No assert names the files
 * after fft4CFG.c's, so they are kept here by address. Written in the GCC lane's round 26a:
 * 0x252290. Placeholders: the codec's names are not in SOCOM 1. Built as nelly.c is (ee-gcc 2.96,
 * -O2 -G8: the encoder's range). */

/* the bit-reversal permutation of n points held as two arrays, j the reverse of i kept by the
 * reversed carry (`j ^= k` while the bit was set); a pair is swapped from its larger index. The
 * carry's bit starts at n and is halved inside the loop's test (the target's `move v1,a2` and its
 * `sra` at the loop's head): starting at n >> 1, loop.c hoists the invariant (a word short). */
void func_00252290(float *pfRe, float *pfIm, int n)
{
	int i, j, k;
	float fT;

	j = n >> 1;
	for (i = 1; i < n - 1; i++) {
		if (j < i) {
			fT = pfRe[i];
			pfRe[i] = pfRe[j];
			pfRe[j] = fT;
			fT = pfIm[i];
			pfIm[i] = pfIm[j];
			pfIm[j] = fT;
		}
		k = n;
		while ((j ^= (k >>= 1)) < k)
			;
	}
}
