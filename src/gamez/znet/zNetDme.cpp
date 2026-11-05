/* zNetDme.cpp -- the network: CZNetwork over Sony's DME library -- the remote objects and their
 * callbacks, the application messages, the per-frame update (SOCOM 1's zNetDme.cpp, the press
 * beta's file references name it: OVL1 from 0x30CA50 -- SOCOM II's additions, defined after
 * SOCOM 1's last function and so ahead of it, read theNetwork's 0x1198-0x11A0 (after trc.cpp's
 * TRC_PowerOffHandler) -- then zNetRemoteObjectUpdateCallback at 0x30CDF0, SOCOM 1's first
 * function; its static initialiser 0x402800 builds theNetwork and registers its
 * destructor 0x30E410, the unit's last function, as SOCOM 1's). The DME calls are OVL2 address
 * placeholders, their shapes the call sites'. Only some of it so far. Defined in reverse of their
 * image order (-inline deferred). */

#include <stdio.h>
#include <string.h>
#include "znet.h"
#include "../zonline/zonline.h"

void *_malloc(unsigned int size, const char *file, int line);
void _free(void *p, const char *file, int line);

extern "C" {
/* the DME library (OVL2) */
int NetGetObject(NetObject **obj, int index);
int func_00620F50(int index, NetCreateObjectCallback create, NetUpdateObjectCallback update,
                  NetDestroyObjectCallback destroy);   /* NetRegisterRemoteObjectCallback */
int func_00621458(char type, int conn, int to, int msg, int size, unsigned char *data);   /* NetSendApplicationMessage */
int func_006210D0(int *type, NetMessageCallback cb);   /* an application message type registered */
int func_00622CC0(int *count, int conn);   /* the connection's object count */
int func_006220F0(int *index, NetOwnershipStatus own, NetObjectLifespan life, unsigned int a, int b,
                  unsigned int c, int d, void *data, char *name, int e);   /* NetCreateObject */
int NetUpdate();
int NetGetLocalTime(int *t);
int NetGetTime(int *t);
int MediusGetLastNetUpdateError();   /* the Medius library's last update error */

int func_003006D0();   /* the Medius connection updated (the lobby's) */
void func_0030BF20();   /* (the IOP stretch's per-frame update) */
}

/* the LAN lobby (zonline.h's CZLANLobby, 0x44FE10: IsActive tests its 'ComeFromLan' valve) */

void zRemoteDelayedAction(CZNetwork::TypeRTimeFixStruct *fix);

/* the network (0x45A0C0): the unit's one dynamically initialised static -- its initialiser 0x402800
 * builds it and registers the implicit destructor 0x30E410, the unit's last function */
CZNetwork theNetwork;

/* a field's broadcast schedule, by a threshold (OVL1 0x30E3F0) or by a compare function (0x30E3D0):
 * SOCOM 1's one constructor (rate, method, double), defined first in its zNetDme.cpp and so its last
 * own function; fts_remote.cpp's eighteen file statics are built with them */
CZBroadcastSchedule::CZBroadcastSchedule(unsigned int rate, NetThresholdMethod method, float threshold,
                                         unsigned char c)
{
    m_rate = rate;
    m_method = method;
    m_threshold = threshold;
    m_c = c;
}

CZBroadcastSchedule::CZBroadcastSchedule(unsigned int rate, NetThresholdMethod method,
                                         bool (*compare)(NetFieldCompare *), unsigned char c)
{
    m_rate = rate;
    m_method = method;
    m_compare = compare;
    m_c = c;
}

/* the DME's join: the game connection's session joined by name, with the object and stream
 * counts, the client events to zNetClientCallback (SOCOM II's parameter block; the names ours) */
struct NetClientEventInfo {
    int m_event;   // 0x0: 0 joined, 1 left, 0xFFFFFF none
    int m_client;   // 0x4
    int m_8;   // 0x8
    CZNetwork *m_net;   // 0xc
};
struct NetJoinParams {
    int m_0;   // 0x0
    int m_conn;   // 0x4
    char m_name[12];   // 0x8
    int m_objects;   // 0x14
    int m_streams;   // 0x18
    int m_1c;   // 0x1c: 2
    int m_20;   // 0x20
    int m_24;   // 0x24
    void (*m_cb)(NetClientEventInfo *);   // 0x28
    CZNetwork *m_user;   // 0x2c
    int m_30;   // 0x30
    int m_34;   // 0x34
};   // 0x38
struct NetJoinResult {
    int m_0;
    int m_4;
};

/* Sony's libinet (prebuilt, OVL1) */
struct sceInetAddress {
    int data[4];
};
extern "C" {
int sceInetName2Address(void *cd, void *net_buf, int flags, sceInetAddress *paddr, const char *name,
                        int ms, int nretry);
int sceInetAddress2String(void *cd, void *net_buf, char *name, int len, sceInetAddress *paddr);
extern unsigned char data_00456780[];   /* the inet RPC client */
extern unsigned char data_00455F80[];   /* and its buffer */

void func_00621DB8(NetJoinParams *p);   /* NetSetDefaultJoinParams */
int func_00621E50(NetJoinParams *p, NetJoinResult *r);   /* NetJoin */
const char *func_002FA7E0(CZOnlineLobby *lobby);   /* the game's session name (the lobby's) */
}

