/* con_ps2.cpp -- the debug console over DECI2: CConsole, its packets (CPacket, CPacketMsg), the
 * handlers it dispatches them to (CHandler) and the list of packets awaiting acknowledgement
 * (CAckList). OVL1 0x29CEC0-0x29E46F, right before ent_main.cpp; its initialiser (0x3FFF60) builds
 * the one console. SOCOM 1 names the unit (__sinit_con_ps2.cpp) and most of its functions, without
 * DWARF: the layouts here are read off the code, and the directory name is ours. Only some of it so
 * far (CHandler's three and AddHandler, 0x29CEC0-0x29D07F, are drafts outside this lane's range).
 * Defined in reverse of their image order (-inline deferred). */

#include <algorithm>
#include <list>
#include <stdarg.h>
#include <string.h>

extern "C" int sceDeci2Close(int s);
extern "C" int sceDeci2ReqSend(int s, char dest);
extern "C" int printf(const char *fmt, ...);
/* MSL's vsprintf (MAIN 0x19E7E8: SOCOM 1's Debug, the same code, calls vsprintf there) */
extern "C" int func_0019E7E8(char *buf, const char *fmt, va_list ap);

class CPacketMsg;
class CConsole;

/* the DECI2 buffers, 64 KB each, reached uncached (| 0x20000000) */
extern char _rbuf[];
extern char _wbuf[];

/* what a packet's data starts with (the names are ours) */
struct _con_packet_hdr {
    unsigned short size;   // 0x0
    unsigned short _2;
    unsigned int _4;
    unsigned int id;   // 0x8
    unsigned int flags;   // 0xc
    unsigned int type;   // 0x10
    unsigned int argc;   // 0x14
};

/* a packet: its data and size (SOCOM 1's names; the members' names are ours) */
class CPacket {
public:
    virtual ~CPacket();
    virtual CPacketMsg *GetPacketMsg();

    unsigned int GetPacketID() const;
    unsigned int GetFlags() const;
    static bool Compare(CPacket *a, CPacket *b);
    unsigned int GetPacketType() const;
    int CopyTo(void *dst, int size);
    /* a packet made from the bytes read: a message when it has arguments (SOCOM 1's) */
    static CPacket *OnRecvPacket(const void *data);

    _con_packet_hdr *m_data;   // 0x4
    unsigned int m_size;   // 0x8
};

/* a message packet: its arguments (SOCOM 1's) */
class CPacketMsg : public CPacket {
public:
    unsigned int GetArgCount() const
    {
        if (m_data)
            return m_data->argc;
        return 0;
    }
    void *GetArg(unsigned int index) const;
    /* the message itself (CPacket's answers 0: slot 0xC of both tables) */
    virtual CPacketMsg *GetPacketMsg();
    /* written, not implicit (0x29D910, 0xA0: not yet here) -- declared so this unit, which defines
     * a virtual of the class, emits no implicit copy */
    virtual ~CPacketMsg();

    void **m_args;   // 0xc
};

/* what a handler is called with (SOCOM 1's CHandler constructor's signature) */
typedef void (*HANDLER)(CPacket *packet, CConsole *console, void *data);

/* a packet type's handler, found by its id or its name (SOCOM 1's; 16 bytes, the name strdup'd --
 * its constructor, destructor and Dispatch are 0x29CF70, 0x29CF10 and 0x29CEC0; the members' names
 * are ours) */
class CHandler {
public:
    CHandler(const char *name, unsigned int id, HANDLER fn, void *data);
    ~CHandler();
    bool Dispatch(CPacket *packet, CConsole *console);
    /* the function and its data replaced (AddHandler's; the name is ours) */
    void Set(HANDLER fn, void *data)
    {
        m_fn = fn;
        m_data = data;
    }

    char *m_name;   // 0x0
    unsigned int m_id;   // 0x4
    HANDLER m_fn;   // 0x8
    void *m_data;   // 0xc
};

/* a packet deleted and its place cleared, for std::transform (the name is ours) */
struct DeletePacket {
    DeletePacket() {}
    CPacket *operator()(CPacket *p) const
    {
        delete p;
        return 0;
    }
};

