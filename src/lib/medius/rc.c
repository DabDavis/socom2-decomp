/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* The RT library's stream cipher (OVL2 0x62A5A8-0x62A870): SOCOM 1's RC object (0x409150:
 * RCInitialize, RCSaltyInitialize, RCEncrypt, RCDecrypt), SCEA's rt_crypt, prebuilt with ee-gcc
 * 2.96 at -O2 -G0 (README "The GCC front"). The object has no `$Header`, so its file's name is not
 * known; this one keeps SOCOM 1's link order (after LargeInt.c's code, as SOCOM 1's follows its
 * LargeInt object). Names are SOCOM 1's by position and body (s1:order): an RC4 kind -- a 256-byte
 * permutation filled with ~i, keyed with steps of 3 over a 64-byte key (and first, salted, with steps
 * of 5 over the salt's four bytes), each byte enciphered with i stepping by 5 and the plain byte fed
 * back into j. SOCOM II moved the counters ahead of the table and made them words (SOCOM 1's were
 * the bytes at 4 and 5 and its table at 6); rt_crypt.c keeps two states per key (KM_* encrypt and
 * decrypt with them).
 *
 * Shapes, measured: j summed on S[i] itself and the swap through a byte temporary of its own (the
 * target zero-extends the swapped byte with an `andi` -- a promoted QImode copy); the counters
 * cleared i, j, then n (one order of six, tools/ccperm.py), as RCEncrypt stores them; the salt
 * indexed off its own stack slot, `((unsigned char *)&nSalt)[k]` (a pointer local keeps sp in a
 * register). OPEN: RCEncrypt (15 words) and RCDecrypt (36): every instruction in its place but the
 * registers -- the target's temporaries sit one register on from these (the parameters' copies in
 * t4/t6/t7 here t3/t5/t6, &S[i] in a0, the swapped byte in a1, &S[j] in v1); the feedback read
 * before the store (encrypt: S[c] loaded ahead of the pData store, so `j += S[c]` comes first), the
 * in-place `^=`, the standard `S[i] + S[j]` index, declaration orders and a block-scoped swap all
 * leave it so. */

typedef struct RCState {
    unsigned int n;                     /* 0x00: the bytes ciphered */
    unsigned int i;                     /* 0x04 */
    unsigned int j;                     /* 0x08 */
    unsigned char state[256];           /* 0x0C */
} RCState;                              /* 0x10C */

void RCInitialize(RCState *pState, unsigned char *pKey)
{
    unsigned char *S = pState->state;
    unsigned char i = 0;
    unsigned char j;
    unsigned char k;
    unsigned char t;

    do {
        S[i] = ~i;
        i++;
    } while (i != 0);
    j = 0;
    k = 0;
    do {
        j = j + pKey[k] + S[i];
        k = (k + 1) & 0x3F;
        t = S[i];
        S[i] = S[j];
        S[j] = t;
        i += 3;
    } while (i != 0);
    pState->i = 0;
    pState->j = 0;
    pState->n = 0;
}

void RCSaltyInitialize(RCState *pState, unsigned char *pKey, unsigned int nSalt)
{
    unsigned char *S = pState->state;
    unsigned char i = 0;
    unsigned char j;
    unsigned char k;
    unsigned char t;

    do {
        S[i] = ~i;
        i++;
    } while (i != 0);
    j = 0;
    k = 0;
    do {
        j = j + ((unsigned char *)&nSalt)[k] + S[i];
        k = (k + 1) & 3;
        t = S[i];
        S[i] = S[j];
        S[j] = t;
        i += 5;
    } while (i != 0);
    j = 0;
    k = 0;
    do {
        j = j + pKey[k] + S[i];
        k = (k + 1) & 0x3F;
        t = S[i];
        S[i] = S[j];
        S[j] = t;
        i += 3;
    } while (i != 0);
    pState->i = 0;
    pState->j = 0;
    pState->n = 0;
}

void RCEncrypt(RCState *pState, unsigned char *pData, unsigned int nLength)
{
    unsigned char i = pState->i;
    unsigned char j = pState->j;
    unsigned char *S = pState->state;
    unsigned int n;
    unsigned char t;
    unsigned char c;

    for (n = 0; n < nLength; n++) {
        i += 5;
        j += S[i];
        t = S[i];
        S[i] = S[j];
        S[j] = t;
        c = pData[n];
        j += S[c];
        pData[n] = c ^ S[(unsigned char)(S[i] + t)];
    }
    pState->i = i;
    pState->j = j;
    pState->n += nLength;
}

void RCDecrypt(RCState *pState, unsigned char *pData, unsigned int nLength)
{
    unsigned char i = pState->i;
    unsigned char j = pState->j;
    unsigned char *S = pState->state;
    unsigned int n;
    unsigned char t;

    for (n = 0; n < nLength; n++) {
        i += 5;
        j += S[i];
        t = S[i];
        S[i] = S[j];
        S[j] = t;
        pData[n] ^= S[(unsigned char)(S[i] + t)];
        j += S[pData[n]];
    }
    pState->i = i;
    pState->j = j;
    pState->n += nLength;
}
