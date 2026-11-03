/* rt_mutex.h: the RT library's recursive mutex (rt_mutex.c over rt_mutex_platform.c, a PS2 semaphore),
 * as far as the hand-written units use it. The layout is the code's: rt_mutex.c keeps the owner and
 * the lock count, the platform file the semaphore. Error codes are the values the functions return. */
#ifndef RT_MUTEX_H
#define RT_MUTEX_H

#define RT_MUTEX_OK                 0
#define RT_MUTEX_ERR_PLATFORM       1   /* the kernel refused (CreateSema/WaitSema/... gave -1) */
#define RT_MUTEX_ERR_INVALID        2   /* no mutex, not created, or no out-parameter */
#define RT_MUTEX_ERR_NOT_OWNER      4   /* unlocked by a thread that does not hold it */
#define RT_MUTEX_ERR_NOT_LOCKED     5

typedef struct rt_mutex {
    unsigned char bInitialized;         /* 0x00: 1 once rt_mutex_create succeeded */
    int nLockCount;                     /* 0x04: the owner's recursion depth */
    int nThreadID;                      /* 0x08: the owner */
    int nSemaID;                        /* 0x0C: the platform's semaphore */
    int nPlatformFlags;                 /* 0x10: zeroed by the platform's create */
} rt_mutex;

extern int rt_mutex_platform_create(rt_mutex *mutex);
extern int rt_mutex_platform_destroy(rt_mutex *mutex);
extern int rt_mutex_platform_lock(rt_mutex *mutex);
extern int rt_mutex_platform_lock_no_block(rt_mutex *mutex, unsigned char *pbLocked);
extern int rt_mutex_platform_unlock(rt_mutex *mutex);
extern int rt_mutex_platform_get_thread_id(int *pThreadID);

#endif
