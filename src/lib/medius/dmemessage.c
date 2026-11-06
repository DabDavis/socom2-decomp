/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* DmeMessage.c: DME's packet handlers (OVL2 0x61DAA8-0x6204B8; dme_client/src/DmeMessage.c 1.172,
 * whose $Header heads the .data they use), SOCOM 1's DmeMessage object grown: the two block finders,
 * the handlers RegisterDMEmessages (0x61FBC8) registers by address, in the order it takes them, and
 * the new fragment/stream helpers after it. SCEA's DME client, prebuilt with ee-gcc 2.96 at -O2 -G0
 * (README "The GCC front"). Names are SOCOM 1's by position and callees (s1:order: the same object,
 * the same callees in the same order, the same message size returned); placeholders elsewhere.
 *
 * A handler takes (connection, unused, source client, message) and returns the size of the message it
 * consumed, -1 when it has none. A connection index is a world index (0-3) here; 0xFFFF is the
 * server's, which the RT layer calls -1. The world and client records are reached by offset until
 * their layout is written down. */

#include "dmelocal.h"

extern int strcmp(const char *a, const char *b);
extern char *strcpy(char *dst, const char *src);
extern char *strncpy(char *dst, const char *src, unsigned int n);

extern int func_0061C720(int nWorld, int nClient, char **ppClient);
extern int func_006126E8(char **ppClient, int nWorld, int nClient);
extern char *NetCreateClient(int nWorld, int nClient);
extern int NetClientJoins(int nWorld, int nClient);
extern int NetClientLeaves(int nWorld, int nClient);
extern int func_00614B18(int nWorld, int nClient, char *szIp, unsigned short nPort);
extern int func_00614F10(unsigned char **ppStream, int nIndex);
extern int NetIpBinaryToString(char *szIp, void *pBinary);
extern int func_006148E0(void *pBinary, const char *szIp);
extern int DmeToRtMsgConnectionIndex(int nConnection);
extern int KM_SetPublicKey(int nConnection, int nClient, void *pKey);
extern int func_0061CB18(unsigned char *pValue);
extern int func_00622C28(char *pAddress);
extern int func_00615558(int nConnection, char *szIp, int nPort, int nMsgType, int nLen, void *pMsg);

extern signed char NetObjectStatus[DME_MAX_OBJECTS];
extern char data_006766A4[];            /* this client's IP, as a string */
extern char data_006766BC[];
extern unsigned short data_0067A6E0;    /* the port */
extern unsigned short data_0067B7A8;
extern int data_0067B7AC;
extern int data_0067B7B0;
extern DmeIp data_0067C5B0;
extern void (*data_0067C5B4)(char *szIp, int nValue);
extern int data_00686A08;               /* the server-response message's type */

int GetFreeNetObjectBlock(int nCount)
{
    int i, j, k;
    int bFree;

    if ((unsigned int)nCount >= DME_MAX_OBJECTS)
        return 256;
    for (i = 0; i < DME_MAX_OBJECTS - nCount; i++) {
        if (NetObjectStatus[i] == 0) {
            bFree = 1;
            for (j = i + 1; j < i + nCount; j++) {
                if (NetObjectStatus[j] != 0) {
                    bFree = 0;
                    break;
                }
            }
            if (bFree) {
                for (k = i; k < i + nCount; k++)
                    NetObjectStatus[k] = 1;
                return i;
            }
        }
    }
    return -1;
}

int FindFreeDataStreamBlock(int nCount)
{
    unsigned char *pStream;
    int i, j;
    int bFree;

    if ((unsigned int)nCount >= DME_MAX_STREAMS)
        return -1;
    for (i = 0; i < DME_MAX_STREAMS - nCount; i++) {
        func_00614F10(&pStream, i);
        if (pStream[0] == 0) {
            bFree = 1;
            for (j = i + 1; j < i + nCount; j++) {
                func_00614F10(&pStream, j);
                if (pStream[0] != 0) {
                    bFree = 0;
                    break;
                }
            }
            if (bFree)
                return i;
        }
    }
    return -1;
}

typedef struct ClientConnectsMsg {
    signed char bSameWorld;
    unsigned char nClient;
    unsigned char ip[4];
    char pad[2];
    unsigned char key[64];
} ClientConnectsMsg;                    /* 0x48 */

