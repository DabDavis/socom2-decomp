/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-optimize-sibling-calls */
/* rt_time: the RT library's clock (OVL2 0x63CF20-0x63D648), SCEA's Medius/DME network stack,
 * prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-optimize-sibling-calls (README "The GCC front": no
 * return of a call here is a sibling call -- rt_time_get_base_diff -- and every function of the
 * unit matches so; parts of 0x62C000-0x62EB38 and DME do make them). Names are SOCOM 1's (most
 * s1:exact: the demo's copy is these bytes). A time stamp is seconds and microseconds; the
 * base stamp is taken once, on first use. */

typedef int RT_RESULT;

#define RT_OK               0
#define RT_ALREADY          1
#define RT_INVALID_PARAM    2
#define RT_OVERFLOW         3

typedef struct RT_TIME {
    unsigned int secs;
    unsigned int usecs;
} RT_TIME;

extern unsigned char gbBaseTimeInitialized;
extern RT_TIME data_00676428;           /* the base stamp */

extern RT_RESULT rt_time_start(RT_TIME *start);
extern RT_RESULT rt_time_get_time_stamp(RT_TIME *t);
extern RT_RESULT rt_time_elapsed(RT_TIME *start, unsigned int *msecs);
extern RT_RESULT rt_time_elapsed_timestamp(RT_TIME *start, RT_TIME *diff);
extern RT_RESULT rt_time_get_time_string(char *s);

RT_RESULT rt_time_delay(unsigned int msecs)
{
    RT_TIME start;
    unsigned int elapsed = 0;
    RT_RESULT r;

    r = rt_time_start(&start);
    if (r != RT_OK)
        return r;
    while (elapsed < msecs) {
        r = rt_time_elapsed(&start, &elapsed);
        if (r != RT_OK)
            return r;
    }
    return RT_OK;
}

RT_RESULT rt_time_initialize(void)
{
    RT_RESULT r;

    if (gbBaseTimeInitialized == 1)
        return RT_ALREADY;
    r = rt_time_get_time_stamp(&data_00676428);
    if (r == RT_OK)
        gbBaseTimeInitialized = 1;
    return r;
}

/* end - start; SOCOM 1's returned RT_ALREADY for an end before the start, SOCOM II's gives a zero
 * difference (round 8b) */
RT_RESULT rt_time_calc_diff(RT_TIME *start, RT_TIME *end, RT_TIME *diff)
{
    if (start == 0)
        return RT_INVALID_PARAM;
    if (end == 0)
        return RT_INVALID_PARAM;
    if (diff == 0)
        return RT_INVALID_PARAM;
    if (start->secs == end->secs) {
        if (start->usecs > end->usecs) {
            diff->secs = 0;
            diff->usecs = 0;
            return RT_OK;
        }
        diff->secs = 0;
        diff->usecs = end->usecs - start->usecs;
    } else if (start->secs < end->secs) {
        if (end->usecs < start->usecs) {
            diff->secs = end->secs - start->secs - 1;
            diff->usecs = 1000000 - (start->usecs - end->usecs);
        } else {
            diff->secs = end->secs - start->secs;
            diff->usecs = end->usecs - start->usecs;
        }
    } else {
        diff->secs = 0;
        diff->usecs = 0;
        return RT_OK;
    }
    return RT_OK;
}

RT_RESULT rt_time_timestamp_to_msecs(RT_TIME *t, unsigned int *msecs)
{
    if (t == 0)
        return RT_INVALID_PARAM;
    if (msecs == 0)
        return RT_INVALID_PARAM;
    if (t->secs > 4294965)
        return RT_OVERFLOW;
    *msecs = t->secs * 1000 + t->usecs / 1000;
    return RT_OK;
}

RT_RESULT rt_time_timestamp_to_usecs(RT_TIME *t, unsigned int *usecs)
{
    if (t == 0)
        return RT_INVALID_PARAM;
    if (usecs == 0)
        return RT_INVALID_PARAM;
    if (t->secs >= 4293)
        return RT_OVERFLOW;
    *usecs = t->secs * 1000000 + t->usecs;
    return RT_OK;
}

RT_RESULT rt_time_get_base_diff(RT_TIME *diff)
{
    RT_RESULT r;

    if (diff == 0)
        return RT_INVALID_PARAM;
    if (!gbBaseTimeInitialized) {
        r = rt_time_initialize();
        if (r != RT_OK)
            return r;
    }
    r = rt_time_elapsed_timestamp(&data_00676428, diff);
    return r;
}

char *rt_time_get_time_string_ptr(void)
{
    static char szTime[16];

    if (rt_time_get_time_string(szTime) != RT_OK)
        strcpy(szTime, "00:00:00");
    return szTime;
}
