/* zNetCnf.cpp -- the network's start-up on Sony's ezcnf/ezctl libraries (OVL1 0x1E8530-0x1E8F00,
 * libezcnf.c and libezctl.c): the IOP's network modules loaded, eznetctl opened, the adaptor's
 * link and state polled each frame (the lobby told of a pulled cable), the interfaces listed, and
 * the inet RPC client closed before an IOP reboot. OVL1 0x30BD40-0x30C75F, before trc.cpp -- the
 * unit SOCOM 1's zNetCnf.cpp held (its netcnf state machines, also followed by the TRC functions in
 * the demo); the name is SOCOM 1's by that place, the functions SOCOM II's, so they keep the map's
 * placeholders. Defined in reverse of their image order (-inline deferred). */

#include <string.h>
#include "znet.h"
#include "../zonline/zonline.h"
#include "../zsys/zsys_public.h"

extern "C" {
/* Sony's libinet RPC (prebuilt, OVL1) and ezcnf/ezctl (OVL1's start) */
int func_00246558(void *cd, void *buf, int *ids, int n);   /* sceInetGetInterfaceList */
int func_002465E8(void *cd, void *buf, int id, int code, void *ptr, int len);   /* sceInetInterfaceControl */
void func_00246A38(void *cd, int a, int b, int c);   /* the RPC client bound */
void func_00246B10(void *cd);   /* and unbound */
void func_001E8530(void);   /* (ezcnf: started) */
void func_001E85F8(void);   /* (ezcnf: stopped) */
void func_001E89E8(void);   /* (ezctl: initialised) */
void func_001E8AB0(void);   /* (ezctl: closed) */
int func_001E8B38(void *cnf);   /* (ezctl: opened on the configuration) */
void func_001E8D20(void *ctl);   /* (ezctl: the state read) */
void func_001E8E38(int a);   /* (ezctl: up) */
void func_001E8EB0(int a);   /* (ezctl: down) */
void func_001BCE48(void);
void func_00363F60(char *out, int size, int id, const char **args, int n);   /* a UI text formatted */
int sceSifMInitRpc(unsigned int mode);

extern unsigned char data_00456780[];   /* the inet RPC client */
extern unsigned char data_00455F80[];   /* and its buffer */
extern unsigned char data_004567C0[];   /* the network configuration */
extern char data_004575C0[];   /* its vendor (and the text after it) */
extern char data_004576C0[];   /* its product */
extern int data_00457AC0;   /* its connection type (2: PPP) */
extern int data_00457ACC;   /* its interface type */
extern unsigned char data_00457AE5;
extern char data_003DF460[];   /* netcnf.irx's arguments on a CD boot (the icon files) */
extern char data_003DF4C0[];   /* and on the host */
extern char data_003DF518[];   /* inetctl.irx's ("-no_auto") */
extern int data_00458080;   /* eznetcnf.irx's module */
extern unsigned char data_00458088;   /* eznetcnf started */
extern int data_00458090;   /* the link's last state */
extern int data_00458098;   /* the time it changed */
}

int zSysLoadIRX(char *name, int argc, char *argp, bool start);

/* eznetctl's state (0x457B00) */
struct EzCtl {
    int m_0;   // 0x0
    int m_id;   // 0x4: > 0 when open
    int m_link;   // 0x8: -1 none, 0 cable out, 1 in
    int m_state;   // 0xc: 3 up, 5 an error
    char m_msg[1];   // 0x10: the error's text
};
extern "C" EzCtl data_00457B00;

class CIOPManager {
public:
    enum RebootPhase { PreReboot = 0 };
    enum RebootMode { FirstBoot = 0 };
    typedef bool (*RebootCallback)(RebootPhase phase, RebootMode mode, void *data);
    int RegisterRebootCallback(RebootCallback cb, void *data);
};
extern CIOPManager theIOPManager;

extern "C" bool func_0030C630(CIOPManager::RebootPhase phase, CIOPManager::RebootMode mode, void *data);
extern "C" int func_0030C430();
extern "C" int func_0030C1B0();