/* The port passed on is data_0067B7A8 when it is set, else data_0067A6E0. Round 25b closed round 23b's
 * two-word open (the target reloads data_0067B7A8 in the bnezl's slot where `x ? x : y`, `x != 0 ? ...`, an
 * if/else into a local and int arms all let CSE reuse the tested value): the test is `== 0`, data_0067A6E0's
 * arm first -- the reload is the else arm, the jump's target, where CSE's path does not carry the tested
 * load. The three KM_SetPublicKey calls are SOCOM 1's (it kept two of them apart; here cross-jumping
 * merges all three into one jal). */
int ClientConnectsPacketHandler(int nConnection, int nUnused, int nClient, ClientConnectsMsg *pMsg)
{
    char szIp[16];
    DmeWorld *pWorld = 0;
    int nWorld = nConnection;
    unsigned short nPort;

    if (pMsg == 0)
        return -1;
    if (func_0061C600(nConnection, &pWorld) != 0 || pWorld == 0)
        return -1;
    NetIpBinaryToString(szIp, pMsg->ip);
    if (pMsg->bSameWorld == 0)
        nWorld = 0;
    if (strcmp(szIp, data_006766A4) != 0 || pMsg->nClient != pWorld->nMyClient) {
        if (nConnection == 0xFFFF)
            KM_SetPublicKey(-1, pMsg->nClient, pMsg->key);
        else if (nConnection == 0)
            KM_SetPublicKey(0, pMsg->nClient, pMsg->key);
        else
            KM_SetPublicKey(DmeToRtMsgConnectionIndex(nConnection), pMsg->nClient, pMsg->key);
    }
    if (data_0067B7A8 == 0)
        nPort = data_0067A6E0;
    else
        nPort = data_0067B7A8;
    func_00614B18(nWorld, pMsg->nClient, szIp, nPort);
    return sizeof(ClientConnectsMsg);
}

int ServerVersionPacketHandler(int nConnection, int nUnused, int nClient, char *pMsg)
{
    DmeWorld *pWorld = 0;

    if (pMsg == 0)
        return -1;
    if ((unsigned int)nConnection >= DME_MAX_WORLDS)
        return 16;
    if (func_0061C600(nConnection, &pWorld) != 0 || pWorld == 0)
        return -1;
    strncpy(pWorld->szServerVersion, pMsg, 16);
    return 16;
}

int ClientLeavesPacketHandler(int nConnection, int nUnused, int nClient, void *pMsg)
{
    if (pMsg == 0)
        return -1;
    if ((unsigned int)nConnection >= DME_MAX_WORLDS)
        return 1;
    if ((unsigned int)nClient >= DME_MAX_CLIENTS)
        return 1;
    NetClientLeaves(nConnection, nClient);
    return 1;
}

typedef struct ClientUpdateMsg {
    unsigned char nClient;
    char pad[7];
    unsigned short w[5];                /* 0x08 */
    char pad2[4];
    char szName[14];                    /* 0x16 */
} ClientUpdateMsg;                      /* 0x24 */

/* OPEN (19 words, the size right): the target keeps the 36 it returns live in v0 from the state
 * test on (the range tests then use v1, and the null-create path sets v0 before its branch); 2.96
 * here materialises the constant at the end. Guard clauses, one `||` guard, a result variable set
 * before the tests and m2c's single exit tried. */
int ClientUpdatePacketHandler(int nConnection, int nUnused, int nClient, ClientUpdateMsg *pMsg)
{
    char *pClient = 0;
    DmeWorld *pWorld = 0;
    char *pLocal = 0;
    unsigned int nIndex;

    if (pMsg == 0)
        return -1;
    if (func_0061C600(nConnection, &pWorld) == 0 && pWorld != 0) {
        if (func_0061C720(nConnection, pWorld->nMyClient, &pLocal) == 0 && pLocal != 0) {
            if ((unsigned int)(*(int *)(pLocal + 20) - 4) < 2 && (unsigned int)nConnection < DME_MAX_WORLDS) {
                nIndex = pMsg->nClient;
                if (nIndex < DME_MAX_CLIENTS) {
                    func_006126E8(&pClient, nConnection, nIndex);
                    if (pClient == 0)
                        pClient = NetCreateClient(nConnection, nIndex);
                    if (pClient != 0) {
                        *(unsigned short *)(pClient + 4) = pMsg->w[0];
                        *(unsigned short *)(pClient + 6) = pMsg->w[1];
                        *(unsigned short *)(pClient + 8) = pMsg->w[2];
                        *(unsigned short *)(pClient + 10) = pMsg->w[3];
                        *(unsigned short *)(pClient + 12) = pMsg->w[4];
                        strcpy(pClient + 14, pMsg->szName);
                        if (*(int *)pClient == 0)
                            NetClientJoins(nConnection, nIndex);
                    }
                }
            }
        }
    }
    return sizeof(ClientUpdateMsg);
}

