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
 * takes v1 where the target's takes v0).
 * Round 14b: the UDP poll closed -- its loop `break`s to the one `return 0` (a `return 0` inside the
 * loop let CSE keep the map call's known-zero v0 live, so the flag took v1). Written: the connect
 * message's builder 0x635940 (its kind unsigned; the buffer pointer's initialiser first), the
 * symmetric-key and callback handlers 0x637230 and 0x637438 (a handler takes the connection, the
 * message type, the message and its length -- an unsigned int, narrowed only at the callback's
 * `unsigned short`: the target's `li 0xfffc; addu; andi`), the memory-write handler 0x637900 and
 * SOCOM 1's is_valid_outgoing_msg (an `||` chain in the target's test order, five types added).
 * 0x637230 is byte-exact only below 0x635940, which its fall-through calls (round 13c's rule: with
 * the callee compiled first the branch keeps `move v0,zero` in a plain slot). */

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
extern int rt_msg_client_isend_target(unsigned short, unsigned short, int, unsigned char *, unsigned short);
extern int rt_msg_client_isend_client_set(unsigned short, unsigned short, unsigned short, int, int, int, unsigned short);
extern int func_00634278(unsigned char *, char *, unsigned short);
extern int rt_msg_client_iget_buffer_status(unsigned short, unsigned int *, unsigned int *, unsigned int *, unsigned int *);
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

typedef struct rt_msg_client_time {
    unsigned int secs;
    unsigned int usecs;
} rt_msg_client_time;

/* rt_circ_buf's ring (rt_circ_buf.c) */
typedef struct rt_circ_buf {
    unsigned int nStart;
    unsigned int nEnd;
    unsigned int nSize;
    unsigned int nUsed;
    char *pBuf;
    void *pNormalizeBuf;
} rt_circ_buf;                          /* 0x18 */

extern int rt_circ_buf_get_used(rt_circ_buf *cb, unsigned int *pnUsed);
extern int rt_circ_buf_copy(void *pDest, rt_circ_buf *cb, unsigned int nBytes);

/* a channel: a connection's (one of two) or the UDP side's -- its rt_comm channel and its two
 * rings, each with the size it was made with (round 15b: the buffer status's sums) */
typedef struct rt_msg_client_comm {
    int nKind;                          /* 0x00: 0 a stream, 1 datagrams to szAddress (0x6385F0) */
    char pad04;
    unsigned char bKind;                /* 0x05: 1 a connection's own channel (0x635078's test) */
    unsigned short nId;                 /* 0x06: its connection's id (rt_msg_client_link's view) */
    char szAddress[16];                 /* 0x08 */
    unsigned short nPort;               /* 0x18 */
    char pad1A[2];
    int nChannelId;                     /* 0x1C: the connection's rt_comm channel */
    rt_msg_client_time tLastSend;       /* 0x20: recv_circ_buf's (the last piece that moved) */
    char pad28[0x18];
    int nValue;                         /* 0x40: 0x6347C8's setting */
    unsigned char bTargetSet;           /* 0x44: a flagged message stored (SOCOM 1's 0x84) */
    char pad45;
    unsigned short nLastTarget;         /* 0x46: its target, 0xFFFE when they differed */
    unsigned int nSendBufSize;          /* 0x48 */
    unsigned int nRecvBufSize;          /* 0x4C */
    rt_circ_buf recvBuf;                /* 0x50 */
    rt_circ_buf sendBuf;                /* 0x68: create_send_buf_space's */
    /* 0x80: the application messages gathered for one send (store_msg_in_send_buf): same type
     * (2 or 3), same target, at most 512 bytes */
    struct rt_msg_client_pending {
        int nType;                      /* 0x80 */
        unsigned short nTarget;         /* 0x84 */
        unsigned short nLength;         /* 0x86 */
        unsigned short nCount;          /* 0x88 */
        unsigned char abData[512];      /* 0x8A */
    } pending;
} rt_msg_client_comm;

/* a connection's state: an enum in SCEA's header, so compared unsigned (is_valid_connection's
 * `sltiu`); 8 is connected */
typedef enum rt_msg_client_state {
    RT_MSG_CLIENT_STATE_FREE,
    RT_MSG_CLIENT_STATE_CONNECTED = 8
} rt_msg_client_state;

