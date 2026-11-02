/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* The Medius client's string and message-id helpers (OVL2 0x64F3D8-0x64F430, SOCOM 1's object at
 * 0x3B1FB0: iTerminateString, iCreateAndAllocateMessageID, iGetMessageIDValues), SCEA's Medius,
 * prebuilt with ee-gcc 2.96 at -O2 -G0 (README "The GCC front"). Names are SOCOM 1's; the id is
 * now the session key and a counter, "%s.%d" (SOCOM 1 packed the type into the counter). */

#define MEDIUS_INVALID_PARAM    (-99)

extern int sprintf(char *s, const char *fmt, ...);

int iTerminateString(char *s, int len)
{
    if (s == 0)
        return MEDIUS_INVALID_PARAM;
    if (len > 0)
        s[len - 1] = 0;
    else
        s[0] = 0;
    return 0;
}

int iCreateAndAllocateMessageID(char *key, int counter, int type, char *out)
{
    sprintf(out, "%s.%d", key, counter);
    return 0;
}