typedef struct ServerResponseMsg {
    int nPort;
    DmeIp ip;
} ServerResponseMsg;

/* The reply is declared in the block that sends it: 2.96 gives an inner block's locals the frame's
 * slots after the outer block's (the reply at sp+48, above the flag at sp+32), whatever the order. */
int RequestServersPacketHandler(int nConnection, int nUnused, int nClient, int *pMsg)
{
    char szIp[16];
    char address[16];
    unsigned char bValue = 0;

    if (pMsg == 0)
        return -1;
    if (func_0061CB18(&bValue) == 0) {
        func_00622C28(address);
        NetIpBinaryToString(szIp, address);
        if (data_0067B7AC != 0 || bValue != 0) {
            if (*pMsg == data_0067B7B0) {
                ServerResponseMsg reply;

                if (data_0067B7AC != 0)
                    reply.ip = data_0067C5B0;
                else
                    func_006148E0(&reply.ip, data_006766BC);
                if (bValue == 1)
                    reply.nPort = data_0067A6E0;
                else
                    reply.nPort = data_0067B7AC;
                func_00615558(0, address, 0, data_00686A08, sizeof(reply), &reply);
            }
        }
    }
    return 4;
}

int ServerResponsePacketHandler(int nConnection, int nUnused, int nClient, ServerResponseMsg *pMsg)
{
    char szIp[16];

    if (pMsg == 0)
        return -1;
    if (data_0067C5B4 != 0) {
        NetIpBinaryToString(szIp, &pMsg->ip);
        data_0067C5B4(szIp, pMsg->nPort);
    }
    return sizeof(ServerResponseMsg);
}

extern unsigned int data_00651178;      /* the world this client is in */
extern int data_00686A10;               /* the ownership grant's message type */
extern int func_00614EC8(char **ppObject, unsigned int nIndex);
extern int func_00615EB0(int nOwner, unsigned int nIndex, int nValue);
extern int func_0061D448(int nClient);
extern int func_00615DE0(int nClass, int nConnection, int nTarget, int nMsgType, int nLen, void *pMsg);
extern int func_00615090(int *pValue, unsigned int nWorld);
extern int func_00613FD0(unsigned int nWorld, int nClient);
extern int func_0061D440(int nClient);
extern int func_0061D450(int nWorld);
extern int func_006448A0(void *pMsg);

typedef struct OwnershipMsg {
    unsigned char nClient;
    unsigned short nIndex;
} OwnershipMsg;

int RequestOwnershipPacketHandler(int nConnection, int nUnused, int nClient, unsigned short *pMsg)
{
    OwnershipMsg reply;
    char *pObject;
    DmeWorld *pWorld;
    unsigned int nIndex;

    pWorld = 0;
    pObject = 0;
    if (pMsg == 0)
        return -1;
    if (data_00651178 >= DME_MAX_WORLDS)
        return -1;
    nIndex = *pMsg;
    if (nIndex >= DME_MAX_OBJECTS)
        return -1;
    func_00614EC8(&pObject, nIndex);
    if (pObject == 0)
        return -1;
    if (func_0061C600(data_00651178, &pWorld) != 0 || pWorld == 0)
        return -1;
    if (*(int *)(pObject + 8) == pWorld->nMyClient) {
        if (*(int *)(pObject + 4) == -1) {
            *(int *)(pObject + 4) = nClient;
            func_00615EB0(*(int *)(pObject + 4), nIndex, -2);
            reply.nClient = func_0061D448(nClient);
            reply.nIndex = nIndex;
            func_00615DE0(80, nConnection, -1, data_00686A10, 4, &reply);
        }
    }
    return 2;
}

extern int func_0061FE18(int nWorld, int nClient, int nValue1, int nValue2);

