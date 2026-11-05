/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* vad: the recogniser's voice-activity block (OVL2 0x4F9DC0-0x4FBA38, SOCOM 1's object at
 * 0x366410: ChangeMike, TestChangeMike, CheckForAbnormalConditions, VadGetSession, VadPutSession,
 * VadFastTrackProcess, ProcessVad, ControlVad, InitVad, ConstructVad -- all but the constructor
 * LOCAL there, the same order here),
 * ScanSoft's speech recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing
 * -funroll-loops (README "The GCC front"). Names are SOCOM 1's. The constructor reads two
 * profile entries through a log record of its own: 45, the inputs, and 37, a flag that adds a
 * second output and that the block keeps. Written so far: ControlVad and ProcessVad (OPEN) (round 32c), the session pair (round 15c: the
 * state copied field by field, the arrays by loops 2.96 unrolls by ten, the two ring positions
 * kept as unsigned indices), ChangeMike, TestChangeMike and CheckForAbnormalConditions
 * (round 19c; ChangeMike closed in 20c) and the constructor; the rest are declared, not yet defined. */
#include "tosblk.h"

typedef struct TOSLOG {
    TOS *tos;
    unsigned char id;
} TOSLOG;

typedef struct VAD {
    TOSBLOCK b;
    unsigned char f40;                  /* +40 */
    char pad41[3];
    float f44[6];                       /* +44 */
    int pad68;
    unsigned short f72[4];              /* +72 */
    float f80;                          /* +80 */
    float hist[20];                     /* +84 */
    int pad164;
    float *histpos;                     /* +168: in hist */
    short f172[150];                    /* +172 */
    char pad472[600];
    int f1072;                          /* +1072 */
    short f1076[40];                    /* +1076 */
    int pad1156;
    short *pos1160;                     /* +1160: in f1076 */
    unsigned char second;               /* +1164: profile 37 */
    char pad1165[7];
    float noise;                        /* +1172 */
    char pad1176[12];
    float f1188;                        /* +1188: the first output's value */
    float floor;                        /* +1192 */
    unsigned char speech;               /* +1196 */
    char pad1197[3];
    float floor2;                       /* +1200 */
    unsigned short count;               /* +1204 */
    unsigned short lastcount;           /* +1206 */
    unsigned short level;               /* +1208 */
    unsigned short reflevel;            /* +1210 */
    unsigned char hold;                 /* +1212 */
    char pad1213;
    unsigned short holdframes;          /* +1214 */
    unsigned short changed;             /* +1216 */
    unsigned short range;               /* +1218 */
    float factor;                       /* +1220 */
    unsigned char mode;                 /* +1224: command 3's second word */
    unsigned char reported;             /* +1225 */
    char pad1226[2];
    int state;                          /* +1228 */
    int time;                           /* +1232 */
    unsigned short frames;              /* +1236 */
    unsigned short lastframes;          /* +1238 */
    unsigned char check;                /* +1240: abnormal conditions checked */
    char pad1241[3];
    unsigned int timeout;               /* +1244 */
    int pad1248;
    float mean;                         /* +1252: the inputs' mean */
} VAD;                                  /* 1256 */

/* a session: the block's state field by field, its two ring positions as indices */
typedef struct VADSESSION {
    unsigned char f40;                  /* +0 */
    char pad1[3];
    float f44[6];                       /* +4 */
    unsigned short f72[4];              /* +28 */
    float f80;                          /* +36 */
    float hist[20];                     /* +40 */
    unsigned short histidx;             /* +120 */
    short f172[150];                    /* +122 */
    int f1072;                          /* +424 */
    short f1076[40];                    /* +428 */
    unsigned short idx1160;             /* +508 */
} VADSESSION;

extern int abs(int);
extern long labs(long);

extern int _tosControl(void *log, unsigned char cmd, int arg, int n);

/* a hold of 50 frames with the level set aside (command 11; an aside of 0 starts none): the level
 * when it is already set aside, else 0. ChangeMike and CheckForAbnormalConditions both inline it
 * (the `aside != 0` test folds away for the latter's constant). Ours (an inline, no copy in the image): the constant argument is copied into a
 * register at the call, before the hold test (integrate.c), as the target's early `li 0xbfff`; its
 * zero results reach the caller's test as values (the target's `move v1,zero`, then `bnez v1`) --
 * a constant 0 known at the test is threaded to the exit */
