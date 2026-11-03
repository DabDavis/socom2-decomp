/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* rt_time_platform: the RT library's PS2 clock (OVL2 0x63D4E0-0x63DCE8, SOCOM 1's object at 0x4066B8:
 * rt_time_platform_get_time_stamp .. rt_time_alarm_func), SCEA's Medius/DME network stack, prebuilt
 * with ee-gcc 2.96 at -O2 -G0 (README "The GCC front"). The base stamp counts the EE's CPU cycles
 * (COP0 Count, 294,912,000 a second) into seconds and microseconds; a sleep is HSYNC alarms (15,734 a
 * second), at most 0xFFFF at a time. Names are SOCOM 1's (s1:exact/callee).
 *
 * The time of day (round 8b): SOCOM 1's rt_time_platform_get_time_string read the RTC itself the
 * first time; SOCOM II reads it in its own function (0x63D648, unnamed), keeps milliseconds too, and
 * adds a getter of the whole date (0x63D8E8, which reads the RTC again past midnight). The date is
 * counted from the RTC's reading by the msecs clock (rt_time_get_msecs). */

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

/* the date and time read from the RTC (SOCOM 1's gCurTimeDate, the milliseconds new), and the
 * msecs clock when it was read */
typedef struct RT_TIME_DATE {
    unsigned int msecs;                 /* 0x00 */
    unsigned int secs;                  /* 0x04 */
    unsigned int mins;                  /* 0x08 */
    unsigned int hours;                 /* 0x0C */
    unsigned int day;                   /* 0x10 */
    unsigned int month;                 /* 0x14 */
    unsigned int year;                  /* 0x18 */
    unsigned int stamp;                 /* 0x1C */
} RT_TIME_DATE;

/* libcdvd's clock: BCD */
typedef struct sceCdCLOCK {
    unsigned char stat;
    unsigned char second;
    unsigned char minute;
    unsigned char hour;
    unsigned char pad;
    unsigned char day;
    unsigned char month;
    unsigned char year;
} sceCdCLOCK;

#define RT_TIME_CYCLES_PER_SEC  294912000U
#define RT_BCD(x)               (((x) >> 4) * 10 + ((x) & 0xF))

extern unsigned char data_00656020;     /* the base stamp is initialised */
extern unsigned char gbTimeDateSet;     /* gCurTimeDate is read */
extern RT_BASE_TIME gBaseTimeStamp;
extern RT_TIME_DATE gCurTimeDate;

extern int GetThreadId(void);
extern int SetAlarm(unsigned short time, void (*handler)(int, unsigned short, void *), void *common);
extern void SleepThread(void);
extern int iWakeupThread(int thid);
extern int sceCdReadClock(sceCdCLOCK *rtc);
extern void sceScfGetLocalTimefromRTC(sceCdCLOCK *rtc);
extern int rt_time_get_msecs(unsigned int *msecs);
extern int sprintf(char *s, const char *format, ...);

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

/* the RTC read into gCurTimeDate, local time (0x63D648) */
static int func_0063D648(void)
{
    sceCdCLOCK rtc;

    sceCdReadClock(&rtc);
    sceScfGetLocalTimefromRTC(&rtc);
    gCurTimeDate.secs = RT_BCD(rtc.second);
    gCurTimeDate.mins = RT_BCD(rtc.minute);
    gCurTimeDate.hours = RT_BCD(rtc.hour);
    gCurTimeDate.day = RT_BCD(rtc.day);
    gCurTimeDate.month = RT_BCD(rtc.month);
    gCurTimeDate.year = RT_BCD(rtc.year);
    gCurTimeDate.msecs = 0;
    if (rt_time_get_msecs(&gCurTimeDate.stamp) != 0)
        return 1;
    return 0;
}

int rt_time_platform_get_time_string(char *str)
{
    RT_TIME_DATE t;
    unsigned int now;

    if (str == 0)
        return 2;
    if (gbTimeDateSet == 0) {
        if (func_0063D648() != 0)
            return 1;
        gbTimeDateSet = 1;
    }
    t = gCurTimeDate;
    if (rt_time_get_msecs(&now) != 0)
        return 1;
    t.msecs += now - gCurTimeDate.stamp;
    t.secs += t.msecs / 1000;
    t.msecs %= 1000;
    t.mins += t.secs / 60;
    t.secs %= 60;
    t.hours += t.mins / 60;
    t.mins %= 60;
    t.hours %= 24;
    sprintf(str, "%02d:%02d:%02d", t.hours, t.mins, t.secs);
    return 0;
}

/* the date and time now (0x63D8E8; func_0063D4C8 in rt_time forwards to it) */
int func_0063D8E8(RT_TIME_DATE *out)
{
    RT_TIME_DATE t;
    unsigned int now;

    if (out == 0)
        return 2;
    if (gbTimeDateSet == 0) {
        if (func_0063D648() != 0)
            return 1;
        gbTimeDateSet = 1;
    }
    t = gCurTimeDate;
    if (rt_time_get_msecs(&now) != 0)
        return 1;
    t.msecs += now - gCurTimeDate.stamp;
    t.secs += t.msecs / 1000;
    t.msecs %= 1000;
    t.mins += t.secs / 60;
    t.secs %= 60;
    t.hours += t.mins / 60;
    t.mins %= 60;
    if (t.hours >= 24) {
        if (func_0063D648() != 0)
            return 1;
        t = gCurTimeDate;
    }
    *out = t;
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
