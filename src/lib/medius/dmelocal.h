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

/* a world's record of one client (0x2C bytes, 256 a world: func_0061C720 hands one out) */
typedef struct DmeClientInfo {
    char szIp[16];                      /* 0x00 */
    unsigned short nPort;               /* 0x10 */
    int nState;                         /* 0x14: 2 connecting, 4 and 5 joined */
    int field_18;
    int field_1C;
    int field_20;
    int nValue;                         /* 0x24 */
    unsigned char bConnected;           /* 0x28 */
    unsigned char field_29;
    unsigned char field_2A;
} DmeClientInfo;

/* a world (4 of them; func_0061C600 hands one out): its clients' records, then this client's index
 * and the world's state */
typedef struct DmeWorld {
    DmeClientInfo *apClients[DME_MAX_CLIENTS];  /* 0x000 */
    unsigned int nMyClient;             /* 0x400 */
    int nSessionMaster;                 /* 0x404 */
    char szName[16];                    /* 0x408 */
    char szServerVersion[16];           /* 0x418 */
    void (*pfnClientCallback)(void *pInfo, DmeClientInfo *pClient, struct DmeWorld *pWorld); /* 0x428 */
    int field_42C;
    int field_430;
    int field_434;
    int field_438;                      /* 0x438 */
    int field_43C;                      /* 0x43C */
    int field_440;                      /* 0x440 */
    int field_444;
    int field_448;                      /* 0x448 */
    int field_44C;                      /* 0x44C: 4 when the world is client-server */
} DmeWorld;

extern int func_0061C600(int nWorld, DmeWorld **ppWorld);

#endif