static __inline__ unsigned short StartHold(VAD *v, unsigned short aside)
{
    if (v->hold != 0)
        return 0;
    if (aside != 0 && v->level != aside) {
        v->hold = 1;
        v->holdframes = 50;
        v->level = 0;
        _tosControl(v, 11, aside, 0);
        return 0;
    }
    return v->level;
}

/* a microphone change: the level the energy asks for (the energy's distance from the floor, when
 * more than 0.2, moves it by a fraction of itself, within 0xFFFF and the range around the last
 * change), a hold started at it (0: none -- state 1), then, when nothing changed and the level
 * moved more than a hundredth for 11 frames, the change kept and command 12's report 5 when the
 * level is at an end and the energy beyond the floor. SOCOM 1's body (0x2D0; SOCOM II's 0x2D8),
 * its three shapes the target's (round 20c, from round 19c's 4-byte miss):
 * - the starting level an inline with a return per arm (CurrentLevel, ours): each return sets the
 *   inline's HImode result in its own block, so the conversion at the join is no combine's to drop
 *   -- the target's `andi s3,v0,0xffff` -- and the hold flag, loaded there as a QImode pseudo, is
 *   zero-extended again where StartHold tests it (`andi v0,s5,0xff`); a result variable assigned
 *   in one arm lets combine see both values and fold the masks away;
 * - the clamp's upper bound an if/else (`if (lo < hi) n = lo; else n = hi;`): if-conversion stores
 *   lo and moves hi in with a movz, hi in a register of its own; `lo < hi ? lo : hi` computes hi
 *   into n's register and moves lo in;
 * - the test of the change an if around the rest with the clear in the else: the else block is laid
 *   out of line, its store goes into the `bnezl`'s slot and the branch to the common `return 0`;
 *   a guard clause `if (v->changed) { v->count = 0; return 0; }` falls through into the clear.
 * Then n in s2 and the distance in s0 come out as the target's. The doubles: 0.2f, and two values
 * that are no float's (the midpoints above 2.7631018f and 1.3815509f: 25 significant bits, two
 * fifths and a fifth of the same 6.90775484 -- folded double arithmetic, as CheckForAbnormalConditions'
 * 5.7564623653888703 is five sixths of it), spelled as their 17-digit decimals (dconst.py). */
static __inline__ unsigned short CurrentLevel(VAD *v)
{
    if (v->hold != 0)
        return 0;
    return v->level;
}

static int ChangeMike(VAD *v)
{
    float e = v->f44[0] + v->f44[3] + v->f44[4] - v->floor2;
    unsigned short old = CurrentLevel(v);
    int n;
    double d;

    if (old == 0)
        return 0;
    v->reflevel = old;
    v->state = 0;
    n = old;
    d = e;
    if (d < 0.0)
        d = 0.0 - d;
    if (d > 0.2f) {
        n = n * (1.0f - v->factor * 0.05f * e);
        if (n > 0xFFFF)
            n = 0xFFFF;
        if (v->changed != 0) {
            int lo = v->changed - v->range;
            int hi = v->changed + v->range;

            if (lo < n)
                lo = n;
            if (lo < hi)
                n = lo;
            else
                n = hi;
        }
        n = StartHold(v, n);
        if (n == 0) {
            v->state = 1;
            return 0;
        }
    }
    if (v->changed == 0) {
        if ((int)(old * 0.01f) < labs(n - old))
            v->count = 0;
        if (v->count >= 11) {
            e = v->f44[0] + v->f44[3] + v->f44[4];
            v->changed = n;
            v->count = 0;
            if (((unsigned short)n == 0xFFFF && e < (double)v->floor2 - 2.7631019353866578)
                || ((unsigned short)n < 6555 && e > (double)v->floor2 + 1.3815509676933289)) {
                if ((unsigned char)_tosControl(v, 12, 5, v->time))
                    return 1;
            }
        }
    } else
        v->count = 0;
    return 0;
}

