/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* fht: the recogniser's real FFT through the fast Hartley transform (OVL2 0x4E71F8-0x4E7C78,
 * SOCOM 1's object at 0x354FD8: fht, realfft, both global there), ScanSoft's speech recogniser,
 * prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops (README "The GCC
 * front"). Names are SOCOM 1's (realfft called by ProcessFFTMod, fht by realfft, in both builds).
 * The file's name is ours. Written: realfft (round 32c); fht (round 33c: Ron Mayer's FHT as
 * published, its FAST_TRIG rotation, the tables costab/sintab its own .data as SOCOM 1's globals). */

#define REAL float

/* the twiddle factors' tables (SOCOM 1's globals of these names at 0x45E990/0x45E9D0, read by its
 * fht at the same instructions): cos and sin of pi / 2^k */
REAL costab[16] = {
    .00000000000000000000000000000000000000000000000000,
    .70710678118654752440084436210484903928483593768847,
    .92387953251128675612818318939678828682241662586364,
    .98078528040323044912618223613424481383178160000000,
    .99518472667219688624483695310947992157547486872985,
    .99879545620517239271477160475910069444320361470461,
    .99969881869620422011576564966617219685006108125772,
    .99992470183914454092164649119638322435060646880221,
    .99998117528260114265699043772856771617391725094433,
    .99999529380957617151158012570011989955298763362218,
    .99999882345170190992902571017152601904826792288976,
    .99999970586288221916022821773876567711626389934930,
    .99999992646571785114473148070738785694820115568892,
    .99999998161642929380834691540290971450507605124278,
    .99999999540410731289097193313960614895889430318945,
    .99999999885102682756267330779455410840053741619428
};
REAL sintab[16] = {
    1.0000000000000000000000000000000000000000000000000,
    .70710678118654752440084436210484903928483593768846,
    .38268343236508977172845998403039886676134456248561,
    .19509032201612826784828486847702224092769161775195,
    .09801714032956060199419556388864184586113667316749,
    .04906767432741801425495497694268265831474536302574,
    .02454122852291228803173452945928292506546611923944,
    .01227153828571992607940826195100321214037231959176,
    .00613588464915447535964023459037258091705788631738,
    .00306795676296597627014536549091984251894461021344,
    .00153398018628476561230369715026407907995486457522,
    .00076699031874270452693856835794857664314091945205,
    .00038349518757139558907246168118138126339502603495,
    .00019174759731070330743990956198900093346887403385,
    .00009587379909597734587051721097647635118706561284,
    .00004793689960306688454900399049465887274686668768
};

#define SQRT2 ((REAL)(2 * 0.70710678118654752440084436210484))

/* the fast Hartley transform of n points in place (n a power of two): Ron Mayer's fht (1988), its
 * FAST_TRIG rotation -- each section's twiddle turned by costab[k] / sintab[k] from (1, 0) --
 * SOCOM 1's global, written from SOCOM II's code (round 33c) */
void fht(REAL *fz, unsigned int n)
{
    unsigned int i, k, k1, k2, k3, k4, kx;
    REAL *fi, *fn, *gi;
    REAL t_c, t_s;

    for (k1 = 1, k2 = 0; k1 < n; k1++) {
        REAL a;
        for (k = n >> 1; (!((k2 ^= k) & k)); k >>= 1)
            ;
        if (k1 > k2) {
            a = fz[k1];
            fz[k1] = fz[k2];
            fz[k2] = a;
        }
    }
    for (k = 0; (1 << k) < n; k++)
        ;
    k &= 1;
    if (k == 0) {
        for (fi = fz, fn = fz + n; fi < fn; fi += 4) {
            REAL f0, f1, f2, f3;
            f1 = fi[0] - fi[1];
            f0 = fi[0] + fi[1];
            f3 = fi[2] - fi[3];
            f2 = fi[2] + fi[3];
            fi[2] = (f0 - f2);
            fi[0] = (f0 + f2);
            fi[3] = (f1 - f3);
            fi[1] = (f1 + f3);
        }
    } else {
        for (fi = fz, fn = fz + n, gi = fi + 1; fi < fn; fi += 8, gi += 8) {
            REAL bs1, bc1, bs2, bc2, bs3, bc3, bs4, bc4,
                bg0, bf0, bf1, bg1, bf2, bg2, bf3, bg3;
            bc1 = fi[0] - gi[0];
            bs1 = fi[0] + gi[0];
            bc2 = fi[2] - gi[2];
            bs2 = fi[2] + gi[2];
            bc3 = fi[4] - gi[4];
            bs3 = fi[4] + gi[4];
            bc4 = fi[6] - gi[6];
            bs4 = fi[6] + gi[6];
            bf1 = (bs1 - bs2);
            bf0 = (bs1 + bs2);
            bg1 = (bc1 - bc2);
            bg0 = (bc1 + bc2);
            bf3 = (bs3 - bs4);
            bf2 = (bs3 + bs4);
            bg3 = SQRT2 * bc4;
            bg2 = SQRT2 * bc3;
            fi[4] = bf0 - bf2;
            fi[0] = bf0 + bf2;
            fi[6] = bf1 - bf3;
            fi[2] = bf1 + bf3;
            gi[4] = bg0 - bg2;
            gi[0] = bg0 + bg2;
            gi[6] = bg1 - bg3;
            gi[2] = bg1 + bg3;
        }
    }
    if (n < 16)
        return;

    do {
        REAL s1, c1;
        k += 2;
        k1 = 1 << k;
        k2 = k1 << 1;
        k4 = k2 << 1;
        k3 = k2 + k1;
        kx = k1 >> 1;
        fi = fz;
        gi = fi + kx;
        fn = fz + n;
        do {
            REAL g0, f0, f1, g1, f2, g2, f3, g3;
            f1 = fi[0] - fi[k1];
            f0 = fi[0] + fi[k1];
            f3 = fi[k2] - fi[k3];
            f2 = fi[k2] + fi[k3];
            fi[k2] = f0 - f2;
            fi[0] = f0 + f2;
            fi[k3] = f1 - f3;
            fi[k1] = f1 + f3;
            g1 = gi[0] - gi[k1];
            g0 = gi[0] + gi[k1];
            g3 = SQRT2 * gi[k3];
            g2 = SQRT2 * gi[k2];
            gi[k2] = g0 - g2;
            gi[0] = g0 + g2;
            gi[k3] = g1 - g3;
            gi[k1] = g1 + g3;
            gi += k4;
            fi += k4;
        } while (fi < fn);
        t_c = costab[k];
        t_s = sintab[k];
        c1 = 1;
        s1 = 0;
        for (i = 1; i < kx; i++) {
            REAL c2, s2;
            {
                REAL t = c1;
                c1 = t * t_c - s1 * t_s;
                s1 = t * t_s + s1 * t_c;
            }
            c2 = c1 * c1 - s1 * s1;
            s2 = 2 * (c1 * s1);
            fn = fz + n;
            fi = fz + i;
            gi = fz + k1 - i;
            do {
                REAL a, b, g0, f0, f1, g1, f2, g2, f3, g3;
                b = s2 * fi[k1] - c2 * gi[k1];
                a = c2 * fi[k1] + s2 * gi[k1];
                f1 = fi[0] - a;
                f0 = fi[0] + a;
                g1 = gi[0] - b;
                g0 = gi[0] + b;
                b = s2 * fi[k3] - c2 * gi[k3];
                a = c2 * fi[k3] + s2 * gi[k3];
                f3 = fi[k2] - a;
                f2 = fi[k2] + a;
                g3 = gi[k2] - b;
                g2 = gi[k2] + b;
                b = s1 * f2 - c1 * g3;
                a = c1 * f2 + s1 * g3;
                fi[k2] = f0 - a;
                fi[0] = f0 + a;
                gi[k3] = g1 - b;
                gi[k1] = g1 + b;
                b = c1 * g2 - s1 * f3;
                a = s1 * g2 + c1 * f3;
                gi[k2] = g0 - a;
                gi[0] = g0 + a;
                fi[k3] = f1 - b;
                fi[k1] = f1 + b;
                gi += k4;
                fi += k4;
            } while (fi < fn);
        }
    } while (k4 < n);
}

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