int func_0061F410(int nConnection, int nUnused, int nClient, unsigned char *pMsg)
{
    DmeWorld *pWorld = 0;
    DmeClientInfo *pClient = 0;
    unsigned int nMyClient;

    if (pMsg == 0)
        return -1;
    if ((unsigned int)nConnection >= DME_MAX_WORLDS)
        return -1;
    if ((unsigned int)nClient >= DME_MAX_CLIENTS)
        return -1;
    if (func_0061C600(nConnection, &pWorld) != 0 || pWorld == 0)
        return -1;
    if (func_0061C720(nConnection, pMsg[1], (char **)&pClient) != 0 || pClient == 0)
        return -1;
    if (pClient->bConnected != 1)
        return -1;
    nMyClient = pWorld->nMyClient;
    if (nMyClient >= DME_MAX_CLIENTS)
        return -1;
    func_00613FD0(nConnection, pMsg[1]);
    if (pWorld->apClients[nMyClient]->nState == 5)
        func_0061FE18(nConnection, pMsg[1], pMsg[2], pMsg[3]);
    return 4;
}

int func_0061F7B0(int nConnection, int nUnused, int nClient, int *pMsg)
{
    DmeWorld *pWorld = 0;
    DmeClientInfo *pLocal = 0;
    int nValue = 256;

    if (pMsg == 0)
        return -1;
    if ((unsigned int)nConnection >= DME_MAX_WORLDS)
        return 4;
    if ((unsigned int)nClient >= DME_MAX_CLIENTS)
        return 4;
    if (data_00651178 >= DME_MAX_WORLDS)
        return 4;
    if (func_0061C600(data_00651178, &pWorld) == 0 && pWorld != 0) {
        if (func_0061C720(data_00651178, pWorld->nMyClient, (char **)&pLocal) == 0 && pLocal != 0) {
            if (func_00615090(&nValue, data_00651178) == 0 && nValue != 256) {
                pLocal->nValue = *pMsg;
                func_00613FD0(data_00651178, -1);
            }
        }
    }
    return 4;
}

int func_0061F8A0(int nConnection, int nUnused, int nClient, void *pMsg)
{
    int bValid;

    if (pMsg == 0)
        return -1;
    bValid = func_0061D450(nConnection);
    if (bValid != 1)
        return -1;
    if (func_0061D440(nClient) != bValid)
        return -1;
    func_006448A0(pMsg);
    return 28;
}

int func_00620008(unsigned int nWorld, int *pMax)
{
    unsigned int i = 0;
    char *pClient = 0;
    int result;

    if (pMax == 0)
        return 23;
    *pMax = 0;
    if (nWorld >= DME_MAX_WORLDS)
        return 23;
    for (i = 0; i < DME_MAX_CLIENTS; i++) {
        result = func_0061C720(nWorld, i, &pClient);
        if (result != 0)
            return result;
        if (pClient != 0 && *pMax < *(int *)(pClient + 36))
            *pMax = *(int *)(pClient + 36);
    }
    (*pMax)++;
    if (*pMax == 0 || *pMax == 0x0FFFFFFF)
        *pMax = 1;
    return 0;
}

/* a message being reassembled from its fragments (0x2C bytes, 256 of them) */
typedef struct DmeFragment {
    int nState;                         /* 0x00: 2 while fragments arrive */
    int field_04[7];
    unsigned int nTime;                 /* 0x20: when the last one came */
    void *pBuffer;                      /* 0x24 */
    void *pData;                        /* 0x28 */
} DmeFragment;

extern DmeFragment data_0066F408[256];
extern int rt_time_get_local(unsigned int *pTime);
extern void func_00614888(void **ppMemory);

int func_006203C8(void)
{
    unsigned int nNow;
    unsigned int i;

    rt_time_get_local(&nNow);
    for (i = 0; i < 256; i++) {
        if (data_0066F408[i].nState == 2 && nNow - data_0066F408[i].nTime > 30000) {
            if (data_0066F408[i].pData != 0) {
                func_00614888(&data_0066F408[i].pData);
                data_0066F408[i].pData = 0;
            }
            if (data_0066F408[i].pBuffer != 0) {
                func_00614888(&data_0066F408[i].pBuffer);
                data_0066F408[i].pBuffer = 0;
            }
            data_0066F408[i].nState = 0;
        }
    }
    return 0;
}

