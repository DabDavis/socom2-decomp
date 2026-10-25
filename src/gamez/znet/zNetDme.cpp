/* zNetDme.cpp -- the network: CZNetwork over Sony's DME library -- the remote objects and their
 * callbacks, the application messages, the per-frame update (SOCOM 1's zNetDme.cpp, the press
 * beta's file references name it: OVL1 from 0x30CDF0, zNetRemoteObjectUpdateCallback, SOCOM 1's
 * first function too; its static initialiser 0x402800 builds theNetwork and registers its
 * destructor 0x30E410, the unit's last function, as SOCOM 1's). The DME calls are OVL2 address
 * placeholders, their shapes the call sites'. Only some of it so far. Defined in reverse of their
 * image order (-inline deferred). */

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
int func_00622CC0(int *count, int conn);   /* the connection's object count */
int func_006220F0(int *index, NetOwnershipStatus own, NetObjectLifespan life, unsigned int a, int b,
                  unsigned int c, int d, void *data, char *name, int e);   /* NetCreateObject */
int func_006236C0();   /* NetUpdate */
int func_00622E10(int *t);   /* NetGetLocalTime */
int func_00622EA8(int *t);   /* NetGetTime */
int func_0064A008();   /* the Medius library's last update error */

int func_003006D0();   /* the Medius connection updated (the lobby's) */
void func_002DFE50(CZOnlineLobby *lobby);   /* CZOnlineLobby::Tick */
void func_0030BF20();   /* (the IOP stretch's per-frame update) */
}

/* the lobby's LAN flag ('ComeFromLan', a valve cached by the second room unit's object 0x44FE10;
 * its class unnamed yet) */
class CLanRoom;
extern "C" {
bool func_003045B0(CLanRoom *room);
extern CLanRoom data_0044FE10;
}

void zRemoteDelayedAction(CZNetwork::TypeRTimeFixStruct *fix);

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
        if (!func_003045B0(&data_0044FE10)) {
            if (func_003006D0() < 0) {
                err = func_0064A008();
                theLobby.RecoverFromNetError(err);
            }
            func_002DFE50(&theLobby);
        } else {
            err = func_006236C0();
            if (err)
                theLobby.RecoverFromNetError(err);
            func_002DFE50(&theLobby);
        }
        func_0030BF20();
        if (m_fix_list.size())
            zNetPlayFixList();
        for (int i = 0; i < 4096; i++) {
            if (m_ObjectFieldUpdate[i]) {
                NetObject *obj;

                NetGetObject(&obj, i);
                if (obj && m_ObjectPostUpdateCallbackList[obj->m_type])
                    m_ObjectPostUpdateCallbackList[obj->m_type]((int)obj->m_data, i, -1);
                m_ObjectFieldUpdate[i] = 0;
            }
        }
        int t = 0;

        if (!func_00622E10(&t))
            m_local_time = t;
        int n = 0;

        if (!func_00622EA8(&n))
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

/* the connection's object count, -1 when it cannot be had (the name ours) */
inline int ObjectCount(int conn)
{
    int n = -1;

    return func_00622CC0(&n, conn) ? -1 : n;
}

/* a network object made, unless the game connection holds 256 already (8 then); a failure
 * handed to the lobby's recovery (SOCOM 1's; OVL1 0x30D670).
 * OPEN, 2 instructions long: the target counts in the if-assign shape (`li v1,-1` in the branch's
 * delay slot, the count loaded on the fall-through), where every spelling tried -- the inline's
 * early return, a ternary returned or assigned to a declared local, `r = -1; if (!f()) r = n;`,
 * the same written in place -- gives the ternary's two arms (beqz, b) or longer */
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
    if (obj && theNetwork.m_ObjectUpdateCallbackList[obj->m_type])
        theNetwork.m_ObjectUpdateCallbackList[obj->m_type]((int)obj->m_data, index, field);
    theNetwork.m_ObjectFieldUpdate[index] = 1;
}