/* the packets sent that wait for an acknowledgement (SOCOM 1's; a pointer list and a timeout) */
class CAckList : public std::list<CPacket *> {
public:
    /* (0x29E3E0, a draft: Clear, then the list's teardown) */
    ~CAckList();
    void Clear();
    CPacket *FindPacket(CPacket *packet);

    float m_timeout;   // 0xc: 10 (the constructor's)
};

/* the console (SOCOM 1's; the DECI2 socket at 0x30, -1 when closed; the members' names are ours) */
class CConsole {
public:
    virtual ~CConsole();
    virtual void OnDestroy();
    virtual void OnDisconnect();

    CHandler *FindHandler(const char *name);
    CHandler *FindHandler(unsigned int id);
    /* a handler for a packet type, made or updated (inline in SOCOM 1's OnConnect; out of line in
     * SOCOM II, 0x29CFC0: a placeholder outside this lane's range, its name ours) */
    unsigned int AddHandler(unsigned int id, const char *name, HANDLER fn, void *data);
    void OnConnect();
    bool RecvPacket(CPacket *packet);
    int Send(CPacket *packet);
    int Tick();
    int Disconnect();
    void Initalize();
    void CheckEvent();
    bool IsConnected() const { return m_socket >= 0; }
    /* debug output at a level, printed when the console's level reaches it (statics: no console
     * is passed in either build) */
    static void Debug(int level, const char *fmt, ...);
    static void Debug(int level, CPacket *packet, const char *prefix);

    /* the debug level (0x3DE2D0) */
    static int m_debug;

    unsigned char _4[0xc];   // 0x4: the console's name (MSL's basic_string)
    CAckList m_acks;   // 0x10
    std::list<CHandler *> m_handlers;   // 0x20
    bool m_connected;   // 0x2c
    int m_socket;   // 0x30
    volatile int m_event;   // 0x34: the last DECI2 event, set by the socket's callback
    int m_lastEvent;   // 0x38: the one CheckEvent reported
    int m_wsize;   // 0x3c: the bytes waiting to be written
    int m_rpos;   // 0x40
    char *m_wbuf;   // 0x44
    char *m_rbuf;   // 0x48
    int m_rsize;   // 0x4c: the bytes read, a packet when positive
    int m_wpos;   // 0x50
    int m_54;   // 0x54
    volatile int m_writing;   // 0x58: 1 while a write is in flight
};

/* every function of the unit is called out of line where it is called (Send's Debug and CopyTo,
 * FindPacket's Compare, RecvPacket's handler lookups and GetFlags, Tick's RecvPacket), as in SOCOM
 * 1: the whole unit is pinned */
#pragma auto_inline off

/* every packet waiting deleted and the list emptied (SOCOM 1's ~CAckList inlined it; out of line
 * in SOCOM II, which the destructor calls first -- the name is ours): std::transform with a
 * deleter onto the list itself, the two walks in s1 and s3, then the erase of the pointer list's
 * own begin() and end() -- the first node's link (this + 8) then kept in s0 from the start; through
 * clear() (the base's begin()) it is read off `this`. The deleter's user-declared empty constructor
 * stores nothing (value-initialised, the empty functor stores a zero byte) */
void CAckList::Clear()
{
    std::transform(begin(), end(), begin(), DeletePacket());
    erase(begin(), end());
}

/* the packet waiting that a received one acknowledges, or none (SOCOM 1's). The iterator assigned
 * (so in memory) with the end in a saved register is `it = begin(); for (iterator e = end(); ...)`;
 * the element read by value through the base iterator, after the call as before it; the compare's
 * negation materialised (sltu, xori) before the branch only as `!(bool)` with a continue */
CPacket *CAckList::FindPacket(CPacket *packet)
{
    iterator it;

    it = begin();
    for (iterator e = end(); it != e; ++it) {
        if (!(bool)CPacket::Compare((CPacket *)*it.i_, packet))
            continue;
        return (CPacket *)*it.i_;
    }
    return 0;
}

/* a new DECI2 event reported once, at the level the console keeps for them (SOCOM 1's: its guarded
 * function-local static is plain data in SOCOM II, 0x3DE2D8) */
