/* con_ps2.cpp -- the debug console over DECI2: CConsole, its packets (CPacket, CPacketMsg) and the
 * list of packets awaiting acknowledgement (CAckList). OVL1 0x29D080-0x29E46F, right before
 * ent_main.cpp; its initialiser (0x3FFF60) builds the one console. SOCOM 1 names the unit
 * (__sinit_con_ps2.cpp) and most of its functions, without DWARF: the layouts here are read off the
 * code, and the directory name is ours. Only some of it so far. Defined in reverse of their image
 * order (-inline deferred). */

#include <string.h>

extern "C" int sceDeci2Close(int s);

class CPacketMsg;

/* what a packet's data starts with (the names are ours) */
struct _con_packet_hdr {
    unsigned int _0;
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

    void **m_args;   // 0xc
};

/* the console (SOCOM 1's; the DECI2 socket at 0x30, -1 when closed) */
class CConsole {
public:
    virtual ~CConsole();
    virtual void OnDestroy();
    virtual void OnDisconnect();

    int Disconnect();
    bool IsConnected() const { return m_socket >= 0; }

    unsigned char _4[0x2c];
    int m_socket;   // 0x30
};

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

CPacketMsg *CPacket::GetPacketMsg()
{
    return 0;
}