/* --- the DME's start (SOCOM 1's zNetInitialize, SOCOM II's newer library) ------------------- */

/* a message from the server, as the DME hands it on in parts (the names ours) */
struct NetServerMessage {
    int m_0;   // 0x0
    unsigned char m_level;   // 0x4: below 128 urgent, below 192 a warning
    unsigned char _5[0x10 - 0x5];
    unsigned char m_last;   // 0x10: the message's last part
    const char *m_text;   // 0x14
};

/* the DME's start-up parameters (SOCOM II's library; 0x20 bytes) */
struct NetInitializeParams {
    int m_0;   // 0x0
    int m_4;   // 0x4: 10472
    void *m_state;   // 0x8: the lobby's DME state
    int m_c;   // 0xc
    int m_10;   // 0x10
    void (*m_message)(NetServerMessage *msg);   // 0x14
    int m_18;   // 0x18
    int m_1c;   // 0x1c
};

/* what the start-up answers (24 bytes) */
struct NetInitializeResult {
    int m_0;   // 0x0
    int m_err;   // 0x4
    int m_8[4];
};

extern "C" {
void NetSetDefaultInitializeParams(NetInitializeParams *p);
int NetInitialize(NetInitializeParams *p, NetInitializeResult *r);
int NetClose();
void func_00623330(int a, int b);   /* (the DME's buffers sized) */
void func_00622AD8(char *ip);   /* (the DME: our address as text -- SOCOM 1's NetGetMyIpAddress) */
extern char data_0045B330[];   /* the server's message so far */
}

/* the server's message, a part at a time: kept until its last part and then shown, its level
 * deciding how (SOCOM II's; OVL1 0x30DCE0, the DME's message callback zNetInitialize registers).
 * The part's fit is a bool of its own (the target materialises the compare into v0 before the
 * branch; tested in the ternary itself MW branches on it), the level an int (each case a `li`) */
extern "C" void func_0030DCE0(NetServerMessage *msg)
{
    const char *text = msg->m_text;

    if (strlen(data_0045B330) + strlen(text) < 1000) {
        strcat(data_0045B330, text);
    } else {
        bool fits = strlen(text) < 1000;

        strcpy(data_0045B330, fits ? text : "[Server message too long]");
    }
    if (msg->m_last) {
        int level = 0;

        if (msg->m_level < 128)
            level = 2;
        else if (msg->m_level < 192)
            level = 1;
        theGame.GetCurrentState()->DisplayMessage(data_0045B330, "Server", 0, 0, 0, level, 0, 1);
        data_0045B330[0] = 0;
    }
}

/* the DME started: its defaults, the lobby's DME state and the server's messages to the callback
 * above, its buffers sized for a LAN game or for the lobby, the network's start flag set (else the
 * DME closed again), our address fetched and the objects' changed flags cleared; the error
 * returned (SOCOM 1's; OVL1 0x30DBB0: UIDMEInit calls it on theNetwork) */
int CZNetwork::zNetInitialize()
{
    NetInitializeParams p;
    NetInitializeResult r = {0};

    NetSetDefaultInitializeParams(&p);
    p.m_4 = 10472;
    p.m_state = theLobby.DMEInit();
    p.m_message = func_0030DCE0;
    p.m_18 = 0;
    NetInitialize(&p, &r);
    if (data_0044FE10.IsActive())
        func_00623330(0x8000, 0x8000);
    else
        func_00623330(8192, 0x8000);
    if (!r.m_err)
        theNetwork.m_network_init = 1;
    else
        NetClose();
    m_my_ip[0] = 0;
    func_00622AD8(m_my_ip);
    for (int i = 0; i < 4096; i++)
        m_ObjectFieldUpdate[i] = 0;
    return r.m_err;
}

/* --- the IOP's network modules (SOCOM 1's zNetLoadIRX) ------------------------------------- */

class CSnd {
public:
    static void Close();
    static void UIOpen();
    static void WaitForCDRead();
};

class CIOPManager {
public:
    enum RebootPhase { PreReboot = 0 };
    enum RebootMode { FirstBoot = 0 };
    typedef bool (*RebootCallback)(RebootPhase phase, RebootMode mode, void *data);
    int RegisterRebootCallback(RebootCallback cb, void *data);
};
extern CIOPManager theIOPManager;

int zSysLoadIRX(char *name, int argc, char *argp, bool start);

