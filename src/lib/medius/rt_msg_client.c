/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* rt_msg_client: the RT library's message client (OVL2 0x632620-0x638880, SOCOM 1's object at
 * 0x40C000; rt_msg/src/rt_msg_client/rt_msg_client.c 1.209, "rt_msg_client version: 1.08.0204"),
 * SCEA's Medius/DME network stack, prebuilt with ee-gcc 2.96 at -O2 -G0 (README "The GCC front").
 * Its public functions are locked wrappers over the internal ones (rt_msg_client_i*): the client
 * started up, the client's mutex locked, the internal call, the mutex unlocked, the call's result
 * returned unless the unlock failed (26). The started test compares the flag with the result
 * variable's first value -- the target keeps that 1 in the register the result later takes
 * (s0); a literal gives a temporary (ccsweep: 1 of 7 shapes, SOCOM 1's copies the same).
 * Names are SOCOM 1's (s1:exact); argument types are the entry's narrowing. Only the functions
 * written so far are here, in image order. */

#define RT_MSG_CLIENT_LOCK_FAILED   26

extern unsigned char gbMutexInitialized;
extern char gMsgClientMutex[];

extern int rt_mutex_lock(void *mutex, int wait);
extern int rt_mutex_unlock(void *mutex, int wait);

extern int rt_msg_client_udp_shutdown(void);
extern int func_00633960(int);
extern int func_00633A08(int, int);
extern int rt_msg_client_idisconnect(unsigned short);
extern int rt_msg_client_idisconnect_all(void);
extern int func_00634078(unsigned short, unsigned char);
extern int rt_msg_client_iudp_disable_heartbeat(void);
extern int rt_msg_client_iupdate_all(unsigned char);
extern int rt_msg_client_isend_target(unsigned short, unsigned short, int, int, unsigned short);
extern int rt_msg_client_isend_client_set(unsigned short, unsigned short, unsigned short, int, int, int, unsigned short);
extern int func_00634278(int, int, unsigned short);
extern int rt_msg_client_iget_buffer_status(unsigned short, int, int, int, int);
extern int func_00634508(int);
extern int func_00634550(int);
extern int rt_msg_client_iget_host_by_name(int, int);
extern int rt_msg_client_iget_connection_ip(unsigned short, int);
extern int func_00634660(unsigned short, unsigned char);
extern int rt_msg_client_iget_metrics(int, int);
extern int func_006347C8(unsigned short, int);
extern int func_00634850(unsigned short, int);
extern int rt_msg_client_iudp_set_default_options(int);
extern int rt_comm_set_default_options(int);
extern int rt_msg_client_idisplay_comm_state(void);
extern int func_006349A8(unsigned short, int);

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

int rt_msg_client_udp_set_default_options(int arg0)
{
    int result = 1;

    if (gbMutexInitialized != result)
        return RT_MSG_CLIENT_LOCK_FAILED;
    if (rt_mutex_lock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    result = rt_msg_client_iudp_set_default_options(arg0);
    if (rt_mutex_unlock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    return result;
}

int func_006335E8(int arg0)
{
    int result = 1;

    if (gbMutexInitialized != result)
        return RT_MSG_CLIENT_LOCK_FAILED;
    if (rt_mutex_lock(gMsgClientMutex, 0) != 0)
        return RT_MSG_CLIENT_LOCK_FAILED;
    result = rt_comm_set_default_options(arg0);
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

