/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* rt_msg_client: the RT library's message client (OVL2 0x6325B8-0x638880, SOCOM 1's object at
 * 0x40C000; rt_msg/src/rt_msg_client/rt_msg_client.c 1.209, "rt_msg_client version: 1.08.0204"),
 * SCEA's Medius/DME network stack, prebuilt with ee-gcc 2.96 at -O2 -G0 (README "The GCC front").
 * Its public functions are locked wrappers over the internal ones (rt_msg_client_i*): the client
 * started up, the client's mutex locked, the internal call, the mutex unlocked, the call's result
 * returned unless the unlock failed (26). The started test compares the flag with the result
 * variable's first value -- the target keeps that 1 in the register the result later takes
 * (s0); a literal gives a temporary (ccsweep: 1 of 7 shapes, SOCOM 1's copies the same).
 * Names are SOCOM 1's (s1:exact); argument types are the entry's narrowing. Only the functions
 * written so far are here, in image order.
 * Round 12b: the object starts at 0x6325B8 (below); the startup, shutdown, ishutdown, iupdate_all
 * and create_send_buf_space written from SOCOM 1's bodies, byte-exact (a positive test around the
 * shutdown's mutex teardown and the space's refill; the table cleared before its count, by store
 * order). OPEN: rt_msg_client_iget_metrics (37s: its copy -- thirteen field assignments, floats
 * through FPRs, tStart a struct member -- is exact; the call's `return result` is a block of its
 * own in the target, a fall-through `b`, as rt_udp's length tests -- an empty asm statement there
 * gives 1 word, the shape rt_udp.c's RT_UDP_BARRIER stands in for), rt_msg_client_iudp_disable_
 * heartbeat (9: SOCOM II's UDP poll under SOCOM 1's name, whose body it is not; the flag's reload
 * takes v1 where the target's takes v0). */

#define RT_MSG_CLIENT_LOCK_FAILED   26

extern unsigned char gbMutexInitialized;
extern char gMsgClientMutex[];

extern int rt_mutex_lock(void *mutex, int wait);
extern int rt_mutex_unlock(void *mutex, int wait);
extern void *memset(void *p, int c, unsigned int n);

extern int rt_msg_client_udp_shutdown(void);
extern int func_00633960();
extern int func_00633A08(int, int);
extern int rt_msg_client_idisconnect(unsigned short);
extern int rt_msg_client_idisconnect_all(void);
extern int func_00634078(unsigned short, unsigned char);
extern int rt_msg_client_iudp_disable_heartbeat(void);
extern int rt_msg_client_iupdate_all(unsigned char);
extern int rt_msg_client_isend_target(unsigned short, unsigned short, int, int, unsigned short);
extern int rt_msg_client_isend_client_set(unsigned short, unsigned short, unsigned short, int, int, int, unsigned short);
extern int func_00634278(unsigned char *, char *, unsigned short);
extern int rt_msg_client_iget_buffer_status(unsigned short, int, int, int, int);
extern int func_00634508(int);
extern int func_00634550(int);
extern int rt_msg_client_iget_host_by_name(char *, char *);
extern int rt_msg_client_iget_connection_ip(unsigned short, char *);
extern int func_00634660(unsigned short, unsigned char);
struct rt_msg_client_metrics;
extern int rt_msg_client_iget_metrics(int, struct rt_msg_client_metrics *);
extern int func_006347C8(unsigned short, int);
extern int func_00634850(unsigned short, int);
/* a connection's options (rt_msg_client_connect's): SOCOM 1's 64-byte block grown to 76, its
 * fields from 0x18 on eight bytes further in, the server's port (0x18) and a first word new */
typedef struct rt_msg_client_connect_options {
    int field_00;
    char pad04[0x14];
    unsigned short nPort;               /* 0x18: 10075 by default */
    char pad1A[6];
    int field_20;                       /* 0x20: 15000 (SOCOM 1's 0x18) */
    int field_24;                       /* 0x24: 30 (SOCOM 1's 0x1C) */
    int field_28;                       /* 0x28: 0x8000 (SOCOM 1's 0x20) */
    int field_2C;                       /* 0x2C: 0x8000 (SOCOM 1's 0x24) */
    int field_30;                       /* 0x30 (SOCOM 1's 0x28) */
    unsigned short field_34;            /* 0x34 (SOCOM 1's 0x2C) */
    char pad36[6];
    int field_3C;                       /* 0x3C (SOCOM 1's 0x34) */
    int field_40;                       /* 0x40 */
    int field_44;                       /* 0x44 */
    int field_48;
} rt_msg_client_connect_options;       /* 0x4C */

/* rt_msg_client_enable_udp's options: one word, the callback the enable-UDP internal (0x633960)
 * keeps (SOCOM 1's were a 28-byte block of ports and timeouts over rt_rel_udp) */
typedef struct rt_msg_client_udp_options {
    void *pfnCallback;
} rt_msg_client_udp_options;

int rt_msg_client_iset_default_connect_options(rt_msg_client_connect_options *pOptions);
int rt_msg_client_iudp_set_default_options(rt_msg_client_udp_options *pOptions);
extern int rt_msg_client_idisplay_comm_state(void);
extern int func_006349A8(unsigned short, int **);

/* The object starts at 0x6325B8, not at the build stamp: 0x6325B8 keeps its state in this
 * object's .data (0x655630, after gbMutexInitialized and the 0x87654321 word; rt_memory.c's
 * callbacks end at 0x65560C) and is rt_udp.c's first function again (0x63DC90) with key-manager
 * id 0 for rt_udp's -1; 0x632610 returns the version pointer at the head of that .data -- SOCOM
 * 1's object opens with rt_msg_client_get_version_string_ptr, the same three words. */
extern int func_0062F818(int nKMID, void *pKey);

static char *gpszVersion = "rt_msg_client version: 1.08.0204";
static int gnKeyState = 0;              /* 0x655630: 1 the client's key pair is set, 2 it is not */

int func_006325B8(void)
{
    if (gnKeyState == 0) {
        if (func_0062F818(0, 0) == 0)
            gnKeyState = 1;
        else
            gnKeyState = 2;
    }
    return gnKeyState;
}

char *rt_msg_client_get_version_string_ptr(void)
{
    return gpszVersion;
}

/* The client's build stamp, rt_msg_client.c's own __DATE__ and __TIME__ (Oct 3 2003, 15:51:32 --
 * spelled out, since a rebuild's would be today's), formatted as Medius's and DME's are and copied
 * out up to the length the caller gives. */
#define RT_MSG_CLIENT_BUILD_DATE    "Oct  3 2003"
#define RT_MSG_CLIENT_BUILD_TIME    "15:51:32"

extern int sprintf(char *buf, const char *fmt, ...);
extern char *strstr(const char *s, const char *find);
extern char *strncpy(char *dst, const char *src, unsigned int n);
extern int atoi(const char *s);

int rt_msg_client_get_build_timestamp(char *szTimeStamp, short nLength)
{
    char szMonths[] = "JanFebMarAprMayJunJulAugSepOctNovDec\0";
    char szMonth[8];
    char szBuf[32];
    char *szDate;
    char *szTime;
    int nMonth;

    if (szTimeStamp == 0)
        return 2;
    szDate = RT_MSG_CLIENT_BUILD_DATE;
    szTime = RT_MSG_CLIENT_BUILD_TIME;
    sprintf(szMonth, "%.3s", szDate);
    nMonth = (strstr(szMonths, szMonth) - szMonths) / 3 + 1;
    sprintf(szBuf, "%.2d.%.2d.%.4s.%.2s.%.2s.%.2s", nMonth, atoi(szDate + 4), szDate + 7, szTime,
            szTime + 3, szTime + 6);
    strncpy(szTimeStamp, szBuf, nLength);
    return 0;
}

extern int rt_mutex_create(void *mutex);
extern int rt_mutex_destroy(void *mutex);
extern int rt_comm_startup(void);
extern void KM_Initialize(int nKMID, void *pKeyPair);
extern unsigned char rt_msg_client_map_comm_error(int nCommError);
extern unsigned char rt_msg_client_ishutdown(void);
extern unsigned char data_00655614;     /* the client is started (SCEA's gbInitialized) */
extern short data_00655616;             /* its startup count (SCEA's gnInitCount) */
extern unsigned short gnConnectionArraySize;
extern struct rt_msg_client_connection **gapConnections;
/* the UDP side: one rt_comm channel of the third kind, its callback (SOCOM 1's gUDPInfo, smaller) */
typedef struct rt_msg_client_udp_info {
    unsigned char bEnabled;             /* 0x00 */
    int nChannel;                       /* 0x04 */
    void *pfnCallback;                  /* 0x08 */
} rt_msg_client_udp_info;

extern rt_msg_client_udp_info gUDPInfo;

/* SOCOM 1's startup: the first call creates the client's mutex, starts rt_comm, clears the
 * connection table and the UDP side and sets the key master's pair; every call counts */
int rt_msg_client_startup(void *pKeyPair)
{
    int result;

    if (data_00655616 == 0) {
        if (rt_mutex_create(gMsgClientMutex) != 0)
            return RT_MSG_CLIENT_LOCK_FAILED;
        gbMutexInitialized = 1;
        result = rt_msg_client_map_comm_error(rt_comm_startup());
        if (result != 0)
            return result;
        gapConnections = 0;
        gnConnectionArraySize = 0;
        memset(&gUDPInfo, 0, 12);
        KM_Initialize(0, pKeyPair);
        data_00655614 = 1;
    }
    data_00655616++;
    return 0;
}

int rt_msg_client_shutdown(void)
{
    int result = 1;

    if (gbMutexInitialized != result)
        return RT_MSG_CLIENT_LOCK_FAILED;
    if (rt_mutex_lock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    result = rt_msg_client_ishutdown();
    if (rt_mutex_unlock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    if (result == 0 && data_00655616 == 0) {
        if (rt_mutex_destroy(gMsgClientMutex) != 0)
            return RT_MSG_CLIENT_LOCK_FAILED;
        gbMutexInitialized = 0;
    }
    return result;
}

int func_006328E0(void)
{
    int result = 1;

    if (gbMutexInitialized != result)
        return RT_MSG_CLIENT_LOCK_FAILED;
    if (rt_mutex_lock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    result = rt_msg_client_udp_shutdown();
    if (rt_mutex_unlock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    return result;
}

int rt_msg_client_enable_udp(int arg0)
{
    int result = 1;

    if (gbMutexInitialized != result)
        return RT_MSG_CLIENT_LOCK_FAILED;
    if (rt_mutex_lock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    result = func_00633960(arg0);
    if (rt_mutex_unlock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    return result;
}

int rt_msg_client_connect(int arg0, int arg1)
{
    int result = 1;

    if (gbMutexInitialized != result)
        return RT_MSG_CLIENT_LOCK_FAILED;
    if (rt_mutex_lock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    result = func_00633A08(arg0, arg1);
    if (rt_mutex_unlock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    return result;
}

int rt_msg_client_disconnect(unsigned short arg0)
{
    int result = 1;

    if (gbMutexInitialized != result)
        return RT_MSG_CLIENT_LOCK_FAILED;
    if (rt_mutex_lock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    result = rt_msg_client_idisconnect(arg0);
    if (rt_mutex_unlock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    return result;
}

int rt_msg_client_disconnect_all(void)
{
    int result = 1;

    if (gbMutexInitialized != result)
        return RT_MSG_CLIENT_LOCK_FAILED;
    if (rt_mutex_lock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    result = rt_msg_client_idisconnect_all();
    if (rt_mutex_unlock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    return result;
}

int rt_msg_client_update(unsigned short arg0, unsigned char arg1)
{
    int result = 1;

    if (gbMutexInitialized != result)
        return RT_MSG_CLIENT_LOCK_FAILED;
    if (rt_mutex_lock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    result = func_00634078(arg0, arg1);
    if (rt_mutex_unlock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    return result;
}

int rt_msg_client_udp_disable_heartbeat(void)
{
    int result = 1;

    if (gbMutexInitialized != result)
        return RT_MSG_CLIENT_LOCK_FAILED;
    if (rt_mutex_lock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    result = rt_msg_client_iudp_disable_heartbeat();
    if (rt_mutex_unlock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    return result;
}

int rt_msg_client_update_all(unsigned char arg0)
{
    int result = 1;

    if (gbMutexInitialized != result)
        return RT_MSG_CLIENT_LOCK_FAILED;
    if (rt_mutex_lock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    result = rt_msg_client_iupdate_all(arg0);
    if (rt_mutex_unlock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    return result;
}

int rt_msg_client_send_target(unsigned short arg0, unsigned short arg1, int arg2, int arg3, unsigned short arg4)
{
    int result = 1;

    if (gbMutexInitialized != result)
        return RT_MSG_CLIENT_LOCK_FAILED;
    if (rt_mutex_lock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    result = rt_msg_client_isend_target(arg0, arg1, arg2, arg3, arg4);
    if (rt_mutex_unlock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    return result;
}

int rt_msg_client_send_client_set(unsigned short arg0, unsigned short arg1, unsigned short arg2, int arg3, int arg4, int arg5, unsigned short arg6)
{
    int result = 1;

    if (gbMutexInitialized != result)
        return RT_MSG_CLIENT_LOCK_FAILED;
    if (rt_mutex_lock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    result = rt_msg_client_isend_client_set(arg0, arg1, arg2, arg3, arg4, arg5, arg6);
    if (rt_mutex_unlock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    return result;
}

int rt_msg_client_send_udp(int arg0, int arg1, unsigned short arg2)
{
    int result = 1;

    if (gbMutexInitialized != result)
        return RT_MSG_CLIENT_LOCK_FAILED;
    if (rt_mutex_lock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    result = func_00634278(arg0, arg1, arg2);
    if (rt_mutex_unlock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    return result;
}

int rt_msg_client_get_buffer_status(unsigned short arg0, int arg1, int arg2, int arg3, int arg4)
{
    int result = 1;

    if (gbMutexInitialized != result)
        return RT_MSG_CLIENT_LOCK_FAILED;
    if (rt_mutex_lock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    result = rt_msg_client_iget_buffer_status(arg0, arg1, arg2, arg3, arg4);
    if (rt_mutex_unlock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    return result;
}

int func_00633078(int arg0)
{
    int result = 1;

    if (gbMutexInitialized != result)
        return RT_MSG_CLIENT_LOCK_FAILED;
    if (rt_mutex_lock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    result = func_00634508(arg0);
    if (rt_mutex_unlock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    return result;
}

int func_00633108(int arg0)
{
    int result = 1;

    if (gbMutexInitialized != result)
        return RT_MSG_CLIENT_LOCK_FAILED;
    if (rt_mutex_lock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    result = func_00634550(arg0);
    if (rt_mutex_unlock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    return result;
}

int rt_msg_client_get_host_by_name(int arg0, int arg1)
{
    int result = 1;

    if (gbMutexInitialized != result)
        return RT_MSG_CLIENT_LOCK_FAILED;
    if (rt_mutex_lock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    result = rt_msg_client_iget_host_by_name(arg0, arg1);
    if (rt_mutex_unlock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    return result;
}

int rt_msg_client_get_connection_ip(unsigned short arg0, int arg1)
{
    int result = 1;

    if (gbMutexInitialized != result)
        return RT_MSG_CLIENT_LOCK_FAILED;
    if (rt_mutex_lock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    result = rt_msg_client_iget_connection_ip(arg0, arg1);
    if (rt_mutex_unlock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    return result;
}

int rt_msg_client_receive_broadcasts(unsigned short arg0, unsigned char arg1)
{
    int result = 1;

    if (gbMutexInitialized != result)
        return RT_MSG_CLIENT_LOCK_FAILED;
    if (rt_mutex_lock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    result = func_00634660(arg0, arg1);
    if (rt_mutex_unlock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    return result;
}

int rt_msg_client_get_metrics(int arg0, int arg1)
{
    int result = 1;

    if (gbMutexInitialized != result)
        return RT_MSG_CLIENT_LOCK_FAILED;
    if (rt_mutex_lock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    result = rt_msg_client_iget_metrics(arg0, arg1);
    if (rt_mutex_unlock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    return result;
}

int func_00633418(unsigned short arg0, int arg1)
{
    int result = 1;

    if (gbMutexInitialized != result)
        return RT_MSG_CLIENT_LOCK_FAILED;
    if (rt_mutex_lock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    result = func_006347C8(arg0, arg1);
    if (rt_mutex_unlock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    return result;
}

int func_006334B8(unsigned short arg0, int arg1)
{
    int result = 1;

    if (gbMutexInitialized != result)
        return RT_MSG_CLIENT_LOCK_FAILED;
    if (rt_mutex_lock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    result = func_00634850(arg0, arg1);
    if (rt_mutex_unlock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    return result;
}

/* The option setters (round 7b: the map had the names a row off, SOCOM 1's s1:exact
 * rt_comm_set_default_options on the UDP setter's internal): DME's connect (SOCOM 1's
 * NetConnect_ClientServerTCP, 0x617788) calls the connect one, its UDP enabler (0x6195E0) the UDP
 * one and then rt_msg_client_enable_udp with the same block, as SOCOM 1's NetEnableUDP does. */
int rt_msg_client_set_default_connect_options(rt_msg_client_connect_options *pOptions)
{
    int result = 1;

    if (gbMutexInitialized != result)
        return RT_MSG_CLIENT_LOCK_FAILED;
    if (rt_mutex_lock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    result = rt_msg_client_iset_default_connect_options(pOptions);
    if (rt_mutex_unlock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    return result;
}

int rt_msg_client_udp_set_default_options(rt_msg_client_udp_options *pOptions)
{
    int result = 1;

    if (gbMutexInitialized != result)
        return RT_MSG_CLIENT_LOCK_FAILED;
    if (rt_mutex_lock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    result = rt_msg_client_iudp_set_default_options(pOptions);
    if (rt_mutex_unlock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    return result;
}

int rt_msg_client_display_comm_state(void)
{
    int result = 1;

    if (gbMutexInitialized != result)
        return RT_MSG_CLIENT_LOCK_FAILED;
    if (rt_mutex_lock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    result = rt_msg_client_idisplay_comm_state();
    if (rt_mutex_unlock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    return result;
}

int func_006336F0(unsigned short arg0, int arg1)
{
    int result = 1;

    if (gbMutexInitialized != result)
        return RT_MSG_CLIENT_LOCK_FAILED;
    if (rt_mutex_lock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    result = func_006349A8(arg0, arg1);
    if (rt_mutex_unlock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    return result;
}

extern int rt_comm_shutdown(void);
extern void KM_Cleanup(int nKMID);
extern int rt_memory_free(void **ppMemory);
extern void *gpScratchBuffer;
extern unsigned int gScratchBufferSize;
int rt_msg_client_is_valid_connection_id(unsigned short nConnection);
unsigned char rt_msg_client_free_connection_index(unsigned short nConnection);

/* SOCOM 1's internal shutdown: the last count disconnects or frees every connection, shuts the UDP
 * side and rt_comm down, frees the table and the scratch buffer and cleans the key masters up */
extern int rt_comm_shutdown(void);
extern void KM_Cleanup(int nKMID);
extern int rt_memory_free(void **ppMemory);
extern void *gpScratchBuffer;
extern unsigned int gScratchBufferSize;
int rt_msg_client_is_valid_connection_id(unsigned short nConnection);
unsigned char rt_msg_client_free_connection_index(unsigned short nConnection);

/* SOCOM 1's internal shutdown: the last count disconnects or frees every connection, shuts the UDP
 * side and rt_comm down, frees the table and the scratch buffer and cleans the key masters up */
unsigned char rt_msg_client_ishutdown(void)
{
    unsigned short i = 0;
    unsigned char result;

    if (data_00655616 <= 0)
        return 5;
    if (data_00655614 != 1)
        return 5;
    if (data_00655616 == 1) {
        for (i = 0; i < gnConnectionArraySize; i++) {
            if (gapConnections[i] != 0) {
                if (rt_msg_client_is_valid_connection_id(i) == 1)
                    rt_msg_client_idisconnect(i);
                else
                    rt_msg_client_free_connection_index(i);
            }
        }
        if (gUDPInfo.bEnabled == 1)
            rt_msg_client_udp_shutdown();
        result = rt_msg_client_map_comm_error(rt_comm_shutdown());
        if (result != 0)
            return result;
        if (gapConnections != 0)
            rt_memory_free((void **)&gapConnections);
        gapConnections = 0;
        gnConnectionArraySize = 0;
        if (gpScratchBuffer != 0) {
            rt_memory_free(&gpScratchBuffer);
            gScratchBufferSize = 0;
        }
        KM_Cleanup(0);
        data_00655614 = 0;
    }
    data_00655616--;
    return 0;
}

/* ---- the internal functions (rt_msg_client_i* and the helpers), below the wrappers ----
 * Nine written (round 6b), the two option setters (round 7b), seventeen more (round 10b: SOCOM 1's
 * idisconnect_all, set/get_header, set_scratch_buf_min_size, is_valid_connection, send_disconnect,
 * the UDP enable and send, the broadcast toggle, the platform queries at the end) -- all
 * byte-exact. The internals return a narrower type than the API's int (map_comm_error,
 * is_valid_connection, the connect step 0x634A08): an int function returning their result makes
 * a jal, not a sibling call, as the target does. is_valid_connection_id (round 6b's OPEN, a beqz
 * where 2.96 gave beqzl) matches once is_valid_connection is defined above it with the state an
 * enum. Not written: the connect/disconnect/update/send bodies and the buffer processing
 * (0x633790-0x638500's large rows). */

typedef struct rt_msg_client_comm {
    char pad0[0x1C];
    int nChannelId;                     /* 0x1C: the connection's rt_comm channel */
} rt_msg_client_comm;

/* a connection's state: an enum in SCEA's header, so compared unsigned (is_valid_connection's
 * `sltiu`); 8 is connected */
typedef enum rt_msg_client_state {
    RT_MSG_CLIENT_STATE_FREE,
    RT_MSG_CLIENT_STATE_CONNECTED = 8
} rt_msg_client_state;

typedef struct rt_msg_client_connection {
    unsigned short nId;                 /* 0x00 */
    char pad02[6];
    rt_msg_client_state nState;         /* 0x08 */
    char pad0C[0x50];
    char szIP[0x1C];                    /* 0x5C: the server's address, dotted */
    struct rt_msg_client_channel *apChannels[2]; /* 0x78 */
    rt_msg_client_comm *pComm;          /* 0x80 */
} rt_msg_client_connection;

extern unsigned char data_00655614;     /* the client is started */
extern unsigned short gnConnectionArraySize;
extern rt_msg_client_connection **gapConnections;

extern char *strcpy(char *dst, const char *src);
extern int rt_comm_get_host_by_name(char *szName, char *szAddress);
extern int func_0062D478(int nChannelId, int **pphPlatform);
/* the internal functions return a narrower type than the API's int (the internals' result
 * returned by an int function is a jal, not a sibling call: the modes differ) */
extern unsigned char rt_msg_client_map_comm_error(int nCommError);
extern unsigned char func_00634A08(unsigned short nConnection, unsigned char bFlag);
extern unsigned char rt_msg_client_is_valid_connection(rt_msg_client_connection *pConnection);
int rt_msg_client_get_connection(unsigned short nConnection, rt_msg_client_connection **ppConnection);
int rt_msg_client_get_valid_connection(unsigned short nConnection, rt_msg_client_connection **ppConnection);

extern void *memset(void *p, int c, unsigned int n);
extern int rt_comm_create(int *pnChannelId, char *szAddress, unsigned short nPort, void *pOptions);
extern int func_0062CEF8(int nChannelId, unsigned char *pbSent, void *pData, unsigned short nPort);
extern unsigned int strlen(const char *s);
extern unsigned char rt_msg_client_map_comm_error(int nCommError);

/* the enable-UDP internal (SOCOM 1's ienable_udp, its rt_rel_udp gone): a channel of the third
 * kind, the caller's callback kept */
int func_00633960(rt_msg_client_udp_options *pOptions)
{
    int anOptions[3];
    int result;

    if (data_00655614 != 1)
        return 5;
    if (pOptions == 0)
        return 2;
    if (gUDPInfo.bEnabled != 0)
        return 1;
    memset(anOptions, 0, sizeof(anOptions));
    anOptions[0] = 3;
    result = rt_msg_client_map_comm_error(rt_comm_create(&gUDPInfo.nChannel, 0, 0, anOptions));
    if (result != 0)
        return result;
    gUDPInfo.pfnCallback = pOptions->pfnCallback;
    gUDPInfo.bEnabled = 1;
    return 0;
}

int rt_msg_client_is_valid_connection_id(unsigned short nConnection);

/* SOCOM 1's idisconnect_all (s1:exact): every valid connection disconnected, the first failure
 * returned */
int rt_msg_client_idisconnect_all(void)
{
    unsigned int i = 0;
    int result;

    if (data_00655614 != 1)
        return 5;
    for (; i < gnConnectionArraySize; i++) {
        if (rt_msg_client_is_valid_connection_id(i) == 1) {
            result = rt_msg_client_idisconnect(i);
            if (result != 0)
                return result;
        }
    }
    return 0;
}

int func_00634078(unsigned short nConnection, unsigned char bFlag)
{
    return func_00634A08(nConnection, bFlag);
}

/* SOCOM 1's update-all: the connect step 0x634A08 over every valid connection, then the UDP side's
 * when it is enabled and the caller asks */
int rt_msg_client_iupdate_all(unsigned char bFlag)
{
    unsigned int i = 0;
    int result;

    if (data_00655614 != 1)
        return 5;
    for (; i < gnConnectionArraySize; i++) {
        if (rt_msg_client_is_valid_connection_id(i) == 1) {
            result = func_00634A08(i, bFlag);
            if (result != 0)
                return result;
        }
    }
    if (gUDPInfo.bEnabled == 1 && bFlag == 1) {
        result = rt_msg_client_iudp_disable_heartbeat();
        if (result != 0)
            return result;
    }
    return 0;
}

/* the UDP send internal (SOCOM 1's isend_udp): an address checked, the datagram sent on the
 * UDP channel */
int func_00634278(unsigned char *pbSent, char *szAddress, unsigned short nPort)
{
    if (pbSent == 0)
        return 2;
    *pbSent = 0;
    if (data_00655614 != 1)
        return 5;
    if (gUDPInfo.bEnabled != 1)
        return 5;
    if (szAddress == 0)
        return 2;
    if (strlen(szAddress) >= 16)
        return 2;
    return rt_msg_client_map_comm_error(func_0062CEF8(gUDPInfo.nChannel, pbSent, szAddress, nPort));
}

int rt_msg_client_iget_host_by_name(char *szName, char *szAddress)
{
    if (data_00655614 != 1)
        return 5;
    if (szName == 0)
        return 2;
    if (szAddress == 0)
        return 2;
    return rt_msg_client_map_comm_error(rt_comm_get_host_by_name(szName, szAddress));
}

int rt_msg_client_iget_connection_ip(unsigned short nConnection, char *szIP)
{
    rt_msg_client_connection *pConnection = 0;
    int result;

    if (data_00655614 != 1)
        return 5;
    if (szIP == 0)
        return 2;
    result = rt_msg_client_get_valid_connection(nConnection, &pConnection);
    if (result != 0)
        return result;
    if (pConnection->nState != 8)
        return 23;
    strcpy(szIP, pConnection->szIP);
    return 0;
}

extern int rt_msg_set_ubyte(unsigned char *msg, unsigned char value);
extern int rt_msg_set_u2byte(unsigned char *msg, unsigned short value);
extern int rt_msg_client_store_msg_in_send_buf(rt_msg_client_comm *pComm, int nType, unsigned short nTarget,
                                               unsigned char *pMsg, int nLength, int nFlags);

/* SOCOM 1's ireceive_broadcasts: message 13 asks the server for broadcasts, or stops them */
int func_00634660(unsigned short nConnection, unsigned char bEnable)
{
    unsigned char abMsg[2];
    rt_msg_client_connection *pConnection = 0;
    int result;

    if (data_00655614 != 1)
        return 5;
    result = rt_msg_client_get_valid_connection(nConnection, &pConnection);
    if (result != 0)
        return result;
    if (rt_msg_set_ubyte(abMsg, 2) != 0)
        return 7;
    if (rt_msg_set_ubyte(abMsg + 1, bEnable == 1) != 0)
        return 7;
    return rt_msg_client_store_msg_in_send_buf(pConnection->pComm, 13, 0xFFFF, abMsg, 2, 0);
}

typedef struct rt_msg_client_time {
    unsigned int secs;
    unsigned int usecs;
} rt_msg_client_time;

typedef struct rt_msg_client_metric {
    unsigned int nCount;
    float fRate;
    float fMaxRate;
} rt_msg_client_metric;

typedef struct rt_msg_client_metrics {  /* rt_comm's rt_comm_metric_data */
    rt_msg_client_time tStart;          /* 0x00 */
    rt_msg_client_metric sends;         /* 0x08 */
    rt_msg_client_metric bytesSent;     /* 0x14 */
    rt_msg_client_metric recvs;         /* 0x20 */
    rt_msg_client_metric bytesReceived; /* 0x2C */
} rt_msg_client_metrics;                /* 0x38 */

extern int rt_comm_get_metrics(rt_msg_client_metrics *pMetrics);

/* SOCOM 1's: rt_comm's metrics copied out field by field (only connection 0 has them) */
int rt_msg_client_iget_metrics(int nConnection, rt_msg_client_metrics *pMetrics)
{
    rt_msg_client_metrics metrics;
    unsigned char result;

    if (pMetrics == 0)
        return 2;
    if (nConnection != 0)
        return 3;
    result = rt_msg_client_map_comm_error(rt_comm_get_metrics(&metrics));
    if (result != 0)
        return result;
    pMetrics->tStart = metrics.tStart;
    pMetrics->sends.nCount = metrics.sends.nCount;
    pMetrics->sends.fRate = metrics.sends.fRate;
    pMetrics->sends.fMaxRate = metrics.sends.fMaxRate;
    pMetrics->bytesSent.nCount = metrics.bytesSent.nCount;
    pMetrics->bytesSent.fRate = metrics.bytesSent.fRate;
    pMetrics->bytesSent.fMaxRate = metrics.bytesSent.fMaxRate;
    pMetrics->recvs.nCount = metrics.recvs.nCount;
    pMetrics->recvs.fRate = metrics.recvs.fRate;
    pMetrics->recvs.fMaxRate = metrics.recvs.fMaxRate;
    pMetrics->bytesReceived.nCount = metrics.bytesReceived.nCount;
    pMetrics->bytesReceived.fRate = metrics.bytesReceived.fRate;
    pMetrics->bytesReceived.fMaxRate = metrics.bytesReceived.fMaxRate;
    return 0;
}

typedef struct rt_msg_client_channel {
    char pad0[0x40];
    int nValue;                         /* 0x40 */
} rt_msg_client_channel;

int func_006347C8(unsigned short nConnection, int nValue)
{
    rt_msg_client_connection *pConnection = 0;
    rt_msg_client_connection *p;
    unsigned int i;
    int result;

    if (data_00655614 != 1)
        return 5;
    result = rt_msg_client_get_valid_connection(nConnection, &pConnection);
    if (result != 0)
        return result;
    p = pConnection;
    for (i = 0; i < 2; i++) {
        if (p->apChannels[i] != 0)
            p->apChannels[i]->nValue = nValue;
    }
    return 0;
}

int func_00634850(unsigned short nConnection, int nValue)
{
    unsigned char abMsg[2];
    rt_msg_client_connection *pConnection = 0;
    int result;

    if (data_00655614 != 1)
        return 5;
    result = rt_msg_client_get_valid_connection(nConnection, &pConnection);
    if (result != 0)
        return result;
    if (rt_msg_set_u2byte(abMsg, nValue) != 0)
        return 7;
    return rt_msg_client_store_msg_in_send_buf(pConnection->pComm, 14, 0xFFFF, abMsg, 2, 0);
}

/* SOCOM 1's iset_default_connect_options, its block grown (above) */
int rt_msg_client_iset_default_connect_options(rt_msg_client_connect_options *pOptions)
{
    if (pOptions == 0)
        return 2;
    memset(pOptions, 0, sizeof(*pOptions));
    pOptions->field_00 = 0;
    pOptions->nPort = 10075;
    pOptions->field_20 = 15000;
    pOptions->field_24 = 30;
    pOptions->field_30 = 0;
    pOptions->field_34 = 0;
    pOptions->field_3C = 0;
    pOptions->field_40 = 0;
    pOptions->field_44 = 0;
    pOptions->field_2C = 0x8000;    /* ccperm: the two sizes last, in this order (3 of 5040) */
    pOptions->field_28 = 0x8000;
    return 0;
}

/* SOCOM 1's rt_comm_set_default_options' six instructions, which the map's s1:exact pass paired
 * with that name (round 7b: rt_comm's own is 0x62CB68) */
int rt_msg_client_iudp_set_default_options(rt_msg_client_udp_options *pOptions)
{
    if (pOptions == 0)
        return 2;
    pOptions->pfnCallback = 0;
    return 0;
}

int func_006349A8(unsigned short nConnection, int **pphPlatform)
{
    rt_msg_client_connection *pConnection = 0;
    int result;

    if (pphPlatform == 0)
        return 2;
    result = rt_msg_client_get_valid_connection(nConnection, &pConnection);
    if (result != 0)
        return result;
    return rt_msg_client_map_comm_error(func_0062D478(pConnection->pComm->nChannelId, pphPlatform));
}

extern int func_0062D178(int nChannelId, unsigned char *pbReceived, char *szAddress, unsigned short *pnPort);

/* SOCOM II's UDP poll (SOCOM 1's function of this name is another body): every datagram waiting
 * on the UDP channel is handed to the enable call's callback with its sender's address and port */
int rt_msg_client_iudp_disable_heartbeat(void)
{
    char szAddress[16];
    unsigned short nPort = 0;
    unsigned char bReceived = 0;
    int result;

    if (data_00655614 != 1)
        return 5;
    if (gUDPInfo.bEnabled != 1)
        return 5;
    do {
        result = rt_msg_client_map_comm_error(func_0062D178(gUDPInfo.nChannel, &bReceived, szAddress, &nPort));
        if (result != 0)
            return result;
        if (bReceived != 1)
            break;
        if (gUDPInfo.pfnCallback != 0)
            ((void (*)(char *, unsigned short))gUDPInfo.pfnCallback)(szAddress, nPort);
    } while (bReceived == 1);
    return 0;
}

typedef struct rt_msg_client_link {    /* an rt_comm channel's view: its connection's id */
    char pad0[6];
    unsigned short nId;                 /* 0x06 */
} rt_msg_client_link;

extern int func_00635898(void *pComm);

/* SOCOM 1's is_valid_connection: the connection past its first states, its channel alive, each of
 * its two links its own and alive */
unsigned char rt_msg_client_is_valid_connection(rt_msg_client_connection *pConnection)
{
    unsigned int i;

    if (pConnection == 0)
        return 0;
    if (pConnection->nState < 2)
        return 0;
    if (!func_00635898(pConnection->pComm))
        return 0;
    for (i = 0; i < 2; i++) {
        rt_msg_client_link *pLink = (rt_msg_client_link *)pConnection->apChannels[i];

        if (pLink != 0) {
            if (pConnection->nId != pLink->nId)
                return 0;
            if (!func_00635898(pLink))
                return 0;
        }
    }
    return 1;
}

int rt_msg_client_is_valid_connection_id(unsigned short nConnection)
{
    if (gapConnections == 0)
        return 0;
    if (nConnection >= gnConnectionArraySize)
        return 0;
    return rt_msg_client_is_valid_connection(gapConnections[nConnection]);
}

int rt_msg_client_get_connection(unsigned short nConnection, rt_msg_client_connection **ppConnection)
{
    if (ppConnection == 0)
        return 2;
    if (gapConnections == 0)
        return 1;
    if (gnConnectionArraySize == 0)
        return 1;
    if (nConnection >= gnConnectionArraySize)
        return 2;
    *ppConnection = gapConnections[nConnection];
    return 0;
}

int rt_msg_client_get_valid_connection(unsigned short nConnection, rt_msg_client_connection **ppConnection)
{
    if (ppConnection == 0)
        return 2;
    if (rt_msg_client_get_connection(nConnection, ppConnection) != 0)
        return 11;
    if (!rt_msg_client_is_valid_connection(*ppConnection))
        return 11;
    return 0;
}

int func_00635CD0(unsigned short nConnection, unsigned short *pnId)
{
    if (pnId == 0)
        return 2;
    *pnId = nConnection + 0x49;
    return 0;
}

/* a message's header: its type byte and its length, network order (SOCOM 1's, s1:exact) */
typedef struct rt_msg_client_header {
    unsigned char nType;                /* 0x00 */
    unsigned short nLength;             /* 0x02 */
} rt_msg_client_header;

int rt_msg_client_set_header(unsigned char *pMsg, rt_msg_client_header *pHeader, unsigned short *pnLength);
extern int rt_msg_client_send_buffer(rt_msg_client_comm *pComm, unsigned char *pMsg, unsigned short nLength,
                                     int nTimeout);

/* SOCOM 1's send_disconnect: a bare header of type 1 sent at once */
int rt_msg_client_send_disconnect(rt_msg_client_comm *pComm)
{
    unsigned char abMsg[16];
    rt_msg_client_header header;
    unsigned short nLength = 0;
    int result;

    if (data_00655614 != 1)
        return 5;
    if (func_00635898(pComm) != 1)
        return 2;
    header.nType = 1;
    header.nLength = 0;
    result = rt_msg_client_set_header(abMsg, &header, &nLength);
    if (result != 0)
        return result;
    return rt_msg_client_send_buffer(pComm, abMsg, nLength, 10000);
}

extern int rt_circ_buf_get_avail_nowrap(void *pBuf, unsigned int *pnAvail);
extern int rt_circ_buf_normalize_with_buf(void *pBuf, int bForce, void *pScratch);
int rt_msg_client_recv_circ_buf(void *pChannel);

/* SOCOM 1's: room for nSize bytes at the send buffer's end without wrapping -- received data
 * taken off first, the buffer normalised through the scratch buffer if still short */
int rt_msg_client_create_send_buf_space(void *pChannel, unsigned int nSize)
{
    unsigned int nAvail = 0;
    void *pBuf;
    int result;

    if (data_00655614 != 1)
        return 5;
    if (func_00635898(pChannel) != 1)
        return 2;
    pBuf = (char *)pChannel + 104;
    if ((pBuf != 0) != 1)
        return 2;
    if (rt_circ_buf_get_avail_nowrap(pBuf, &nAvail) != 0)
        return 21;
    if (nAvail < nSize) {
        result = rt_msg_client_recv_circ_buf(pChannel);
        if (result != 0)
            return result;
        if (rt_circ_buf_normalize_with_buf(pBuf, 1, gpScratchBuffer) != 0)
            return 21;
        if (rt_circ_buf_get_avail_nowrap(pBuf, &nAvail) != 0)
            return 21;
        if (nAvail < nSize)
            return 20;
    }
    return 0;
}

extern int rt_msg_set_ubyte(unsigned char *msg, unsigned char value);
extern int rt_msg_get_ubyte(const unsigned char *msg, unsigned char *value);
extern int rt_msg_set_u2byte(unsigned char *msg, unsigned short value);
extern int rt_msg_get_u2byte(const unsigned char *msg, unsigned short *value);

int rt_msg_client_set_header(unsigned char *pMsg, rt_msg_client_header *pHeader, unsigned short *pnLength)
{
    if (pMsg == 0)
        return 2;
    if (pHeader == 0)
        return 2;
    if (pnLength == 0)
        return 2;
    *pnLength = 0;
    if (rt_msg_set_ubyte(pMsg, pHeader->nType) != 0)
        return 7;
    (*pnLength)++;
    if (rt_msg_set_u2byte(pMsg + 1, pHeader->nLength) != 0)
        return 7;
    *pnLength += 2;
    return 0;
}

int rt_msg_client_get_header(unsigned char *pMsg, rt_msg_client_header *pHeader, unsigned short *pnLength)
{
    if (pMsg == 0)
        return 2;
    if (pHeader == 0)
        return 2;
    if (pnLength == 0)
        return 2;
    *pnLength = 0;
    if (rt_msg_get_ubyte(pMsg, &pHeader->nType) != 0)
        return 7;
    (*pnLength)++;
    if (rt_msg_get_u2byte(pMsg + 1, &pHeader->nLength) != 0)
        return 7;
    *pnLength += 2;
    return 0;
}

int rt_msg_client_is_wait_state(int nState)
{
    if (nState == 2 || nState == 4 || nState == 3 || nState == 5 || nState == 6 || nState == 7 || nState == 9
        || nState == 10)
        return 1;
    return 0;
}


/* the link's types to the server's, and the reverse map's */
int func_006380C0(int *pnType, unsigned char nType)
{
    if (pnType == 0)
        return 2;
    switch (nType) {
    case 0:
        *pnType = 2;
        break;
    case 1:
        *pnType = 4;
        break;
    case 2:
        *pnType = 5;
        break;
    case 3:
        *pnType = 6;
        break;
    case 4:
        *pnType = 7;
        break;
    case 5:
        *pnType = 8;
        break;
    case 6:
        *pnType = 9;
        break;
    default:
        return 1;
    }
    return 0;
}

int func_00638178(int *pnType, unsigned char nType)
{
    if (pnType == 0)
        return 2;
    switch (nType) {
    case 0:
        *pnType = 0;
        break;
    case 1:
        *pnType = 1;
        break;
    case 2:
        *pnType = 2;
        break;
    case 3:
        *pnType = 3;
        break;
    case 4:
        *pnType = 4;
        break;
    default:
        return 1;
    }
    return 0;
}

extern void *gpScratchBuffer;
extern unsigned int gScratchBufferSize;
extern int rt_memory_allocate_init_zero(void **ppMemory, unsigned long nSize);
extern int rt_memory_free(void **ppMemory);

int rt_msg_client_set_scratch_buf_min_size(unsigned int nSize)
{
    if (nSize == 0)
        return 2;
    if (gScratchBufferSize < nSize) {
        if (gpScratchBuffer != 0)
            rt_memory_free(&gpScratchBuffer);
        if (rt_memory_allocate_init_zero(&gpScratchBuffer, nSize) != 0)
            return 8;
        gScratchBufferSize = nSize;
    }
    return 0;
}

/* the platform's queries: a connection's crypt state and the link's capabilities */
extern int KM_Encrypt(unsigned short nConnection, unsigned short nTarget, int a2, int a3);

int func_006386F8(unsigned short nConnection, int *pnState)
{
    if (pnState == 0)
        return 2;
    *pnState = KM_Encrypt(nConnection, 0xFFFF, 0, 0);
    return 0;
}

int func_00638740(unsigned short nConnection, unsigned char *pbState)
{
    int nState = 0;
    int result;

    if (pbState == 0)
        return 2;
    *pbState = 0;
    result = func_006386F8(nConnection, &nState);
    if (result != 0)
        return result;
    if (nState != 0)
        *pbState = 1;
    return 0;
}

int func_00638798(unsigned int nType, unsigned char a1, unsigned char a2, int a3, unsigned char *pbOut)
{
    if (pbOut == 0)
        return 2;
    *pbOut = 0;
    if (nType >= 2)
        return 2;
    if (a1 != 0 && a2 >= 7)
        *pbOut = 1;
    return 0;
}

int func_006387E0(unsigned int nType, char *pa, char *pb, char *pc)
{
    if (nType >= 2)
        return 2;
    if (pa == 0)
        return 2;
    if (pb == 0)
        return 2;
    if (pc == 0)
        return 2;
    *pa = 1;
    *pb = 8;
    *pc = -124;
    return 0;
}

