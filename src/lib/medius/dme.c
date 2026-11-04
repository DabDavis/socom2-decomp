/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* DME's public API, thread-safe (OVL2 0x620908-0x6244B0): SCEA's DME client ("dme version:
 * 1.32.0070"), prebuilt with ee-gcc 2.96 at -O2 -G0 (README "The GCC front"). SOCOM 1's DME has no
 * such layer -- its NetSendMessage is the body -- while SOCOM II's public Net* functions each test
 * that DME is initialised (17) and running thread-safe (43), lock DME's mutex, call the internal
 * function (the body SOCOM 1's same-named function has) with the arguments passed through, and
 * unlock it, the internal call's result returned unless the unlock failed (43). The file is
 * likely Dme.c (dme_client/src/Dme.c's $Header and its build date, 'Oct 9 2003', follow
 * DmeLocal.c's and DmeMessage.c's in .data, and NetGetBuildTimeStamp, 0x620518, which formats
 * that date, sits just before these). Names are the map's: SOCOM 1's where an earlier pass
 * carried them to a wrapper (NetSendMessage, NetGetObject ...), placeholders elsewhere; the
 * internal functions keep theirs. Argument types are the entry's narrowing (an unsigned short is
 * re-extended with `andi`, a char with `sll/sra`); arguments past the eighth come on the stack. */

#define DME_NOT_INITIALIZED 17
#define DME_LOCK_FAILED     43

extern unsigned char data_006511FF;
extern unsigned char data_00651204;
extern char data_00672008[];
#define DmeInitialized  data_006511FF   /* NetInitialize has run */
#define DmeThreadSafe   data_00651204   /* the mutex is made */
#define DmeMutex        data_00672008

extern int rt_mutex_lock(void *mutex, int wait);
extern int rt_mutex_unlock(void *mutex, int wait);

extern int func_006142A8(int, int);
extern int func_006144A0(int, int, int, int, int);
extern int func_00614590(int, int, int);
extern int NetGetBandwidthInfo(int);
extern int func_00623EF8(void);
extern int func_00623F00(int, int, int, int);
extern int func_00623F08(void);
int func_00623F30(int *pnField, int nOffset, int nSize, int nType, int nCount, char *szName);
int func_00624180(int *pnStruct, char *szName, int nSize);
int func_00624460(unsigned int nClass, int nArg1, int nArg2, int nArg3);
extern unsigned char func_00624418(int, int, int);
int func_00624440(int arg0, int arg1);
extern int func_00615558(char, int, int, int, int, int);
extern int func_006158F8(char, int, int, int, int, int, void *);
extern int func_00615CD0(char, int, int, int, int, int, int);
extern int func_00615D88(char, int, int, int, int, int);
int func_00624338(unsigned int *pnClass, int nArg1, int nArg2, int nArg3);
int func_006243C0(unsigned int *pnType, int nArg1);
extern int func_00616C10(int, int);
extern int func_00617610(int);
extern int func_00617690(int, int);
extern int func_00618158(int);
extern int func_006181B8(int, int);
extern int func_00618458(int);
extern int NetSetDefaultResolveAddrParams(int);
extern int func_006184E8(int, int);
extern int func_00618670(int);
extern int func_006186B0(int, int);
extern int func_006189C8(int, int);
extern int func_00618A58(int);
extern int func_00618AD0(int, int);
extern int func_00616DA0(int);
extern int func_006193F8(int, int, int);
extern int func_006195E0(int, int);
extern int func_006199D0(int, int, int, int, int, int, int, int, int, int);
extern int func_006197A0(int, int, int, int, int, int, int, int, int, int, int);
extern int func_00619DD8(int, int, int, int, int, int, int, unsigned short, unsigned short);
extern int func_0061ABD8(int, int, int);
extern int func_0061A078(int);
extern int func_0061A800(int, int);
extern int func_00612AB0(int, int);
extern int func_006148E0(int, int);
extern int NetIpBinaryToString(int, int);
extern int func_00614EC8(int, int);
extern int func_00614F10(int, int);
extern int func_00612750(int, int, int);
extern int func_006126E8(int, int, int);
extern int func_006149E8(int);
extern int func_00614A48(int, int, int);
extern int func_00614E98(int);
extern int func_006146D8(int, int);
extern int func_00614758(int, int);
extern int func_00615028(int);
extern int func_00615068(int);
extern int func_00615090(int, int);
extern int func_006150F8(int, int);
extern int func_00615160(int, int);
extern int func_00615200(int, int);
extern int func_0061B650(int, int);
extern int func_006152B0(int, int);
extern int func_00614290(int, int);
extern int func_00612D80(int);
extern int func_006161D0(int);
extern int NetFreeAllObjects(void);
extern int func_0061A118(int);
extern int func_0061A240(int);
extern int func_0061BEE8(void);
extern int func_0061C0F0(int);
extern int func_0061BEB8(void);
extern int func_00618F70(int);
extern int func_0061C150(int);
extern int func_00643168(int, int);
extern int func_00643220(int, int);
extern int func_006432C8(int, int, int);
extern int func_006433D8(int, int, int);
extern int func_00643530(int, int);
extern int func_006435B8(int, int);
extern int func_00643650(int);

/* DME's build stamp, Dme.c's own __DATE__ and __TIME__ (Oct 9 2003, 18:10:08 -- spelled out, since a
 * rebuild's would be today's); SOCOM 1's function, byte for byte, with its own date. */
#define DME_BUILD_DATE  "Oct  9 2003"
#define DME_BUILD_TIME  "18:10:08"

extern int sprintf(char *buf, const char *fmt, ...);
extern char *strstr(const char *s, const char *find);
extern int atoi(const char *s);

int NetGetBuildTimeStamp(char *szTimeStamp)
{
    char szMonths[] = "JanFebMarAprMayJunJulAugSepOctNovDec\0";
    char szMonth[8];
    char *szDate;
    char *szTime;
    int nMonth;

    if (szTimeStamp == 0)
        return 23;
    szDate = DME_BUILD_DATE;
    szTime = DME_BUILD_TIME;
    sprintf(szMonth, "%.3s", szDate);
    nMonth = (strstr(szMonths, szMonth) - szMonths) / 3 + 1;
    sprintf(szTimeStamp, "Time of build: %.2d.%.2d.%.4s.%.2s.%.2s.%.2s", nMonth, atoi(szDate + 4),
            szDate + 7, szTime, szTime + 3, szTime + 6);
    return 0;
}

/* ---- round 7b: the defaults, the initialise and the close, as Dme.c has them before the locked
 * wrappers. NetInitialize counts its callers (0x651200): the first makes DME's mutex and runs
 * DmeLocal.c's body (0x616820, SOCOM 1's NetInitialize) under it, the others only count; NetClose
 * undoes the count and, at the last, runs the close body (0x61C2F0) and destroys the mutex. ---- */

/* what NetInitialize is given (SOCOM 1's NetSetDefaultInitializeParams filled 5 words of it) */
typedef struct NetInitializeParams {
    int nVersion;                       /* 0x00: 1, else NetInitialize answers 38 */
    int nSoftwareID;                    /* 0x04 */
    int Field08;
    int Field0C;                        /* 0x0C: 3658 by default */
    int Field10;
    void *pfnConnectCallback;           /* 0x14 */
    int Field18;
} NetInitializeParams;                  /* 0x1C */

typedef struct NetInitializeResult {
    int Field00;
    int nError;                         /* 0x04 */
} NetInitializeResult;

extern unsigned int data_00651200;      /* NetInitialize's callers, counted */
extern void *memset(void *p, int c, unsigned int n);
extern int KM_GetSoftwareID(void);
extern int rt_mutex_create(void *mutex);
extern int rt_mutex_destroy(void *mutex);
extern int func_00616820(NetInitializeParams *pParams, NetInitializeResult *pResult);
extern int func_0061C2F0(void);

int NetSetDefaultInitializeParams(NetInitializeParams *pParams)
{
    int nSoftwareID;

    if (pParams == 0)
        return 23;
    memset(pParams, 0, sizeof(*pParams));
    nSoftwareID = KM_GetSoftwareID();
    pParams->nVersion = 1;
    pParams->Field0C = 3658;
    pParams->Field18 = 0;
    pParams->Field08 = 0;
    pParams->pfnConnectCallback = 0;
    pParams->nSoftwareID = nSoftwareID;
    return 0;
}

int NetInitialize(NetInitializeParams *pParams, NetInitializeResult *pResult)
{
    int result;

    if (pResult == 0)
        return 23;
    pResult->nError = 18;
    pResult->Field00 = 1;
    if (pParams == 0) {
        pResult->nError = 23;
        return 23;
    }
    if (pParams->nVersion != 1) {
        pResult->nError = 38;
        return 23;
    }
    if (data_00651200 != 0) {
        data_00651200++;
        pResult->nError = 0;
        return 0;
    }
    if (DmeThreadSafe == 1 || rt_mutex_create(DmeMutex) != 0) {
        pResult->nError = DME_LOCK_FAILED;
        return DME_LOCK_FAILED;
    }
    DmeThreadSafe = 1;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_00616820(pParams, pResult);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    if (result != 0) {
        rt_mutex_destroy(DmeMutex);
        DmeThreadSafe = 0;
        DmeInitialized = 0;
        return result;
    }
    data_00651200 = 1;
    DmeInitialized = 1;
    return 0;
}

int NetClose(void)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (data_00651200 == 0)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (data_00651200 > 1) {
        data_00651200--;
        return 0;
    }
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_0061C2F0();
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    if (result == 0) {
        if (rt_mutex_destroy(DmeMutex) != 0)
            return DME_LOCK_FAILED;
        DmeThreadSafe = 0;
        DmeInitialized = 0;
        data_00651200 = 0;
    }
    return result;
}

