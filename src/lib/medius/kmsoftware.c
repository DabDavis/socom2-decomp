/* sc: compiler=ee-gcc2.96; flags=-O1 -G0 */
/* The title's software identity for the RT key master (OVL2 0x64F5F8-0x64F71C): SOCOM 1's object at
 * 0x420FC0 (KM_GetSoftwareID, KM_GetSoftwareKeyPair), the title's glue to SCEA's rt_crypt. The object
 * has no `$Header`; its file's name is not known (this one says what it holds). The id is read from a
 * word in .data (SOCOM 1's from small data) -- 10472 here; the key pair is two function-local
 * statics, SOCOM 1's `pubKey.0` and `priKey.1` (data.txt), copied out whole. DME's
 * NetSetDefaultInitializeParams asks for both. Round 8b.
 *
 * Built WITHOUT instruction scheduling, unlike the libraries around it: both functions are byte-exact
 * under ee-gcc 2.96 -O1 -G0 (and -O2 with -fno-schedule-insns -fno-schedule-insns2), and at -O2 miss
 * by 2 and 67 words on every ee-gcc toolchain (the id's address split through v1, the second key's
 * address hoisted above the first copy) -- the title's own build of the glue, not SCEA's. */

typedef struct RSAKey {
    unsigned int n[16];
} RSAKey;                               /* 0x40 */

extern int data_00656340;               /* the software id, 10472 */

int KM_GetSoftwareID(void)
{
    return data_00656340;
}

int KM_GetSoftwareKeyPair(RSAKey *pPublicKey, RSAKey *pPrivateKey)
{
    static RSAKey pubKey = { {
        0x19CDC3C5, 0xFC897A31, 0x10D4382D, 0x542BB311, 0x977D16A4, 0xEB642C8A, 0xF8617579, 0x5464535E,
        0xB3DC6EB2, 0xC99B1C28, 0x19619706, 0xCCA076A4, 0x4FAC70B1, 0x06C48694, 0xE7B3B8D4, 0xCC9C9DBA
    } };
    static RSAKey priKey = { {
        0xCAD4BD71, 0x0ED75880, 0x72175F5A, 0x7043B7E7, 0xF6D50427, 0x788F17F3, 0x4BDD7CFC, 0x1B2D06FC,
        0x8C65A403, 0x1F703C15, 0x0D6FE68B, 0x6C54F384, 0x484C3BA9, 0xE5771A12, 0xA7D79E15, 0x6C52EA17
    } };

    *pPublicKey = pubKey;
    *pPrivateKey = priKey;
    return 0;
}