extern "C" {
int sceSifMInitRpc(unsigned int mode);
void sceLibnetInitialize(void *cd, int a, int b, int c);   /* the RPC client bound */
bool func_0030C090();   /* (zNetCnf.cpp's: the network configuration's own start-up) */
bool func_0030E260(CIOPManager::RebootPhase phase, CIOPManager::RebootMode mode, void *data);   /* (the inet client closed before a reboot) */
extern bool data_0045B320;   /* the modules up */
}

/* the network's IOP modules loaded: the sound's CD reads finished and its streams closed, then --
 * unless the configuration's own start-up does it -- inet.irx, netcnf.irx with the icon files as
 * its arguments, inetctl.irx with the connection's options and configuration file, ppp.irx for a
 * modem, an986.irx for the USB adaptor or smap.irx for the network adaptor (the IOP reboot callback
 * registered with it), msifrpc.irx and libnetb.irx, and the RPC client bound; the sound's interface
 * opened again and whether the modules are up returned, false at the first module that fails
 * (SOCOM 1's zNetLoadIRX, which returned at once when they were up; OVL1 0x30DEE0:
 * CSMInitNetwork::WaitForNetwork calls it on theNetwork). The option, count and file locals are
 * the icon arguments' cursor and count again (s1/s0 throughout), the cursor set before the first
 * icon name is fetched (in that call's delay slot) */
bool CZNetwork::zNetLoadIRX()
{
    CSnd::WaitForCDRead();
    CSnd::Close();
    if (m_bNetCnfRequired)
        data_0045B320 = func_0030C090();
    else {
        char name[256];
        char args[256] = {0};
        /* one cursor and one count serve both argument lists (the target keeps them in s1/s0
         * throughout; the configuration file's name is the third) */
        char *cnf;
        char *p;
        int n;

        if (zSysLoadIRX("run/irx/inet.irx", 0, 0, true) < 0)
            return false;
        p = args;
        CFileIO::PS2FileName("RUN/ICONS/NETCNF/SYS_NET.ICO", name, false);
        n = sprintf(p, "icon=%s", name);
        p += n + 1;
        CFileIO::PS2FileName("RUN/ICONS/NETCNF/ICON.SYS", name, false);
        n += sprintf(p, "iconsys=%s", name);
        if (zSysLoadIRX("run/irx/netcnf.irx", n, args, true) < 0)
            return false;
        if (m_connection_type == USB_MODEM || m_connection_type == NA_MODEM) {
            p = "-verbose\0-no_decode";
            cnf = "net_ex.cnf";
            n = 20;
        } else if (m_connection_type == USB_ETHERNET) {
            p = "-no_decode";
            cnf = "net003.cnf";
            n = 11;
        } else if (m_connection_type == NA_ETHERNET) {
            p = "-no_decode";
            cnf = "net005.cnf";
            n = 11;
        }
        memcpy(args, p, n);
        CFileIO::PS2FileName(cnf, name, false);
        strcpy(args + n, name);
        if (zSysLoadIRX("run/irx/inetctl.irx", n + strlen(name) + 1, args, true) < 0)
            return false;
        if (m_connection_type == USB_MODEM || m_connection_type == NA_MODEM) {
            if (zSysLoadIRX("run/irx/ppp.irx", 0, 0, true) < 0)
                return false;
        }
        if (m_connection_type == USB_ETHERNET) {
            if (zSysLoadIRX("run/irx/an986.irx", 0, 0, true) < 0)
                return false;
        } else if (m_connection_type == NA_ETHERNET || m_connection_type == NA_MODEM) {
            if (zSysLoadIRX("run/irx/smap.irx", 0, 0, true) < 0)
                return false;
            theIOPManager.RegisterRebootCallback(func_0030E260, 0);
        }
        if (zSysLoadIRX("run/irx/msifrpc.irx", 0, 0, true) < 0)
            return false;
        if (zSysLoadIRX("run/irx/libnetb.irx", 0, 0, true) < 0)
            return false;
        sceSifMInitRpc(0);
        sceLibnetInitialize(data_00456780, 0x800, 0x2000, 0x20);
        data_0045B320 = true;
    }
    CSnd::UIOpen();
    return data_0045B320;
}

/* a host name's address as text, 30 when it cannot be had (SOCOM II's; OVL1 0x30DE30: the server
 * list's connect, 0x2FDF60, asks it of theNetwork; the name ours) */
int CZNetwork::zNetLookupHost(const char *host, char *ip, int len)
{
    sceInetAddress addr;
    char name[256];
    int err = 0;

    strncpy(name, host, 64);
    if (!sceInetName2Address(data_00456780, data_00455F80, 0, &addr, name, 6000, 4)) {
        if (sceInetAddress2String(data_00456780, data_00455F80, ip, len, &addr))
            err = 30;
    } else
        err = 30;
    return err;
}

/* the network's work for a frame: the queued messages sent, the Medius connection (or, on a LAN,
 * the DME's) updated and the lobby ticked, the delayed actions played, the post-update callbacks
 * of every object whose fields changed, the clocks read; the error returned (SOCOM 1's; OVL1
 * 0x30DA00) */
