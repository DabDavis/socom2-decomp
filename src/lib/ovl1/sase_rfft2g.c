/* sc: compiler=ee-gcc2.96; flags=-O2 -G8 */
/* Nellymoser's shared real FFT, rFft2G.c (OVL1 0x252338-0x2525C0; "../../shared/rFft2G.c", the
 * file its .rodata names after the two twiddle tables): the split of a complex transform of N
 * points into the half spectrum of 2N real ones (0x252338, the forward pass the FFT interface
 * tail-calls) and its inverse merge (0x252470), Numerical Recipes' realft over split real and
 * imaginary halves, the twiddles (cos, -sin of pi k / N, k = 1 .. N/2 - 1) read from a table per
 * size. Both assert `(N==256) || (N=64)` -- an assignment where a test was meant, so the assert
 * always holds: GCC drops the call (its strings stay in .rodata) and keeps the side effect, the
 * target's `xor; movn a2,64`: any N but 256 runs as 64. GCC round 33a. Placeholders: the codec's
 * names are not in SOCOM 1. Built as nelly.c is (ee-gcc 2.96, -O2 -G8: the encoder's range). */

extern void __assert(const char *file, int line, const char *expr);
#define assert(e) ((e) ? (void)0 : __assert(__FILE__, __LINE__, #e))

