/* util_crc.cpp -- the CRC-32 of a string (OVL1 0x3A2370-0x3A250F): SOCOM II's, after the memory
 * card's unit and before the code that loads rdr tables by name. Its table is built on first use
 * from the classic bit-reflecting generator (polynomial 0x04C11DB7); the sound system hashes its
 * cue names with it (docs/audio.md: crc32_string, the r0004 0x3C2BC0). SOCOM 1 has neither
 * function: the file's name and the functions' are ours. Defined in reverse of their image order
 * (-inline deferred). */

#include <string.h>

static unsigned int s_crcTable[256];
static bool s_crcTableBuilt;

/* ref's low ch bits reversed (ours: the generator's classic helper, inlined) */
static inline unsigned int Reflect(unsigned int ref, char ch)
{
    unsigned int value = 0;

    for (int i = 1; i < ch + 1; i++) {
        if (ref & 1)
            value |= 1 << (ch - i);
        ref >>= 1;
    }
    return value;
}

/* the table, once: each byte reflected, run through the polynomial eight times, the result
 * reflected back (ours; called out of line: pinned) */
#pragma auto_inline off
static void BuildCRCTable()
{
    if (!s_crcTableBuilt) {
        unsigned int polynomial = 0x04C11DB7;

        for (int i = 0; i <= 0xFF; i++) {
            s_crcTable[i] = Reflect(i, 8) << 24;
            for (int j = 0; j < 8; j++)
                s_crcTable[i] = (s_crcTable[i] << 1) ^ (s_crcTable[i] & (1 << 31) ? polynomial : 0);
            s_crcTable[i] = Reflect(s_crcTable[i], 32);
        }
        s_crcTableBuilt = true;
    }
}
#pragma auto_inline reset

/* the string's CRC-32, or 0 for no string (ours) */
unsigned int CRC32String(const char *text)
{
    unsigned int crc = 0;

    if (text) {
        int len = strlen(text);
        const unsigned char *p = (const unsigned char *)text;

        BuildCRCTable();
        crc = 0xFFFFFFFF;
        while (len--)
            crc = (crc >> 8) ^ s_crcTable[(crc & 0xFF) ^ *p++];
        crc = ~crc;
    }
    return crc;
}