int CZNetwork::zNetUpdate()
{
    int err = 0;

    if (m_bNetwork) {
        if (m_message_list.size())
            zNetFlushMessages();
        if (!data_0044FE10.IsActive()) {
            if (func_003006D0() < 0) {
                err = MediusGetLastNetUpdateError();
                theLobby.RecoverFromNetError(err);
            }
            theLobby.Tick();
        } else {
            err = NetUpdate();
            if (err)
                theLobby.RecoverFromNetError(err);
            theLobby.Tick();
        }
        func_0030BF20();
        if (m_fix_list.size())
            zNetPlayFixList();
        for (int i = 0; i < 4096; i++) {
            if (m_ObjectFieldUpdate[i]) {
                NetObject *obj;

                NetGetObject(&obj, i);
                if (obj && m_ObjectPostUpdateCallbackList[obj->m_type_index])
                    m_ObjectPostUpdateCallbackList[obj->m_type_index](obj->m_client, i, -1);
                m_ObjectFieldUpdate[i] = 0;
            }
        }
        int t = 0;

        if (!NetGetLocalTime(&t))
            m_local_time = t;
        int n = 0;

        if (!NetGetTime(&n))
            m_net_time = n;
    }
    return err;
}

/* the queued application messages sent and freed, the queue emptied (SOCOM 1's zNetUpdate did it
 * in place; OVL1 0x30D900, the name ours). The end into a local, then clear(): include/list's
 * begin() reads the first node through a reference to the sentinel, so &end_.next_ is computed
 * once at the walk and the clear's begin() loads through it (s0), as the target does */
#pragma auto_inline off
void CZNetwork::zNetFlushMessages()
{
    std::list<TypeAppMessage *>::iterator e = m_message_list.end();

    for (std::list<TypeAppMessage *>::iterator i = m_message_list.begin(); i != e; ++i) {
        TypeAppMessage *m = *i;

        func_00621458(m->m_type, m->m_conn, m->m_to, m->m_msg, m->m_size, m->m_data);
        _free(m, 0, 0);
    }
    m_message_list.clear();
}
#pragma auto_inline reset

/* a delayed action kept until the update plays it (SOCOM 1's; OVL1 0x30D870) */
void CZNetwork::AddToFixList(TypeRTimeFixStruct *fix)
{
    TypeRTimeFixStruct *f = new TypeRTimeFixStruct;

    *f = *fix;

    m_fix_list.push_back(f);
}

/* the delayed actions played and freed, the list emptied (SOCOM 1's zNetUpdate did it in place;
 * OVL1 0x30D7D0, the name ours), as zNetFlushMessages */
#pragma auto_inline off
void CZNetwork::zNetPlayFixList()
{
    std::list<TypeRTimeFixStruct *>::iterator e = m_fix_list.end();

    for (std::list<TypeRTimeFixStruct *>::iterator i = m_fix_list.begin(); i != e; ++i) {
        TypeRTimeFixStruct *f = *i;

        zRemoteDelayedAction(f);
        _free(f, 0, 0);
    }
    m_fix_list.clear();
}
#pragma auto_inline reset

/* the DME's sub-heap (zsys_heap.cpp's class; the instance at 0x45B328, made by the network's start-up
 * and wrapped at 0x30E300-0x30E3C8) */
class CHeap {
public:
    ~CHeap();
};
extern "C" {
extern CHeap *data_0045B328;
bool func_0030C030();   /* (zNetCnf.cpp's: eznetctl closed) */
}

/* the network closed: when it is up, the DME closed, its sub-heap deleted, and eznetctl closed
 * (SOCOM 1's zNetClose -- NetClose and the connected flag cleared, here the heap and the network
 * configuration too -- where SOCOM 1 has it, after zNetCreateObject; OVL1 0x30D770: the chat unit's
 * and zanim_menu.cpp's 0x30D770 calls on theNetwork). The flag stores ahead of the last call, the
 * connected one in its delay slot */
void CZNetwork::zNetClose()
{
    if (m_bNetwork) {
        NetClose();
        delete data_0045B328;
        data_0045B328 = 0;
        m_network_init = 0;
        m_bConnected = 0;
        func_0030C030();
    }
}

/* --- the connection to a game's server (SOCOM 1's zNetConnect, SOCOM II's newer DME) ---------- */

/* a server's addresses and where it is (the DME SDK's NetAddressList and NetConnectionInfo, the
 * chat unit's) */
struct NetAddressList {
    int AddressType0;   // 0x0
    char Address0[16];   // 0x4
    int BinaryAddress0;   // 0x14
    int AddressType1;   // 0x18
    char Address1[16];   // 0x1c
    int BinaryAddress1;   // 0x2c
};   // 0x30

struct NetConnectionInfo {
    NetConnectionInfo() {}