/* cos(pi k / 256), -sin(pi k / 256), k = 1 .. 127 (0x3EB8A0) */
static const float afTw256[254] = {
	0x1.fff622p-1F, -0x1.921d22p-7F, 0x1.ffd886p-1F, -0x1.92156p-6F,
	0x1.ffa72cp-1F, -0x1.2d8654p-5F, 0x1.ff621ep-1F, -0x1.91f65cp-5F,
	0x1.ff0954p-1F, -0x1.f656e8p-5F, 0x1.fe9cd8p-1F, -0x1.2d520ap-4F,
	0x1.fe1cbp-1F, -0x1.5f6dp-4F, 0x1.fd88dap-1F, -0x1.917a6ap-4F,
	0x1.fce15ep-1F, -0x1.c3785cp-4F, 0x1.fc2644p-1F, -0x1.f564e6p-4F,
	0x1.fb5796p-1F, -0x1.139f0ep-3F, 0x1.fa7556p-1F, -0x1.2c8106p-3F,
	0x1.f97f92p-1F, -0x1.45576ap-3F, 0x1.f8764ep-1F, -0x1.5e2144p-3F,
	0x1.f75998p-1F, -0x1.76dd9ep-3F, 0x1.f6297ap-1F, -0x1.8f8b82p-3F,
	0x1.f4e604p-1F, -0x1.a82ap-3F, 0x1.f38f3ap-1F, -0x1.c0b826p-3F,
	0x1.f2253p-1F, -0x1.d934fep-3F, 0x1.f0a7eep-1F, -0x1.f19f96p-3F,
	0x1.ef1788p-1F, -0x1.04fb8p-2F, 0x1.ed740cp-1F, -0x1.111d24p-2F,
	0x1.ebbd8ap-1F, -0x1.1d3442p-2F, 0x1.e9f414p-1F, -0x1.294062p-2F,
	0x1.e817bap-1F, -0x1.35410cp-2F, 0x1.e6288ep-1F, -0x1.4135c8p-2F,
	0x1.e426a2p-1F, -0x1.4d1e24p-2F, 0x1.e2121p-1F, -0x1.58f9a6p-2F,
	0x1.dfeae6p-1F, -0x1.64c7dcp-2F, 0x1.ddb13cp-1F, -0x1.708852p-2F,
	0x1.db6526p-1F, -0x1.7c3a92p-2F, 0x1.d906bap-1F, -0x1.87de2ap-2F,
	0x1.d69616p-1F, -0x1.9372a6p-2F, 0x1.d4134cp-1F, -0x1.9ef792p-2F,
	0x1.d17e76p-1F, -0x1.aa6c82p-2F, 0x1.ced7bp-1F, -0x1.b5d0fep-2F,
	0x1.cc1f1p-1F, -0x1.c1249ep-2F, 0x1.c954b2p-1F, -0x1.cc66eap-2F,
	0x1.c678b2p-1F, -0x1.d79776p-2F, 0x1.c38b2cp-1F, -0x1.e2b5d2p-2F,
	0x1.c08c4p-1F, -0x1.edc194p-2F, 0x1.bd7c0ap-1F, -0x1.f8ba4ep-2F,
	0x1.ba5aa4p-1F, -0x1.01cfc6p-1F, 0x1.b72832p-1F, -0x1.07387ap-1F,
	0x1.b3e4d4p-1F, -0x1.0c9704p-1F, 0x1.b090a6p-1F, -0x1.11eb34p-1F,
	0x1.ad2bcap-1F, -0x1.1734d4p-1F, 0x1.a9b662p-1F, -0x1.1c73b2p-1F,
	0x1.a6309p-1F, -0x1.21a798p-1F, 0x1.a29a78p-1F, -0x1.26d054p-1F,
	0x1.9ef44p-1F, -0x1.2bedb2p-1F, 0x1.9b3e04p-1F, -0x1.30ff7ep-1F,
	0x1.9777fp-1F, -0x1.36058cp-1F, 0x1.93a224p-1F, -0x1.3affap-1F,
	0x1.8fbcc8p-1F, -0x1.3fed94p-1F, 0x1.8bc804p-1F, -0x1.44cf3p-1F,
	0x1.87c402p-1F, -0x1.49a448p-1F, 0x1.83b0dep-1F, -0x1.4e6caap-1F,
	0x1.7f8ecep-1F, -0x1.532828p-1F, 0x1.7b5dfp-1F, -0x1.57d694p-1F,
	0x1.771e74p-1F, -0x1.5c77bap-1F, 0x1.72d084p-1F, -0x1.610b76p-1F,
	0x1.6e7446p-1F, -0x1.659192p-1F, 0x1.6a09e6p-1F, -0x1.6a09e6p-1F,
	0x1.659192p-1F, -0x1.6e7446p-1F, 0x1.610b76p-1F, -0x1.72d084p-1F,
	0x1.5c77bap-1F, -0x1.771e74p-1F, 0x1.57d694p-1F, -0x1.7b5dfp-1F,
	0x1.532828p-1F, -0x1.7f8ecep-1F, 0x1.4e6caap-1F, -0x1.83b0dep-1F,
	0x1.49a448p-1F, -0x1.87c402p-1F, 0x1.44cf3p-1F, -0x1.8bc804p-1F,
	0x1.3fed94p-1F, -0x1.8fbcc8p-1F, 0x1.3affap-1F, -0x1.93a224p-1F,
	0x1.36058cp-1F, -0x1.9777fp-1F, 0x1.30ff7ep-1F, -0x1.9b3e04p-1F,
	0x1.2bedb2p-1F, -0x1.9ef44p-1F, 0x1.26d054p-1F, -0x1.a29a78p-1F,
	0x1.21a798p-1F, -0x1.a6309p-1F, 0x1.1c73b2p-1F, -0x1.a9b662p-1F,
	0x1.1734d4p-1F, -0x1.ad2bcap-1F, 0x1.11eb34p-1F, -0x1.b090a6p-1F,
	0x1.0c9704p-1F, -0x1.b3e4d4p-1F, 0x1.07387ap-1F, -0x1.b72832p-1F,
	0x1.01cfc6p-1F, -0x1.ba5aa4p-1F, 0x1.f8ba4ep-2F, -0x1.bd7c0ap-1F,
	0x1.edc194p-2F, -0x1.c08c4p-1F, 0x1.e2b5d2p-2F, -0x1.c38b2cp-1F,
	0x1.d79776p-2F, -0x1.c678b2p-1F, 0x1.cc66eap-2F, -0x1.c954b2p-1F,
	0x1.c1249ep-2F, -0x1.cc1f1p-1F, 0x1.b5d0fep-2F, -0x1.ced7bp-1F,
	0x1.aa6c82p-2F, -0x1.d17e76p-1F, 0x1.9ef792p-2F, -0x1.d4134cp-1F,
	0x1.9372a6p-2F, -0x1.d69616p-1F, 0x1.87de2ap-2F, -0x1.d906bap-1F,
	0x1.7c3a92p-2F, -0x1.db6526p-1F, 0x1.708852p-2F, -0x1.ddb13cp-1F,
	0x1.64c7dcp-2F, -0x1.dfeae6p-1F, 0x1.58f9a6p-2F, -0x1.e2121p-1F,
	0x1.4d1e24p-2F, -0x1.e426a2p-1F, 0x1.4135c8p-2F, -0x1.e6288ep-1F,
	0x1.35410cp-2F, -0x1.e817bap-1F, 0x1.294062p-2F, -0x1.e9f414p-1F,
	0x1.1d3442p-2F, -0x1.ebbd8ap-1F, 0x1.111d24p-2F, -0x1.ed740cp-1F,
	0x1.04fb8p-2F, -0x1.ef1788p-1F, 0x1.f19f96p-3F, -0x1.f0a7eep-1F,
	0x1.d934fep-3F, -0x1.f2253p-1F, 0x1.c0b826p-3F, -0x1.f38f3ap-1F,
	0x1.a82ap-3F, -0x1.f4e604p-1F, 0x1.8f8b82p-3F, -0x1.f6297ap-1F,
	0x1.76dd9ep-3F, -0x1.f75998p-1F, 0x1.5e2144p-3F, -0x1.f8764ep-1F,
	0x1.45576ap-3F, -0x1.f97f92p-1F, 0x1.2c8106p-3F, -0x1.fa7556p-1F,
	0x1.139f0ep-3F, -0x1.fb5796p-1F, 0x1.f564e6p-4F, -0x1.fc2644p-1F,
	0x1.c3785cp-4F, -0x1.fce15ep-1F, 0x1.917a6ap-4F, -0x1.fd88dap-1F,
	0x1.5f6dp-4F, -0x1.fe1cbp-1F, 0x1.2d520ap-4F, -0x1.fe9cd8p-1F,
	0x1.f656e8p-5F, -0x1.ff0954p-1F, 0x1.91f65cp-5F, -0x1.ff621ep-1F,
	0x1.2d8654p-5F, -0x1.ffa72cp-1F, 0x1.92156p-6F, -0x1.ffd886p-1F,
	0x1.921d22p-7F, -0x1.fff622p-1F,
};

