/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* The Medius client's string and message-id helpers (OVL2 0x64F3D8-0x64F430, SOCOM 1's object at
 * 0x3B1FB0: iTerminateString, iCreateAndAllocateMessageID, iGetMessageIDValues), SCEA's Medius,
 * prebuilt with ee-gcc 2.96 at -O2 -G0 (README "The GCC front"). Names are SOCOM 1's; the id is
 * now the session key and a counter, "%s.%d" (SOCOM 1 packed the type into the counter). */

#define MEDIUS_INVALID_PARAM    (-99)

extern int sprintf(char *s, const char *fmt, ...);
extern char *strncpy(char *dst, const char *src, unsigned int n);
extern int atoi(const char *s);

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

/* SOCOM 1's iGetMessageIDValues (s1:calls, its size): the key before the '.' copied out, the number
 * after it split into a counter (bits 7-15) and a type (bits 0-6) */
int iGetMessageIDValues(char *id, char *key, int *pnCounter, int *pnType)
{
    char *p = id;
    int n = 0;
    unsigned int v;

    while (*p != '.') {
        p++;
        n++;
    }
    if (*p == '.')
        p++;
    strncpy(key, id, n - 1);
    v = atoi(p);
    *pnCounter = (v >> 7) & 0x1FF;
    *pnType = v & 0x7F;
    return 0;
}