    int Type;   // 0x0
    NetAddressList AddressList;   // 0x4
    int WorldID;   // 0x34
    unsigned int ServerKey[16];   // 0x38
    char SessionKey[17];   // 0x78
    char AccessKey[17];   // 0x89
};   // 0x9c

/* a client connecting or leaving (the connect's two callbacks, fts_remote.cpp's 0x2BC620 and
 * 0x2BC530: SOCOM 1's cbClientConnect and cbClientDisconnect took the connection, the client and
 * its name, SOCOM II's read them through a pointer -- fts_remote.cpp's ClientConnectArgs) */
typedef void (*NetClientConnectCallback)(struct ClientConnectArgs *);

/* the voice's stream as the DME carries it (zNetVoice.cpp's 0x30FC70 fills it from the radio;
 * 0x38 bytes) */
struct NetVoiceStreamParams {
    unsigned char _0[0x38];
};

/* the DME's connect parameters (SOCOM II's library: 0x128 bytes, the server at 8; the names
 * ours) */
struct NetConnectParams {
    int m_0;   // 0x0
    int m_4;   // 0x4
    NetConnectionInfo m_info;   // 0x8
    unsigned char _a4[0xb4 - 0xa4];
    NetClientConnectCallback m_connect;   // 0xb4
    int m_b8;   // 0xb8
    NetClientConnectCallback m_disconnect;   // 0xbc
    unsigned char _c0[0xcc - 0xc0];
    NetVoiceStreamParams m_voice;   // 0xcc
    unsigned char _104[0x120 - 0x104];
    unsigned int m_flags;   // 0x120: 1 a spectator, 2 our clan's team; on a LAN the password's hash
    int m_124;   // 0x124
};   // 0x128

/* what the connect answers: its error, the connection's index */
struct NetConnectResult {
    int m_0;   // 0x0
    int m_err;   // 0x4
    int m_index;   // 0x8
};

class VoiceRadio;

extern "C" {
void NetSetDefaultConnectParams(NetConnectParams *p);
int NetConnect(NetConnectParams *p, NetConnectResult *r);
int NetDisconnect(int conn);
void func_0030FC70(VoiceRadio *radio, NetVoiceStreamParams *p);   /* the voice's stream (zNetVoice.cpp's) */
extern VoiceRadio *data_0045B718;   /* the radio (zNetVoice.cpp's) */
unsigned int func_003A59C0(const char *s);   /* a text's CRC-32 (CRC32String's wrapper) */
int func_002E9B30();   /* we are on a clan's team (the clan unit's) */
int func_002E9820();   /* and it is our clan's (the clan unit's) */
int func_002F6D00(CZOnlineLobby *lobby);   /* we join as a spectator (the lobby unit's) */
}

/* a variable's text when it holds one (zNetMediusClan.cpp's ZString) */
inline char *VarText(_zrdr *z)
{
    return z->type == 3 ? z->string : 0;
}

/* the game's server connected, the client events to the two callbacks: the room's connection info,
 * the voice's stream, on a LAN the game's password hashed into the flags -- elsewhere our clan's
 * team -- and a spectator's flag; the connection kept, dropped again when the connect failed; the
 * error returned (SOCOM 1's zNetConnect, which returned nothing; OVL1 0x30D380: 0x2B85C0 calls it) */
int CZNetwork::zNetConnect(NetClientConnectCallback connect, NetClientConnectCallback disconnect)
{
    NetConnectResult r;
    NetConnectParams p;

    memset(&p, 0, sizeof(p));
    NetSetDefaultConnectParams(&p);
    theLobby.SetConnectionInfo(&p.m_info);
    func_0030FC70(data_0045B718, &p.m_voice);
    p.m_connect = connect;
    p.m_disconnect = disconnect;
    if (data_0044FE10.IsActive()) {
        m_e8 = 0;
        CUIVariable *var = theUIVarManager.Find("PlayerGamePassword");

        if (var && var->type == 3)
            m_e8 = func_003A59C0(VarText(var)) & 0x7F0000FE;
        p.m_flags = m_e8;
    } else if (func_002E9B30() && func_002E9820()) {
        p.m_flags |= 2;
    }
    if (func_002F6D00(&theLobby))
        p.m_flags |= 1;
    NetConnect(&p, &r);
    int err = r.m_err;

    m_GameConnectionIndex = r.m_index;
    if (err)
        NetDisconnect(r.m_index);
    else
        m_bConnected = 1;
    m_bJoined = 0;
    return err;
}

/* the DME's other connect, a LAN game's (SOCOM II's library: its parameters 0x80 bytes, the
 * clients' limit at 0x1c, the voice's stream at 0x20, the flags at 0x74; the names ours) */