/* cos(pi k / 64), -sin(pi k / 64), k = 1 .. 31 (0x3EBC98) */
static const float afTw64[62] = {
	0x1.ff621ep-1F, -0x1.91f65cp-5F, 0x1.fd88dap-1F, -0x1.917a6ap-4F,
	0x1.fa7556p-1F, -0x1.2c8106p-3F, 0x1.f6297ap-1F, -0x1.8f8b82p-3F,
	0x1.f0a7eep-1F, -0x1.f19f96p-3F, 0x1.e9f414p-1F, -0x1.294062p-2F,
	0x1.e2121p-1F, -0x1.58f9a6p-2F, 0x1.d906bap-1F, -0x1.87de2ap-2F,
	0x1.ced7bp-1F, -0x1.b5d0fep-2F, 0x1.c38b2cp-1F, -0x1.e2b5d2p-2F,
	0x1.b72832p-1F, -0x1.07387ap-1F, 0x1.a9b662p-1F, -0x1.1c73b2p-1F,
	0x1.9b3e04p-1F, -0x1.30ff7ep-1F, 0x1.8bc804p-1F, -0x1.44cf3p-1F,
	0x1.7b5dfp-1F, -0x1.57d694p-1F, 0x1.6a09e6p-1F, -0x1.6a09e6p-1F,
	0x1.57d694p-1F, -0x1.7b5dfp-1F, 0x1.44cf3p-1F, -0x1.8bc804p-1F,
	0x1.30ff7ep-1F, -0x1.9b3e04p-1F, 0x1.1c73b2p-1F, -0x1.a9b662p-1F,
	0x1.07387ap-1F, -0x1.b72832p-1F, 0x1.e2b5d2p-2F, -0x1.c38b2cp-1F,
	0x1.b5d0fep-2F, -0x1.ced7bp-1F, 0x1.87de2ap-2F, -0x1.d906bap-1F,
	0x1.58f9a6p-2F, -0x1.e2121p-1F, 0x1.294062p-2F, -0x1.e9f414p-1F,
	0x1.f19f96p-3F, -0x1.f0a7eep-1F, 0x1.8f8b82p-3F, -0x1.f6297ap-1F,
	0x1.2c8106p-3F, -0x1.fa7556p-1F, 0x1.917a6ap-4F, -0x1.fd88dap-1F,
	0x1.91f65cp-5F, -0x1.ff621ep-1F,
};