/* eznetcnf started once (OVL1 0x30C6E0) */
extern "C" int func_0030C6E0()
{
    if (!data_00458088) {
        func_0030C430();
        if ((data_00458080 = zSysLoadIRX("run/irx/eznetcnf.irx", 0, 0, true)) < 0)
            return 0;
        func_001E8530();
        data_00458088 = 1;
    }
    return 1;
}

/* (OVL1 0x30C6A0) */
extern "C" void func_0030C6A0()
{
    if (data_00458088) {
        func_001E85F8();
        data_00458088 = 0;
    }
}

/* before an IOP reboot: eznetctl closed (taken down first when it was up), the RPC client unbound
 * (OVL1 0x30C630) */
extern "C" bool func_0030C630(CIOPManager::RebootPhase phase, CIOPManager::RebootMode mode, void *data)
{
    if (data_00457B00.m_id > 0) {
        func_001E8D20(&data_00457B00);
        if (data_00457B00.m_state == 3)
            func_001E8EB0(0);
        func_001E8AB0();
    }
    func_00246B10(data_00456780);
    func_001BCE48();
    return false;
}

/* netcnf.irx (with the icons' arguments), inet.irx and inetctl.irx loaded (ours) */
static inline bool LoadNetCnfModules()
{
    if (zSys.isCdBoot) {
        if (zSysLoadIRX("run/irx/netcnf.irx", 0x56, data_003DF460, true) < 0)
            return false;
    } else if (zSysLoadIRX("run/irx/netcnf.irx", 0x54, data_003DF4C0, true) < 0)
        return false;
    if (zSysLoadIRX("run/irx/inet.irx", 0, 0, true) < 0)
        return false;
    char args[] = "-no_auto";

    if (zSysLoadIRX("run/irx/inetctl.irx", 9, args, true) < 0)
        return false;
    return true;
}

/* every network module loaded, the RPC client bound, its reboot handler registered (OVL1
 * 0x30C430). Pinned, as the next: the target calls both out of line */
#pragma auto_inline off
extern "C" int func_0030C430()
{
    if (!LoadNetCnfModules())
        return 0;
    if (zSysLoadIRX("run/irx/ppp.irx", 0, 0, true) < 0)
        return 0;
    if (zSysLoadIRX("run/irx/pppoe.irx", 0, 0, true) < 0)
        return 0;
    zSysLoadIRX("run/irx/smap.irx", 0, 0, false);
    if (zSysLoadIRX("run/irx/msifrpc.irx", 0, 0, true) < 0)
        return 0;
    if (zSysLoadIRX("run/irx/libnetb.irx", 8, "-decode", true) < 0)
        return 0;
    sceSifMInitRpc(0);
    func_00246A38(data_00456780, 0x800, 0x2000, 0x20);
    theIOPManager.RegisterRebootCallback(func_0030C630, 0);
    return 1;
}

/* the modules the configuration needs loaded, the RPC client bound (OVL1 0x30C1B0) */
extern "C" int func_0030C1B0()
{
    if (!LoadNetCnfModules())
        return 0;
    if (data_00457AC0 == 2) {
        if (zSysLoadIRX("run/irx/ppp.irx", 0, 0, true) < 0)
            return 0;
        if (data_00457AE5 && data_00457AE5 != 0xFF &&
            zSysLoadIRX("run/irx/pppoe.irx", 0, 0, true) < 0)
            return 0;
    }
    if (data_00457ACC != 2 || (data_00457AE5 && data_00457AE5 != 0xFF)) {
        if (!strncmp(data_004575C0, "SCE", 3))
            zSysLoadIRX("run/irx/smap.irx", 0, 0, false);
    }
    if (zSysLoadIRX("run/irx/msifrpc.irx", 0, 0, true) < 0)
        return 0;
    if (zSysLoadIRX("run/irx/libnetb.irx", 8, "-decode", true) < 0)
        return 0;
    sceSifMInitRpc(0);
    func_00246A38(data_00456780, 0x800, 0x2000, 0x20);
    theIOPManager.RegisterRebootCallback(func_0030C630, 0);
    return 1;
}
#pragma auto_inline reset

/* the network started: the modules, eznetctl opened and brought up; a failure told to the lobby
 * (OVL1 0x30C090) */