void CConsole::CheckEvent()
{
    static int level = 4;

    if (m_lastEvent != m_event) {
        m_lastEvent = m_event;
        switch (m_event) {
        case 1:
            Debug(level, "DECI2_READ\n");
            break;
        case 2:
            Debug(level, "DECI2_READDONE\n");
            break;
        case 3:
            Debug(level, "DECI2_WRITE\n");
            break;
        case 4:
            Debug(level, "DECI2_WRITEDONE\n");
            break;
        case 5:
            Debug(level, "DECI2_CHSTATUS\n");
            break;
        case 6:
            Debug(level, "DECI2_ERROR\n");
            break;
        }
    }
}

/* the socket closed, if it is open: the inline bool compared `!= false` keeps its value (slt,
 * xori) before the branch, as the target has it; a plain truth test branches with bltz */
int CConsole::Disconnect()
{
    if (IsConnected() != false) {
        sceDeci2Close(m_socket);
        m_socket = -1;
    }
    return 0;
}

/* a packet read, if one came in: made, the read buffer reset, handled, and deleted unless the
 * console kept it (SOCOM 1's, the same code). The count taken in a ternary gives the target's
 * registers (this in s0, the count s1); the if-assign form swaps them */
int CConsole::Tick()
{
    CheckEvent();

    int n = IsConnected() != false ? m_rsize : 0;

    if (n > 0) {
        CPacket *packet = CPacket::OnRecvPacket(m_rbuf);

        m_rsize = 0;
        m_rpos = 0;
        m_rbuf = (char *)((int)_rbuf | 0x20000000);
        if (RecvPacket(packet))
            delete packet;
    }
    return n;
}

/* a packet written to the socket and waited for: what DECI2 answered (SOCOM 1's, the same code) */
int CConsole::Send(CPacket *packet)
{
    int r = 0;

    Debug(1, packet, "send");
    m_wsize = packet->CopyTo(m_wbuf, 0x10000);
    if (m_wsize) {
        r = sceDeci2ReqSend(m_socket, 'H');
        if (r < 0)
            Debug(0, packet, "ERROR sending");
        else {
            while (m_writing == 1)
                ;
            Debug(4, "write complete.\n");
        }
    } else
        Debug(0, packet, "ERROR write failed");
    m_wpos = 0;
    m_wsize = 0;
    m_wbuf = (char *)((int)_wbuf | 0x20000000);
    return r;
}

/* formatted debug output, when the level allows it (SOCOM 1's, the same code). `m_debug >= level`
 * materialises the test in v1 as the target does; `level <= m_debug` branches on `slt at` */
void CConsole::Debug(int level, const char *fmt, ...)
{
    if (m_debug >= level) {
        char buf[1024];
        va_list ap;

        va_start(ap, fmt);
        func_0019E7E8(buf, fmt, ap);
        printf(buf);
    }
}

/* a packet's header described (SOCOM 1's, the same code) */
void CConsole::Debug(int level, CPacket *packet, const char *prefix)
{
    if (m_debug >= level) {
        unsigned int flags = packet->GetFlags();
        unsigned int type = packet->GetPacketType();

        Debug(level, "%s: id=%d, cmd=%d, ", prefix, packet->GetPacketID(), type);
        if (flags & 1)
            Debug(level, "FLAG_WANTACK ");
        if (flags & 2)
            Debug(level, "FLAG_ACK ");
        Debug(level, "\n");
    }
}

/* whether two packets are the same: id, type and size alike (SOCOM 1's, the same code). The ids'
 * test is a bool assigned true (daddiu), each && its own materialised step; the two headers in
 * locals, a's first, give the target's registers (read straight off the packets, b's loads first) */
bool CPacket::Compare(CPacket *a, CPacket *b)
{
    _con_packet_hdr *da = a->m_data;
    _con_packet_hdr *db = b->m_data;
    bool same = false;

    if (da->id == db->id)
        same = true;
    bool r = same && da->type == db->type;

    return r && a->m_size == b->m_size;
}

/* the data copied out, if the room is enough: its size, or -1 */
int CPacket::CopyTo(void *dst, int size)
{
    if ((unsigned int)size < m_size)
        return -1;
    memcpy(dst, m_data, m_size);
    return m_size;
}