typedef struct rt_msg_client_connection {
    unsigned short nId;                 /* 0x00 */
    char pad02[2];
    unsigned int nConnectKind;          /* 0x04: the connect message's kind (0x635940: 0 or 1) */
    rt_msg_client_state nState;         /* 0x08 */
    char pad0C[0x16];
    unsigned short nPort;               /* 0x22: the connect message's port */
    char pad24[0x18];
    void *pConnectData;                 /* 0x3C: the data the connect message carries */
    unsigned short nConnectDataLength;  /* 0x40 */
    char pad42[0x1A];
    char szIP[0x1C];                    /* 0x5C: the server's address, dotted */
    rt_msg_client_comm *apChannels[2];  /* 0x78: the connection's channels */
    rt_msg_client_comm *pComm;          /* 0x80: the one it sends on */
    /* 0x84: told when the connect fails while it waits (0x634A08: the last argument 1) */
    void (*pfnConnect)(unsigned short nId, int a1, int a2, int a3, int a4);
    /* 0x88: the application's messages (types 2, 3 and 10): the connection, the source client, the
     * message and its length */
    void (*pfnAppMessage)(unsigned short nId, unsigned short nSource, unsigned char *pMsg,
                          unsigned short nLength);
    char pad8C[4];
    void (*pfnDisconnect)(unsigned short nId, int nReason); /* 0x90 */
    /* 0x94: the application's message callback (round 14b: 0x637438) */
    void (*pfnCallback)(unsigned short nId, unsigned char b0, unsigned char b1, unsigned char b2,
                        unsigned char b3, unsigned short nLength, unsigned char *pData);
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
unsigned char rt_msg_client_get_connection(unsigned short nConnection, rt_msg_client_connection **ppConnection);
unsigned char rt_msg_client_get_valid_connection(unsigned short nConnection, rt_msg_client_connection **ppConnection);

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
extern int rt_msg_client_change_state(rt_msg_client_connection *pConnection, unsigned int nState);
extern int rt_msg_client_send_buffer(rt_msg_client_comm *pComm, unsigned char *pMsg, unsigned int nLength,
                                     unsigned int nTimeout);
extern int rt_msg_client_send_disconnect(rt_msg_client_comm *pComm);
extern int rt_msg_client_set_scratch_buf_min_size(unsigned int nSize);
extern int rt_comm_destroy(int nChannelId);
extern void *gpScratchBuffer;

/* SOCOM 1's idisconnect, grown: a connected connection's unsent bytes flushed through the scratch
 * buffer and its disconnect sent (states 9, then 10), both its channels destroyed (SOCOM 1 had
 * one), then state 1 and its index freed; a connection already in state 1 is only freed */
int rt_msg_client_idisconnect(unsigned short nConnection)
{
    rt_msg_client_connection *pConnection = 0;
    unsigned int nUsed = 0;
    rt_msg_client_comm *pComm;
    unsigned int i;
    int result;

    if (data_00655614 != 1)
        return 5;
    result = rt_msg_client_get_connection(nConnection, &pConnection);
    if (result != 0)
        return result;
    if (pConnection != 0) {
        if (pConnection->nState != 1) {
            if (pConnection->nState == 8) {
                pComm = pConnection->pComm;
                if (pComm != 0 && rt_circ_buf_get_used(&pComm->sendBuf, &nUsed) == 0) {
                    if (nUsed != 0) {
                        rt_msg_client_set_scratch_buf_min_size(nUsed);
                        if (rt_circ_buf_copy(gpScratchBuffer, &pComm->sendBuf, nUsed) == 0)
                            rt_msg_client_send_buffer(pComm, gpScratchBuffer, nUsed, 10000);
                    }
                    rt_msg_client_change_state(pConnection, 9);
                    rt_msg_client_send_disconnect(pComm);
                }
            }
            rt_msg_client_change_state(pConnection, 10);
            for (i = 0; i < 2; i++) {
                pComm = pConnection->apChannels[i];
                if (pComm != 0)
                    rt_comm_destroy(pComm->nChannelId);
            }
            rt_msg_client_change_state(pConnection, 1);
        }
        rt_msg_client_free_connection_index(nConnection);
    }
    return 0;
}

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

extern int rt_msg_client_store_msg_in_send_buf(rt_msg_client_comm *pComm, int nType, unsigned short nTarget,
                                               unsigned char *pMsg, unsigned short nLength, unsigned char bFlag);

/* SOCOM 1's isend_target, grown: a message of at most 512 bytes for one client (type 3), every client
 * (0xFFFE, type 2) or the server (0xFFFF, type 11) stored in the connection's send ring -- on its
 * second channel when the connection has one of its own kind and the flags ask for neither 0x10 nor
 * 0x40; flag 0x80 passed on */
int rt_msg_client_isend_target(unsigned short nConnection, unsigned short nTarget, int nFlags,
                               unsigned char *pData, unsigned short nLength)
{
    rt_msg_client_connection *pConnection = 0;
    rt_msg_client_comm *pComm;
    rt_msg_client_comm *pAlt;
    int nType;
    int result;

    if (data_00655614 != 1)
        return 5;
    if (pData == 0)
        return 2;
    if (nLength > 512)
        return 2;
    result = rt_msg_client_get_valid_connection(nConnection, &pConnection);
    if (result != 0)
        return result;
    if (pConnection->nState != 8)
        return 23;
    pComm = pConnection->pComm;
    if (pConnection->nConnectKind == 1) {
        pAlt = pConnection->apChannels[1];
        if (pAlt != 0 && pAlt->bKind == 1 && (nFlags & 0x50) == 0)
            pComm = pAlt;
    }
    if (nTarget == 0xFFFE)
        nType = 2;
    else if (nTarget == 0xFFFF)
        nType = 11;
    else
        nType = 3;
    return rt_msg_client_store_msg_in_send_buf(pComm, nType, nTarget, pData, nLength, (nFlags >> 7) & 1);
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

/* SOCOM 1's iget_buffer_status over both channels: the bytes waiting in the send and receive rings,
 * summed, and each as a percentage of the rings' sizes (at most 100; 1 when a ring has none).
 * The receive percentage is POSITIVE with its error in an else (round 14b's rule): with the guard,
 * loop.c's exit-block move (find_and_verify_loops) puts the loop's `return 21` block after that
 * guard's barrier, here after the last statement, as the target. OPEN (10 words, registers only):
 * the send sum takes s2 and the strength-reduced channel offset s1, the target's the other way round;
 * 40 declaration orders tried (build/scratch/gcc/declperm.py) move only the zeroed slots' stores. */
int rt_msg_client_iget_buffer_status(unsigned short nConnection, unsigned int *pnSendUsed,
                                     unsigned int *pnRecvUsed, unsigned int *pnSendPercent,
                                     unsigned int *pnRecvPercent)
{
    unsigned int nSendBytes = 0;
    unsigned int nRecvBytes = 0;
    rt_msg_client_connection *pConnection = 0;
    unsigned int nSendSize = 0;
    unsigned int nRecvUsed = 0;
    unsigned int nRecvSize = 0;
    rt_msg_client_comm *pComm;
    unsigned int i;
    int result;
    unsigned int nSendUsed = 0;

    if (data_00655614 != 1)
        return 5;
    result = rt_msg_client_get_valid_connection(nConnection, &pConnection);
    if (result != 0)
        return result;
    for (i = 0; i < 2; i++) {
        pComm = pConnection->apChannels[i];
        if (pComm != 0) {
            if (rt_circ_buf_get_used(&pComm->sendBuf, &nSendBytes) != 0)
                return 21;
            nSendUsed += nSendBytes;
            nSendSize += pComm->nSendBufSize;
            if (rt_circ_buf_get_used(&pComm->recvBuf, &nRecvBytes) != 0)
                return 21;
            nRecvSize += pComm->nRecvBufSize;
            nRecvUsed += nRecvBytes;
        }
    }
    if (pnSendUsed != 0)
        *pnSendUsed = nSendUsed;
    if (pnRecvUsed != 0)
        *pnRecvUsed = nRecvUsed;
    if (pnSendPercent != 0) {
        if (nSendSize == 0)
            return 1;
        *pnSendPercent = nSendUsed * 100 / nSendSize;
        if (*pnSendPercent > 100)
            *pnSendPercent = 100;
    }
    if (pnRecvPercent != 0) {
        if (nRecvSize != 0) {
            *pnRecvPercent = nRecvUsed * 100 / nRecvSize;
            if (*pnRecvPercent > 100)
                *pnRecvPercent = 100;
        } else
            return 1;
    }
    return 0;
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
                                               unsigned char *pMsg, unsigned short nLength, unsigned char bFlag);

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

extern unsigned char func_00634AD8(rt_msg_client_connection *pConnection, unsigned char bFlag);
extern int rt_msg_client_is_wait_state(int nState);

/* one connection updated (iupdate_all's step): its channels serviced (0x634AD8); when that fails
 * while the connection still waits to connect, its connect callback is told (the last argument 1)
 * and the connection goes to state 11 */
unsigned char func_00634A08(unsigned short nConnection, unsigned char bFlag)
{
    rt_msg_client_connection *pConnection = 0;
    unsigned char result;
    unsigned short nId;

    result = rt_msg_client_get_valid_connection(nConnection, &pConnection);
    if (result != 0)
        return result;
    result = func_00634AD8(pConnection, bFlag);
    if (result != 0 && bFlag == 1 && rt_msg_client_is_wait_state(pConnection->nState) == bFlag) {
        if (pConnection->pfnConnect != 0) {
            nId = pConnection->nId;
            pConnection->pfnConnect(nConnection, 0, 0, 0, 1);
            if (rt_msg_client_is_valid_connection_id(nId) == 0)
                return 11;
        }
        rt_msg_client_change_state(pConnection, 11);
    }
    return result;
}

extern int func_0062D178(int nChannelId, unsigned char *pbReceived, char *szAddress, unsigned short *pnPort);

extern int func_00637D50(rt_msg_client_comm *pComm);
extern int rt_msg_client_process_recv_buf(rt_msg_client_connection *pConnection, rt_msg_client_comm *pComm);

/* one connection's channel updated (0x635078): the connection and its channel valid and its own, the
 * channel's pending input taken (0x637D50), then processed when the caller asks */
int func_00635078(rt_msg_client_connection *pConnection, rt_msg_client_comm *pComm, unsigned char bProcess)
{
    int result;

    if (data_00655614 != 1)
        return 5;
    if (rt_msg_client_is_valid_connection(pConnection) != 1)
        return 2;
    if (func_00635898(pComm) != 1)
        return 2;
    if (pComm->bKind != 1)
        return 2;
    if (pConnection->nId != pComm->nId)
        return 2;
    result = func_00637D50(pComm);
    if (result != 0)
        return result;
    if (bProcess == 1) {
        result = rt_msg_client_process_recv_buf(pConnection, pComm);
        if (result != 0)
            return result;
    }
    return 0;
}

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

extern int rt_circ_buf_destroy(rt_circ_buf *cb);
extern void KM_Destroy(int nKMID);

/* SOCOM 1's free_connection_index, grown (round 16b): each of the connection's two channels has its
 * rings destroyed and is freed (SOCOM 1's one channel was in the record), then the connect data,
 * the record cleared and freed, its slot emptied; the connection's key is destroyed either way */
unsigned char rt_msg_client_free_connection_index(unsigned short nConnection)
{
    unsigned int i;

    if (data_00655614 != 1)
        return 5;
    if (nConnection >= gnConnectionArraySize)
        return 2;
    if (gapConnections[nConnection] != 0) {
        for (i = 0; i < 2; i++) {
            if (gapConnections[nConnection]->apChannels[i] != 0) {
                rt_circ_buf_destroy(&gapConnections[nConnection]->apChannels[i]->recvBuf);
                rt_circ_buf_destroy(&gapConnections[nConnection]->apChannels[i]->sendBuf);
                rt_memory_free((void **)&gapConnections[nConnection]->apChannels[i]);
            }
        }
        if (gapConnections[nConnection]->pConnectData != 0)
            rt_memory_free(&gapConnections[nConnection]->pConnectData);
        memset(gapConnections[nConnection], 0, sizeof(rt_msg_client_connection));
        rt_memory_free((void **)&gapConnections[nConnection]);
        gapConnections[nConnection] = 0;
    }
    KM_Destroy(nConnection);
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

unsigned char rt_msg_client_get_connection(unsigned short nConnection, rt_msg_client_connection **ppConnection)
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

unsigned char rt_msg_client_get_valid_connection(unsigned short nConnection, rt_msg_client_connection **ppConnection)
{
    if (ppConnection == 0)
        return 2;
    if (rt_msg_client_get_connection(nConnection, ppConnection) != 0)
        return 11;
    if (!rt_msg_client_is_valid_connection(*ppConnection))
        return 11;
    return 0;
}

int func_00635CD0(unsigned short nDataLength, unsigned short *pnSize);
extern int func_00635A70(void *pMsg, unsigned short nSize, unsigned int nKind, unsigned short nId, unsigned short nPort,
                         void *pData, unsigned short nDataLength);
extern int rt_memory_allocate_init_zero(void **ppMemory, unsigned long nSize);

/* a connect message (round 14b): its size for the data it carries (0x635CD0), built by 0x635A70 in
 * a buffer of its own and queued on the channel -- type 21, or 0 for the first kind */
int func_00635940(unsigned int nKind, rt_msg_client_comm *pComm, unsigned short nPort, void *pData,
                  unsigned short nDataLength)
{
    void *pMsg = 0;
    unsigned short nSize = 0;
    int result;

    if (data_00655614 != 1)
        return 5;
    if (func_00635898(pComm) != 1)
        return 2;
    if (pData == 0 && nDataLength != 0)
        return 2;
    if (nKind >= 2)
        return 2;
    result = func_00635CD0(nDataLength, &nSize);
    if (result != 0)
        return result;
    if (rt_memory_allocate_init_zero(&pMsg, nSize) != 0)
        return 8;
    result = func_00635A70(pMsg, nSize, nKind, pComm->nId, nPort, pData, nDataLength);
    if (result == 0)
        result = rt_msg_client_store_msg_in_send_buf(pComm, nKind != 0 ? 21 : 0, 0xFFFF, pMsg, nSize, 1);
    if (pMsg != 0)
        rt_memory_free(&pMsg);
    return result;
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
extern int rt_msg_client_send_buffer(rt_msg_client_comm *pComm, unsigned char *pMsg, unsigned int nLength,
                                     unsigned int nTimeout);

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

/* a target set on the channel (0x635D98): none for 0xFFFF, message 15 for 0xFFFE (every client),
 * else message 16 with the target's id */
int func_00635D98(rt_msg_client_comm *pComm, unsigned short nTarget)
{
    unsigned char abMsg[2];
    unsigned char *pMsg;
    int nLength = 0;
    int nType;
    int result;

    if (data_00655614 != 1)
        return 5;
    if (func_00635898(pComm) != 1)
        return 2;
    if (nTarget != 0xFFFF) {
        if (nTarget == 0xFFFE) {
            nType = 15;
            pMsg = 0;
        } else {
            nType = 16;
            if (rt_msg_set_u2byte(abMsg, nTarget) != 0)
                return 7;
            pMsg = abMsg;
            nLength = 2;
        }
        result = rt_msg_client_store_msg_in_send_buf(pComm, nType, 0xFFFF, pMsg, nLength, 0);
        if (result != 0)
            return result;
    }
    return 0;
}

extern int func_00638298(rt_msg_client_comm *pComm);
extern void *memcpy(void *dst, const void *src, unsigned int n);
int rt_msg_client_is_valid_outgoing_msg(int nType);

/* SOCOM 1's store_msg_in_send_buf, grown: an application message (type 2 or 3) joins the pending
 * ones when it has their type and target and still fits in 512 bytes, else the pending ones are sent
 * first (0x638298); any other type is sent at once. A flagged message records its target (SOCOM 1's
 * isend_target did): the first one's, kept while they agree, 0xFFFE once they differ (the server's
 * 0xFFFF never counts against it). */
int rt_msg_client_store_msg_in_send_buf(rt_msg_client_comm *pComm, int nType, unsigned short nTarget,
                                        unsigned char *pMsg, unsigned short nLength, unsigned char bFlag)
{
    struct rt_msg_client_pending *pPending;
    int bAppend;
    int result;

    if (func_00635898(pComm) != 1)
        return 2;
    if (data_00655614 != 1)
        return 5;
    if (pMsg == 0 && nLength != 0)
        return 2;
    if (rt_msg_client_is_valid_outgoing_msg(nType) != 1)
        return 2;
    if (nLength > 512)
        return 2;
    pPending = &pComm->pending;
    if (pPending == 0)
        return 9;
    if (pPending->nCount != 0) {
        bAppend = 0;
        if ((nType == 2 || nType == 3) && pPending->nType == nType && pPending->nTarget == nTarget) {
            bAppend = 1;
            if (pPending->nLength + nLength > 512)
                bAppend = 0;
        }
    } else
        bAppend = 1;
    if (bAppend == 0) {
        result = func_00638298(pComm);
        if (result != 0)
            return result;
    }
    pPending->nType = nType;
    pPending->nTarget = nTarget;
    memcpy(pPending->abData + pPending->nLength, pMsg, nLength);
    pPending->nLength += nLength;
    pPending->nCount++;
    if (nType != 2 && nType != 3)
        func_00638298(pComm);
    if (bFlag == 1) {
        if (pComm->bTargetSet == 1) {
            if (pComm->nLastTarget == 0xFFFF)
                pComm->nLastTarget = nTarget;
            else if (pComm->nLastTarget != nTarget && nTarget != 0xFFFF)
                pComm->nLastTarget = 0xFFFE;
        } else
            pComm->nLastTarget = nTarget;
        pComm->bTargetSet = 1;
    }
    return 0;
}

extern int rt_circ_buf_get_avail_nowrap(void *pBuf, unsigned int *pnAvail);
extern int rt_circ_buf_normalize_with_buf(void *pBuf, int bForce, void *pScratch);
int rt_msg_client_recv_circ_buf(rt_msg_client_comm *pChannel);

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

extern int rt_circ_buf_get_used_nowrap(rt_circ_buf *cb, unsigned int *pnUsed);
extern int rt_circ_buf_is_wrapped(rt_circ_buf *cb, unsigned char *pbWrapped);
extern int rt_circ_buf_get_start_used_ptr(rt_circ_buf *cb, unsigned char **ppStart);
extern int rt_circ_buf_free(rt_circ_buf *cb, unsigned int nBytes);
extern int func_006364C0(rt_msg_client_connection *pConnection, rt_msg_client_comm *pComm, int nType,
                         unsigned char *pMsg, unsigned short nLength);
unsigned char rt_msg_client_get_header(unsigned char *pMsg, rt_msg_client_header *pHeader, unsigned short *pnLength);

/* SOCOM 1's process_recv_buf, grown (round 16b): the connection and its channel valid and its own
 * (0x635078's tests), the receive ring normalised when it wraps, then every whole message at its
 * start processed (0x6364C0) and freed while three bytes are left and the connection's state
 * holds; a flagged message (type bit 7) with a body carries a 4-byte trailer */
int rt_msg_client_process_recv_buf(rt_msg_client_connection *pConnection, rt_msg_client_comm *pComm)
{
    rt_msg_client_header header;
    unsigned char bWrapped;
    unsigned char *pMsg = 0;
    unsigned int nUsed = 0;
    unsigned short nHeaderLength = 0;
    rt_circ_buf *pBuf;
    rt_msg_client_state nState;
    unsigned int nSize;
    int result;

    if (rt_msg_client_is_valid_connection(pConnection) != 1)
        return 2;
    if (func_00635898(pComm) != 1)
        return 2;
    if (pComm->bKind != 1)
        return 2;
    if (pConnection->nId != pComm->nId)
        return 2;
    pBuf = &pComm->recvBuf;
    if (rt_circ_buf_is_wrapped(pBuf, &bWrapped) != 0)
        return 21;
    if (bWrapped == 1) {
        if (rt_circ_buf_normalize_with_buf(pBuf, 1, gpScratchBuffer) != 0)
            return 21;
    }
    if (rt_circ_buf_get_used_nowrap(pBuf, &nUsed) != 0)
        return 21;
    if (nUsed >= 3) {
        nState = pConnection->nState;
        for (;;) {
            if (rt_circ_buf_get_start_used_ptr(pBuf, &pMsg) != 0)
                return 21;
            result = rt_msg_client_get_header(pMsg, &header, &nHeaderLength);
            if (result != 0)
                return result;
            nSize = nHeaderLength + header.nLength;
            if (header.nLength != 0 && (header.nType & 0x80))
                nSize += 4;
            if (nUsed < nSize)
                break;
            pMsg += nHeaderLength;
            result = func_006364C0(pConnection, pComm, header.nType, pMsg, header.nLength);
            if (result != 0)
                return result;
            if (rt_circ_buf_free(pBuf, nSize) != 0)
                return 21;
            if (rt_circ_buf_get_used_nowrap(pBuf, &nUsed) != 0)
                return 21;
            if (nUsed < 3)
                break;
            if (nState != pConnection->nState)
                break;
        }
    }
    return 0;
}

extern int rt_msg_set_ubyte(unsigned char *msg, unsigned char value);
extern int rt_msg_get_ubyte(const unsigned char *msg, unsigned char *value);
extern int rt_msg_set_u2byte(unsigned char *msg, unsigned short value);
extern int rt_msg_get_u2byte(const unsigned char *msg, unsigned short *value);

extern int rt_msg_get_u4byte(const unsigned char *msg, unsigned int *value);
extern void *memcpy(void *dst, const void *src, unsigned int n);

extern int KM_AddSymmetricKey(int nKMID, int nClientID, void *pKey);
extern int rt_msg_client_change_state(rt_msg_client_connection *pConnection, unsigned int nState);

extern int func_00638178(int *pnReason, unsigned char nReason);

/* SOCOM 1's process_app_message: an application message for the connection's callback -- types 2 and
 * 3 carry their source client first (two bytes), type 10 is the server's (0xFFFF) */
int rt_msg_client_process_app_message(rt_msg_client_connection *pConnection, int nType, unsigned char *pMsg,
                                      unsigned int nLength)
{
    unsigned short nSource = 0;
    unsigned int nOffset = 0;
    unsigned short nId;

    if (rt_msg_client_is_valid_connection(pConnection) != 1)
        return 2;
    if (pMsg == 0)
        return 2;
    if (nType != 2 && nType != 3 && nType != 10)
        return 2;
    if (nType == 10) {
        nSource = 0xFFFF;
    } else {
        if (nLength > nOffset + 1) {
            if (rt_msg_get_u2byte(pMsg, &nSource) != 0)
                return 7;
            pMsg += 2;
            nOffset = 2;
        } else
            return 1;
    }
    if (pConnection->pfnAppMessage != 0) {
        nId = pConnection->nId;
        pConnection->pfnAppMessage(nId, nSource, pMsg, nLength - nOffset);
        if (rt_msg_client_is_valid_connection_id(nId) == 0)
            return 11;
    }
    return 0;
}

/* SOCOM 1's process_disconnect_message: the server's reason byte mapped (0x638178, SOCOM 1's
 * map_fdisconnect_reason) and handed to the connection's disconnect callback */
int rt_msg_client_process_disconnect_message(rt_msg_client_connection *pConnection, int nType,
                                             unsigned char *pMsg, unsigned int nLength)
{
    unsigned char nByte;
    int nReason = 0;
    unsigned int nOffset = 0;
    unsigned short nId;
    int result;

    if (data_00655614 != 1)
        return 5;
    if (rt_msg_client_is_valid_connection(pConnection) != 1)
        return 2;
    if (pMsg == 0)
        return 2;
    if (nLength > nOffset) {
        if (rt_msg_get_ubyte(pMsg, &nByte) != 0)
            return 7;
        result = func_00638178(&nReason, nByte);
        if (result != 0)
            return result;
        if (pConnection->pfnDisconnect != 0) {
            nId = pConnection->nId;
            pConnection->pfnDisconnect(nId, nReason);
            if (rt_msg_client_is_valid_connection_id(nId) == 0)
                return 11;
        }
    } else
        return 1;
    return 0;
}

/* the server's symmetric key (message 27 carries a key id first): kept in the key master under the
 * connection's id, then the connection's next step when it waits for the key */
int func_00637230(rt_msg_client_connection *pConnection, int nType, unsigned char *pMsg, int nLength)
{
    unsigned short nKeyID = 0xFFFF;
    int result;

    if (data_00655614 != 1)
        return 5;
    if (pConnection == 0)
        return 2;
    if (pMsg == 0)
        return 2;
    if (nType == 27) {
        rt_msg_get_u2byte(pMsg, &nKeyID);
        pMsg += 2;
        nLength -= 2;
    }
    if (nLength != 64)
        return 2;
    KM_AddSymmetricKey(pConnection->nId, nKeyID, pMsg);
    if (pConnection->nState == 3) {
        result = func_00635940(pConnection->nConnectKind, pConnection->pComm, pConnection->nPort,
                               pConnection->pConnectData, pConnection->nConnectDataLength);
        if (result != 0)
            return result;
        result = rt_msg_client_change_state(pConnection, 4);
        if (result != 0)
            return result;
    }
    return 0;
}

/* a message for the connection's callback: four bytes, then the rest with its length */
int func_00637438(rt_msg_client_connection *pConnection, int nType, unsigned char *pMsg, unsigned int nLength)
{
    unsigned char b0, b1, b2, b3;

    if (data_00655614 != 1)
        return 5;
    if (rt_msg_client_is_valid_connection(pConnection) != 1)
        return 2;
    if (pMsg == 0)
        return 2;
    if (pConnection->pfnCallback != 0) {
        rt_msg_get_ubyte(pMsg, &b0);
        rt_msg_get_ubyte(pMsg + 1, &b1);
        rt_msg_get_ubyte(pMsg + 2, &b2);
        rt_msg_get_ubyte(pMsg + 3, &b3);
        pConnection->pfnCallback(pConnection->nId, b0, b1, b2, b3, nLength - 4, pMsg + 4);
    }
    return 0;
}

/* a message that writes memory (0x637900, new in SOCOM II): its first word an address, its second
 * a length, the rest copied there when the length is the message's own, on a valid connection */
int func_00637900(rt_msg_client_connection *pConnection, int nUnused1, int nUnused2, unsigned char *pMsg,
                  unsigned int nLength)
{
    unsigned int nAddress;
    unsigned int nBytes;

    if (data_00655614 != 1)
        return 5;
    if (rt_msg_client_is_valid_connection(pConnection) != 1)
        return 2;
    if (pMsg == 0)
        return 2;
    if (nLength < 9)
        return 0;
    rt_msg_get_u4byte(pMsg, &nAddress);
    rt_msg_get_u4byte(pMsg + 4, &nBytes);
    if (nBytes != nLength - 8)
        return 0;
    memcpy((void *)nAddress, pMsg + 8, nBytes);
    return 0;
}

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

unsigned char rt_msg_client_get_header(unsigned char *pMsg, rt_msg_client_header *pHeader, unsigned short *pnLength)
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

extern int rt_time_start(rt_msg_client_time *pStart);
extern int rt_time_elapsed(rt_msg_client_time *pStart, unsigned int *pnMsecs);
extern int func_006385F0(rt_msg_client_comm *pComm, unsigned char *pMsg, unsigned int nLength,
                         unsigned short *pnSent);

/* SOCOM 1's send_buffer: the bytes sent in pieces (0x6385F0) until none is left, failing after the
 * timeout's milliseconds */
int rt_msg_client_send_buffer(rt_msg_client_comm *pComm, unsigned char *pMsg, unsigned int nLength,
                              unsigned int nTimeout)
{
    rt_msg_client_time tStart;
    unsigned int nElapsed = 0;
    unsigned short nSent = 0;
    int result;

    if (data_00655614 != 1)
        return 5;
    if (func_00635898(pComm) != 1)
        return 2;
    if (pMsg == 0)
        return 2;
    if (rt_time_start(&tStart) != 0)
        return 24;
    do {
        result = func_006385F0(pComm, pMsg, nLength, &nSent);
        if (result != 0)
            return result;
        nLength -= nSent;
        pMsg += nSent;
        if (rt_time_elapsed(&tStart, &nElapsed) != 0)
            return 24;
        if (nElapsed > nTimeout)
            return 13;
    } while (nLength != 0);
    return 0;
}

extern int rt_circ_buf_get_used_nowrap(rt_circ_buf *cb, unsigned int *pnUsed);
extern int rt_circ_buf_is_wrapped(rt_circ_buf *cb, unsigned char *pbWrapped);
extern int rt_circ_buf_get_start_used_ptr(rt_circ_buf *cb, unsigned char **ppStart);
extern int rt_circ_buf_free(rt_circ_buf *cb, unsigned int nBytes);

/* SOCOM 1's recv_circ_buf (s1:align; it drains the SEND ring): the bytes waiting sent in pieces
 * (0x6385F0) while each piece moves some -- a datagram channel's ring normalised first, so a
 * piece never wraps -- and the channel's send time taken whenever one did */
int rt_msg_client_recv_circ_buf(rt_msg_client_comm *pComm)
{
    unsigned char bWrapped;
    unsigned char *pData = 0;
    unsigned int nUsed = 0;
    unsigned short nSent = 0;
    rt_circ_buf *pBuf;
    int result;

    if (data_00655614 != 1)
        return 5;
    if (func_00635898(pComm) != 1)
        return 2;
    pBuf = &pComm->sendBuf;
    if ((pBuf != 0) != 1)
        return 2;
    if (rt_circ_buf_get_used_nowrap(pBuf, &nUsed) != 0)
        return 21;
    if (nUsed != 0) {
        if (pComm->nKind == 1) {
            bWrapped = 1;
            if (rt_circ_buf_is_wrapped(pBuf, &bWrapped) != 0)
                return 21;
            if (bWrapped == 1) {
                if (rt_circ_buf_normalize_with_buf(pBuf, 1, gpScratchBuffer) != 0)
                    return 21;
                if (rt_circ_buf_get_used_nowrap(pBuf, &nUsed) != 0)
                    return 21;
            }
        }
        do {
            if (rt_circ_buf_get_start_used_ptr(pBuf, &pData) != 0)
                return 21;
            nSent = 0;
            result = func_006385F0(pComm, pData, nUsed, &nSent);
            if (result != 0)
                return result;
            if (nSent != 0) {
                if (rt_time_start(&pComm->tLastSend) != 0)
                    return 24;
            }
            if (rt_circ_buf_free(pBuf, nSent) != 0)
                return 21;
            if (rt_circ_buf_get_used_nowrap(pBuf, &nUsed) != 0)
                return 21;
        } while (nUsed != 0 && nSent != 0);
    }
    return 0;
}

/* SOCOM 1's is_valid_outgoing_msg, the message types an application may send: SOCOM 1's ten in its
 * order, 21-23, 27 and 29 added */
int rt_msg_client_is_valid_outgoing_msg(int nType)
{
    if (nType == 2 || nType == 11 || nType == 5 || nType == 14 || nType == 15 || nType == 16 ||
        nType == 0 || nType == 21 || nType == 22 || nType == 23 || nType == 13 || nType == 18 ||
        nType == 27 || nType == 3 || nType == 29)
        return 1;
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

/* how many bytes of whole messages (each its header, its body and, when the type's 0x80 bit says
 * so, a four-byte trailer) fit in nMax -- the buffer whole when it fits */
unsigned char func_00638500(unsigned char *pMsg, unsigned int nLength, unsigned int nMax, unsigned int *pnCount)
{
    rt_msg_client_header header;
    unsigned short nHeaderLength = 0;
    unsigned char *pEnd;
    unsigned short nSize;
    unsigned int nCount;
    unsigned char result;

    if (pnCount == 0)
        return 2;
    *pnCount = 0;
    if (pMsg == 0)
        return 2;
    pEnd = pMsg + nLength;
    if (nLength <= nMax) {
        *pnCount = nLength;
        return 0;
    }
    while (pMsg < pEnd) {
        result = rt_msg_client_get_header(pMsg, &header, &nHeaderLength);
        if (result != 0)
            return result;
        nSize = nHeaderLength + header.nLength;
        if (header.nLength != 0 && (header.nType & 0x80))
            nSize += 4;
        nCount = *pnCount + nSize;
        if (nCount > nMax)
            return 0;
        pMsg += nSize;
        *pnCount = nCount;
    }
    return 0;
}

extern int rt_comm_send(int nChannelId, void *pData, unsigned short nLength, unsigned short *pnSent);
extern int rt_comm_send_to(int nChannelId, char *szAddress, unsigned short nPort, void *pData,
                           unsigned short nLength, unsigned short *pnSent);

/* one piece of a send: a stream channel sends at most 4096 bytes, a datagram channel (kind 1) the
 * whole messages that fit in 584 to its address and port. OPEN (17 words): the target's final
 * `return 0` is a join the nothing-to-send path reaches (`move v0,zero` kept; without one 2.96 folds
 * `if (result) return result; return 0;` into `return result`, 3 words short) -- the label here is a
 * stand-in for SCEA's shape, not known; nKind then takes v1 where the target's is v0, and send_to's
 * channel and port loads come in the other order. The internals' results are unsigned char (no `andi`
 * on any returned result: get_connection, get_valid_connection, get_header and 0x638500 so typed). */
int func_006385F0(rt_msg_client_comm *pComm, unsigned char *pMsg, unsigned int nLength, unsigned short *pnSent)
{
    unsigned int nSend = 0;
    int nCommResult;
    unsigned char result;

    if (pnSent == 0)
        return 2;
    *pnSent = 0;
    if (func_00635898(pComm) != 1)
        return 2;
    if (pMsg == 0)
        return 2;
    if (pComm->nKind == 0) {
        nSend = nLength;
        if (nLength > 4096)
            nSend = 4096;
        nCommResult = rt_comm_send(pComm->nChannelId, pMsg, nSend, pnSent);
    } else {
        if (pComm->nKind != 1)
            return 3;
        result = func_00638500(pMsg, nLength, 584, &nSend);
        if (result != 0)
            return result;
        if (nSend == 0)
            goto done;
        nCommResult = rt_comm_send_to(pComm->nChannelId, pComm->szAddress, pComm->nPort, pMsg, nSend, pnSent);
    }
    result = rt_msg_client_map_comm_error(nCommResult);
    if (result != 0)
        return result;
done:
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

