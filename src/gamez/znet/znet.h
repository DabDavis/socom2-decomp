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

/* a network object as the DME hands it out (NetGetObject; CZNetwork::zNetGetObject): the type it
 * was registered under indexes the callback lists, its owner is the `client` the type's callbacks
 * are given (zNetRemoteObjectUpdateCallback reads both, 0x30CDF0: lbu 0, lw 4), and the game's
 * units read the kind's name and the owner's data (fts_remote.cpp, fts_net_seal.cpp,
 * fts_net_persona.cpp, fts_net_grenade.cpp and fts_net_bomb.cpp: a letter at 0xc, a pointer at
 * 0x1c). Not in SOCOM 1's DWARF (the DME is Sony's prebuilt code); the names are ours */
struct NetObject {
    unsigned char m_type_index;   // 0x0: zNetRegisterRemoteObjectCallback's index
    int m_client;   // 0x4: the owner
    unsigned char _8[0xc - 0x8];
    union {
        char m_name[0x10];   // 0xc: its kind's name (zNetCreateObject's),
        char m_type;   //      its first letter ('S' a SEAL, 'B' a blather object, 'T' ...)
    };
    void *m_data;   // 0x1c: the owner's data (zNetCreateObject's)
};   // 0x20

/* an object's ownership and life (int-sized: passed through as they come) */
enum NetOwnershipStatus {
    NetOwnershipStatus_Extent = 0x7FFFFFFF
};
enum NetObjectLifespan {
    NetObjectLifespan_Extent = 0x7FFFFFFF
};

/* how the console reaches the network (SOCOM 1's MP_CONNECTION, its DWARF's values; a byte enum) */
enum MP_CONNECTION {
    USB_MODEM = 0,
    USB_ETHERNET = 1,
    NA_MODEM = 2,
    NA_ETHERNET = 3
};

typedef void (*NetCreateObjectCallback)(int, int);
typedef void (*NetUpdateObjectCallback)(int, int, int);
typedef void (*NetDestroyObjectCallback)(int, int);

/* how a schedule decides a field has changed enough to send (SOCOM 1's NetThresholdMethod, by its
 * constructor's mangled name: its position schedules 2 with a threshold, its flags 1 with none --
 * the enumerators ours; int-sized, the constructors store it with sw) */
enum NetThresholdMethod {
    NET_THRESHOLD_ANY = 1,
    NET_THRESHOLD_DELTA = 2,
    NetThresholdMethod_Extent = 0x7FFFFFFF
};

/* what a schedule's compare function is handed (fts_remote.cpp's: the field's index, the new and
 * the old value) */
struct NetFieldCompare;

/* a network field's broadcast schedule (SOCOM 1's CZBroadcastSchedule, 16 bytes here: SOCOM 1's
 * double threshold is a float, or a compare function in its place, and SOCOM II passes the byte at
 * 0xC that SOCOM 1 zeroed). The DME copies it whole when a field is registered with it (two ldl/ldr
 * pairs, NetObjectField's 0x623F30), so nothing types the byte: unsigned char, as fts_remote.cpp's.
 * Both constructors are zNetDme.cpp's, its last own functions (OVL1 0x30E3D0, 0x30E3F0; SOCOM 1's
 * one at 0x322300, right before ~CZNetwork as here) */
class CZBroadcastSchedule {
public:
    CZBroadcastSchedule(unsigned int rate, NetThresholdMethod method, float threshold, unsigned char c);
    CZBroadcastSchedule(unsigned int rate, NetThresholdMethod method, bool (*compare)(NetFieldCompare *),
                        unsigned char c);

    unsigned int m_rate;   // 0x0: in ms
    NetThresholdMethod m_method;   // 0x4
    union {
        float m_threshold;   // 0x8
        bool (*m_compare)(NetFieldCompare *);
    };
    unsigned char m_c;   // 0xc (SOCOM II's)
};

/* --- the network ------------------------------------------------------------------------- */

class CZNetwork {
public:
    /* a remote action held until its time comes (SOCOM 1's nested struct, 28 bytes: fts_remote.cpp's
     * zRemoteDelayedAction plays it). The kind is an 8-bit field of an int group: the switch reads
     * it with lbu (0x2BBFA0, SOCOM 1's 0x203C90 alike) while AddToFixList's implicit assignment
     * copies the group as a word, seven lw/sw pairs in all (0x30D870, SOCOM 1's 0x321940) -- an
     * unsigned char kind copies with lbu/sb, an int kind is switched on with lw, copy construction
     * is an FPR block behind a null test; the senders write the kind as a word (sw 2) into a zeroed
     * 28-byte local, fts_remote.cpp's NetMessage. Measured 2026-11-04, the online lane's round 17 */
    struct TypeRTimeFixStruct {
        unsigned int m_kind : 8;   // 0x0: 2 a client gone, 4 the game server joined, 5 the game's objects made
        int m_client;   // 0x4
        int m_8, m_c, m_10, m_14, m_18;
    };   // 0x1c

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

