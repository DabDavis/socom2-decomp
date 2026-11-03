/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* rt_time_platform: the RT library's PS2 clock (OVL2 0x63D4E0-0x63DCE8, SOCOM 1's object at 0x4066B8:
 * rt_time_platform_get_time_stamp .. rt_time_alarm_func), SCEA's Medius/DME network stack, prebuilt
 * with ee-gcc 2.96 at -O2 -G0 (README "The GCC front"). The base stamp counts the EE's CPU cycles
 * (COP0 Count, 294,912,000 a second) into seconds and microseconds; a sleep is HSYNC alarms (15,734 a
 * second), at most 0xFFFF at a time. Names are SOCOM 1's (s1:exact/callee); the time-string functions
 * between (0x63D648 .. 0x63D8E8) are not written yet. */

typedef struct RT_TIME {
    unsigned int secs;
    unsigned int usecs;
} RT_TIME;

typedef struct RT_BASE_TIME {
    unsigned int secs;                  /* 0x00 */
    unsigned int cycles;                /* 0x04: within the second */
    unsigned int usecs;                 /* 0x08 */
    unsigned int lastCount;             /* 0x0C: COP0 Count at the last update */
} RT_BASE_TIME;

#define RT_TIME_CYCLES_PER_SEC  294912000U

extern unsigned char data_00656020;     /* the base stamp is initialised */
extern RT_BASE_TIME gBaseTimeStamp;

extern int GetThreadId(void);
extern int SetAlarm(unsigned short time, void (*handler)(int, unsigned short, void *), void *common);
extern void SleepThread(void);
extern int iWakeupThread(int thid);

static int rt_time_platform_update_base(void);
static int rt_time_platform_init(void);
static unsigned int rt_time_platform_timer_getcount(void);
static void rt_time_alarm_func(int id, unsigned short time, void *common);

int rt_time_platform_get_time_stamp(RT_TIME *t)
{
    if (t == 0)
        return 2;
    if (rt_time_platform_update_base() != 0)
        return 1;
    t->secs = gBaseTimeStamp.secs;
    t->usecs = gBaseTimeStamp.usecs;
    return 0;
}

int rt_time_sleep_usecs_platform(unsigned int usecs)
{
    float fTicks = (float)usecs * 0.015734f;
    unsigned short nTicks;

    while (fTicks > 0.0) {
        if (fTicks > 65535.0f) {
            nTicks = 0xFFFF;
            fTicks -= 65535.0f;
        } else {
            nTicks = (unsigned int)fTicks;
            fTicks = 0;
        }
        SetAlarm(nTicks, rt_time_alarm_func, (void *)GetThreadId());
        SleepThread();
    }
    return 0;
}

static unsigned int rt_time_platform_timer_getcount(void)
{
    unsigned int count;

    asm volatile ("mfc0 %0, $9\n\tsync.p" : "=r" (count));
    return count;
}

static int rt_time_platform_init(void)
{
    if (data_00656020 == 0) {
        gBaseTimeStamp.secs = 0;
        gBaseTimeStamp.cycles = 0;
        gBaseTimeStamp.usecs = 0;
        gBaseTimeStamp.lastCount = rt_time_platform_timer_getcount();
        data_00656020 = 1;
    }
    return 0;
}

static int rt_time_platform_update_base(void)
{
    unsigned int count;
    unsigned int delta;

    if (data_00656020 == 0) {
        if (rt_time_platform_init() != 0)
            return 1;
    }
    count = rt_time_platform_timer_getcount();
    delta = count - gBaseTimeStamp.lastCount;
    gBaseTimeStamp.lastCount = count;
    if (delta != 0) {
        gBaseTimeStamp.cycles += delta;
        while (gBaseTimeStamp.cycles > RT_TIME_CYCLES_PER_SEC) {
            gBaseTimeStamp.secs++;
            gBaseTimeStamp.cycles -= RT_TIME_CYCLES_PER_SEC;
        }
        gBaseTimeStamp.usecs = (unsigned int)((float)gBaseTimeStamp.cycles * 0.003390842f);
    }
    return 0;
}

static void rt_time_alarm_func(int id, unsigned short time, void *common)
{
    iWakeupThread((int)common);
}
