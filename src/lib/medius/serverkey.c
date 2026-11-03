/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* The title's Medius server key (OVL2 0x64F4E0, before the fifteen 8-byte stubs at 0x64F580 and
 * kmsoftware.c), SCEA's glue for the title, prebuilt with ee-gcc 2.96 at -O2 -G0 (README "The GCC
 * front"). SOCOM 1 has no such object: the game's lobby copies the 64-byte key into its connection
 * parameters' server key (zNetMediusUniverse.cpp) when it connects securely, else zeroes it. The
 * unit's name is ours (no `$Header`, no demo counterpart).
 * The key is a byte record: the copy is the block move of a 1-aligned type (ldl/ldr and sdl/sdr on
 * both sides, as the target's), not memcpy of an array (DATA_ALIGNMENT word-aligns that); the copy
 * sits inside the positive test, the failure's own `jr ra` last (a guard clause if-converts it).
 * OPEN (37 words, registers only): the block move's source address takes v0 (high) and a3 and its
 * scratches v1, a1, a2 in the target, v1/t0 and a1-a3 here; memcpy, a void * or char * parameter
 * and the array's own copy all give these. */

typedef struct SERVER_KEY {
    unsigned char abKey[64];
} SERVER_KEY;

static SERVER_KEY gServerKey = {
    {
        0x55, 0x01, 0x86, 0x14, 0x98, 0xE0, 0x6D, 0xBD,
        0x11, 0x0B, 0xB3, 0x24, 0xD1, 0xD1, 0x34, 0x54,
        0x73, 0x6E, 0x57, 0xAE, 0x7F, 0xB8, 0xAB, 0x3E,
        0x7E, 0x9E, 0x45, 0xEF, 0xA0, 0x54, 0x80, 0x00,
        0x02, 0x28, 0x9D, 0x56, 0x4E, 0x26, 0x2E, 0x85,
        0x41, 0x4F, 0x16, 0x9C, 0xDB, 0x11, 0x32, 0xFC,
        0x9B, 0xED, 0x85, 0xFF, 0x91, 0x9F, 0x68, 0x25,
        0x23, 0x5D, 0x83, 0xEC, 0x16, 0x57, 0xF7, 0xC4
    }
};

int func_0064F4E0(SERVER_KEY *pKey)
{
    if (pKey != 0) {
        *pKey = gServerKey;
        return 0;
    }
    return -1;
}