struct NetLanConnectParams {
    int m_0;   // 0x0
    int m_4;   // 0x4
    int m_8;   // 0x8
    NetClientConnectCallback m_connect;   // 0xc
    int m_10;   // 0x10
    NetClientConnectCallback m_disconnect;   // 0x14
    int m_18;   // 0x18
    int m_maxClients;   // 0x1c
    NetVoiceStreamParams m_voice;   // 0x20
    unsigned char _58[0x74 - 0x58];
    unsigned int m_flags;   // 0x74
    unsigned char _78[0x80 - 0x78];
};   // 0x80

struct LanGameInfo;   /* (below: the game-info exchange's) */

extern "C" {
int func_006218B8(NetLanConnectParams *p);   /* (the DME: its LAN connect's defaults) */
int func_00621950(NetLanConnectParams *p, NetConnectResult *r);   /* (and the connect) */
int func_00622FE8(int *status, int conn);   /* (a connection's state: 2 still connecting) */
}

/* the LAN game's server connected and waited for: the LAN lobby's game info (its clients' limit),
 * the voice's stream, the game's password hashed into the flags, a spectator's flag; the DME
 * updated until the connection is no longer under way, dropped again on an error (SOCOM II's;
 * OVL1 0x30D1B0: DMEConnectToGameServer calls it on theNetwork, where SOCOM 1's called
 * zNetConnect). The room's connection info is fetched and not used. The loop's two calls are
 * assigned inside their tests: the target branches on v0 and copies it to s1 in the delay slot,
 * where `err = f(); if (err)` branches on the copy */
extern "C" int func_0030D1B0(CZNetwork *net, NetClientConnectCallback connect,
                             NetClientConnectCallback disconnect)
{
    int status;
    NetConnectResult r;
    NetLanConnectParams p;
    NetConnectionInfo info;
    LANGameInfo game;
    int err;

    err = func_006218B8(&p);
    if (err)
        return err;
    theLobby.SetConnectionInfo(&info);
    data_0044FE10.GetGameInfo(&game);
    p.m_maxClients = game.m_maxPlayers;
    func_0030FC70(data_0045B718, &p.m_voice);
    p.m_connect = connect;
    p.m_disconnect = disconnect;
    p.m_4 = 0;
    net->m_e8 = 0;
    CUIVariable *var = theUIVarManager.Find("PlayerGamePassword");

    if (var && var->type == 3)
        net->m_e8 = func_003A59C0(VarText(var)) & 0x7F0000FE;
    p.m_flags = net->m_e8;
    if (func_002F6D00(&theLobby))
        p.m_flags |= 1;
    err = func_00621950(&p, &r);
    if (err)
        return err;
    net->m_bConnected = 1;
    net->m_bJoined = 0;
    net->m_GameConnectionIndex = r.m_index;
    status = 2;
    do {
        if ((err = NetUpdate()) != 0 && err != 12) {
            NetDisconnect(net->m_GameConnectionIndex);
            return err;
        }
        if ((err = func_00622FE8(&status, net->m_GameConnectionIndex)) != 0) {
            NetDisconnect(net->m_GameConnectionIndex);
            return err;
        }
    } while (status == 2);
    return err;
}

/* a client's event in the joined game, past to the join's callbacks unless it is the lobby's own
 * connection (SOCOM II's; OVL1 0x30D110: zNetJoin hands it to the DME; the name ours) */
void zNetClientCallback(NetClientEventInfo *info)
{
    CZNetwork *net = info->m_net;
    int client = info->m_client;

    if (client == theLobby.MediusConnectionIndex())
        return;
    switch (info->m_event) {
    case 0:
        net->m_join_cb(client, info->m_8);
        break;
    case 1:
        net->m_leave_cb(client, info->m_8);
        break;
    case 0xFFFFFF:
        break;
    }
}

/* the game connection's session joined, the callbacks kept for its client events; the DME's
 * answer returned (SOCOM 1's, whose NetJoin took the counts as arguments; OVL1 0x30D060) */
int CZNetwork::zNetJoin(NetCreateObjectCallback join, NetCreateObjectCallback leave)
{
    NetJoinParams p;
    NetJoinResult r;

    m_join_cb = join;
    m_leave_cb = leave;
    func_00621DB8(&p);
    m_bJoined = 0;
    p.m_conn = m_GameConnectionIndex;
    strncpy(p.m_name, func_002FA7E0(&theLobby), 12);
    p.m_name[11] = 0;
    p.m_objects = m_NetObjectCount;
    p.m_streams = m_DataStreamCount;
    p.m_1c = 2;
    p.m_cb = zNetClientCallback;
    p.m_user = this;
    int err = func_00621E50(&p, &r);

    if (err)
        return err;
    m_bJoined = 1;
    return err;
}

extern "C" int func_00623868(int conn);   /* (the DME: the session left -- SOCOM 1's NetLeave) */

/* the game's session left and the game's server disconnected, the flags cleared (SOCOM 1's
 * zNetLeaveAndDisconnect, between zNetSendApplicationMessage and zNetJoin in both builds; SOCOM 1's
 * disconnected only when the leave succeeded and reset two more words; OVL1 0x30D020) */