extern int func_00614F58(int nStream, unsigned char **ppStream);
extern int rt_circ_buf_destroy(void *pCirc);
extern void (*data_006869C8[])(int nClient, int nStream);   /* the streams' end callbacks, by type */
extern int data_0067E948[];

/* a data stream's end (0x61F2B8; SOCOM 1 has none): the type's callback told, the stream closed unless this
 * client is the session's own and the type wants it kept. Round 25b, unwritten before. */
int func_0061F2B8(int nConnection, int nUnused, int nClient, unsigned char *pMsg)
{
    unsigned char *pStream = 0;
    DmeWorld *pWorld = 0;
    int nValue = 256;

    if (pMsg == 0)
        return -1;
    if (func_00614F58(pMsg[0], &pStream) != 0)
        return 1;
    if (pStream != 0 && pStream[0] == 2) {
        if (data_006869C8[pStream[3]] != 0)
            data_006869C8[pStream[3]](nClient, pMsg[0]);
        if (func_0061C600(data_00651178, &pWorld) != 0)
            return 1;
        if (pWorld != 0) {
            func_00615090(&nValue, nConnection);
            if (pWorld->nMyClient == nValue && pStream[5] != 0 && data_0067E948[pStream[3]] != 0)
                pStream[4] = 1;
            else {
                pStream[0] = 1;
                if (pStream[2] != 0) {
                    func_00614888((void **)(pStream + 16));
                    rt_circ_buf_destroy(pStream + 44);
                    *(int *)(pStream + 16) = 0;
                    *(int *)(pStream + 20) = 0;
                }
            }
        }
    }
    return 1;
}

extern int func_006200D0(int nConnection, int nClient, void *pMsg, unsigned char **ppFrag);
extern void *memcpy(void *dst, const void *src, unsigned int n);
extern int func_0063CB48(int nA, int nB, int (**ppfn)(int, int, int, void *));

/* a fragmented message's fragment (0x61E510; the record is func_006200D0's): copied in by its offset, its
 * slot marked, and once every slot holds the whole message handed to its handler and the buffers freed.
 * Round 25b, unwritten before. */
typedef struct DmeFragMsg {
    unsigned char nA;                   /* 0x00: the handler's two keys */
    unsigned char nB;                   /* 0x01 */
    unsigned short nLength;             /* 0x02: this fragment's */
    unsigned short x04;
    unsigned short nSlot;               /* 0x06 */
    int x08;
    int x0C;
    int nOffset;                        /* 0x10 */
    unsigned char abData[4];            /* 0x14 */
} DmeFragMsg;

typedef struct DmeFrag {
    int bInUse;                         /* 0x00 */
    unsigned int nSlots;                /* 0x04 */
    int x08;
    int nHave;                          /* 0x0C */
    char x10[16];
    int tLast;                          /* 0x20 */
    unsigned char *pSlots;              /* 0x24 */
    unsigned char *pData;               /* 0x28 */
} DmeFrag;

int func_0061E510(int nConnection, int nUnused, int nClient, DmeFragMsg *pMsg)
{
    DmeFrag *pFrag = 0;
    int (*pfn)(int, int, int, void *);
    int bWhole = 1;
    unsigned int i;

    if (pMsg == 0)
        return -1;
    if (func_006200D0(nConnection, nClient, pMsg, (unsigned char **)&pFrag) == 0 && pFrag != 0) {
        memcpy(pFrag->pData + pMsg->nOffset, pMsg->abData, pMsg->nLength);
        pFrag->nHave += pMsg->nLength;
        rt_time_get_local((unsigned int *)&pFrag->tLast);
        pFrag->pSlots[pMsg->nSlot] = bWhole;
        for (i = 0; i < pFrag->nSlots; i++) {
            if (pFrag->pSlots[i] == 0) {
                bWhole = 0;
                break;
            }
        }
        if (bWhole != 0) {
            pfn = 0;
            func_0063CB48(pMsg->nA, pMsg->nB, &pfn);
            if (pfn != 0)
                pfn(nConnection, nUnused, nClient, pFrag->pData);
            if (pFrag->pData != 0) {
                func_00614888((void **)&pFrag->pData);
                pFrag->pData = 0;
            }
            if (pFrag->pSlots != 0) {
                func_00614888((void **)&pFrag->pSlots);
                pFrag->pSlots = 0;
            }
            pFrag->bInUse = 0;
        }
    }
    return pMsg->nLength + 20;
}
