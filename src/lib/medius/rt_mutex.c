/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* rt_mutex: the RT library's recursive mutex (OVL2 0x6390F0-0x6393E0, SOCOM 1's object at 0x411318:
 * rt_mutex_create .. rt_mutex_unlock), SCEA's Medius/DME network stack, prebuilt with ee-gcc 2.96 at
 * -O2 -G0 (README "The GCC front"). A thread that holds the mutex may lock it again; the count says
 * how deep, and only the last unlock gives the platform's semaphore back. Names are SOCOM 1's
 * (s1:exact: every function here is byte-identical to SOCOM 1's but for relocations).
 *
 * The vendor's shapes, each measured (ccsweep variants): one guard clause per test, each returning
 * its constant (an `||` of them makes 2.96 build a shared return block the target does not have); a
 * lock taken again by its owner and a lock newly taken each count themselves -- the two increments
 * 2.96 cross-jumps into one store, after which rt_mutex_lock reloads the count it hands back and
 * rt_mutex_lock_no_block, which stores it in each branch, does not. */

#include "rt_mutex.h"

int rt_mutex_create(rt_mutex *mutex)
{
    int result;

    if (mutex == 0)
        return RT_MUTEX_ERR_INVALID;
    mutex->bInitialized = 0;
    mutex->nLockCount = 0;
    mutex->nThreadID = 0;
    result = rt_mutex_platform_create(mutex);
    if (result == RT_MUTEX_OK)
        mutex->bInitialized = 1;
    return result;
}

int rt_mutex_destroy(rt_mutex *mutex)
{
    int result;

    if (mutex == 0)
        return RT_MUTEX_ERR_INVALID;
    if (mutex->bInitialized != 1)
        return RT_MUTEX_ERR_INVALID;
    result = rt_mutex_platform_destroy(mutex);
    if (result == RT_MUTEX_OK)
        mutex->bInitialized = 0;
    return result;
}

int rt_mutex_lock(rt_mutex *mutex, int *pLockCount)
{
    int result;
    int nThreadID;

    if (mutex == 0)
        return RT_MUTEX_ERR_INVALID;
    if (mutex->bInitialized != 1)
        return RT_MUTEX_ERR_INVALID;
    result = rt_mutex_platform_get_thread_id(&nThreadID);
    if (result != RT_MUTEX_OK)
        return result;
    if (mutex->nLockCount != 0 && nThreadID == mutex->nThreadID) {
        mutex->nLockCount++;
    } else {
        result = rt_mutex_platform_lock(mutex);
        if (result != RT_MUTEX_OK)
            return result;
        result = rt_mutex_platform_get_thread_id(&mutex->nThreadID);
        if (result != RT_MUTEX_OK)
            return result;
        mutex->nLockCount++;
    }
    if (pLockCount != 0)
        *pLockCount = mutex->nLockCount;
    return RT_MUTEX_OK;
}

int rt_mutex_lock_no_block(rt_mutex *mutex, int *pLockCount)
{
    unsigned char bLocked = 0;
    int nThreadID;
    int result;

    if (mutex == 0)
        return RT_MUTEX_ERR_INVALID;
    if (mutex->bInitialized != 1)
        return RT_MUTEX_ERR_INVALID;
    if (pLockCount == 0)
        return RT_MUTEX_ERR_INVALID;
    *pLockCount = 0;
    result = rt_mutex_platform_get_thread_id(&nThreadID);
    if (result != RT_MUTEX_OK)
        return result;
    if (mutex->nLockCount != 0 && nThreadID == mutex->nThreadID) {
        *pLockCount = ++mutex->nLockCount;
    } else {
        result = rt_mutex_platform_lock_no_block(mutex, &bLocked);
        if (result != RT_MUTEX_OK)
            return result;
        if (bLocked == 1) {
            result = rt_mutex_platform_get_thread_id(&mutex->nThreadID);
            if (result != RT_MUTEX_OK)
                return result;
            *pLockCount = ++mutex->nLockCount;
        }
    }
    return RT_MUTEX_OK;
}

int rt_mutex_unlock(rt_mutex *mutex, int *pLockCount)
{
    int result;
    int nThreadID;

    if (mutex == 0)
        return RT_MUTEX_ERR_INVALID;
    if (mutex->bInitialized != 1)
        return RT_MUTEX_ERR_INVALID;
    result = rt_mutex_platform_get_thread_id(&nThreadID);
    if (result != RT_MUTEX_OK)
        return result;
    if (nThreadID != mutex->nThreadID)
        return RT_MUTEX_ERR_NOT_OWNER;
    if (mutex->nLockCount == 0)
        return RT_MUTEX_ERR_NOT_LOCKED;
    mutex->nLockCount--;
    if (pLockCount != 0)
        *pLockCount = mutex->nLockCount;
    if (mutex->nLockCount == 0) {
        mutex->nThreadID = 0;
        result = rt_mutex_platform_unlock(mutex);
        if (result != RT_MUTEX_OK)
            return result;
    }
    return RT_MUTEX_OK;
}
