/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* md5.c: RSA Data Security's MD5 (RFC 1321's MD5Init, MD5Update, MD5Final; OVL2 0x629B38-0x629DA0),
 * as SCEA's rt_crypt keeps it, prebuilt with ee-gcc 2.96 at -O2 -G0 (README "The GCC front"); the
 * update's last copy is a sibling call. The object has no `$Header`: it links after LargeInt.c
 * and before the voice library's peer lists. SCEA counts the input in BYTES (a 64-bit count, the
 * bits made only in MD5Final), and the transform and Encode are md5_platform.c's (rt_crypt/src/ps2,
 * whose $Header heads the data they sit in: 0x6641D0, 0x664F80 -- R5900 assembly linked into OVL2's
 * data section, medius/md5_platform.s since round 23b), the transform taking the context and reading
 * its own buffer. PADDING is RFC 1321's. */

typedef unsigned int UINT4;

typedef struct {
    UINT4 state[4];                     /* state (ABCD) */
    UINT4 count[2];                     /* number of bytes, modulo 2^64 (lsb first) */
    unsigned char buffer[64];           /* input buffer */
} MD5_CTX;

extern void *memcpy(void *d, const void *s, unsigned int n);
extern void *memset(void *p, int c, unsigned int n);

/* md5_platform.c's */
extern void func_006641D0(MD5_CTX *context);
extern void func_00664F80(unsigned char *output, UINT4 *input, unsigned int len);

static unsigned char PADDING[64] = {
    0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* MD5 initialization. Begins an MD5 operation, writing a new context. */
void func_00629B38(MD5_CTX *context)
{
    context->count[0] = context->count[1] = 0;
    /* Load magic initialization constants. */
    context->state[0] = 0x67452301;
    context->state[1] = 0xefcdab89;
    context->state[2] = 0x98badcfe;
    context->state[3] = 0x10325476;
}

/* MD5 block update operation. Continues an MD5 message-digest operation, processing another
 * message block, and updating the context. */
void func_00629B78(MD5_CTX *context, unsigned char *input, unsigned int inputLen)
{
    unsigned int index, partLen;

    /* Compute number of bytes mod 64 */
    index = context->count[0] & 0x3F;

    /* Update number of bytes */
    if ((context->count[0] += inputLen) < inputLen)
        context->count[1]++;

    partLen = 64 - index;

    /* Transform as many times as possible. */
    if (inputLen >= partLen) {
        memcpy(&context->buffer[index], input, partLen);
        index = 0;
        func_006641D0(context);
        inputLen -= partLen;
        input += partLen;
        while (inputLen >= 64) {
            memcpy(context->buffer, input, 64);
            inputLen -= 64;
            input += 64;
            func_006641D0(context);
        }
    }

    /* Buffer remaining input */
    memcpy(&context->buffer[index], input, inputLen);
}

/* MD5 finalization. Ends an MD5 message-digest operation, writing the the message digest and
 * zeroizing the context. */
void func_00629CD8(unsigned char digest[16], MD5_CTX *context)
{
    unsigned char bits[8];
    UINT4 count[2];
    unsigned int index, padLen;

    count[0] = context->count[0];
    count[1] = context->count[1];

    /* Pad out to 56 mod 64. */
    index = count[0] & 0x3f;
    padLen = (index < 56) ? (56 - index) : (120 - index);
    func_00629B78(context, PADDING, padLen);

    /* Append length (in bits, before padding) */
    count[1] = (count[1] << 3) + (count[0] >> 29);
    count[0] <<= 3;
    func_00664F80(bits, count, 8);
    func_00629B78(context, bits, 8);

    /* Store state in digest */
    func_00664F80(digest, context->state, 16);

    /* Zeroize sensitive information. */
    memset(context, 0, sizeof(*context));
}
