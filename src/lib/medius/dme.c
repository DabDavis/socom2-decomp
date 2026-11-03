/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* DME's public API, thread-safe (OVL2 0x620908-0x623EF8): SCEA's DME client ("dme version:
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
extern int func_00623F30(int, int, int, int, int, int);
extern int func_00624180(int, int, int);
extern int func_00624460(int, int, int, int);
extern int func_00624418(int, int, int);
extern int func_00624440(int, int);
extern int func_00615558(char, int, int, int, int, int);
extern int func_006158F8(char, int, int, int, int, int, void *);
extern int func_00615CD0(char, int, int, int, int, int, int);
extern int func_00615D88(char, int, int, int, int, int);
extern int func_00624338(int, int, int, int);
extern int func_006243C0(int, int);
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

int NetObjectField(int arg0, int arg1, int arg2, int arg3, int arg4, int arg5)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_00623F30(arg0, arg1, arg2, arg3, arg4, arg5);
    if (rt_mutex_unlock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    return result;
}

int NetRegisterStructure(int arg0, int arg1, int arg2)
{
    int result;

    if (DmeInitialized != 1)
        return DME_NOT_INITIALIZED;
    if (DmeThreadSafe != 1)
        return DME_LOCK_FAILED;
    if (rt_mutex_lock(DmeMutex, 0) != 0)
        return DME_LOCK_FAILED;
    result = func_00624180(arg0, arg1, arg2);
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

int func_00621778(int arg0)
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

int func_00621810(int arg0, int arg1)
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

int func_00622E10(int arg0)
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

int func_00622EA8(int arg0)
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

int func_006236C0(void)
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

int func_00623900(int arg0)
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

