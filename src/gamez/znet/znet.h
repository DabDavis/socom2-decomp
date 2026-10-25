/* The network: CZNetwork, theNetwork (SOCOM 1's zNetDme.cpp's class; the demo's DWARF gives
 * SOCOM 1's layout, 0x1264 bytes). SOCOM II drops SOCOM 1's two team indices ahead of the flags
 * (everything from m_bSessionMaster sits 8 bytes earlier), widens the strings by 0x28, adds a word
 * after m_ClientIndex and three after m_ObjectFieldUpdate -- the join's two callbacks among them --
 * and keeps its message and fix lists at 0x1248 and 0x1254. Offsets are SOCOM II's, read off the
 * code; the holes are what nothing here touches yet. The header's name is ours. */
#ifndef ZNET_H
#define ZNET_H

#include <list>

/* --- the DME library's types (Sony's prebuilt code in OVL2; only what is passed) ------------- */

/* a network object: its type indexes the callback lists, its data is the owner's */
struct NetObject {
    unsigned char m_type;   // 0x0
    void *m_data;   // 0x4
};

/* an object's ownership and life (int-sized: passed through as they come) */
enum NetOwnershipStatus {
    NetOwnershipStatus_Extent = 0x7FFFFFFF
};
enum NetObjectLifespan {
    NetObjectLifespan_Extent = 0x7FFFFFFF
};

typedef void (*NetCreateObjectCallback)(int, int);
typedef void (*NetUpdateObjectCallback)(int, int, int);
typedef void (*NetDestroyObjectCallback)(int, int);

/* --- the network ------------------------------------------------------------------------- */

class CZNetwork {
public:
    /* a remote action held until its time comes (SOCOM 1's nested struct, 28 bytes: fts_remote.cpp's
     * zRemoteDelayedAction plays it) */
    struct TypeRTimeFixStruct {
        int m_0, m_4, m_8, m_c, m_10, m_14, m_18;
    };

    /* an application message queued for the next update: the arguments the send was given, the
     * payload after them (the name ours; SOCOM 1 allocated it the same way) */
    struct TypeAppMessage {
        char m_type;   // 0x0
        int m_conn;   // 0x4
        int m_to;   // 0x8
        int m_msg;   // 0xc
        int m_size;   // 0x10
        unsigned char m_data[4];   // 0x14
    };   // 0x18

    int zNetLookupHost(const char *host, char *ip, int len);
    int zNetUpdate();
    void zNetFlushMessages();
    void AddToFixList(TypeRTimeFixStruct *fix);
    void zNetPlayFixList();
    int zNetJoin(NetCreateObjectCallback join, NetCreateObjectCallback leave);
    int zNetCreateObject(int *index, NetOwnershipStatus own, NetObjectLifespan life, unsigned int a,
                         int b, unsigned int c, int d, void *data, char *name, int e);
    int zNetSendApplicationMessage(char type, int conn, int to, int msg, int size,
                                   unsigned char *data, bool now);
    void zNetRegisterRemoteObjectCallback(int index, NetCreateObjectCallback create,
                                          NetUpdateObjectCallback update,
                                          NetUpdateObjectCallback postupdate,
                                          NetDestroyObjectCallback destroy);
    static void zNetRemoteObjectUpdateCallback(int client, int index, int field);

    unsigned char m_bSessionMaster;   // 0x0 (SOCOM 1's at 0x8)
    unsigned char m_bNetwork;   // 0x1
    unsigned char m_bConnected;   // 0x2
    unsigned char m_bJoined;   // 0x3
    unsigned char m_menu_skip;   // 0x4
    unsigned char m_network_init;   // 0x5
    unsigned char m_monitor_ok;   // 0x6
    unsigned char _7[0xe0 - 0x7];   // the addresses and ports (SOCOM 1's 0xf-0xbf, wider)
    int m_WorldID;   // 0xe0 (SOCOM 1's at 0xc0)
    int m_ClientIndex;   // 0xe4
    int m_e8;   // 0xe8 (SOCOM II's)
    int m_NetObjectCount;   // 0xec (SOCOM 1's at 0xc8)
    int m_DataStreamCount;   // 0xf0
    int m_GameConnectionIndex;   // 0xf4
    int m_LobbyConnectionIndex;   // 0xf8
    int m_sessionmaster_index;   // 0xfc
    int m_net_time;   // 0x100
    int m_local_time;   // 0x104
    unsigned char m_bMediusEnabled;   // 0x108 (SOCOM 1's at 0xe4)
    unsigned char m_bNetCnfRequired;   // 0x109
    unsigned char m_10a;   // 0x10a (SOCOM II's)
    char m_ObjectFieldUpdate[4096];   // 0x10b: an object's fields changed, by index
    NetCreateObjectCallback m_join_cb;   // 0x110c (SOCOM II's: the join's two callbacks)
    NetCreateObjectCallback m_leave_cb;   // 0x1110
    NetUpdateObjectCallback m_ObjectUpdateCallbackList[16];   // 0x1114 (SOCOM 1's at 0x10e8)
    NetUpdateObjectCallback m_ObjectPostUpdateCallbackList[16];   // 0x1154
    unsigned char _1194[0x1248 - 0x1194];
    std::list<TypeAppMessage *> m_message_list;   // 0x1248
    std::list<TypeRTimeFixStruct *> m_fix_list;   // 0x1254
};   // 0x1260

extern CZNetwork theNetwork;

#endif