int func_00620908(int arg0, int arg1)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_006142A8(arg0, arg1);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_006209B0(int arg0, int arg1, int arg2, int arg3, int arg4)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_006144A0(arg0, arg1, arg2, arg3, arg4);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_00620A88(int arg0, int arg1, int arg2)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_00614590(arg0, arg1, arg2);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_00620B40(int arg0)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = NetGetBandwidthInfo(arg0);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_00620BD8(void)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_00623EF8();
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_00620C60(int arg0, int arg1, int arg2, int arg3)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_00623F00(arg0, arg1, arg2, arg3);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int NetRegisterObjectStart(void)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_00623F08();
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int NetObjectField(int *pnField, int nOffset, int nSize, int nType, int nCount, char *szName)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_00623F30(pnField, nOffset, nSize, nType, nCount, szName);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int NetRegisterStructure(int *pnStruct, char *szName, int nSize)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_00624180(pnStruct, szName, nSize);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_00620F50(int arg0, int arg1, int arg2, int arg3)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_00624460(arg0, arg1, arg2, arg3);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_00621018(int arg0, int arg1, int arg2)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_00624418(arg0, arg1, arg2);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_006210D0(int arg0, int arg1)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_00624440(arg0, arg1);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_00621178(char arg0, int arg1, int arg2, int arg3, int arg4, int arg5)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_00615558(arg0, arg1, arg2, arg3, arg4, arg5);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int NetSendMessage(char msgClass, int connIndex, int toClient, int reliable, int msgType, int len,
                   void *msg)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_006158F8(msgClass, connIndex, toClient, reliable, msgType, len, msg);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_00621360(char arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_00615CD0(arg0, arg1, arg2, arg3, arg4, arg5, arg6);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_00621458(char arg0, int arg1, int arg2, int arg3, int arg4, int arg5)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_00615D88(arg0, arg1, arg2, arg3, arg4, arg5);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_00621548(int arg0, int arg1, int arg2, int arg3)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_00624338(arg0, arg1, arg2, arg3);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_00621610(int arg0, int arg1)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_006243C0(arg0, arg1);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_006216D0(int arg0, int arg1)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_00616C10(arg0, arg1);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int NetSetDefaultConnectParams(int arg0)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_00617610(arg0);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int NetConnect(int arg0, int arg1)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_00617690(arg0, arg1);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_006218B8(int arg0)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_00618158(arg0);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_00621950(int arg0, int arg1)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_006181B8(arg0, arg1);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_006219F8(int arg0)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_00618458(arg0);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_00621A90(int arg0)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = NetSetDefaultResolveAddrParams(arg0);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_00621B28(int arg0, int arg1)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_006184E8(arg0, arg1);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_00621BD0(int arg0)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_00618670(arg0);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_00621C68(int arg0, int arg1)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_006186B0(arg0, arg1);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_00621D10(int arg0, int arg1)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_006189C8(arg0, arg1);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_00621DB8(int arg0)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_00618A58(arg0);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_00621E50(int arg0, int arg1)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_00618AD0(arg0, arg1);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_00621EF8(int arg0)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_00616DA0(arg0);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_00621F90(int arg0, int arg1, int arg2)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_006193F8(arg0, arg1, arg2);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_00622048(int arg0, int arg1)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_006195E0(arg0, arg1);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_006220F0(int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_006199D0(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_00622200(int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_006197A0(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_00622318(int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, unsigned short arg7, unsigned short arg8)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_00619DD8(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_00622428(int arg0, int arg1, int arg2)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_0061ABD8(arg0, arg1, arg2);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_006224E0(int arg0)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_0061A078(arg0);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_00622578(int arg0, int arg1)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_0061A800(arg0, arg1);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int NetSendMyClientUpdate(int arg0, int arg1)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_00612AB0(arg0, arg1);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_006226C8(int arg0, int arg1)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_006148E0(arg0, arg1);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_00622770(int arg0, int arg1)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = NetIpBinaryToString(arg0, arg1);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int NetGetObject(int arg0, int arg1)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_00614EC8(arg0, arg1);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_006228C0(int arg0, int arg1)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_00614F10(arg0, arg1);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int NetGetClientStatus(int arg0, int arg1, int arg2)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_00612750(arg0, arg1, arg2);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int NetGetClient(int arg0, int arg1, int arg2)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_006126E8(arg0, arg1, arg2);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_00622AD8(int arg0)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_006149E8(arg0);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_00622B70(int arg0, int arg1, int arg2)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_00614A48(arg0, arg1, arg2);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_00622C28(int arg0)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_00614E98(arg0);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_00622CC0(int arg0, int arg1)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_006146D8(arg0, arg1);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_00622D68(int arg0, int arg1)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_00614758(arg0, arg1);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int NetGetLocalTime(int arg0)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_00615028(arg0);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int NetGetTime(int arg0)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_00615068(arg0);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_00622F40(int arg0, int arg1)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_00615090(arg0, arg1);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_00622FE8(int arg0, int arg1)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_006150F8(arg0, arg1);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_00623090(int arg0, int arg1)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_00615160(arg0, arg1);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_00623138(int arg0, int arg1)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_00615200(arg0, arg1);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_006231E0(int arg0, int arg1)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_0061B650(arg0, arg1);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_00623288(int arg0, int arg1)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_006152B0(arg0, arg1);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_00623330(int arg0, int arg1)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_00614290(arg0, arg1);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int NetSetMyClientObject(int arg0)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_00612D80(arg0);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int NetFreeObject(int arg0)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_006161D0(arg0);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_00623508(void)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = NetFreeAllObjects();
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_00623590(int arg0)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_0061A118(arg0);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_00623628(int arg0)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_0061A240(arg0);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int NetUpdate(void)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_0061BEE8();
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_00623748(int arg0)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_0061C0F0(arg0);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_006237E0(void)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_0061BEB8();
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_00623868(int arg0)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_00618F70(arg0);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int NetDisconnect(int arg0)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_0061C150(arg0);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_00623998(int arg0, int arg1)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_00643168(arg0, arg1);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_00623A40(int arg0, int arg1)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_00643220(arg0, arg1);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_00623AE8(int arg0, int arg1, int arg2)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_006432C8(arg0, arg1, arg2);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_00623BA0(int arg0, int arg1, int arg2)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_006433D8(arg0, arg1, arg2);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_00623C58(int arg0, int arg1)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_00643530(arg0, arg1);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_00623D00(int arg0, int arg1)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_006435B8(arg0, arg1);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

/* not a wrapper: DME's thread-safety record, filled under the lock (0x10 bytes, its first word the
 * thread-safe flag) */
int func_00623DA8(int *pInfo)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (pInfo == 0)
        return 23;
    result = rt_mutex_lock(DmeMutex, 0);
    if (result != 0)
        return DME_LOCK_FAILED;
    memset(pInfo, 0, 16);
    pInfo[0] = 1;
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int func_00623E60(int arg0)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_00643650(arg0);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

/* ---- round 20b: the object and structure descriptions (0x623EF8-0x624338), SOCOM 1's six-function
 * object NetRegisterMessageStart .. NetRegisterStructure (0x420A08, gcc2_compiled) in its order, grown
 * by guards: a field is 32 bytes here (40 there) and its 16-byte name is copied unaligned; an
 * object's fields together must stay under 475 bytes (31). The first three are matched drafts.
 * Shapes, measured: a pointer initialised at its declaration (`NetField *pField = 0;`) has two sets,
 * so alias analysis finds no base for it and its stores may clobber any global -- the target reloads
 * NetFieldCount after the record's stores (an uninitialised pField, one set from NetFieldList, lets
 * CSE keep the value: 26 words); the count bumped in its own statement after both arrays' stores
 * (`apFields[n++]` computes nFields' address once and loop.c hoists it: 65s); an index zeroed at its
 * declaration is the prologue's `move s3,zero`. ---- */

extern int data_00651180;               /* NetInitialize has run */

typedef struct NetField {               /* SOCOM 1's NetFieldList entry, shrunk */
    int nOffset;                        /* 0x00 */
    int nSize;                          /* 0x04 */
    int nCount;                         /* 0x08 */
    int nType;                          /* 0x0C: a structure's index + 16 from 16 */
    char szName[16];                    /* 0x10 */
} NetField;                             /* 0x20 */

typedef struct NetStructure {
    int nSize;                          /* 0x000 */
    int nFields;                        /* 0x004 */
    char szName[32];                    /* 0x008 */
    NetField *apFields[64];             /* 0x028 */
    int anOffsets[64];                  /* 0x128 */
} NetStructure;                         /* 0x228 */

extern int GlobalObjectFieldCount;      /* 0x651208 */
extern unsigned int NetFieldCount;      /* 0x67C59C */
extern unsigned int NetStructureCount;  /* 0x67C5A0 */
extern int GlobalObjectFieldList[64];   /* 0x672020 */
extern NetField NetFieldList[1024];     /* 0x67E988 */
extern NetStructure NetStructureList[16]; /* 0x67C5C8 */
extern void *memcpy(void *dst, const void *src, unsigned int n);
extern char *strncpy(char *dst, const char *src, unsigned int n);

/* NetObjectField's body (0x623F30): one field of the global object described; the fields' bytes,
 * the new one's with them, summed into the multiply-accumulator (madd from the entry's mtlo) */
int func_00623F30(int *pnField, int nOffset, int nSize, int nType, int nCount, char *szName)
{
    unsigned int nTotal = 0;
    NetField *pField = 0;
    int i;

    if (data_00651180 == 0)
        return 17;
    if (pnField == 0)
        return 23;
    if (nSize <= 0)
        return 23;
    if (nCount <= 0)
        return 23;
    if (GlobalObjectFieldCount >= 64)
        return 8;
    if (NetFieldCount >= 1024)
        return 8;
    for (i = 0; i < GlobalObjectFieldCount; i++)
        nTotal += NetFieldList[NetFieldCount - GlobalObjectFieldCount + i].nSize *
                  NetFieldList[NetFieldCount - GlobalObjectFieldCount + i].nCount;
    nTotal += nSize * nCount;
    if (nTotal >= 475)
        return 31;
    pField = &NetFieldList[NetFieldCount];
    if (pField == 0)
        return 10;
    pField->nOffset = nOffset;
    pField->nSize = nSize;
    pField->nType = nType;
    pField->nCount = nCount;
    if (szName != 0)
        memcpy(pField->szName, szName, 16);
    GlobalObjectFieldList[GlobalObjectFieldCount] = NetFieldCount;
    *pnField = GlobalObjectFieldCount;
    GlobalObjectFieldCount++;
    NetFieldCount++;
    return 0;
}

/* SOCOM 1's RegisterChildStructure (0x624070): a structure's fields flattened into another's, a
 * structure field recursing with its offset added */
void RegisterChildStructure(NetStructure *pStruct, NetStructure *pChild, int nOffset)
{
    NetField *pField = 0;
    int i;

    if (pStruct == 0)
        return;
    if (pChild == 0)
        return;
    for (i = 0; i < pChild->nFields; i++) {
        pField = pChild->apFields[i];
        if (pField->nType >= 16) {
            RegisterChildStructure(pStruct, &NetStructureList[pField->nType - 16], nOffset + pField->nOffset);
        } else {
            pStruct->anOffsets[pStruct->nFields] = nOffset + pField->nOffset;
            pStruct->apFields[pStruct->nFields] = pField;
            pStruct->nFields++;
        }
    }
}

/* NetRegisterStructure's body (0x624180): a structure of the global object's fields so far */
int func_00624180(int *pnStruct, char *szName, int nSize)
{
    NetStructure *pStruct;
    NetField *pField = 0;
    int i = 0;

    if (data_00651180 == 0)
        return 17;
    if (pnStruct == 0)
        return 23;
    if (szName == 0)
        return 23;
    if (nSize <= 0)
        return 23;
    if (NetStructureCount >= 16)
        return 8;
    pStruct = &NetStructureList[NetStructureCount];
    if (pStruct == 0)
        return 10;
    pStruct->nSize = nSize;
    pStruct->nFields = 0;
    strncpy(pStruct->szName, szName, 32);
    for (i = 0; i < GlobalObjectFieldCount; i++) {
        pField = &NetFieldList[GlobalObjectFieldList[i]];
        if (pField == 0)
            continue;
        if (pField->nType >= 16) {
            RegisterChildStructure(pStruct, &NetStructureList[pField->nType - 16], pField->nOffset);
        } else {
            pStruct->anOffsets[pStruct->nFields] = pField->nOffset;
            pStruct->apFields[pStruct->nFields] = pField;
            pStruct->nFields++;
        }
    }
    *pnStruct = NetStructureCount;
    NetStructureCount++;
    return 0;
}

/* ---- round 7b: Dme.c's own registration tables, after the wrappers (NetInitialize must have run:
 * 17): a handler per message class (16 of them, three words each) and the stream types ---- */

extern unsigned int data_0067C5A4;      /* the stream types registered */
extern unsigned int data_0067C5A8;      /* the message classes registered */
extern int data_0067E848[];
extern int data_0067E948[16];
extern int data_00686988[16];
extern int data_006869C8[16];
extern int data_0067A6E8[16];
extern int data_0067A728[16];
extern int data_0067A768[16];

int func_00624338(unsigned int *pnClass, int nArg1, int nArg2, int nArg3)
{
    unsigned int n;

    if (data_00651180 == 0)
        return DME_NOT_INITIALIZED;
    if (data_0067C5A8 >= 16)
        return 22;
    if (pnClass == 0)
        return 23;
    *pnClass = -1;
    n = data_0067C5A8;
    *pnClass = n;
    data_00686988[n] = nArg1;
    data_006869C8[n] = nArg2;
    data_0067E948[n] = nArg3;
    data_0067C5A8++;
    return 0;
}

int func_006243C0(unsigned int *pnType, int nArg1)
{
    unsigned int n;

    if (data_00651180 == 0)
        return DME_NOT_INITIALIZED;
    if (pnType == 0)
        return 23;
    n = data_0067C5A4;
    *pnType = n;
    data_0067E848[n] = nArg1;
    data_0067C5A4++;
    return 0;
}

/* 0x624418 returns a narrower type than this int: its result returned is a jal, no sibling call */
int func_00624440(int arg0, int arg1)
{
    return func_00624418(arg0, 2, arg1);
}

int func_00624460(unsigned int nClass, int nArg1, int nArg2, int nArg3)
{
    if (data_00651180 == 0)
        return DME_NOT_INITIALIZED;
    if (nClass >= 16)
        return 23;
    data_0067A6E8[nClass] = nArg1;      /* ccperm: the argument order (1 of 6) */
    data_0067A728[nClass] = nArg2;
    data_0067A768[nClass] = nArg3;
    return 0;
}

