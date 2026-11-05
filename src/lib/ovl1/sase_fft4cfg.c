/* sc: compiler=ee-gcc2.96; flags=-O2 -G8 */
/* Nellymoser's shared FFT, fft4CFG.c and after (OVL1 0x251C80-0x252338): the transforms the FFT
 * interface (sase_fftif.c) dispatches to by the codec's mode -- the radix-4 transform 0x251C80
 * (its asserts fft4CFG.c's), the VU0 transform 0x252098 (its microcode at 0x3D5980, which the
 * map's OVL1 text ends in: VU0's, not VU1's) and its set-up 0x252150 -- and the bit-reversal
 * permutation of a split complex vector (0x252290). No assert names the files after fft4CFG.c's,
 * so they are kept here by address; the real-FFT split and merge after them are rFft2G.c's
 * (sase_rfft2g.c: its .rodata names the file, GCC round 33a). Written in the GCC lane's
 * round 26a: 0x252098, 0x252150, 0x252290. Placeholders: the codec's names are not in SOCOM 1.
 * Built as nelly.c is (ee-gcc 2.96, -O2 -G8: the encoder's range). */

extern void *memcpy(void *, const void *, unsigned int);

#define VU0_MEM_RE ((void *)0x11004000)		/* VU0 data memory, the microprogram's buffers */
#define VU0_MEM_IM ((void *)0x11004400)

#define VU0_MICRO_MEM ((void *)0x11000000)
#define VU0_MEM_TAB ((void *)0x11004800)

extern char data_003D5980[];		/* the transform's VU0 microcode (OVL1's text ends in it) ... */
extern char data_003D6750[];		/* ... and its end */
extern char data_003EB088[];		/* its table, 2016 bytes */

/* mode 2's set-up: the microcode into VU0's micro memory and its table into data memory (a
 * constant-size memcpy, GCC's block move: the run-time alignment test and its two loops) */
void func_00252150(void)
{
	memcpy(VU0_MICRO_MEM, data_003D5980, data_003D6750 - data_003D5980);
	memcpy(VU0_MEM_TAB, data_003EB088, 2016);
}

/* the transform on VU0 (the codec's mode 1): both halves copied into VU0's data memory, the
 * microprogram at 0 called with vi1 set unless n is 256, the results copied back */
void *func_00252098(float *pfRe, float *pfIm, int n)
{
	memcpy(VU0_MEM_RE, pfRe, n * sizeof(float));
	memcpy(VU0_MEM_IM, pfIm, n * sizeof(float));
	__asm__ __volatile__(
		"ctc2 %0, $vi1\n"
		"\tsync.p\n"
		"\tsync\n"
		"\tsync.p\n"
		"\tvnop\n"
		"\tvcallms 0\n"
		"\tvnop"
		: : "r" (n != 256));
	memcpy(pfRe, VU0_MEM_RE, n * sizeof(float));
	return memcpy(pfIm, VU0_MEM_IM, n * sizeof(float));
}

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