void CZNetwork::zNetLeaveAndDisconnect()
{
    func_00623868(m_GameConnectionIndex);
    m_bJoined = 0;
    NetDisconnect(m_GameConnectionIndex);
    m_bConnected = 0;
    m_bSessionMaster = 0;
    m_monitor_ok = 0;
}

/* the connection's object count, -1 when it cannot be had (the name ours). The success arm first:
 * MW lays the count's load on the fall-through and retargets the branch past the -1, which goes
 * into its delay slot (`bnez v0,join; li v1,-1; lw v1,n`) -- `f() ? -1 : n` lays the -1 arm first
 * (beqz, b: 8 bytes longer), and `!f() ? n : -1` does too: only `f() == 0` gives the target's (the
 * online lane's round 17) */
inline int ObjectCount(int conn)
{
    int n = -1;

    return func_00622CC0(&n, conn) == 0 ? n : -1;
}

/* a network object made, unless the game connection holds 256 already (8 then); a failure
 * handed to the lobby's recovery (SOCOM 1's; OVL1 0x30D670) */
int CZNetwork::zNetCreateObject(int *index, NetOwnershipStatus own, NetObjectLifespan life,
                                unsigned int a, int b, unsigned int c, int d, void *data, char *name,
                                int e)
{
    if (ObjectCount(theNetwork.m_GameConnectionIndex) == 256)
        return 8;
    int err = func_006220F0(index, own, life, a, b, c, d, data, name, e);

    if (err)
        theLobby.RecoverFromNetError(333);
    return err;
}

/* an application message type registered with the DME: its id handed back through `type`, the
 * handler kept; the name stays here (`move a0,a1; j; move a1,a2` -- SOCOM II's, called as
 * fts_remote.cpp's and the radio's registrations' last step for each type, the way
 * zNetRegisterRemoteObjectCallback closes an object type's; the name ours; OVL1 0x30CFF0) */
int CZNetwork::zNetRegisterMessage(unsigned int *type, NetMessageCallback handler, const char *name)
{
    return func_006210D0((int *)type, handler);
}

/* an application message sent now, or queued for the next update: copied whole, the payload
 * after the arguments (SOCOM 1's; OVL1 0x30CEF0) */
int CZNetwork::zNetSendApplicationMessage(char type, int conn, int to, int msg, int size,
                                          unsigned char *data, bool now)
{
    if (now)
        return func_00621458(type, conn, to, msg, size, data);
    TypeAppMessage *m = (TypeAppMessage *)_malloc(size + sizeof(TypeAppMessage) - 1, 0, 0);

    m->m_type = type;
    m->m_conn = conn;
    m->m_to = to;
    m->m_msg = msg;
    m->m_size = size;
    memcpy(m->m_data, data, size);
    m_message_list.push_back(m);
    return 0;
}

/* a remote object type's callbacks: creation and destruction to the DME, the update through ours,
 * which calls the type's own (SOCOM 1's; OVL1 0x30CE80) */
void CZNetwork::zNetRegisterRemoteObjectCallback(int index, NetCreateObjectCallback create,
                                                 NetUpdateObjectCallback update,
                                                 NetUpdateObjectCallback postupdate,
                                                 NetDestroyObjectCallback destroy)
{
    func_00620F50(index, create, zNetRemoteObjectUpdateCallback, destroy);
    m_ObjectUpdateCallbackList[index] = update;
    m_ObjectPostUpdateCallbackList[index] = postupdate;
}

/* a remote object's field updated: its type's callback called, the object marked for the
 * post-update pass (SOCOM 1's; OVL1 0x30CDF0) */
void CZNetwork::zNetRemoteObjectUpdateCallback(int client, int index, int field)
{
    NetObject *obj;

    NetGetObject(&obj, index);
    if (obj && theNetwork.m_ObjectUpdateCallbackList[obj->m_type_index])
        theNetwork.m_ObjectUpdateCallbackList[obj->m_type_index](obj->m_client, index, field);
    theNetwork.m_ObjectFieldUpdate[index] = 1;
}

/* --- SOCOM II's additions, defined after SOCOM 1's last function (so ahead of it in the image,
 * OVL1 0x30CA50-0x30CDEF): the LAN game-info exchange over two DME message types, and three
 * wrappers. The DME calls are OVL2 placeholders; the names of the functions are still the map's. */

/* the game's description a LAN host sends (400 bytes, the DME message type m_infoType; the fields
 * as 0x30CB90 registers them, the names ours) */
struct LanGameInfo {
    char m_name[21];   // 0x0
    int m_18;   // 0x18
    int m_1c;   // 0x1c: the request's first word
    short m_20;   // 0x20
    short m_22;   // 0x22
    short m_24;   // 0x24
    int m_28[8];   // 0x28
    int m_48;   // 0x48
    char m_desc[64];   // 0x4c
    char m_text[256];   // 0x8c
    unsigned char m_18c;   // 0x18c
};   // 0x190