/* the split: pfRe/pfIm hold the N-point complex transform of the even and odd samples; out come
 * the 2N-point real transform's bins 0 .. N (the Nyquist term in pfRe[N], the imaginary parts at
 * 0 and N cleared), scaled by 1/2, the last imaginary part's sign turned. void, though the FFT
 * interface declares it int to tail-call it: an int function falling off its end keeps v0 live
 * at the exit, so the final index's shift could not take v0, and i -- tied to that shift's v1 --
 * pushed every pointer one register on (34 words). The pointers walk from the first stores, as
 * the target's do (indexed, they are formed apart). */
void func_00252338(float *pfRe, float *pfIm, int N)
{
	const float *pfTw;
	float *pfRe1 = pfRe, *pfIm1 = pfIm, *pfRe2, *pfIm2;
	float fRe1, fIm1, fRe2, fIm2, h1r, h1i, h2r, h2i, wr, wi, tr, ti;
	int i, N2;

	assert((N==256) || (N=64));
	fRe1 = *pfRe1;
	fIm1 = *pfIm1;
	pfRe2 = pfRe + N;
	pfIm2 = pfIm + N;
	*pfRe1++ = fRe1 + fIm1;
	*pfRe2-- = fRe1 - fIm1;
	*pfIm1++ = 0.0F;
	*pfIm2-- = 0.0F;
	pfTw = (N == 256) ? afTw256 : afTw64;
	N2 = N >> 1;
	for (i = 1; i < N2; i++) {
		fRe1 = *pfRe1;
		fIm1 = *pfIm1;
		fRe2 = *pfRe2;
		fIm2 = *pfIm2;
		wr = *pfTw++;
		wi = *pfTw++;
		h1r = (fRe1 + fRe2) * 0.5F;
		h1i = (fIm1 - fIm2) * 0.5F;
		h2r = (fRe1 - fRe2) * 0.5F;
		h2i = (fIm1 + fIm2) * 0.5F;
		tr = h2r * wr - h2i * wi;
		ti = h2i * wr + h2r * wi;
		*pfRe1++ = h1r + ti;
		*pfRe2-- = h1r - ti;
		*pfIm1++ = h1i - tr;
		*pfIm2-- = -(h1i + tr);
	}
	pfIm[i] = -pfIm[i];
}

/* the merge: the inverse of the split (unscaled), back to the N-point complex transform */
void func_00252470(float *pfRe, float *pfIm, int N)
{
	const float *pfTw;
	float *pfRe1 = pfRe, *pfIm1 = pfIm, *pfRe2, *pfIm2;
	float fRe1, fIm1, fRe2, fIm2, h1r, h1i, h2r, h2i, wr, wi, tr, ti;
	int i, N2;

	assert((N==256) || (N=64));
	pfRe2 = pfRe + N;
	pfIm2 = pfIm + N;
	fRe1 = *pfRe1;
	fIm1 = *pfIm1;
	fRe2 = *pfRe2--;
	fIm2 = *pfIm2--;
	*pfRe1++ = (fRe1 + fRe2) - (fIm1 + fIm2);
	*pfIm1++ = (fRe1 - fRe2) + (fIm1 - fIm2);
	pfTw = (N == 256) ? afTw256 : afTw64;
	N2 = N >> 1;
	pfRe[N2] = pfRe[N2] + pfRe[N2];
	pfIm[N2] = pfIm[N2] + pfIm[N2];
	for (i = 1; i < N2; i++) {
		fRe1 = *pfRe1;
		fIm1 = *pfIm1;
		fRe2 = *pfRe2;
		fIm2 = *pfIm2;
		wr = *pfTw++;
		wi = *pfTw++;
		h1r = fRe1 + fRe2;
		h1i = fIm1 - fIm2;
		h2r = fRe1 - fRe2;
		h2i = fIm1 + fIm2;
		tr = h2r * wr + h2i * wi;
		ti = h2i * wr - h2r * wi;
		*pfRe1++ = h1r - ti;
		*pfIm1++ = h1i + tr;
		*pfRe2-- = h1r + ti;
		*pfIm2-- = -(h1i - tr);
	}
}
