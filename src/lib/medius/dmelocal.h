/* dmelocal.h: DME's internal records, as far as the hand-written units use them (SCEA's own header is
 * dme_client/include/DmeLocal.h, whose $Header .data keeps). The layouts are the code's: each field
 * is where the functions reach it, named where a function says what it holds, field_<offset>
 * elsewhere. The limits are the bounds the functions test. */
#ifndef DMELOCAL_H
#define DMELOCAL_H

#define DME_MAX_OBJECTS     4096
#define DME_MAX_STREAMS     2048
#define DME_MAX_WORLDS      4
#define DME_MAX_CLIENTS     256

typedef struct DmeIp { unsigned char b[4]; } DmeIp;

/* a client's state in its world's record: an enum in SCEA's header -- its own alias class, so a
 * store to it does not order an int load around it (clients.c's func_00613158 loads the new
 * master's index across the state's store) */
typedef enum DmeClientState {
    DME_CLIENT_STATE_0,
    DME_CLIENT_STATE_1,
    DME_CLIENT_CONNECTING,              /* 2: connecting, or left */
    DME_CLIENT_STATE_3,
    DME_CLIENT_JOINED,                  /* 4 */
    DME_CLIENT_SESSION_MASTER           /* 5 */
} DmeClientState;

/* a world's record of one client (0x2C bytes, 256 a world: func_0061C720 hands one out) */
typedef struct DmeClientInfo {
    char szIp[16];                      /* 0x00 */
    unsigned short nPort;               /* 0x10 */
    DmeClientState nState;              /* 0x14: 2 connecting, 4 and 5 joined */
    int field_18;
    int field_1C;
    int field_20;
    int nValue;                         /* 0x24 */
    unsigned char bConnected;           /* 0x28 */
    unsigned char field_29;
    unsigned char field_2A;
} DmeClientInfo;

/* a world's connection state (DmeWorld 0x438; 3 is tested by func_0061D3C8): an enum too (gcc r21b) --
 * the two link-lost reports 0x617560 and 0x61D530 load it ahead of their int report's stores, which an
 * int field would order (the report's address is taken; the world pointer has no known base) */
typedef enum DmeWorldState {
    DME_WORLD_STATE_0,
    DME_WORLD_STATE_1,
    DME_WORLD_STATE_2,
    DME_WORLD_STATE_3                   /* 3: func_0061D3C8 closes the world */
} DmeWorldState;

/* a connection's kind (DmeConnectKind 0x00, DmeWorld 0x44C): an enum (its loads pass the world's int stores) */
typedef enum DmeConnectType {
    DME_CONNECT_0,
    DME_CONNECT_TCP,                    /* 1 */
    DME_CONNECT_PEER,                   /* 2 */
    DME_CONNECT_TCP_AUX,                /* 3 */
    DME_CONNECT_CLIENT_SERVER           /* 4 */
} DmeConnectType;

struct DmeWorld;
typedef void (*DmeClientCallback)(void *pInfo, DmeClientInfo *pClient, struct DmeWorld *pWorld);

/* a world (4 of them; func_0061C600 hands one out): its clients' records, then this client's index
 * and the world's state */
typedef struct DmeWorld {
    DmeClientInfo *apClients[DME_MAX_CLIENTS];  /* 0x000 */
    unsigned int nMyClient;             /* 0x400 */
    int nSessionMaster;                 /* 0x404 */
    char szName[16];                    /* 0x408 */
    char szServerVersion[16];           /* 0x418 */
    DmeClientCallback pfnClientCallback; /* 0x428 */
    void *field_42C;                    /* 0x42C: pfnClientCallback's user word */
    void (*field_430)(void *pReport);   /* 0x430: a client's leaving told */
    void *field_434;                    /* 0x434: its user word */
    DmeWorldState field_438;            /* 0x438 */
    int field_43C;                      /* 0x43C */
    int field_440;                      /* 0x440 */
    int field_444;
    int field_448;                      /* 0x448 */
    DmeConnectType field_44C;           /* 0x44C: 4 when the world is client-server */
} DmeWorld;

extern int func_0061C600(int nWorld, DmeWorld **ppWorld);

#endif
