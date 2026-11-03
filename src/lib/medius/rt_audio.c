/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* rt_audio.c: the voice library's RT front (OVL2 0x62BCA8-0x62C1B8), SCEA's ("rt_audio version:
 * 1.08.0009", the .data pointer 0x6555B0 its first function returns; its `$Header` follows
 * RSA.c's in the alphabetical link order), prebuilt with ee-gcc 2.96 at -O2 -G0 (README "The GCC
 * front"), sibling calls on (the forwarders end in `j`). SOCOM 1 has no voice library: every name is
 * a placeholder but mcDelayThread (the map's, s1:calls), and the bodies say what each does.
 *
 * The object: its version, the dotted-quad conversions, the API forwarders into audio.c (0x62BDF0-
 * 0x62C048, matched drafts, not written here), then its own copy of the platform's timing helpers --
 * the CPU's count register (rt_time_platform.c's timer_getcount, the same two instructions), a
 * millisecond clock over it (294912 counts a millisecond: the EE's 294.912 MHz), the alarm callback
 * that wakes the sleeping thread and libmc's delay (SetAlarm, then SleepThread).
 *
 * Shapes, measured: the address parser's digit test is a range on a `char` (2.96's range fold makes
 * it `(unsigned char)(c - '0') < 10`, the target's `andi; sltiu`); the clock's mask constant is
 * hoisted out of the printer's loop, so `(n & 0xFF000000) >> 24` keeps its `and`. */

extern int GetThreadId(void);
extern int SetAlarm(unsigned short time, void (*handler)(int, unsigned short, void *), void *common);
extern void SleepThread(void);
extern int iWakeupThread(int thid);

static char *gpszVersion = "rt_audio version: 1.08.0009";
static unsigned int gnClockMsecs = 1;           /* 0x6555B8 */
static unsigned int gnClockLastCount = 0;       /* 0x6555BC */

char *func_0062BCA8(void)
{
    return gpszVersion;
}

/* "a.b.c.d" to an address, the first part in the high byte */
unsigned int func_0062BCB8(char *szAddress)
{
    unsigned int nAddress = 0;
    unsigned int nPart = 0;
    char c;

    if (szAddress != 0) {
        while ((c = *szAddress++) != 0) {
            if (c == '.') {
                nAddress = (nAddress << 8) + nPart;
                nPart = 0;
            } else if (c >= '0' && c <= '9')
                nPart = nPart * 10 + (c - '0');
        }
        nAddress = (nAddress << 8) + nPart;
    }
    return nAddress;
}

/* an address to "a.b.c.d": a hundreds digit goes on to the tens (one block, which the target
 * enters from both; written twice, 2.96 schedules the copies apart and cross-jumping misses) */
void func_0062BD30(unsigned int nAddress, char *szAddress)
{
    unsigned int i;
    unsigned int n;

    if (szAddress == 0)
        return;
    for (i = 0; i < 4; i++) {
        if (i != 0)
            *szAddress++ = '.';
        n = (nAddress & 0xFF000000) >> 24;
        nAddress <<= 8;
        if (n >= 100) {
            *szAddress++ = n / 100 + '0';
            n %= 100;
            goto tens;
        }
        if (n >= 10) {
tens:
            *szAddress++ = n / 10 + '0';
            n %= 10;
        }
        *szAddress++ = n + '0';
    }
    *szAddress = 0;
}

unsigned int func_0062C060(void)
{
    unsigned int count;

    asm volatile ("mfc0 %0, $9\n\tsync.p" : "=r" (count));
    return count;
}

/* the clock in milliseconds, from 1: the counts since the last whole millisecond carried over
 * (the quotient a variable of its own: computed twice, the constants take each other's registers) */
unsigned int func_0062C070(void)
{
    unsigned int nCount = func_0062C060();
    int nDelta;
    unsigned int n;

    if (gnClockMsecs == 1 && gnClockLastCount == 0)
        gnClockLastCount = nCount;
    nDelta = nCount - gnClockLastCount;
    if (nDelta >= 294912) {
        n = (unsigned int)nDelta / 294912;
        gnClockMsecs += n;
        gnClockLastCount += n * 294912;
    } else if (nDelta < 0) {
        gnClockMsecs += (unsigned int)nDelta / 294912;
        gnClockLastCount = nCount;
    }
    return gnClockMsecs != 0 ? gnClockMsecs : 1;
}

static void func_0062C168(int id, unsigned short time, void *common)
{
    iWakeupThread((int)common);
}

void mcDelayThread(int time)
{
    SetAlarm(time, func_0062C168, (void *)GetThreadId());
    SleepThread();
}
