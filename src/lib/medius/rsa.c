/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* RSA.c: the RT library's RSA over LargeInt.c (OVL2 0x62B548-0x62BCA8), SCEA's rt_crypt, prebuilt
 * with ee-gcc 2.96 at -O2 -G0 (README "The GCC front"). SOCOM 1's RSA object (0x409418:
 * RSAGenerateKeyPair, RSAEncryptBlock, RSADecryptBlock, RSAEncrypt, RSADecrypt) in its order, the
 * sizes agreeing (RSADecryptBlock 0x298 and RSADecrypt 0x94 both builds): the names are SOCOM 1's
 * (s1:order). A key or a block is 64 bytes (512 bits) of a byte record, so every copy is an
 * unaligned block move (`ldl/ldr`, `sdl/sdr` both sides); a block goes into the low half of a
 * 32-word LINT whose high half is cleared. SOCOM 1 built the public exponent 17 on the stack;
 * SOCOM II's encrypting pair each passes a static one (0x655530, 0x655570: one per function).
 *
 * The parameters, read off the wrappers: RSADecryptBlock(modulus, exponent, input, output) --
 * EMOD(result, input, exponent, modulus) is input^exponent mod modulus -- and RSADecrypt walks a
 * buffer of whole blocks, returning the bytes done. Not written: RSAGenerateKeyPair (0x62B548). */

#define LINT_WORDS  32

typedef struct LINT {
    unsigned int d[LINT_WORDS];
} LINT;                                 /* 0x80 */

typedef struct RSA_BLOCK {
    unsigned char b[64];
} RSA_BLOCK;

extern void *memset(void *p, int c, unsigned int n);
extern void *memcpy(void *d, const void *s, unsigned int n);
extern void func_006283F8(LINT *pResult, LINT *pBase, LINT *pExp, LINT *pMod);     /* EMOD */

void RSADecryptBlock(RSA_BLOCK *pMod, RSA_BLOCK *pExp, RSA_BLOCK *pIn, RSA_BLOCK *pOut);
unsigned int RSADecrypt(RSA_BLOCK *pMod, RSA_BLOCK *pExp, unsigned char *pIn, unsigned char *pOut,
                        unsigned int nLength);

void RSAEncryptBlock(RSA_BLOCK *pMod, RSA_BLOCK *pIn, RSA_BLOCK *pOut)
{
    static RSA_BLOCK e = { { 17 } };

    RSADecryptBlock(pMod, &e, pIn, pOut);
}

void RSADecryptBlock(RSA_BLOCK *pMod, RSA_BLOCK *pExp, RSA_BLOCK *pIn, RSA_BLOCK *pOut)
{
    LINT exp;
    LINT mod;
    LINT in;
    LINT result;

    memcpy(&exp, pExp, 64);
    memset((char *)&exp + 64, 0, 64);
    memcpy(&mod, pMod, 64);
    memset((char *)&mod + 64, 0, 64);
    memcpy(&in, pIn, 64);
    memset((char *)&in + 64, 0, 64);
    func_006283F8(&result, &in, &exp, &mod);
    memcpy(pOut, &result, 64);
}

void RSAEncrypt(RSA_BLOCK *pMod, unsigned char *pIn, unsigned char *pOut, unsigned int nLength)
{
    static RSA_BLOCK e = { { 17 } };

    RSADecrypt(pMod, &e, pIn, pOut, nLength);
}

unsigned int RSADecrypt(RSA_BLOCK *pMod, RSA_BLOCK *pExp, unsigned char *pIn, unsigned char *pOut,
                        unsigned int nLength)
{
    unsigned int nDone = 0;

    while (nLength >= 64) {
        RSADecryptBlock(pMod, pExp, (RSA_BLOCK *)pIn, (RSA_BLOCK *)pOut);
        nDone += 64;
        nLength -= 64;
        pIn += 64;
        pOut += 64;
    }
    return nDone;
}
