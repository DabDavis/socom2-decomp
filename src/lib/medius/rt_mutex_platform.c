/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* rt_mutex_platform: the RT mutex's PS2 half (OVL2 0x6393E0-0x639568, SOCOM 1's object at 0x4115C8:
 * rt_mutex_platform_create .. rt_mutex_platform_get_thread_id), a kernel semaphore of one; SCEA's
 * Medius/DME network stack, prebuilt with ee-gcc 2.96 at -O2 -G0 (README "The GCC front"). Names are
 * SOCOM 1's (s1:exact). Each kernel call's -1 is the platform error, 1. */

#include "rt_mutex.h"

struct SemaParam {
    int currentCount;
    int maxCount;
    int initCount;
    int numWaitThreads;
    unsigned int attr;
    unsigned int option;
};

extern int CreateSema(struct SemaParam *param);
extern int DeleteSema(int semaID);
extern int WaitSema(int semaID);
extern int PollSema(int semaID);
extern int SignalSema(int semaID);
extern int GetThreadId(void);

int rt_mutex_platform_create(rt_mutex *mutex)
{
    struct SemaParam param;
    int semaID;

    if (mutex == 0)
        return RT_MUTEX_ERR_INVALID;
    mutex->nPlatformFlags = 0;
    param.maxCount = 1;
    param.initCount = 1;
    semaID = CreateSema(&param);
    if (semaID == -1)
        return RT_MUTEX_ERR_PLATFORM;
    mutex->nSemaID = semaID;
    return RT_MUTEX_OK;
}

int rt_mutex_platform_destroy(rt_mutex *mutex)
{
    if (mutex == 0)
        return RT_MUTEX_ERR_INVALID;
    return DeleteSema(mutex->nSemaID) == -1;
}

int rt_mutex_platform_lock(rt_mutex *mutex)
{
    if (mutex == 0)
        return RT_MUTEX_ERR_INVALID;
    return WaitSema(mutex->nSemaID) == -1;
}

int rt_mutex_platform_lock_no_block(rt_mutex *mutex, unsigned char *pbLocked)
{
    if (mutex == 0)
        return RT_MUTEX_ERR_INVALID;
    if (mutex->bInitialized != 1)
        return RT_MUTEX_ERR_INVALID;
    if (pbLocked == 0)
        return RT_MUTEX_ERR_INVALID;
    *pbLocked = 0;
    if (PollSema(mutex->nSemaID) != -1)
        *pbLocked = 1;
    return RT_MUTEX_OK;
}

int rt_mutex_platform_unlock(rt_mutex *mutex)
{
    if (mutex == 0)
        return RT_MUTEX_ERR_INVALID;
    return SignalSema(mutex->nSemaID) == -1;
}

int rt_mutex_platform_get_thread_id(int *pThreadID)
{
    if (pThreadID == 0)
        return RT_MUTEX_ERR_INVALID;
    *pThreadID = GetThreadId();
    return RT_MUTEX_OK;
}