/* the microphone change test, once a frame: a hold counted down, then -- past the frames the test
 * needs and when the count moved -- either a change tried (ChangeMike: 1 when it was made) or, in
 * state 1, the level compared with the reference within a hundredth of it; the count kept for the
 * next frame. SOCOM 1's body, its float conversion inline. The frames needed are a variable of
 * their own (`li 2; movz` -- in the comparison fold splits the conditional into two tests), and
 * the distance is abs()'s: GCC's builtin expands it as a branch around the negation, the target's
 * bgez, where `if (d < 0) d = -d` is if-converted to a movn */
static int TestChangeMike(VAD *v)
{
    int lim;

    if (v->hold) {
        if (--v->holdframes == 0) {
            v->hold = 0;
            v->state = 0;
            v->count = 0;
        }
    }
    lim = v->changed ? 2 : 1;
    if (v->count >= lim && v->count != v->lastcount) {
        if (v->state == 0) {
            if (ChangeMike(v))
                return 1;
        } else if (v->level != 0 && v->state == 1) {
            v->state = 0;
            if (v->changed == 0) {
                int d = abs(v->level - v->reflevel);

                if ((unsigned short)(v->reflevel * 0.01f) < d)
                    v->count = 0;
                if (v->count >= 11) {
                    v->changed = v->level;
                    v->count = 0;
                }
            } else
                v->count = 0;
        }
    }
    v->lastcount = v->count;
    return 0;
}

/* the abnormal conditions, reported through the scheduler's command 12 with their kind: 0 the noise
 * too high, 1 the energy above the floor, 2 below it, 3 nothing heard in time -- the first three
 * once per four frames, the last starting a hold (the level set aside as 0xBFFF, command 11).
 * SOCOM 1's body, with SOCOM II's constants: the floats 0.69077546f and -6.9077545f (SOCOM 1's an
 * ulp and two above), the double 5.7564623653888703 (build/scratch/gcc/r19c/dconst.py: 2.96's
 * parser on it gives the target's 0x4017069E12000000). The energy report and the floor report are
 * an if/else (a made report skips the second test: the target's branch to the final speech test),
 * the scheduler's result masked at each test (`andi` into v1: an int result tested as an unsigned
 * char), the floor's difference subtracted in place (`e -= v->floor`: one FPR) */
static int CheckForAbnormalConditions(VAD *v, float energy)
{
    if (v->speech == 0 || v->changed != 0) {
        float e;

        if (v->noise < 5.7564623653888703 && v->frames - v->lastframes >= 4) {
            if ((unsigned char)_tosControl(v, 12, 0, v->time))
                return 1;
            v->lastframes = v->frames;
        }
        if (energy - v->floor > 0.69077546f && v->lastframes != v->frames) {
            v->lastframes = v->frames;
            if ((unsigned char)_tosControl(v, 12, 1, v->time))
                return 1;
        } else {
            e = v->f44[0];
            if (v->speech)
                e += v->f44[3] + v->f44[4];
            e -= v->floor;
            if (e < -6.9077545f && v->frames - v->lastframes >= 4) {
                if ((unsigned char)_tosControl(v, 12, 2, v->time))
                    return 1;
                v->lastframes = v->frames;
            }
        }
        if (v->speech == 0)
            return 0;
    }
    if (v->timeout == 0 || v->timeout >= v->time || v->frames != 0)
        return 0;
    if ((unsigned char)_tosControl(v, 12, 3, v->time))
        return 1;
    if (v->reported == 0 && v->speech != 0) {
        v->reported = 1;
        if (StartHold(v, 0xBFFF))
            return 1;
    }
    v->time = 0;
    return 0;
}