extern "C" bool func_0030C090()
{
    bool ok = false;

    if (func_0030C1B0()) {
        if (zSysLoadIRX("run/irx/eznetctl.irx", 0, 0, true) < 0)
            return 0;
        func_001E89E8();
        func_001E8EB0(0);
        data_00457B00.m_id = func_001E8B38(data_004567C0);
        if (data_00457B00.m_id < 0) {
            const char *args[2] = {0};
            char text[256];

            args[0] = data_004575C0;
            args[1] = data_004576C0;
            func_00363F60(text, sizeof(text), 0x429, args, 2);
            LobbyEvent("MP_MEDIUS_ERROR", text);
            func_001E8AB0();
        } else {
            func_001E8E38(0);
            ok = true;
        }
    }
    return ok;
}

/* eznetctl closed, taken down first when it was up (OVL1 0x30C030) */
extern "C" bool func_0030C030()
{
    bool closed = false;

    if (data_00457B00.m_id > 0) {
        func_001E8D20(&data_00457B00);
        if (data_00457B00.m_state == 3)
            func_001E8EB0(0);
        func_001E8AB0();
        closed = true;
    }
    return closed;
}

/* the frame's poll: eznetctl's state and the cable's, the lobby told of an error or a pulled cable;
 * whether anything changed (OVL1 0x30BF20; CZNetwork::zNetUpdate calls it) */
extern "C" bool func_0030BF20()
{
    bool changed = false;

    if (data_00457B00.m_id > 0) {
        func_001E8D20(&data_00457B00);
        switch (data_00457B00.m_state) {
        case 0:
        case 1:
        case 2:
        case 4:
            break;
        case 3:
            changed = true;
            break;
        case 5:
            LobbyEvent("MP_MEDIUS_ERROR", data_00457B00.m_msg);
            changed = true;
            break;
        }
        switch (data_00457B00.m_link) {
        case -1:
            break;
        case 0:
            changed = true;
            LobbyEvent("MP_MEDIUS_ERROR", "The network cable is disconnected.  \xa6 Continue");
            theNetwork.m_10a = 0;
            break;
        case 1:
            theNetwork.m_10a = 1;
            break;
        }
    }
    return changed;
}

/* the time since the cable's state last changed (OVL1 0x30BE80) */
extern "C" int func_0030BE80()
{
    int link;

    if (data_00457B00.m_id > 0 &&
        !func_002465E8(data_00456780, data_00455F80, data_00457B00.m_id, 0x200, &link, 4) &&
        data_00458090 != link) {
        theNetwork.zNetGetTime();
        data_00458098 = theNetwork.zNetGetTime();
        data_00458090 = link;
    }
    return theNetwork.zNetGetTime() - data_00458098;
}

/* an interface's id, flags, vendor and product (0x240 bytes; the names ours) */
struct NetIfInfo {
    int m_id;   // 0x0
    int m_flags;   // 0x4
    char m_vendor[256];   // 0x8
    char m_product[256];   // 0x108
    unsigned char _208[0x240 - 0x208];
};

/* the interfaces listed, at most `max`; their count (OVL1 0x30BD40).
 * OPEN, 2 instructions, scheduling: the target zeroes the counter in the loop guard's delay slot
 * (`beqz at; move s2,zero`) where MW puts it ahead of the branch; the pointer's and the counter's
 * registers and the pointer's sinking need `int i; int *id;` and `for (i = 0, id = ids; ...)`
 * (the other orders swap registers or hoist the pointer). */
extern "C" int func_0030BD40(NetIfInfo *info, int max)
{
    int ids[256];
    int n;

    n = func_00246558(data_00456780, data_00455F80, ids, 256);
    if (n < 0)
        return 0;
    if (max < n)
        n = max;
    if (n > 0) {
        int i;
        int *id;

        for (i = 0, id = ids; i < n; i++, info++, id++) {
            memset(info, 0, sizeof(*info));
            info->m_id = *id;
            func_002465E8(data_00456780, data_00455F80, info->m_id, 8, &info->m_flags, 4);
            func_002465E8(data_00456780, data_00455F80, info->m_id, 2, info->m_vendor, 256);
            func_002465E8(data_00456780, data_00455F80, info->m_id, 3, info->m_product, 256);
        }
    }
    return n;
}