/* the header's fields, or 0 without data: called out of line by the packet Debug (in SOCOM 1 too) */
unsigned int CPacket::GetPacketType() const
{
    if (m_data)
        return m_data->type;
    return 0;
}

unsigned int CPacket::GetFlags() const
{
    if (m_data)
        return m_data->flags;
    return 0;
}

unsigned int CPacket::GetPacketID() const
{
    if (m_data)
        return m_data->id;
    return 0;
}

/* an argument, or none past the count (SOCOM 1's, the same code).
 * OPEN, 4 bytes long: the target tests the data with `beqz` to a return 0 placed last and the count
 * with `index >= argc` materialised (sltu v0) to its own return 0 -- this form has the second half
 * exactly, but branches over an early return 0 first (`bnez`, the zero in a b's delay slot). Through
 * the inline GetArgCount() the null case merges into the count (also 4 long); nested ifs, an else
 * on either test and ternaries are 4 or 12 bytes long */
void *CPacketMsg::GetArg(unsigned int index) const
{
    if (!m_data)
        return 0;
    if (index >= m_data->argc)
        return 0;
    return m_args[index];
}

/* (OVL1 0x29D890, CPacketMsg's table slot 0xC) */
CPacketMsg *CPacketMsg::GetPacketMsg()
{
    return this;
}

/* the three handlers the console registers on connecting (SOCOM 1's, the same order and code):
 * a status message asks the packet for its message, a ping and an error need nothing */
void _on_status(CPacket *packet, CConsole *console, void *data)
{
    packet->GetPacketMsg();
}

void _on_ping(CPacket *packet, CConsole *console, void *data)
{
}

void _on_error(CPacket *packet, CConsole *console, void *data)
{
}

CPacketMsg *CPacket::GetPacketMsg()
{
    return 0;
}

/* the console's handlers, registered as it connects (SOCOM 1's, which inlined AddHandler).
 * Byte-exact once 0x29CFC0 is named AddHandler (measured with the row renamed, AddHandler too):
 * until then its three calls name a placeholder outside the lane's range (WRONG REFERENCE) */
void CConsole::OnConnect()
{
    AddHandler(2, "msg", _on_status, 0);
    AddHandler(1, "ping", _on_ping, 0);
    AddHandler(3, "error", _on_error, 0);
}

/* the handler of a packet type, or none (SOCOM 1's). The id read into a local first puts the
 * handler's in the compare's first register, as the target has it */
CHandler *CConsole::FindHandler(unsigned int id)
{
    if (!m_handlers.size())
        return 0;

    std::list<CHandler *>::iterator it;

    for (it = m_handlers.begin(); it != m_handlers.end(); ++it) {
        unsigned int hid = (*it)->m_id;

        if (hid == id)
            return *it;
    }
    return 0;
}

/* the handler of a name, or none (new in SOCOM II: RecvPacket looks a message's first argument up
 * when no handler has its type). The test a bool local from a ternary whose false arm is an int
 * 0: the compare's arm is narrowed (andi) on its own, as the target has it; `: false` narrows at
 * the join */
CHandler *CConsole::FindHandler(const char *name)
{
    if (!m_handlers.size())
        return 0;

    std::list<CHandler *>::iterator it;

    for (it = m_handlers.begin(); it != m_handlers.end(); ++it) {
        CHandler *h = *it;
        bool same = name && h->m_name ? !strcmp(name, h->m_name) : 0;

        if (same)
            return h;
    }
    return 0;
}

/* a handler for a packet type: the one there updated, or a new one listed -- the id given back
 * (SOCOM 1's OnConnect inlined it three times; out of line in SOCOM II, 0x29CFC0, a placeholder row
 * outside this lane's range -- until it is renamed, OnConnect's three calls name another address
 * and this copy has no target) */
unsigned int CConsole::AddHandler(unsigned int id, const char *name, HANDLER fn, void *data)
{
    CHandler *h = FindHandler(id);

    if (h)
        h->Set(fn, data);
    else {
        h = new CHandler(name, id, fn, data);
        m_handlers.push_back(h);
    }
    return id;
}