static void VadGetSession(VAD *v, VADSESSION *ss)
{
    int i;

    ss->f40 = v->f40;
    ss->f44[0] = v->f44[0];
    ss->f44[1] = v->f44[1];
    ss->f44[2] = v->f44[2];
    ss->f44[3] = v->f44[3];
    ss->f44[4] = v->f44[4];
    ss->f44[5] = v->f44[5];
    ss->f72[0] = v->f72[0];
    ss->f72[1] = v->f72[1];
    ss->f72[2] = v->f72[2];
    ss->f72[3] = v->f72[3];
    ss->f80 = v->f80;
    for (i = 0; i < 20; i++)
        ss->hist[i] = v->hist[i];
    ss->histidx = v->histpos - v->hist;
    for (i = 0; i < 150; i++)
        ss->f172[i] = v->f172[i];
    ss->f1072 = v->f1072;
    for (i = 0; i < 40; i++)
        ss->f1076[i] = v->f1076[i];
    ss->idx1160 = v->pos1160 - v->f1076;
}

static void VadPutSession(VAD *v, VADSESSION *ss)
{
    int i;

    v->f40 = ss->f40;
    v->f44[0] = ss->f44[0];
    v->f44[1] = ss->f44[1];
    v->f44[2] = ss->f44[2];
    v->f44[3] = ss->f44[3];
    v->f44[4] = ss->f44[4];
    v->f44[5] = ss->f44[5];
    v->f72[0] = ss->f72[0];
    v->f72[1] = ss->f72[1];
    v->f72[2] = ss->f72[2];
    v->f72[3] = ss->f72[3];
    v->f80 = ss->f80;
    for (i = 0; i < 20; i++)
        v->hist[i] = ss->hist[i];
    v->histpos = &v->hist[ss->histidx];
    for (i = 0; i < 150; i++)
        v->f172[i] = ss->f172[i];
    v->f1072 = ss->f1072;
    for (i = 0; i < 40; i++)
        v->f1076[i] = ss->f1076[i];
    v->pos1160 = &v->f1076[ss->idx1160];
}

extern void VadFastTrackProcess(VAD *v, float e);

/* a frame (the block's process): the single input's value, or with several the mean of those
 * present (none present, nothing); the frame timed and fast-tracked, the first output given the
 * value at +1188, the second (when kept) the frame's value and the five levels, and the abnormal
 * conditions checked when enabled unless a microphone change is pending. SOCOM 1's LOCAL, written
 * from SOCOM II's code (round 32c). The value's pointer lives in the frame, stored and reloaded at
 * every use (SOCOM 1's too): volatile here gives exactly that. OPEN (round 32c, the right size, 2
 * words): the target zeroes sum ($f2) and copies it into cnt ($f0), cnt taking $f0 for the loop;
 * written so, sum's extra read (the copy) ranks it first (6,410 against 6,315) and every add's
 * registers swap (17 words); zeroing cnt first gives the loop's registers and reverses the copy.
 * Chained, both zeroed, x's store moved, the adds or the declarations swapped, `cnt++`, the
 * quotient through cnt or sum: no closer. */
static void ProcessVad(VAD *v, float **in, int n)
{
    float *volatile x;
    int i = 0;
    float sum, cnt;
    float *q;

    if (v->b.ninputs != 1) {
        cnt = 0.0f;
        x = 0;
        sum = cnt;
        for (i = 0; i < n && in[i] == 0; i++)
            ;
        if (i == n)
            goto out;
        for (i = 0; i < n; i++) {
            if (in[i] != 0) {
                cnt += 1.0f;
                sum += *in[i];
            }
        }
        v->mean = sum / cnt;
        x = &v->mean;
    } else
        x = in[0];
out:
    if (x != 0) {
        v->time++;
        VadFastTrackProcess(v, *x);
        *(float *)qEnQueueOne(v->b.outputs[0].q) = v->f1188;
        if (v->second) {
            q = qEnQueueOne(v->b.outputs[1].q);
            *q++ = *x;
            *q++ = v->f44[0];
            *q++ = v->f44[1];
            *q++ = v->f44[2];
            *q++ = v->f44[4];
            *q = v->f44[3];
        }
        if ((v->speech == 0 || TestChangeMike(v) == 0) && v->check != 0)
            CheckForAbnormalConditions(v, *x);
    }
}

/* the EE's largest float (no infinity): 0x7FFFFFFF through 2.96's parser (gender.c's) */
#define FLT_MAX 6.80564694e+38f

/* commands 3 and 4's three words */
typedef struct VADPARAM {
    unsigned int speech;
    unsigned int mode;
    unsigned int timeout;               /* in hundredths */
} VADPARAM;