    /* built by zNetDme.cpp's initialiser 0x402800, inlined there (SOCOM 1's set its two team
     * indices, a stream count of 2 and the character strings too; SOCOM II's starts with 100
     * objects, no streams and its game-info exchange cleared) */
    CZNetwork()
        : m_bSessionMaster(0), m_bNetwork(0), m_bConnected(0), m_bJoined(0), m_network_init(0),
          m_monitor_ok(0), m_WorldID(10), m_ClientIndex(256), m_NetObjectCount(100),
          m_DataStreamCount(0), m_bMediusEnabled(0), m_bNetCnfRequired(1), m_10a(1),
          m_infoRequestType(0), m_infoType(0)
    {
    }

    int zNetInitialize();
    bool zNetLoadIRX();
    int zNetLookupHost(const char *host, char *ip, int len);
    int zNetGetMyGameClientIndex();
    /* the game session's master client, -1 when the DME has none (0x30D560: it asks about the
     * game connection through the global, theNetwork's +0xf4, as SOCOM 1's 0x321710 does) */
    int zNetGetGameSessionMasterClientIndex();
    /* an object by index, 0 when the DME has none (0x30D630, SOCOM 1's 0x321820) */
    NetObject *zNetGetObject(int index);
    /* the game connection (an accessor, inlined: fts_remote.cpp's senders load it ahead of an
     * out-of-line call among their arguments, NetSendRequestTeamSwitch's master index) */
    int GetGameConnectionIndex() { return m_GameConnectionIndex; }
    /* the clients in the game (0x30D510; the LAN lobby counts its players with it) */
    int zNetGetValidGameClientCount();
    int zNetGetTime();
    /* the local clock, in milliseconds (unsigned: the active lobby's refresh compares it with
     * sltu, 0x2EE540) */
    unsigned int zNetGetLocalTime();
    int zNetUpdate();
    void zNetFlushMessages();
    void AddToFixList(TypeRTimeFixStruct *fix);
    void zNetPlayFixList();
    int zNetJoin(NetCreateObjectCallback join, NetCreateObjectCallback leave);
    int zNetConnect(void (*connect)(struct ClientConnectArgs *), void (*disconnect)(struct ClientConnectArgs *));
    int zNetCreateObject(int *index, NetOwnershipStatus own, NetObjectLifespan life, unsigned int a,
                         int b, unsigned int c, int d, void *data, char *name, int e);
    int zNetSendApplicationMessage(char type, int conn, int to, int msg, int size,
                                   unsigned char *data, bool now);
    void zNetRegisterRemoteObjectCallback(int index, NetCreateObjectCallback create,
                                          NetUpdateObjectCallback update,
                                          NetUpdateObjectCallback postupdate,
                                          NetDestroyObjectCallback destroy);
    static void zNetRemoteObjectUpdateCallback(int client, int index, int field);
    /* this console's client object set (SOCOM II's forwarder to the DME, 0x30CDE0) */
    void zNetSetMyClientObject(int object);

    unsigned char m_bSessionMaster;   // 0x0 (SOCOM 1's at 0x8)
    unsigned char m_bNetwork;   // 0x1
    unsigned char m_bConnected;   // 0x2
    unsigned char m_bJoined;   // 0x3
    unsigned char m_menu_skip;   // 0x4
    unsigned char m_network_init;   // 0x5
    unsigned char m_monitor_ok;   // 0x6
    unsigned char _7[0xd0 - 0x7];   // the addresses and ports (SOCOM 1's 0xf-0xbf, wider)
    char m_my_ip[16];   // 0xd0: our address as text (SOCOM 1's at 0xaf: zNetInitialize asks the DME for it)
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
    unsigned char _1194[0x1198 - 0x1194];
    /* SOCOM II's game-info exchange on a LAN: the callback the info goes to and the two DME
     * message types registered for the request and the reply (0x30CB90; the names ours) */
    void (*m_infoCallback)(void *info);   // 0x1198
    int m_infoRequestType;   // 0x119c
    int m_infoType;   // 0x11a0
    MP_CONNECTION m_connection_type;   // 0x11a4 (SOCOM 1's at 0x1168)
    unsigned char _11a5[0x1248 - 0x11a5];
    std::list<TypeAppMessage *> m_message_list;   // 0x1248
    std::list<TypeRTimeFixStruct *> m_fix_list;   // 0x1254
};   // 0x1260

extern CZNetwork theNetwork;

#endif
