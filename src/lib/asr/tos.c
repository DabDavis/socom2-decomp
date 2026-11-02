/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* tos: the recogniser's block scheduler (OVL2 0x50B2C0-0x50D560, SOCOM 1's object at 0x3759E8):
 * ScanSoft's speech recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing
 * -funroll-loops (README "The GCC front"). Names are SOCOM 1's. A block reports through a log
 * record (the scheduler and its block id); the first error or warning is kept and passed to the
 * host's callback. The loggers return the code as an unsigned char, which is why their callers'
 * `return _tosErrorLog(...)` is no sibling call (2.96 declines one when the modes differ). */

typedef struct TOS TOS;

typedef void (*TOSLOGCB)(TOS *t, int code);

struct TOS {
    char pad0[32];
    TOSLOGCB error_cb;                  /* +32 */
    TOSLOGCB warning_cb;                /* +36 */
    char pad40[12];
    int error;                          /* +52: the first error, block << 16 | code */
    int warning;                        /* +56 */
};

typedef struct TOSLOG {
    TOS *tos;                           /* +0 */
    unsigned char block;                /* +4 */
} TOSLOG;

unsigned char _tosErrorLog(TOSLOG *log, unsigned short code)
{
    TOS *t = log->tos;
    int code32 = log->block << 16 | code;

    if (t->error == 0)
        t->error = code32;
    if (t->error_cb != 0)
        t->error_cb(t, t->error);
    return code;
}

unsigned char _tosWarningLog(TOSLOG *log, unsigned short code)
{
    TOS *t = log->tos;
    int code32 = log->block << 16 | code;

    if (t->warning == 0)
        t->warning = code32;
    if (t->warning_cb != 0)
        t->warning_cb(t, t->warning);
    return code;
}