/* the speech flag set (command 3's first word): when it changes, on resets the level and the hold
 * and sends the scheduler command 10, the threshold moved down by the two margins (and the frame
 * counts cleared), off moves it back up. 1 when the scheduler refuses the command */
static __inline__ unsigned char SetSpeech(VAD *v, unsigned char on)
{
    if (v->speech == on)
        return 0;
    v->speech = on;
    if (on != 0) {
        v->level = 0;
        v->hold = 0;
        if (_tosControl(v, 10, 0, 0) != 0)
            return 1;
        v->frames = 0;
        v->lastframes = 0;
        v->changed = 0;
        v->f44[0] -= v->f44[3] + v->f44[4];
    } else
        v->f44[0] += v->f44[3] + v->f44[4];
    return 0;
}

/* the block's commands: 1 the noise in hundredths of a dB, 8 its address, 2 a level set (the hold
 * cleared), 5 a reset (no argument) or the session put back, 6 the session taken (each 512 bytes
 * at *off), 3 the speech flag, the mode (its range, the counts cleared) and the timeout set, 4 the
 * three read back, 7 a frame counted, 0xFF the block destroyed; 1 when handled. SOCOM 1's LOCAL,
 * written from SOCOM II's code (round 32c); the bodies in the image's order. Case 5's put first
 * (the target branches to the reset); the reset's stores in the order changed, frames, lastframes,
 * histpos (sched1 gives the target's changed, histpos, frames, lastframes); the range an if/else
 * (a ?: loads the true arm's constant before the test, into a register of its own); SetSpeech's
 * equal case returned first (written as the outer if's condition, the flag's register is v1, the
 * result's, and the first branch's slot is annulled). */
static int ControlVad(VAD *v, unsigned char cmd, void *arg, int *off)
{
    VADPARAM *p = arg;
    unsigned int i;

    switch (cmd) {
    case 1:
        *(short *)arg = v->noise * 4.3429448f * 100.0;
        break;
    case 8:
        *(float **)arg = &v->noise;
        break;
    case 2:
        v->level = (int)arg;
        v->hold = 0;
        break;
    case 5:
        if (arg != 0) {
            VadPutSession(v, (VADSESSION *)((char *)arg + *off));
            *off += 512;
        } else {
            v->f40 = 0;
            v->f72[2] = _tosGetProfileU32(v, 6, 2);
            for (i = 0; i < 20; i++)
                v->hist[i] = FLT_MAX;
            v->changed = 0;
            v->frames = 0;
            v->lastframes = 0;
            v->histpos = v->hist;
        }
        break;
    case 6:
        if (arg != 0)
            VadGetSession(v, (VADSESSION *)((char *)arg + *off));
        *off += 512;
        break;
    case 3:
        if (SetSpeech(v, p->speech))
            return 0;
        if (v->mode != (unsigned char)p->mode) {
            v->mode = p->mode;
            if (p->mode != 0)
                v->range = 0x3333;
            else
                v->range = 0x1EB8;
            v->frames = 0;
            v->lastframes = 0;
            v->changed = 0;
        }
        v->timeout = p->timeout * 100;
        break;
    case 4:
        p->speech = v->speech;
        p->mode = v->mode;
        p->timeout = v->timeout * 0.01f;
        break;
    case 7:
        v->time = 0;
        v->frames++;
        break;
    case 0xFF:
        tosBaseBlockDestruct(&v->b);
        break;
    default:
        return 0;
    }
    return 1;
}


extern int InitVad(VAD *v);

TOSBLOCK *ConstructVad(TOS *tos, unsigned char id)
{
    TOSLOG log;
    unsigned char second;
    VAD *v;

    log.tos = tos;
    log.id = id;
    second = _tosGetProfileU32(&log, 37, 0);
    v = (VAD *)tosBaseBlockConstruct(tos, id, _tosGetProfileU32(&log, 45, 1), second ? 2 : 1,
                                     (TOSFUNC)ProcessVad, (TOSFUNC)InitVad, (TOSFUNC)ControlVad,
                                     sizeof(VAD));
    v->second = second;
    return &v->b;
}
