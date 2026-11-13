/* sc: compiler=ee-gcc2.96; flags=-O2 -G8 */
/* Nellymoser's shared FFT, fft4CFG.c and after (OVL1 0x251C80-0x252338): the transforms the FFT
 * interface (sase_fftif.c) dispatches to by the codec's mode -- the radix-4 transform 0x251C80
 * (its asserts fft4CFG.c's), the VU0 transform 0x252098 (its microcode at 0x3D5980, which the
 * map's OVL1 text ends in: VU0's, not VU1's) and its set-up 0x252150 -- and the bit-reversal
 * permutation of a split complex vector (0x252290). No assert names the files after fft4CFG.c's,
 * so they are kept here by address; the real-FFT split and merge after them are rFft2G.c's
 * (sase_rfft2g.c: its .rodata names the file, GCC round 33a). Written in the GCC lane's
 * round 26a: 0x252098, 0x252150, 0x252290; the radix-4 transform 0x251C80 in the OVL1 lane's
 * round 1. Placeholders: the codec's names are not in SOCOM 1.
 * Built as nelly.c is (ee-gcc 2.96, -O2 -G8: the encoder's range). */

#if Z_PORT
#include <zport.h>   /* the port: the hooks the asm sites use */
#endif
extern void *memcpy(void *, const void *, unsigned int);

#define VU0_MEM_RE ((void *)0x11004000)		/* VU0 data memory, the microprogram's buffers */
#define VU0_MEM_IM ((void *)0x11004400)

#define VU0_MICRO_MEM ((void *)0x11000000)
#define VU0_MEM_TAB ((void *)0x11004800)

extern char data_003D5980[];		/* the transform's VU0 microcode (OVL1's text ends in it) ... */
extern char data_003D6750[];		/* ... and its end */
extern char data_003EB088[];		/* its table, 2016 bytes */

extern void __assert(const char *file, int line, const char *expr);
#define assert(e) ((e) ? (void)0 : __assert(__FILE__, __LINE__, #e))

/* the radix-4 transform on the CPU (the dispatcher's 0x251B78 when the VU0 flag 0x1D55A0 is clear;
 * GCC lane OVL1 round 1, unwritten before): decimation in frequency over split re/im arrays of 256 or
 * 64 points, the same twiddle table the VU0 microprogram takes (data_003EB088, 2016 bytes: per chunk
 * of four points six 4-wide rows W1r W1i W2r W2i W3r W3i), each stage's slice read ending at pfW --
 * 1536 bytes for 256's first stage, so 256 starts at +384 floats and 64 at +480, the tail of 256's --
 * the groups and the chunks walked from the last backwards, four points a chunk; the twiddle-free
 * last stage; and the bit reversal func_00252290 is, written out again over nGroups * 4 (the
 * function after it in the file: 2.96 cannot inline it). Read off the target:
 *  - each point's eight values loaded into variables first (fR0..fI3), in both radix loops: summed
 *    straight from the arrays the allocation and the order of the address givs are others (37 words);
 *  - the six twiddles loaded into variables before any store (through pfW they would reload after
 *    each store to the data: may alias);
 *  - `nGroups = 1` before the table's `if` keeps the second size test (the target re-tests 256 after
 *    the assert where this compile threaded the jump past it);
 *  - the bit reversal's carry is its own variable (sharing the chunk counter's, it took t5 and its
 *    `k = n` was hoisted into the branch's slot) and runs over nGroups itself (`sll s6,s6,2`);
 *  - the first pointer's offset is `4 * q * (nGroups - 1) + q - 4` (q first in the mult; `(nGroups -
 *    1) * 4 * q` puts it second). */