/* the request: two words */
struct LanInfoRequest {
    int m_0;
    int m_4;
};

extern "C" {
void func_00620BD8(void);   /* a message type's description begun */
void func_00620C60(void *base, void *field, int size, int count);   /* a field described */
int func_00621178(int flags, void *to, int a, int type, int size, void *data);   /* a message sent */
int func_00622C28(int *client);   /* (the DME's: our client) */
int NetGetClient(void **client, int conn, int index);   /* a client of the connection */
void func_00623138(int a, int conn);
void NetSetMyClientObject(int a);
int func_0030BE80(void);   /* (the network start-up's: the time since the link changed) */
}

/* this console's client object set: the DME's own call, `this` unused (`j NetSetMyClientObject;
 * move a0,a1` -- SOCOM II's forwarder: SOCOM 1's ftsNetworkPlayerInit calls NetSetMyClientObject
 * itself at the point SOCOM II's 0x2B7DA4 calls this; the name ours, after the class's zNet* calls;
 * OVL1 0x30CDE0) */
void CZNetwork::zNetSetMyClientObject(int object)
{
    NetSetMyClientObject(object);
}

/* (OVL1 0x30CDD0) */
extern "C" void func_0030CDD0(CZNetwork *net, int a)
{
    func_00623138(a, net->m_GameConnectionIndex);
}

/* a client's halfword at +4 of the game connection, -1 when there is none (OVL1 0x30CD90) */
extern "C" int func_0030CD90(CZNetwork *net, int index)
{
    unsigned char *client = 0;

    NetGetClient((void **)&client, net->m_GameConnectionIndex, index);
    if (client)
        return *(unsigned short *)(client + 4);
    return -1;
}

/* (OVL1 0x30CD80) */
extern "C" void func_0030CD80(CZNetwork *net)
{
    func_0030BE80();
}

extern "C" int func_0030CA80(int a, int b, int c, void *data);
extern "C" int func_0030CA50(int a, int b, int c, void *data);

/* the two message types described and registered, the info's callback kept (OVL1 0x30CB90) */
extern "C" void func_0030CB90(CZNetwork *net, void (*cb)(void *info))
{
    LanInfoRequest req;
    LanGameInfo info;

    func_00620BD8();
    func_00620C60(&req, &req.m_0, 4, 1);
    func_00620C60(&req, &req.m_4, 4, 1);
    func_006210D0(&net->m_infoRequestType, func_0030CA80);
    func_00620BD8();
    func_00620C60(&info, &info.m_name, 21, 21);
    func_00620C60(&info, &info.m_18, 4, 1);
    func_00620C60(&info, &info.m_1c, 4, 1);
    func_00620C60(&info, &info.m_20, 2, 1);
    func_00620C60(&info, &info.m_22, 2, 1);
    func_00620C60(&info, &info.m_24, 2, 1);
    func_00620C60(&info, &info.m_28[0], 4, 1);
    func_00620C60(&info, &info.m_28[1], 4, 1);
    func_00620C60(&info, &info.m_28[2], 4, 1);
    func_00620C60(&info, &info.m_28[3], 4, 1);
    func_00620C60(&info, &info.m_28[4], 4, 1);
    func_00620C60(&info, &info.m_28[5], 4, 1);
    func_00620C60(&info, &info.m_28[6], 4, 1);
    func_00620C60(&info, &info.m_28[7], 4, 1);
    func_00620C60(&info, &info.m_48, 4, 1);
    func_00620C60(&info, &info.m_desc, 64, 64);
    func_00620C60(&info, &info.m_text, 256, 256);
    func_00620C60(&info, &info.m_18c, 1, 1);
    func_006210D0(&net->m_infoType, func_0030CA50);
    net->m_infoCallback = cb;
}

/* the info asked of a client (OVL1 0x30CB20) */
extern "C" void func_0030CB20(CZNetwork *net, void *to, int value)
{
    int id;
    LanInfoRequest req;

    memcpy(&id, to, 4);
    req.m_4 = value;
    req.m_0 = id;
    func_00621178(0x40, to, 2, net->m_infoRequestType, sizeof(req), &req);
}

/* a request answered with the room's game info (OVL1 0x30CA80) */
extern "C" int func_0030CA80(int a, int b, int c, void *data)
{
    int me;

    if (!func_00622C28(&me)) {
        LanGameInfo info = {0};

        data_0044FE10.GetGameInfo((LANGameInfo *)&info);
        info.m_1c = ((int *)data)[0];
        info.m_28[3] = ((int *)data)[1];
        func_00621178(0x40, &me, 2, theNetwork.m_infoType, sizeof(info), &info);
    }
    return 8;
}

/* the info handed to the callback (OVL1 0x30CA50) */
extern "C" int func_0030CA50(int a, int b, int c, void *data)
{
    theNetwork.m_infoCallback(data);
    return 400;
}
