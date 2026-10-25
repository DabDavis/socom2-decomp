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
 * in place; OVL1 0x30D900, the name ours).
 * OPEN, 16 instructions, registers only: the target computes the first node's address
 * (&end_.next_, s0) before the loop and loads the clear's begin() through it after -- only a
 * reference to end_.next_ taken first gives that (with clear() as include/list has it, MW reloads
 * the first node from `this` and the function is an instruction short); with the reference the
 * size is right but the sentinel, the reference and the message take s0, s3 and s2 where the
 * target has s2, s0 and s3 (declaration orders tried: the end iterator or the walk's iterator
 * ahead of the reference, the sentinel as a reference local). zNetPlayFixList is the same */
#pragma auto_inline off
void CZNetwork::zNetFlushMessages()
{
    typedef std::__list_deleter<const void *, std::allocator<const void *> > L;
    L::node_base *&first = m_message_list.end_node().next_;

    for (std::list<TypeAppMessage *>::iterator i = m_message_list.begin(); i != m_message_list.end();
         ++i) {
        TypeAppMessage *m = *i;

        func_00621458(m->m_type, m->m_conn, m->m_to, m->m_msg, m->m_size, m->m_data);
        _free(m, 0, 0);
    }
    m_message_list.L::erase(L::iterator(first), L::iterator(&m_message_list.end_node()));
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
 * OVL1 0x30D7D0, the name ours). OPEN, registers only, as zNetFlushMessages */
#pragma auto_inline off
void CZNetwork::zNetPlayFixList()
{
    typedef std::__list_deleter<const void *, std::allocator<const void *> > L;
    L::node_base *&first = m_fix_list.end_node().next_;

    for (std::list<TypeRTimeFixStruct *>::iterator i = m_fix_list.begin(); i != m_fix_list.end();
         ++i) {
        TypeRTimeFixStruct *f = *i;

        zRemoteDelayedAction(f);
        _free(f, 0, 0);
    }
    m_fix_list.L::erase(L::iterator(first), L::iterator(&m_fix_list.end_node()));
}
#pragma auto_inline reset

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