void func_00251C80(float *pfRe, float *pfIm, int size)
{
	float *pfR, *pfI;
	const float *pfW;
	int nGroups, q, g, c, m, i, j, k;
	float fR02s, fR02d, fR13s, fR13d, fI02s, fI02d, fI13s, fI13d;
	float fA, fB, fC, fD, fE, fH;
	float fW1r, fW1i, fW2r, fW2i, fW3r, fW3i;
	float fR0, fR1, fR2, fR3, fI0, fI1, fI2, fI3;
	float fT;

#line 127 "../../shared/fft4CFG.c"
	assert((size==256) || (size==64));
	nGroups = 1;
	if (size == 256)
		pfW = (const float *)data_003EB088 + 384;
	else
		pfW = (const float *)data_003EB088 + 480;
	do {
		q = size >> 2;
		pfR = pfRe + 4 * q * (nGroups - 1) + q - 4;
		pfI = pfIm + 4 * q * (nGroups - 1) + q - 4;
		for (g = 0; g < nGroups; g++) {
			pfW -= (size >> 4) * 24;
			for (c = q - 4; c >= 0; c -= 4) {
				for (m = 0; m < 4; m++) {
					fR0 = pfR[m];
					fR1 = pfR[m + q];
					fR2 = pfR[m + 2 * q];
					fR3 = pfR[m + 3 * q];
					fI0 = pfI[m];
					fI1 = pfI[m + q];
					fI2 = pfI[m + 2 * q];
					fI3 = pfI[m + 3 * q];
					fR02s = fR0 + fR2;
					fR02d = fR0 - fR2;
					fR13s = fR1 + fR3;
					fR13d = fR1 - fR3;
					fI02s = fI0 + fI2;
					fI02d = fI0 - fI2;
					fI13s = fI1 + fI3;
					fI13d = fI1 - fI3;
					fW1r = pfW[0];
					fW1i = pfW[4];
					fW2r = pfW[8];
					fW2i = pfW[12];
					fW3r = pfW[16];
					fW3i = pfW[20];
					fA = fR02s - fR13s;
					fB = fI02s - fI13s;
					fC = fR02d + fI13d;
					fD = fI02d - fR13d;
					fE = fR02d - fI13d;
					fH = fI02d + fR13d;
					pfR[m] = fR02s + fR13s;
					pfI[m] = fI02s + fI13s;
					pfR[m + q] = fA * fW1r - fB * fW1i;
					pfI[m + q] = fB * fW1r + fA * fW1i;
					pfR[m + 2 * q] = fC * fW2r - fD * fW2i;
					pfI[m + 2 * q] = fD * fW2r + fC * fW2i;
					pfR[m + 3 * q] = fE * fW3r - fH * fW3i;
					pfI[m + 3 * q] = fH * fW3r + fE * fW3i;
					pfW++;
				}
				pfR -= 4;
				pfI -= 4;
				pfW += 20;
			}
			pfR -= 3 * q;
			pfI -= 3 * q;
		}
		nGroups *= 4;
		size /= 4;
		pfW += (size >> 4) * 24;
	} while (size > 4);
	pfR = pfRe + nGroups * 4 - 4;
	pfI = pfIm + nGroups * 4 - 4;
	for (g = 0; g < nGroups; g++) {
		fR0 = pfR[0];
		fR1 = pfR[1];
		fR2 = pfR[2];
		fR3 = pfR[3];
		fI0 = pfI[0];
		fI1 = pfI[1];
		fI2 = pfI[2];
		fI3 = pfI[3];
		fR02s = fR0 + fR2;
		fR02d = fR0 - fR2;
		fR13s = fR1 + fR3;
		fR13d = fR1 - fR3;
		fI02s = fI0 + fI2;
		fI02d = fI0 - fI2;
		fI13s = fI1 + fI3;
		fI13d = fI1 - fI3;
		pfR[0] = fR02s + fR13s;
		pfI[0] = fI02s + fI13s;
		pfR[2] = fR02d + fI13d;
		pfI[2] = fI02d - fR13d;
		pfR[1] = fR02s - fR13s;
		pfI[1] = fI02s - fI13s;
		pfR[3] = fR02d - fI13d;
		pfI[3] = fI02d + fR13d;
		pfR -= 4;
		pfI -= 4;
	}
	nGroups *= 4;
	j = nGroups >> 1;
	for (i = 1; i < nGroups - 1; i++) {
		if (j < i) {
			fT = pfRe[i];
			pfRe[i] = pfRe[j];
			pfRe[j] = fT;
			fT = pfIm[i];
			pfIm[i] = pfIm[j];
			pfIm[j] = fT;
		}
		k = nGroups;
		while ((j ^= (k >>= 1)) < k)
			;
	}
}

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
#if Z_PORT
	/* vi1 = (n != 256), the syncs, and the FFT micro program func_00252150 copied into VU0's micro
	 * memory run from 0 on the data copied above */
	z_vu0.vi[1] = n != 256;
	z_vu0_callms(0);
#else
	__asm__ __volatile__(
		"ctc2 %0, $vi1\n"
		"\tsync.p\n"
		"\tsync\n"
		"\tsync.p\n"
		"\tvnop\n"
		"\tvcallms 0\n"
		"\tvnop"
		: : "r" (n != 256));
#endif
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
