/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* rt_p2p.c: the RT library's peer-to-peer layer (OVL2 0x6395B8-0x63C700), SCEA's rt_p2p/src/rt_p2p.c
 * (its $Header in .data; the event table 0x665710 and the message magic 0xEFF0CDAB are its own),
 * prebuilt with ee-gcc 2.96 at -O2 -G0 (README "The GCC front"). It runs over one rt_udp endpoint
 * (rt_udp.c, 256 connections) and the key manager (rt_crypt.c's KM_*, key-manager id -1): a table
 * of peers (0x28 each: a state, the outgoing and incoming connection, flags, an address and port)
 * and 32 slots, and a callback for the public address. SOCOM 1 has no such object: the names are
 * placeholders. Error codes are the values returned: 2 a bad argument, 3 out of memory, 4 the UDP
 * endpoint, 5 the public address, 6 no connection to send on, 7 rt_udp failed, 8 an index out of
 * range, 9 no free peer.
 *
 * Round 21b: a two-way choice's arms each hold the whole tail SCEA wrote twice -- 0x639940's
 * address query tests the connection in both arms (cross-jumping merges the call and the copy
 * after reload, so each arm keeps its own offset in s1 and its -1) and the send 0x63C078 is an
 * if / else if / else of two sends and `return 6`; a range test whose `return 8` the target
 * branches past is `if (n >= 0 && n < count) {...} else return 8;`; 0x6398C0's tests are positive
 * on the connection (`!= -1` first, the else arm second).
 * OPEN: the peer searches 0x639A90 and 0x63B410: the loop is the target's as `for (i = 0; ; i++)
 * { if (i >= n) break; ... }` (bound hoisted, i * 40 recomputed, unrotated) -- what is left is the
 * second guard: the target keeps `if (pP2p == 0) return 2;` as its own `jr ra` (not if-converted,
 * the -2 stored through v0), ours if-converts it (ce: IF-CASE-1) and shares the first guard's
 * v0 = 2; posnest/else/goto/dup-store/result spellings all convert or move the loop;
 * 0x639FB8 (2: the stores'
 * order -- sched1 issues the three entry stores by register weight, the base's and the 7's deaths
 * putting nState first; every source order and a chained -1 tried). The rows this note once listed as not written
 * (0x6395B8, 0x639B08, 0x639D70, 0x63A038, 0x63A250, 0x63ACA0, 0x63B708, 0x63B9A0, 0x63C4F0) are written since: all
 * byte-exact but 0x63ACA0, whose OPEN note stands above it.
 * Round 22b: 0x63A618 (a send to one peer or every one in use), 0x63B2F0 (a peer found by its second
 * address: the first comparison's verdict is a variable of its own, never read -- assigned to the one
 * verdict variable twice, the registers move) and 0x63C3D0 (the leave message: its first loop's state
 * tests are two ifs, two movz; `== 3 || == 4` folds to the range test its second loop has) byte-exact.
 * OPEN: 0x63B5D0 (2: the peer's description -- the magic's lui and the local index's load issue the
 * other way round in sched2's dual-issue cycle; the stores' and the test's orders tried. Round 23b
 * read sched2's dump: the lui (371) precedes the reload of nLocal after memset (101) by LUID, but
 * 101 ranks higher -- the stores 95 and 98 through pInfo carry REG_DEP_ANTI on it, three dependants
 * to the lui's one, because purge_addressof gives that reload a bare MEM (alias set 0, no
 * MEM_SCALAR_P) where the entry's store to nLocal keeps int's set and the scalar flag, so
 * fixed_scalar_and_varying_struct_p cannot clear it; fourteen spellings of the declarations, the
 * stores, the compare and the constant leave the deps) and 0x63C170 (2: the
 * connection-lost handler -- each arm keeps its own copy of the reset, cross-jumped, and the state 0 is
 * assigned after the -2 initialisers; left: the join callback's `beq` keeps its argument load
 * `lw a1,8(sp)` in a plain slot (sched1 issued it before the pointer's load), here `beql` from the
 * target thread; the callee's definition order, its type, a local copy, the test's sense tried). */

extern void *memset(void *p, int c, unsigned int n);
extern char *strncpy(char *dst, const char *src, unsigned int n);
extern int strncmp(const char *s1, const char *s2, unsigned int n);
extern int rt_memory_free(void **ppMemory);
extern void KM_Cleanup(int nKMID);
extern void KM_Destroy(int nKMID);

typedef struct RT_UDP RT_UDP;

extern int func_0063DEA0(RT_UDP *pUdp);
extern int func_0063E030(RT_UDP *pUdp, int n);
extern int func_0063E230(RT_UDP *pUdp, void *pServer);
extern int func_0063E390(RT_UDP *pUdp, char *szAddress, unsigned short *pnPort);
extern int func_0063E438(RT_UDP *pUdp, char *szAddress, unsigned short *pnPort);
extern int func_0063E4A0(RT_UDP *pUdp, int nConn, char *szAddress, unsigned short *pnPort);
extern int func_0063E830(RT_UDP *pUdp, int nConn);
extern int func_0063E928(RT_UDP *pUdp, int nConn, unsigned char nType, void *pData, unsigned short nLength);
extern int func_0063EBA0(RT_UDP *pUdp, char *szAddress, unsigned short nPort, void *pData, unsigned short nLength);
extern int func_0063F270(RT_UDP *pUdp, int nConn);
extern int func_0063F3A8(RT_UDP *pUdp);
extern int func_0063F4C8(RT_UDP *pUdp);

typedef struct RT_P2P_ENTRY {           /* a peer */
    int nState;                         /* 0x00 */
    int nConn;                          /* 0x04: the outgoing connection, -1 none */
    int nConn2;                         /* 0x08: the incoming connection, -1 none */
    unsigned char bSend;                /* 0x0C: nConn may send */
    unsigned char bSend2;               /* 0x0D: nConn2 may send */
    unsigned char bClosed;              /* 0x0E */
    unsigned char bClosed2;             /* 0x0F */
    unsigned char bUsed;                /* 0x10: 0x63A618 sends only where it is set */
    unsigned char x11;
    char szAddress[16];                 /* 0x12 */
    unsigned short nPort;               /* 0x22 */
    void *pKeyChain;                    /* 0x24 */
} RT_P2P_ENTRY;                         /* 0x28 */

typedef struct RT_P2P_SLOT {
    int nConn;                          /* -1 none */
    int x04;
    int x08;
} RT_P2P_SLOT;                          /* 0x0C */

typedef struct RT_P2P {
    RT_P2P_SLOT aSlots[32];             /* 0x000 */
    RT_UDP *pUdp;                       /* 0x180 */
    RT_P2P_ENTRY *pEntries;             /* 0x184 */
    int nEntries;                       /* 0x188: at most 256 */
    unsigned int nBufSize;              /* 0x18C */
    char *pBuf;                         /* 0x190 */
    unsigned char bClosed;              /* 0x194 */
    unsigned char nTag;                 /* 0x195: sent with every broadcast */
    char x196[2];
    void (*pfnReset)(int nIndex, int n, int nError);    /* 0x198: told (-2, 0, 6) when the table is reset */
    void (*pfn19C)(int a);                              /* 0x19C */
    void (*pfnJoin)(int nIndex, int nLocal);            /* 0x1A0: the peer that takes a leaver's place */
    void (*pfnLeave)(int nIndex, unsigned int n);       /* 0x1A4 */
    void (*pfnData)(int nIndex, char *pData, unsigned short nLength);   /* 0x1A8: a peer's message that is not the layer's */
    void (*pfnDatagram)(char *szAddress, unsigned short nPort, char *pData, unsigned short nLength); /* 0x1AC */
    void (*pfnAddress)(char *szAddress, unsigned short nPort, int nError); /* 0x1B0 */
    void (*pfn1B4)(int nIndex, unsigned char bFlag);    /* 0x1B4 */
} RT_P2P;                               /* 0x1B8 */

typedef struct RT_P2P_MSG {             /* the broadcast (8 bytes) */
    unsigned int nMagic;                /* 0xEFF0CDAB */
    unsigned char nTag;
} RT_P2P_MSG;

typedef struct RT_P2P_MSG2 {            /* 0x63C3D0's message, type 7 (12 bytes) */
    unsigned int nMagic;                /* 0xEFF0CDAB */
    int nFirst;
    int x08;
} RT_P2P_MSG2;

typedef struct RT_P2P_PEER_ADDR {       /* a peer's two addresses, as 0x63B2F0 matches them */
    char x00[64];
    char szAddress[16];                 /* 0x40: its first connection's (nWhich 0) */
    char szAddress2[16];                /* 0x50: its second's (nWhich 1) */
    unsigned short nPort;               /* 0x60 */
    unsigned short nPort2;              /* 0x62 */
} RT_P2P_PEER_ADDR;

int func_0063B2F0(RT_P2P *pP2p, RT_P2P_PEER_ADDR *pAddr, int *pnIndex);
int func_0063B520(RT_P2P *pP2p, int *pnIndex);

typedef struct RT_P2P_CONNECT {         /* rt_udp's connect parameters (24 bytes) */
    char szAddress[16];                 /* 0x00 */
    unsigned short nPort;               /* 0x10 */
    int nIndex;                         /* 0x14 */
} RT_P2P_CONNECT;

extern int func_0063F2D8(RT_UDP *pUdp, RT_P2P_CONNECT *pConnect, int *pnConn);
extern int func_0063E5E8(RT_UDP *pUdp, RT_P2P_CONNECT *pConnect, int *pnConn);
extern int KM_SetPublicKey(int nKMID, int nClientID, void *pKey);
extern void *GetKeyMaster(int nKMID);
extern void *GetKeyChain(void *pMaster, int nClientID);

extern int rt_time_start(void *pTime);
extern int rt_time_elapsed(void *pTime, unsigned int *pnMsecs);
extern int KM_CreateRandomRCKey(void *pKey);
extern int KM_AddSymmetricKey(int nKMID, int nClientID, void *pKey);
extern int func_0063E8E8(RT_UDP *pUdp, int nConn, int nIndex);
extern int func_0063F168(RT_UDP *pUdp, int nConn);

int func_0063B410(RT_P2P *pP2p, int nConn, int *pnIndex);
int func_0063B480(RT_P2P *pP2p, int *pnState, int *pnIndex);
int func_0063C078(RT_P2P *pP2p, int nIndex, unsigned char nType, void *pData, unsigned short nLength);
int func_0063C170(RT_P2P *pP2p, int nConn, int nError);
extern int func_0063B708(RT_P2P *pP2p);
extern int func_0063BED0(RT_P2P *pP2p);
extern void func_0063AAC0(RT_P2P *pP2p, int nConn, int nError);
extern void func_0063ACA0(RT_P2P *pP2p, int nConn, int nError);
extern void func_0063B0B8(RT_P2P *pP2p, int nConn, int nError);
void func_0063A9A8(RT_P2P *pP2p, int nConn, int nError);
void func_0063AA38(RT_P2P *pP2p, int nConn, int nError);
void func_0063AC98(RT_P2P *pP2p, int nConn, int nError);
void func_0063B048(RT_P2P *pP2p, int nConn, int nError);
void func_0063B290(RT_P2P *pP2p, int nConn, int nError);
void func_0063B2B0(RT_P2P *pP2p, int nConn, int nError);
void func_0063B2D0(RT_P2P *pP2p, int nConn, int nError);

/* events 7, 8 and 9 hand over through a statement macro: a call inside a loop statement is never a
 * sibling call (README "The recogniser and Medius by hand", round 7b), and these three are jals */
#define RT_P2P_RELAY(p, c, e)   do { func_0063C170(p, c, e); } while (0)


typedef struct RT_P2P_INFO {            /* a peer described (0x70 bytes) */
    unsigned int nMagic;                /* 0x00: 0xEFF0CDAB */
    char szAddress[16];                 /* 0x04: the first connection's (nWhich 0) */
    char szAddress2[16];                /* 0x14: the second's */
    unsigned short nPort;               /* 0x24 */
    unsigned short nPort2;              /* 0x26 */
    int nIndex;                         /* 0x28 */
    unsigned char bLocal;               /* 0x2C: the peer is this one */
    unsigned char bJoined;              /* 0x2D: the table's answer to a joiner */
    char x2E[2];
    char abKey[64];                     /* 0x30: its public key */
} RT_P2P_INFO;

extern int KM_GetPublicKey(int nKMID, int nClientID, void *pKey);
extern int func_0062F818(int nKMID, void *pKey);

int func_0063C328(RT_P2P *pP2p, int *pnIndex);
int func_0063C3D0(RT_P2P *pP2p, int nLast, int nFirst);
extern int func_0063E0F0(RT_UDP *pUdp, int nConn, int nWhich, int *pa, int *pb, int *pc);

extern void *memcpy(void *d, const void *s, unsigned int n);
extern int func_0063ECC8(RT_UDP *pUdp, int nConn, char *pBuf, unsigned int nSize, unsigned int *pnLength);
int func_0063B9A0(RT_P2P *pP2p, int nIndex, char *pData, unsigned int nLength);
extern void KM_SetLocalClientID(int nKMID, int nClientID);

typedef struct RT_P2P_PARAMS {          /* the create's parameters */
    unsigned short nPort;               /* 0x00 */
    unsigned int nEntries;              /* 0x04: at most 256 */
    unsigned int nBufSize;              /* 0x08 */
    int x0C;
    int x10;
    int x14;
    int x18;
    int x1C;                            /* KM_Initialize's second */
    void (*pfn19C)(int a);                      /* 0x20 */
    void (*pfnJoin)(int nIndex, int nLocal);    /* 0x24 */
    void (*pfnLeave)(int nIndex, unsigned int n);   /* 0x28 */
    void (*pfnData)(int nIndex, char *pData, unsigned short nLength);   /* 0x2C */
    void (*pfnDatagram)(char *szAddress, unsigned short nPort, char *pData, unsigned short nLength);    /* 0x30 */
    void (*pfn1B4)(int nIndex, unsigned char bFlag);    /* 0x34 */
} RT_P2P_PARAMS;

typedef struct RT_UDP_PARAMS {          /* func_0063DD18's parameters (40 bytes) */
    unsigned short nPort;               /* 0x00 */
    int x04;
    int nConns;                         /* 0x08 */
    unsigned int x0C;
    int x10;
    int x14;
    int x18;
    int x1C;
    void (*pfnEvent)();                 /* 0x20 */
    void *pArg;                         /* 0x24 */
} RT_UDP_PARAMS;

extern int rt_memory_allocate(void **ppMemory, unsigned long nSize);
extern int rt_memory_allocate_init_zero(void **ppMemory, unsigned long nSize);
extern int func_0063DD18(RT_UDP **ppUdp, RT_UDP_PARAMS *pParams);
extern void func_0063A8C0();
extern void KM_Initialize(int nKMID, int n);
extern void KM_Create(int nKMID, int a, int b);

extern void func_0063C138(RT_P2P *pP2p, int nIndex);

#define RT_P2P_MARK(p, i, c) \
    if ((p)->pEntries[i].nConn == (c)) \
        (p)->pEntries[i].bSend = 1; \
    else if ((p)->pEntries[i].nConn2 == (c)) \
        (p)->pEntries[i].bSend2 = 1;


extern int KM_CopyKeys(int nKMID, int nFrom, int nKMID2, int nTo);
extern int KM_DeleteKeys(int nKMID, int nClientID);
int func_006395B8(RT_P2P_PARAMS *pParams, RT_P2P **ppP2p);
int func_00639808(RT_P2P *pP2p);
int func_00639870(RT_P2P *pP2p, void *pServer, void (*pfnAddress)(char *, unsigned short, int));
int func_006398C0(RT_P2P *pP2p, int nIndex, int *pnWhich);
int func_00639940(RT_P2P *pP2p, int nIndex, int nWhich, char *szAddress, unsigned short *pnPort);
int func_00639A30(RT_P2P *pP2p, int nWhich, char *szAddress, unsigned short *pnPort);
int func_00639A90(RT_P2P *pP2p, int *pnIndex);
int func_00639B08(RT_P2P *pP2p, unsigned int *pnBytes, int *pnZero, int *pnPercent, int *pnPercent2);
int func_00639D70(RT_P2P *pP2p, int nIndex, int *pnBytesSent, int *pnPacketsSent, int *pnBytesRecv, int *pnPacketsRecv);
int func_00639F88(RT_P2P *pP2p, int n);
int func_00639FB8(RT_P2P *pP2p, int *pnIndex);
int func_0063A038(RT_P2P *pP2p, RT_P2P_PEER_ADDR *pAddr);
int func_0063A250(RT_P2P *pP2p, RT_P2P_PEER_ADDR *pAddr, void (*pfnReset)(int nIndex, int n, int nError));
int func_0063A4A0(RT_P2P *pP2p);
int func_0063A590(RT_P2P *pP2p);
int func_0063A5C0(RT_P2P *pP2p);
int func_0063A618(RT_P2P *pP2p, int nIndex, unsigned char nType, void *pData, unsigned short nLength);
int func_0063A760(RT_P2P *pP2p, unsigned char nTag);
int func_0063A838(RT_P2P *pP2p, char *szAddress, unsigned short nPort, void *pData, unsigned short nLength);
int func_0063A870(RT_P2P *pP2p, int *pnEntries);
int func_0063A898(RT_P2P *pP2p, unsigned int nEntries);
void func_0063A8C0(RT_UDP *pUdp, int nConn, unsigned int nEvent, int nError, RT_P2P *pP2p);
void func_0063A9A8(RT_P2P *pP2p, int nConn, int nError);
void func_0063AA38(RT_P2P *pP2p, int nConn, int nError);
void func_0063AAC0(RT_P2P *pP2p, int nConn, int nError);
void func_0063AC98(RT_P2P *pP2p, int nConn, int nError);
void func_0063ACA0(RT_P2P *pP2p, int nConn, int nError);
void func_0063B048(RT_P2P *pP2p, int nConn, int nError);
void func_0063B0B8(RT_P2P *pP2p, int nConn, int nError);
void func_0063B290(RT_P2P *pP2p, int nConn, int nError);
void func_0063B2B0(RT_P2P *pP2p, int nConn, int nError);
void func_0063B2D0(RT_P2P *pP2p, int nConn, int nError);
int func_0063B2F0(RT_P2P *pP2p, RT_P2P_PEER_ADDR *pAddr, int *pnIndex);
int func_0063B410(RT_P2P *pP2p, int nConn, int *pnIndex);
int func_0063B480(RT_P2P *pP2p, int *pnState, int *pnIndex);
int func_0063B520(RT_P2P *pP2p, int *pnIndex);
int func_0063B5D0(RT_P2P *pP2p, int nIndex, RT_P2P_INFO *pInfo);
int func_0063B708(RT_P2P *pP2p);
int func_0063B9A0(RT_P2P *pP2p, int nIndex, char *pData, unsigned int nLength);
int func_0063BED0(RT_P2P *pP2p);
int func_0063C078(RT_P2P *pP2p, int nIndex, unsigned char nType, void *pData, unsigned short nLength);
void func_0063C138(RT_P2P *pP2p, int nIndex);
int func_0063C170(RT_P2P *pP2p, int nConn, int nError);
int func_0063C328(RT_P2P *pP2p, int *pnIndex);
int func_0063C3D0(RT_P2P *pP2p, int nLast, int nFirst);
int func_0063C4F0(RT_P2P *pP2p, RT_P2P_INFO *pInfo, RT_P2P_ENTRY *pEntry);

/* the layer's create (0x6395B8): the record, 32 free slots, the peer table (every connection -1), the receive buffer and
 * the UDP endpoint (func_0063DD18), then the key manager */
int func_006395B8(RT_P2P_PARAMS *pParams, RT_P2P **ppP2p)
{
    RT_UDP_PARAMS udp;
    RT_P2P *pP2p = 0;
    unsigned int i;
    int r1, r2, r3, r4;

    if (pParams == 0)
        return 2;
    if (ppP2p == 0)
        return 2;
    if (pParams->nEntries >= 257)
        return 2;
    r1 = rt_memory_allocate_init_zero((void **)&pP2p, sizeof(RT_P2P));
    if (pP2p == 0)
        return 3;
    if (r1 != 0)
        return 3;
    for (i = 0; i < 32; i++)
        pP2p->aSlots[i].nConn = -1;
    *ppP2p = pP2p;
    pP2p->nEntries = pParams->nEntries;
    pP2p->nBufSize = pParams->nBufSize;
    pP2p->pfn19C = pParams->pfn19C;
    pP2p->pfnJoin = pParams->pfnJoin;
    pP2p->pfnLeave = pParams->pfnLeave;
    pP2p->pfnData = pParams->pfnData;
    pP2p->pfnDatagram = pParams->pfnDatagram;
    pP2p->pfn1B4 = pParams->pfn1B4;
    memset(&udp, 0, sizeof(udp));
    udp.nPort = pParams->nPort;
    udp.x04 = 0;
    udp.nConns = 256;
    udp.x0C = pParams->nBufSize;
    udp.x10 = pParams->x0C;
    udp.x14 = pParams->x10;
    udp.x18 = pParams->x14;
    udp.x1C = pParams->x18;
    udp.pfnEvent = func_0063A8C0;
    udp.pArg = pP2p;
    r2 = rt_memory_allocate_init_zero((void **)&pP2p->pEntries, 256 * sizeof(RT_P2P_ENTRY));
    if (pP2p == 0)
        return 3;
    if (r2 != 0)
        return 3;
    for (i = 0; i < 256; i++) {
        pP2p->pEntries[i].nConn = -1;
        pP2p->pEntries[i].nConn2 = -1;
    }
    r3 = rt_memory_allocate((void **)&pP2p->pBuf, pP2p->nBufSize);
    if (pP2p == 0)
        return 3;
    if (r3 != 0)
        return 3;
    r4 = func_0063DD18(&pP2p->pUdp, &udp);
    if (pP2p->pUdp == 0)
        return 4;
    if (r4 != 0)
        return 4;
    KM_Initialize(-1, pParams->x1C);
    KM_Create(-1, 0, 0);
    return 0;
}

int func_00639808(RT_P2P *pP2p)
{
    RT_P2P *pFree = 0;

    if (pP2p == 0)
        return 2;
    pFree = pP2p;
    KM_Destroy(-1);
    KM_Cleanup(-1);
    func_0063DEA0(pFree->pUdp);
    rt_memory_free((void **)&pFree->pBuf);
    rt_memory_free((void **)&pFree->pEntries);
    rt_memory_free((void **)&pFree);
    return 0;
}

int func_00639870(RT_P2P *pP2p, void *pServer, void (*pfnAddress)(char *, unsigned short, int))
{
    if (pP2p == 0)
        return 2;
    if (pfnAddress == 0)
        return 2;
    if (func_0063E230(pP2p->pUdp, pServer) != 0)
        return 5;
    pP2p->pfnAddress = pfnAddress;
    return 0;
}

int func_006398C0(RT_P2P *pP2p, int nIndex, int *pnWhich)
{
    if (pnWhich == 0)
        return 2;
    *pnWhich = -1;
    if (pP2p == 0)
        return 2;
    if (nIndex < 0 || nIndex >= pP2p->nEntries)
        return 8;
    if (pP2p->pEntries[nIndex].nConn != -1)
        *pnWhich = 0;
    else if (pP2p->pEntries[nIndex].nConn2 != -1)
        *pnWhich = 1;
    return 0;
}

int func_00639940(RT_P2P *pP2p, int nIndex, int nWhich, char *szAddress, unsigned short *pnPort)
{
    if (pP2p == 0)
        return 2;
    if (nIndex >= 0 && nIndex < pP2p->nEntries) {
        if (nWhich == 0) {
            if (pP2p->pEntries[nIndex].nConn != -1) {
                if (func_0063E4A0(pP2p->pUdp, pP2p->pEntries[nIndex].nConn, szAddress, pnPort) != 0)
                    return 7;
            } else {
                strncpy(szAddress, pP2p->pEntries[nIndex].szAddress, 16);
                *pnPort = pP2p->pEntries[nIndex].nPort;
            }
        } else if (nWhich == 1) {
            if (pP2p->pEntries[nIndex].nConn2 != -1) {
                if (func_0063E4A0(pP2p->pUdp, pP2p->pEntries[nIndex].nConn2, szAddress, pnPort) != 0)
                    return 7;
            } else {
                strncpy(szAddress, pP2p->pEntries[nIndex].szAddress, 16);
                *pnPort = pP2p->pEntries[nIndex].nPort;
            }
        } else
            return 2;
    } else
        return 8;
    return 0;
}

int func_00639A30(RT_P2P *pP2p, int nWhich, char *szAddress, unsigned short *pnPort)
{
    if (pP2p == 0)
        return 2;
    if (nWhich == 0) {
        if (func_0063E438(pP2p->pUdp, szAddress, pnPort) != 0)
            return 7;
    } else if (nWhich == 1) {
        if (func_0063E390(pP2p->pUdp, szAddress, pnPort) != 0)
            return 7;
    } else
        return 2;
    return 0;
}

int func_00639A90(RT_P2P *pP2p, int *pnIndex)
{
    int i;

    if (pnIndex == 0)
        return 2;
    *pnIndex = -2;
    if (pP2p == 0)
        return 2;
    for (i = 0; ; i++) {
        if (i >= pP2p->nEntries)
            break;
        if (pP2p->pEntries[i].nState == 4 || pP2p->pEntries[i].nState == 7) {
            *pnIndex = i;
            break;
        }
    }
    return 0;
}

/* every peer's traffic in one (0x639B08): the bytes, nothing for the second count, and percentages over the packets */
/* every peer's traffic in one (0x639B08): the bytes, a count SCEA never adds to, and percentages over the packets */
int func_00639B08(RT_P2P *pP2p, unsigned int *pnBytes, int *pnZero, int *pnPercent, int *pnPercent2)
{
    int nA = 0;
    int nB = 0;
    int nC = 0;
    unsigned int nZero = 0;
    unsigned int nTotal = 0;
    unsigned int nPackets = 0;
    unsigned int nBytes = 0;
    int i;

    if (pP2p == 0)
        return 2;
    if (pnBytes == 0)
        return 2;
    if (pnZero == 0)
        return 2;
    if (pnPercent == 0)
        return 2;
    if (pnPercent2 == 0)
        return 2;
    *pnZero = 0;
    *pnBytes = 0;
    *pnPercent2 = 0;
    *pnPercent = 0;
    for (i = 0; i < pP2p->nEntries; i++) {
        if (pP2p->pEntries[i].nConn != -1) {
            if (func_0063E0F0(pP2p->pUdp, pP2p->pEntries[i].nConn, 0, &nA, &nB, &nC) != 0)
                return 0;
            nPackets += nA;
            nBytes += nB;
            if (func_0063E0F0(pP2p->pUdp, pP2p->pEntries[i].nConn, 1, &nA, &nB, &nC) != 0)
                return 0;
            nTotal += nA;
            nTotal += nB;
        }
        if (pP2p->pEntries[i].nConn2 != -1) {
            if (func_0063E0F0(pP2p->pUdp, pP2p->pEntries[i].nConn2, 0, &nA, &nB, &nC) != 0)
                return 0;
            nPackets += nA;
            nBytes += nB;
            if (func_0063E0F0(pP2p->pUdp, pP2p->pEntries[i].nConn2, 1, &nA, &nB, &nC) != 0)
                return 0;
            nTotal += nA;
            nTotal += nB;
        }
    }
    *pnBytes = nBytes;
    *pnZero = 0;
    if (nPackets != 0)
        *pnPercent = nBytes / nPackets * 100;
    else
        *pnPercent = 0;
    if (nTotal != 0)
        *pnPercent2 = nZero / nTotal * 100;
    else
        *pnPercent2 = 0;
    return 0;
}

/* one peer's traffic counts (0x639D70): the sum over its two connections of rt_udp's counters, sent and received */
int func_00639D70(RT_P2P *pP2p, int nIndex, int *pnBytesSent, int *pnPacketsSent, int *pnBytesRecv, int *pnPacketsRecv)
{
    int nPackets = 0;
    int nBytes = 0;
    int nOther = 0;

    if (pP2p == 0)
        return 2;
    if (nIndex > pP2p->nEntries)
        return 2;
    if (pnBytesSent == 0)
        return 2;
    if (pnBytesRecv == 0)
        return 2;
    if (pnPacketsSent == 0)
        return 2;
    if (pnPacketsRecv == 0)
        return 2;
    *pnPacketsSent = 0;
    *pnBytesSent = 0;
    *pnPacketsRecv = 0;
    *pnBytesRecv = 0;
    if (pP2p->pEntries[nIndex].nConn != -1) {
        if (func_0063E0F0(pP2p->pUdp, pP2p->pEntries[nIndex].nConn, 0, &nPackets, &nBytes, &nOther) != 0)
            return 0;
        *pnPacketsSent += nPackets;
        *pnBytesSent += nBytes;
        if (func_0063E0F0(pP2p->pUdp, pP2p->pEntries[nIndex].nConn, 1, &nPackets, &nBytes, &nOther) != 0)
            return 0;
        *pnPacketsRecv += nPackets;
        *pnBytesRecv += nBytes;
    }
    if (pP2p->pEntries[nIndex].nConn2 != -1) {
        if (func_0063E0F0(pP2p->pUdp, pP2p->pEntries[nIndex].nConn2, 0, &nPackets, &nBytes, &nOther) != 0)
            return 0;
        *pnPacketsSent += nPackets;
        *pnBytesSent += nBytes;
        if (func_0063E0F0(pP2p->pUdp, pP2p->pEntries[nIndex].nConn2, 1, &nPackets, &nBytes, &nOther) != 0)
            return 0;
        *pnPacketsRecv += nPackets;
        *pnBytesRecv += nBytes;
    }
    return 0;
}

int func_00639F88(RT_P2P *pP2p, int n)
{
    if (pP2p == 0)
        return 2;
    if (func_0063E030(pP2p->pUdp, n) != 0)
        return 7;
    return 0;
}

int func_00639FB8(RT_P2P *pP2p, int *pnIndex)
{
    int nState = 0;
    int nIndex = -2;

    if (pnIndex == 0)
        return 2;
    *pnIndex = -2;
    if (pP2p == 0)
        return 2;
    func_0063B480(pP2p, &nState, &nIndex);
    if (nState == 0) {
        pP2p->pEntries[0].nConn2 = -1;
        pP2p->pEntries[0].nConn = -1;
        pP2p->pEntries[0].nState = 7;
        *pnIndex = 0;
        return 0;
    }
    return 9;
}

/* a peer joins (0x63A038, state 7 only): its public key and a fresh symmetric key taken, then a connection to each of
 * its two addresses, the second cleared when both are the same; 9 for a peer already known, 10 for no free entry */
int func_0063A038(RT_P2P *pP2p, RT_P2P_PEER_ADDR *pAddr)
{
    RT_P2P_CONNECT connect;
    char abKey[64];
    int nState = 0;
    int nIndex = -2;
    int result;
    RT_P2P_ENTRY *pEntry;

    if (pP2p == 0)
        return 2;
    if (pAddr == 0)
        return 2;
    func_0063B480(pP2p, &nState, &nIndex);
    if (nState == 7) {
        nIndex = -2;
        result = func_0063B2F0(pP2p, pAddr, &nIndex);
        if (result != 0)
            return result;
        if (nIndex == -2) {
            result = func_0063B520(pP2p, &nIndex);
            if (result != 0)
                return result;
            if (nIndex == -2)
                return 10;
            pEntry = &pP2p->pEntries[nIndex];
            KM_SetPublicKey(-1, nIndex, pAddr);
            pEntry->pKeyChain = GetKeyChain(GetKeyMaster(-1), nIndex);
            KM_CreateRandomRCKey(abKey);
            KM_AddSymmetricKey(-1, nIndex, abKey);
            connect.nIndex = nIndex;
            if (strncmp(pAddr->szAddress, pAddr->szAddress2, 16) == 0 && pAddr->nPort == pAddr->nPort2) {
                pAddr->szAddress[0] = 0;
                pAddr->nPort = 0;
            }
            if (pAddr->szAddress[0] != 0 && pAddr->nPort != 0) {
                strncpy(connect.szAddress, pAddr->szAddress, 16);
                connect.nPort = pAddr->nPort;
                result = func_0063F2D8(pP2p->pUdp, &connect, &pEntry->nConn);
                if (result != 21) {
                    if (result != 0)
                        return 7;
                    pEntry->nState = 2;
                }
            }
            if (pAddr->szAddress2[0] != 0 && pAddr->nPort2 != 0) {
                strncpy(connect.szAddress, pAddr->szAddress2, 16);
                connect.nPort = pAddr->nPort2;
                result = func_0063F2D8(pP2p->pUdp, &connect, &pEntry->nConn2);
                if (result != 21) {
                    if (result != 0)
                        return 7;
                    pEntry->nState = 2;
                }
            }
        } else
            return 9;
    }
    return 0;
}

/* the table opens with this peer in entry 0 (0x63A250, state 0 only): its key, then a listening connection on each of
 * its addresses; one must take, and the reset callback is kept */
int func_0063A250(RT_P2P *pP2p, RT_P2P_PEER_ADDR *pAddr, void (*pfnReset)(int nIndex, int n, int nError))
{
    RT_P2P_CONNECT connect;
    RT_P2P_CONNECT connect2;
    char abKey[64];
    int nState = 0;
    int nIndex = -2;
    int result;
    RT_P2P_ENTRY *pEntry;

    if (pP2p == 0)
        return 2;
    if (pAddr == 0)
        return 2;
    if (pfnReset == 0)
        return 2;
    func_0063B480(pP2p, &nState, &nIndex);
    if (nState != 0)
        return 9;
    pEntry = pP2p->pEntries;
    memset(pEntry, 0, sizeof(RT_P2P_ENTRY));
    pEntry->nConn = -1;
    pEntry->nConn2 = -1;
    KM_SetPublicKey(-1, 0, pAddr);
    pEntry->pKeyChain = GetKeyChain(GetKeyMaster(-1), 0);
    KM_CreateRandomRCKey(abKey);
    KM_AddSymmetricKey(-1, 0, abKey);
    if (strncmp(pAddr->szAddress, pAddr->szAddress2, 16) == 0 && pAddr->nPort == pAddr->nPort2) {
        pAddr->szAddress[0] = 0;
        pAddr->nPort = 0;
    }
    if (pAddr->szAddress2[0] != 0 && pAddr->nPort2 != 0) {
        memset(&connect2, 0, sizeof(connect2));
        strncpy(connect2.szAddress, pAddr->szAddress2, 16);
        connect2.nPort = pAddr->nPort2;
        connect2.nIndex = 0;
        result = func_0063E5E8(pP2p->pUdp, &connect2, &pEntry->nConn2);
        if (result != 21) {
            if (result != 0)
                return 7;
            if (pEntry->nConn2 == -1)
                return 7;
            pEntry->nState = 1;
        }
    }
    if (pAddr->szAddress[0] != 0 && pAddr->nPort != 0) {
        memset(&connect, 0, sizeof(connect));
        strncpy(connect.szAddress, pAddr->szAddress, 16);
        connect.nPort = pAddr->nPort;
        connect.nIndex = 0;
        result = func_0063E5E8(pP2p->pUdp, &connect, &pEntry->nConn);
        if (result != 21) {
            if (result != 0)
                return 7;
            if (pEntry->nConn == -1)
                return 7;
            pEntry->nState = 1;
        }
    }
    if (pEntry->nState != 1)
        return 7;
    pP2p->pfnReset = pfnReset;
    return 0;
}

int func_0063A4A0(RT_P2P *pP2p)
{
    int i;
    RT_P2P_ENTRY *pEntry;

    if (pP2p == 0)
        return 2;
    pEntry = pP2p->pEntries;
    for (i = 0; i < pP2p->nEntries; i++) {
        if (pEntry->nConn != -1)
            func_0063E830(pP2p->pUdp, pEntry->nConn);
        if (pEntry->nConn2 != -1)
            func_0063E830(pP2p->pUdp, pEntry->nConn2);
        memset(&pP2p->pEntries[i], 0, sizeof(RT_P2P_ENTRY));
        pEntry->nConn2 = -1;
        pEntry->nConn = -1;
        pEntry++;
    }
    for (i = 0; i < 32; i++) {
        if (pP2p->aSlots[i].nConn != -1) {
            func_0063F270(pP2p->pUdp, pP2p->aSlots[i].nConn);
            pP2p->aSlots[i].nConn = -1;
        }
    }
    return 0;
}

int func_0063A590(RT_P2P *pP2p)
{
    if (pP2p == 0)
        return 2;
    if (func_0063F3A8(pP2p->pUdp) != 0)
        return 7;
    return 0;
}

int func_0063A5C0(RT_P2P *pP2p)
{
    int result;

    if (pP2p == 0)
        return 2;
    if (func_0063F4C8(pP2p->pUdp) != 0)
        return 7;
    if ((result = func_0063B708(pP2p)) != 0)
        return result;
    if ((result = func_0063BED0(pP2p)) != 0)
        return result;
    return 0;
}

/* a send to one peer, or with nIndex -1 to every peer in use whose state is 3 or 4 (6, no connection
 * to send on, skipped) */
int func_0063A618(RT_P2P *pP2p, int nIndex, unsigned char nType, void *pData, unsigned short nLength)
{
    int result = 0;
    int i;

    if (pP2p == 0)
        return 2;
    if (pData == 0)
        return 2;
    if (nLength == 0)
        return 2;
    if (nIndex == -1) {
        for (i = 0; i < pP2p->nEntries; i++) {
            if (pP2p->pEntries[i].bUsed != 0) {
                if (pP2p->pEntries[i].nState == 3 || pP2p->pEntries[i].nState == 4)
                    result = func_0063C078(pP2p, i, nType, pData, nLength);
                if (result != 0 && result != 6)
                    return result;
            }
        }
    } else {
        if (pP2p->pEntries[nIndex].nState == 3 || pP2p->pEntries[nIndex].nState == 4)
            result = func_0063C078(pP2p, nIndex, nType, pData, nLength);
    }
    return result;
}

int func_0063A760(RT_P2P *pP2p, unsigned char nTag)
{
    int result = 0;
    int i;
    RT_P2P_MSG msg;

    msg.nMagic = 0xEFF0CDAB;
    msg.nTag = nTag;
    if (pP2p == 0)
        return 2;
    pP2p->nTag = nTag;
    for (i = 0; i < pP2p->nEntries; i++) {
        if (pP2p->pEntries[i].nState == 3 || pP2p->pEntries[i].nState == 4)
            result = func_0063C078(pP2p, i, 7, &msg, 8);
        if (result != 0 && result != 6)
            return result;
    }
    return 0;
}

int func_0063A838(RT_P2P *pP2p, char *szAddress, unsigned short nPort, void *pData, unsigned short nLength)
{
    if (pP2p == 0)
        return 2;
    if (func_0063EBA0(pP2p->pUdp, szAddress, nPort, pData, nLength) != 0)
        return 7;
    return 0;
}

int func_0063A870(RT_P2P *pP2p, int *pnEntries)
{
    if (pnEntries == 0)
        return 2;
    *pnEntries = 0;
    if (pP2p == 0)
        return 2;
    *pnEntries = pP2p->nEntries;
    return 0;
}

int func_0063A898(RT_P2P *pP2p, unsigned int nEntries)
{
    if (pP2p == 0)
        return 2;
    if (nEntries > 256)
        return 2;
    pP2p->nEntries = nEntries;
    return 0;
}

void func_0063A8C0(RT_UDP *pUdp, int nConn, unsigned int nEvent, int nError, RT_P2P *pP2p)
{
    if (pP2p == 0)
        return;
    switch (nEvent) {
    case 0:
        func_0063A9A8(pP2p, nConn, nError);
        break;
    case 1:
        func_0063AA38(pP2p, nConn, nError);
        break;
    case 2:
        func_0063AAC0(pP2p, nConn, nError);
        break;
    case 3:
        func_0063AC98(pP2p, nConn, nError);
        break;
    case 4:
        func_0063ACA0(pP2p, nConn, nError);
        break;
    case 5:
        func_0063B048(pP2p, nConn, nError);
        break;
    case 6:
        func_0063B0B8(pP2p, nConn, nError);
        break;
    case 7:
        func_0063B290(pP2p, nConn, nError);
        break;
    case 8:
        func_0063B2B0(pP2p, nConn, nError);
        break;
    case 9:
        func_0063B2D0(pP2p, nConn, nError);
        break;
    }
}

void func_0063A9A8(RT_P2P *pP2p, int nConn, int nError)
{
    int nIndex;

    if (nConn == -2) {
        pP2p->bClosed = 1;
        return;
    }
    nIndex = -2;
    if (func_0063B410(pP2p, nConn, &nIndex) != 0 || nIndex == -2) {
        func_0063E830(pP2p->pUdp, nConn);
        return;
    }
    pP2p->pEntries[nIndex].bClosed2 = 1;
}

void func_0063AA38(RT_P2P *pP2p, int nConn, int nError)
{
    int nIndex;

    if (nConn == -2)
        return;
    nIndex = -2;
    if (func_0063B410(pP2p, nConn, &nIndex) != 0 || nIndex == -2) {
        func_0063E830(pP2p->pUdp, nConn);
        return;
    }
    pP2p->pEntries[nIndex].bClosed = 1;
}

/* a connection asks in (RT_P2P_RELAY's event 2, 0x63AAC0): while the table joins (state 7) it takes the next
 * free peer, with a fresh symmetric key; otherwise it is remembered for 30 seconds in a slot of its own, and
 * dropped if it asks again after them, or when no slot is free. Round 25b, unwritten before. */
void func_0063AAC0(RT_P2P *pP2p, int nConn, int nError)
{
    char abKey[64];
    int nState = 0;
    int nLocal = -2;
    int nIndex = -2;
    unsigned int nElapsed;
    unsigned int nFree;
    unsigned int i;

    if (func_0063B480(pP2p, &nState, &nLocal) != 0)
        return;
    if (nState == 7) {
        if (func_0063B520(pP2p, &nIndex) != 0)
            return;
        if (nIndex != -2) {
            pP2p->pEntries[nIndex].nState = 2;
            pP2p->pEntries[nIndex].nConn = nConn;
            pP2p->pEntries[nIndex].nConn2 = -1;
            func_0063E8E8(pP2p->pUdp, nConn, nIndex);
            KM_CreateRandomRCKey(abKey);
            KM_AddSymmetricKey(-1, nIndex, abKey);
            func_0063F168(pP2p->pUdp, nConn);
        } else
            func_0063F270(pP2p->pUdp, nConn);
    } else {
        nFree = 32;
        for (i = 0; i < 32; i++) {
            if (pP2p->aSlots[i].nConn == nConn) {
                rt_time_elapsed(&pP2p->aSlots[i].x04, &nElapsed);
                if (nElapsed <= 30000)
                    return;
                func_0063F270(pP2p->pUdp, nConn);
                pP2p->aSlots[i].nConn = -1;
                return;
            }
            if (pP2p->aSlots[i].nConn == -1 && nFree == 32)
                nFree = i;
        }
        if (nFree != 32) {
            pP2p->aSlots[nFree].nConn = nConn;
            rt_time_start(&pP2p->aSlots[nFree].x04);
        } else
            func_0063F270(pP2p->pUdp, nConn);
    }
}

void func_0063AC98(RT_P2P *pP2p, int nConn, int nError)
{
}

/* a connection is up (0x63ACA0): marked on its peer; while the table joins (state 7) the new peer is told of this one and
 * of every live peer, its own description sent back marked, and every live peer told of it */
/* OPEN (round 26b): d=3: the target's `move s0,zero` stands before `lw a1,120(sp)`; `i = 0` above the `if` gives d=7.
 * Commit d1d7fb79 calls this row byte-exact: it is not. */
void func_0063ACA0(RT_P2P *pP2p, int nConn, int nError)
{
    RT_P2P_INFO info;
    int nState = 0;
    int nLocal = -2;
    int nIndex = -2;
    int i;
    int result;

    if (func_0063B480(pP2p, &nState, &nLocal) != 0)
        return;
    if (func_0063B410(pP2p, nConn, &nIndex) != 0 || nIndex == -2) {
        func_0063E830(pP2p->pUdp, nConn);
        return;
    }
    if (nState == 7) {
        if (pP2p->pEntries[nIndex].nState == 2) {
            pP2p->pEntries[nIndex].nState = 3;
            RT_P2P_MARK(pP2p, nIndex, nConn)
            if (nLocal != 0) {
                if (func_0063B5D0(pP2p, nLocal, &info) != 0)
                    return;
                if (func_0063C078(pP2p, nIndex, 7, &info, sizeof(info)) != 0)
                    return;
            }
            for (i = 0; i < pP2p->nEntries; i++) {
                if (pP2p->pEntries[i].nState == 3 && i != nIndex) {
                    func_0063B5D0(pP2p, i, &info);
                    if (func_0063C078(pP2p, nIndex, 7, &info, sizeof(info)) != 0)
                        return;
                }
            }
            if (func_0063B5D0(pP2p, nIndex, &info) != 0)
                return;
            info.bJoined = 1;
            if (func_0063C078(pP2p, nIndex, 7, &info, sizeof(info)) != 0)
                return;
            result = func_0063B5D0(pP2p, nIndex, &info);
            if (result != 0)
                return;
            if (result < pP2p->nEntries) {
                i = 0;
                do {
                    if (pP2p->pEntries[i].nState == 3 && i != nIndex) {
                        if (func_0063C078(pP2p, i, 7, &info, sizeof(info)) != 0)
                            return;
                    }
                    i++;
                } while (i < pP2p->nEntries);
            }
            func_0063C138(pP2p, nIndex);
            return;
        }
        RT_P2P_MARK(pP2p, nIndex, nConn)
    } else if (pP2p->pEntries[nIndex].nState == 2) {
        pP2p->pEntries[nIndex].nState = 3;
        RT_P2P_MARK(pP2p, nIndex, nConn)
        func_0063C138(pP2p, nIndex);
    } else if (pP2p->pEntries[nIndex].nState == 1) {
        pP2p->pEntries[nIndex].nState = 4;
        RT_P2P_MARK(pP2p, nIndex, nConn)
        func_0063C138(pP2p, nIndex);
    } else {
        RT_P2P_MARK(pP2p, nIndex, nConn)
    }
}

void func_0063B048(RT_P2P *pP2p, int nConn, int nError)
{
    char szAddress[16];
    unsigned short nPort = 0;

    if (nError == 0) {
        if (func_0063E390(pP2p->pUdp, szAddress, &nPort) == 0)
            pP2p->pfnAddress(szAddress, nPort, 0);
    } else
        pP2p->pfnAddress(szAddress, 0, 5);
}

/* a connection's address learned (RT_P2P_RELAY's event, 0x63B0B8): kept on the peer that holds the connection; with
 * the peer's other connection up this one is closed, else func_0063C170 is told. A connection no peer holds is
 * closed. The second arm writes its call first, so the two calls cross-jump. Round 25b, unwritten before. */
void func_0063B0B8(RT_P2P *pP2p, int nConn, int nError)
{
    char szAddress[16];
    unsigned short nPort;
    int nIndex = -2;

    if (func_0063E4A0(pP2p->pUdp, nConn, szAddress, &nPort) != 0)
        return;
    if (func_0063B410(pP2p, nConn, &nIndex) != 0 || nIndex == -2) {
        func_0063E830(pP2p->pUdp, nConn);
        return;
    }
    if (nConn == pP2p->pEntries[nIndex].nConn) {
        strncpy(pP2p->pEntries[nIndex].szAddress, szAddress, 16);
        pP2p->pEntries[nIndex].nPort = nPort;
        if (pP2p->pEntries[nIndex].nConn2 != -1) {
            func_0063E830(pP2p->pUdp, nConn);
            pP2p->pEntries[nIndex].nConn = -1;
        } else
            func_0063C170(pP2p, nConn, nError);
    } else if (nConn == pP2p->pEntries[nIndex].nConn2) {
        strncpy(pP2p->pEntries[nIndex].szAddress, szAddress, 16);
        pP2p->pEntries[nIndex].nPort = nPort;
        if (pP2p->pEntries[nIndex].nConn == -1)
            func_0063C170(pP2p, nConn, nError);
        else {
            func_0063E830(pP2p->pUdp, nConn);
            pP2p->pEntries[nIndex].nConn2 = -1;
        }
    }
}

void func_0063B290(RT_P2P *pP2p, int nConn, int nError)
{
    RT_P2P_RELAY(pP2p, nConn, nError);
}

void func_0063B2B0(RT_P2P *pP2p, int nConn, int nError)
{
    RT_P2P_RELAY(pP2p, nConn, nError);
}

void func_0063B2D0(RT_P2P *pP2p, int nConn, int nError)
{
    RT_P2P_RELAY(pP2p, nConn, nError);
}

/* the peer whose second address and port are pAddr's: each peer's first address is compared too, its
 * verdict never read (only its strncmp call is left; one verdict variable assigned twice moves the
 * registers: pP2p and the index take s1/s2 for s0/s1) */
int func_0063B2F0(RT_P2P *pP2p, RT_P2P_PEER_ADDR *pAddr, int *pnIndex)
{
    char szAddress[16];
    unsigned short nPort;
    int i;
    int bFound, bFound2;

    if (pnIndex == 0)
        return 2;
    *pnIndex = -2;
    if (pP2p == 0)
        return 2;
    if (pAddr == 0)
        return 2;
    for (i = 0; i < pP2p->nEntries; i++) {
        func_00639940(pP2p, i, 0, szAddress, &nPort);
        bFound = strncmp(szAddress, pAddr->szAddress, 16) == 0 && nPort == pAddr->nPort;
        func_00639940(pP2p, i, 1, szAddress, &nPort);
        bFound2 = strncmp(szAddress, pAddr->szAddress2, 16) == 0 && nPort == pAddr->nPort2;
        if (bFound2) {
            *pnIndex = i;
            break;
        }
    }
    return 0;
}

int func_0063B410(RT_P2P *pP2p, int nConn, int *pnIndex)
{
    int i;

    if (pnIndex == 0)
        return 2;
    *pnIndex = -2;
    if (pP2p == 0)
        return 2;
    for (i = 0; ; i++) {
        if (i >= pP2p->nEntries)
            break;
        if (nConn == pP2p->pEntries[i].nConn || nConn == pP2p->pEntries[i].nConn2) {
            *pnIndex = i;
            break;
        }
    }
    return 0;
}

/* the table's state and the peer that set it: the first entry in state 6, 7 or 5. Round 23b (open
 * since round 11b, 16 bytes short): each state its own test, the arm written three times -- the
 * first test and its stores lie within expand_end_loop's 30-insn scan, so the rotation moves them to
 * the bottom with the bound (the copy at the entry peels the first step, two strength-reduced
 * pointers), and the 7 and 5 arms cross-jump into one found block; one `||` test is one arm, and
 * the rotated region then holds the whole body */
int func_0063B480(RT_P2P *pP2p, int *pnState, int *pnIndex)
{
    int i;

    if (pnState == 0)
        return 2;
    *pnState = 0;
    if (pnIndex == 0)
        return 2;
    *pnIndex = -2;
    if (pP2p == 0)
        return 2;
    for (i = 0; i < pP2p->nEntries; i++) {
        if (pP2p->pEntries[i].nState == 6) {
            *pnState = pP2p->pEntries[i].nState;
            *pnIndex = i;
            break;
        }
        if (pP2p->pEntries[i].nState == 7) {
            *pnState = pP2p->pEntries[i].nState;
            *pnIndex = i;
            break;
        }
        if (pP2p->pEntries[i].nState == 5) {
            *pnState = pP2p->pEntries[i].nState;
            *pnIndex = i;
            break;
        }
    }
    return 0;
}

int func_0063B520(RT_P2P *pP2p, int *pnIndex)
{
    int i;

    if (pnIndex == 0)
        return 2;
    *pnIndex = -2;
    for (i = 0; i < pP2p->nEntries; i++) {
        if (pP2p->pEntries[i].nState == 0) {
            *pnIndex = i;
            memset(&pP2p->pEntries[i], 0, sizeof(RT_P2P_ENTRY));
            pP2p->pEntries[i].nConn = -1;
            pP2p->pEntries[i].nConn2 = -1;
            break;
        }
    }
    return 0;
}

/* a peer's addresses and public key (this peer's own when nIndex is the local one) */
int func_0063B5D0(RT_P2P *pP2p, int nIndex, RT_P2P_INFO *pInfo)
{
    int nState = 0;
    int nLocal = -2;
    int result;

    if (pInfo == 0)
        return 2;
    result = func_0063B480(pP2p, &nState, &nLocal);
    if (result != 0)
        return result;
    memset(pInfo, 0, sizeof(RT_P2P_INFO));
    pInfo->nMagic = 0xEFF0CDAB;
    pInfo->nIndex = nIndex;
    if (nIndex == nLocal) {
        pInfo->bLocal = 1;
        result = func_00639A30(pP2p, 0, pInfo->szAddress, &pInfo->nPort);
        if (result != 0)
            return result;
        result = func_00639A30(pP2p, 1, pInfo->szAddress2, &pInfo->nPort2);
        func_0062F818(-1, pInfo->abKey);
        if (result != 0)
            return result;
    } else {
        result = func_00639940(pP2p, nIndex, 0, pInfo->szAddress, &pInfo->nPort);
        if (result != 0)
            return result;
        result = func_00639940(pP2p, nIndex, 1, pInfo->szAddress2, &pInfo->nPort2);
        KM_GetPublicKey(-1, nIndex, pInfo->abKey);
        if (result != 0)
            return result;
    }
    return 0;
}

/* the layer's receive poll (0x63B708): each peer with data waiting has both connections read until neither yields; a
 * message with the magic is the layer's own (func_0063B9A0), the rest the application's; then the unconnected socket's
 * datagrams go to their callback */
int func_0063B708(RT_P2P *pP2p)
{
    char szAddress[16];
    unsigned short nPort;
    unsigned int nLength;
    unsigned int nMagic;
    int bRead;
    int i;
    int result;

    for (i = 0; i < pP2p->nEntries; i++) {
        while (pP2p->pEntries[i].bClosed2 != 0) {
            bRead = 0;
            if (pP2p->pEntries[i].nConn != -1) {
                nMagic = 0;
                if (func_0063ECC8(pP2p->pUdp, pP2p->pEntries[i].nConn, pP2p->pBuf, pP2p->nBufSize, &nLength) == 0
                    && nLength != 0) {
                    memcpy(&nMagic, pP2p->pBuf, sizeof(nMagic));
                    if (nMagic != 0xEFF0CDAB) {
                        pP2p->pfnData(i, pP2p->pBuf, nLength);
                    } else {
                        result = func_0063B9A0(pP2p, i, pP2p->pBuf, nLength);
                        if (result != 0)
                            return result;
                    }
                    bRead = 1;
                }
            }
            if (pP2p->pEntries[i].nConn2 != -1) {
                nMagic = 0;
                if (func_0063ECC8(pP2p->pUdp, pP2p->pEntries[i].nConn2, pP2p->pBuf, pP2p->nBufSize, &nLength) == 0
                    && nLength != 0) {
                    memcpy(&nMagic, pP2p->pBuf, sizeof(nMagic));
                    if (nMagic != 0xEFF0CDAB) {
                        pP2p->pfnData(i, pP2p->pBuf, nLength);
                    } else {
                        result = func_0063B9A0(pP2p, i, pP2p->pBuf, nLength);
                        if (result != 0)
                            return result;
                    }
                    bRead = 1;
                }
            }
            if (bRead == 0)
                pP2p->pEntries[i].bClosed2 = 0;
        }
    }
    while (pP2p->bClosed != 0) {
        if (func_0063E4A0(pP2p->pUdp, -2, szAddress, &nPort) != 0)
            return 7;
        if (func_0063ECC8(pP2p->pUdp, -2, pP2p->pBuf, pP2p->nBufSize, &nLength) == 0 && nLength != 0)
            pP2p->pfnDatagram(szAddress, nPort, pP2p->pBuf, nLength);
        else
            pP2p->bClosed = 0;
    }
    return 0;
}

/* a message of the layer's own (0x63B9A0), told by its length: 112 a peer's description; 4 a peer leaves; 8 its flag;
 * 12 a peer's connection to drop */
int func_0063B9A0(RT_P2P *pP2p, int nIndex, char *pData, unsigned int nLength)
{
    RT_P2P_CONNECT connect;
    char abKey[64];
    int result;
    RT_P2P_ENTRY *pEntry;
    RT_P2P_INFO *pInfo = (RT_P2P_INFO *)pData;

    if (nLength == sizeof(RT_P2P_INFO)) {
        int nState = 0;
        int nLocal = -2;

        if (pInfo->nIndex < 0 || pInfo->nIndex >= pP2p->nEntries)
            return 14;
        pEntry = &pP2p->pEntries[pInfo->nIndex];
        result = func_0063B480(pP2p, &nState, &nLocal);
        if (result != 0)
            return result;
        if (nState == 0 || nState == 5) {
            if (pInfo->bLocal != 0 && pInfo->nIndex != 0) {
                RT_P2P_ENTRY *pFirst = pP2p->pEntries;

                *pEntry = *pFirst;
                memset(pFirst, 0, sizeof(RT_P2P_ENTRY));
                pFirst->nConn = -1;
                pFirst->nConn2 = -1;
                KM_CopyKeys(-1, 0, -1, pInfo->nIndex);
                KM_DeleteKeys(-1, 0);
                pEntry->pKeyChain = GetKeyChain(GetKeyMaster(-1), pInfo->nIndex);
                func_0063E8E8(pP2p->pUdp, pEntry->nConn, pInfo->nIndex);
                func_0063E8E8(pP2p->pUdp, pEntry->nConn2, pInfo->nIndex);
                return 0;
            }
            if (nState == 0 && pInfo->bJoined != 0) {
                KM_SetLocalClientID(-1, pInfo->nIndex);
                pEntry->nState = 5;
                return 0;
            }
            KM_SetPublicKey(-1, pInfo->nIndex, pInfo->abKey);
            pEntry->pKeyChain = GetKeyChain(GetKeyMaster(-1), pInfo->nIndex);
            KM_CreateRandomRCKey(abKey);
            KM_AddSymmetricKey(-1, pInfo->nIndex, abKey);
            if (func_0063C4F0(pP2p, pInfo, pEntry) != 0)
                return 0;
            if (pInfo->szAddress[0] != 0 && pInfo->nPort != 0) {
                strncpy(connect.szAddress, pInfo->szAddress, 16);
                connect.nPort = pInfo->nPort;
                connect.nIndex = pInfo->nIndex;
                if (nState == 0)
                    result = func_0063E5E8(pP2p->pUdp, &connect, &pEntry->nConn);
                else
                    result = func_0063F2D8(pP2p->pUdp, &connect, &pEntry->nConn);
                if (result != 21) {
                    if (result != 0)
                        return 7;
                    pEntry->nState = 2;
                }
            }
            if (pInfo->szAddress2[0] != 0 && pInfo->nPort2 != 0) {
                strncpy(connect.szAddress, pInfo->szAddress2, 16);
                connect.nPort = pInfo->nPort2;
                connect.nIndex = pInfo->nIndex;
                if (nState == 0)
                    result = func_0063E5E8(pP2p->pUdp, &connect, &pEntry->nConn2);
                else
                    result = func_0063F2D8(pP2p->pUdp, &connect, &pEntry->nConn2);
                if (result != 21) {
                    if (result != 0)
                        return 7;
                    pEntry->nState = 2;
                }
            }
        } else if (nState == 6) {
            KM_SetPublicKey(-1, pInfo->nIndex, pInfo->abKey);
            pEntry->pKeyChain = GetKeyChain(GetKeyMaster(-1), pInfo->nIndex);
            KM_CreateRandomRCKey(abKey);
            KM_AddSymmetricKey(-1, pInfo->nIndex, abKey);
            connect.nIndex = pInfo->nIndex;
            if (func_0063C4F0(pP2p, pInfo, pEntry) != 0)
                return 0;
            if (pInfo->szAddress[0] != 0 && pInfo->nPort != 0) {
                strncpy(connect.szAddress, pInfo->szAddress, 16);
                connect.nPort = pInfo->nPort;
                result = func_0063F2D8(pP2p->pUdp, &connect, &pEntry->nConn);
                if (result != 21) {
                    if (result != 0)
                        return 7;
                    pEntry->nState = 2;
                }
            }
            if (pInfo->szAddress2[0] != 0 && pInfo->nPort2 != 0) {
                strncpy(connect.szAddress, pInfo->szAddress2, 16);
                connect.nPort = pInfo->nPort2;
                result = func_0063F2D8(pP2p->pUdp, &connect, &pEntry->nConn2);
                if (result != 21) {
                    if (result != 0)
                        return 7;
                    pEntry->nState = 2;
                }
            }
        }
        return 0;
    } else if (nLength == 4) {
        int nState2 = 0;
        int nLocal2 = -2;

        result = func_0063B480(pP2p, &nState2, &nLocal2);
        if (result != 0)
            return result;
        if ((unsigned int)(nState2 - 6) < 2)
            pP2p->pfnLeave(nIndex, 0);
        else
            pP2p->pEntries[nIndex].x11 = 1;
    } else if (nLength == 8) {
        pP2p->pEntries[nIndex].bUsed = ((RT_P2P_MSG *)pData)->nTag;
        pP2p->pfn1B4(nIndex, ((RT_P2P_MSG *)pData)->nTag);
    } else if (nLength == 12) {
        if (pP2p->pEntries[((RT_P2P_MSG2 *)pData)->nFirst].nConn != -1)
            func_0063C170(pP2p, pP2p->pEntries[((RT_P2P_MSG2 *)pData)->nFirst].nConn, 16);
        else if (pP2p->pEntries[((RT_P2P_MSG2 *)pData)->nFirst].nConn2 != -1)
            func_0063C170(pP2p, pP2p->pEntries[((RT_P2P_MSG2 *)pData)->nFirst].nConn2, 16);
    }
    return 0;
}

/* the peers' state counted (0x63BED0): with a peer in state 5, none in 1 or 2 and some in 3 or 4, that peer goes
 * to state 6, pfnReset is told it and the count plus one, and each peer in 3 or 4 is sent the magic alone (7),
 * pfnLeave told where its byte at 0x11 was set. Round 25b, unwritten before. */
int func_0063BED0(RT_P2P *pP2p)
{
    unsigned int nMagic;
    int nState = 0;
    int nIndex = -2;
    int nJoined = 0;
    int nConnecting = 0;
    int result;
    int i;

    for (i = 0; i < pP2p->nEntries; i++) {
        switch (pP2p->pEntries[i].nState) {
        case 1:
        case 2:
            nConnecting++;
            break;
        case 3:
        case 4:
            nJoined++;
            break;
        case 5:
            nState = 5;
            nIndex = i;
            break;
        case 0:
        case 6:
        case 7:
            break;
        }
    }
    if (nState == 5 && nConnecting == 0 && nJoined != 0) {
        pP2p->pEntries[nIndex].nState = 6;
        pP2p->pfnReset(nIndex, (unsigned short)(nJoined + 1), 0);
        for (i = 0; i < pP2p->nEntries; i++) {
            if ((unsigned int)(pP2p->pEntries[i].nState - 3) < 2) {
                nMagic = 0xEFF0CDAB;
                result = func_0063C078(pP2p, i, 7, &nMagic, 4);
                if (result != 0)
                    return result;
                if (pP2p->pEntries[i].x11 != 0) {
                    pP2p->pEntries[i].x11 = 0;
                    pP2p->pfnLeave(i, 0);
                }
            }
        }
    }
    return 0;
}

int func_0063C078(RT_P2P *pP2p, int nIndex, unsigned char nType, void *pData, unsigned short nLength)
{
    if (nIndex < 0 || nIndex >= pP2p->nEntries)
        return 8;
    if (pP2p->pEntries[nIndex].nConn != -1 && pP2p->pEntries[nIndex].bSend != 0) {
        if (func_0063E928(pP2p->pUdp, pP2p->pEntries[nIndex].nConn, nType, pData, nLength) != 0)
            return 7;
    } else if (pP2p->pEntries[nIndex].nConn2 != -1 && pP2p->pEntries[nIndex].bSend2 != 0) {
        if (func_0063E928(pP2p->pUdp, pP2p->pEntries[nIndex].nConn2, nType, pData, nLength) != 0)
            return 7;
    } else
        return 6;
    return 0;
}

void func_0063C138(RT_P2P *pP2p, int nIndex)
{
    RT_P2P_MSG msg;

    msg.nMagic = 0xEFF0CDAB;
    msg.nTag = pP2p->nTag;
    func_0063C078(pP2p, nIndex, 7, &msg, 8);
}

/* a connection lost (RT_P2P_RELAY's events): its peer's connections closed and the peer freed, the
 * leave and join callbacks told; a connection no peer holds, or a table not yet joined (state 0 or
 * 5), resets the table: 6 */
int func_0063C170(RT_P2P *pP2p, int nConn, int nError)
{
    int nIndex = -2;
    int nState;
    int nLocal = -2;
    int nNext = -2;
    int result;
    RT_P2P_ENTRY *pEntry;

    nState = 0;
    if (func_0063B410(pP2p, nConn, &nIndex) != 0 || nIndex == -2) {
        func_0063E830(pP2p->pUdp, nConn);
        result = func_0063B480(pP2p, &nState, &nLocal);
        if (result != 0)
            return result;
        if (nState == 5 || nState == 0) {
            func_0063A4A0(pP2p);
            pP2p->pfnReset(-2, 0, 6);
            return 6;
        }
        return 0;
    }
    pEntry = &pP2p->pEntries[nIndex];
    result = func_0063B480(pP2p, &nState, &nLocal);
    if (result != 0)
        return result;
    if (nState == 5 || nState == 0) {
        func_0063A4A0(pP2p);
        pP2p->pfnReset(-2, 0, 6);
        return 6;
    }
    if (pEntry->nConn != -1)
        func_0063E830(pP2p->pUdp, pEntry->nConn);
    if (pEntry->nConn2 != -1)
        func_0063E830(pP2p->pUdp, pEntry->nConn2);
    pP2p->pfnLeave(nIndex, 1);
    if (pEntry->nState == 4)
        func_0063C328(pP2p, &nNext);
    memset(pEntry, 0, sizeof(RT_P2P_ENTRY));
    pEntry->nConn = -1;
    pEntry->nConn2 = -1;
    if (nNext != -2)
        pP2p->pfnJoin(nNext, nLocal);
    func_0063C3D0(pP2p, nLocal, nIndex);
    return 0;
}

int func_0063C328(RT_P2P *pP2p, int *pnIndex)
{
    int i;

    if (pnIndex == 0)
        return 2;
    *pnIndex = -2;
    for (i = 0; i < pP2p->nEntries; i++) {
        if (pP2p->pEntries[i].nState == 3) {
            *pnIndex = i;
            pP2p->pEntries[i].nState = 4;
            break;
        }
        if (pP2p->pEntries[i].nState == 6) {
            *pnIndex = i;
            pP2p->pEntries[i].nState = 7;
            break;
        }
    }
    return 0;
}

/* unless a peer above nFirst (up to nLast, or the last) is in state 3 or 4, the message to every
 * peer that is */
int func_0063C3D0(RT_P2P *pP2p, int nLast, int nFirst)
{
    RT_P2P_MSG2 msg;
    int i = 0;
    int bBusy = 0;

    msg.nMagic = 0xEFF0CDAB;
    msg.nFirst = nFirst;
    if (nLast != 0) {
        if (nFirst < nLast)
            i = nLast - 1;
    } else
        i = pP2p->nEntries - 1;
    for (; nFirst < i; i--) {
        if (pP2p->pEntries[i].nState == 3)
            bBusy = 1;
        if (pP2p->pEntries[i].nState == 4)
            bBusy = 1;
    }
    if (bBusy == 0) {
        for (i = 0; i < pP2p->nEntries; i++) {
            if (pP2p->pEntries[i].nState == 3 || pP2p->pEntries[i].nState == 4)
                func_0063C078(pP2p, i, 7, &msg, 12);
        }
    }
    return 0;
}

/* a connection held in a slot turns out to be a peer's (0x63C4F0): its address and port are one of the peer's two; the
 * entry takes the connection and a fresh symmetric key. 1 when found */
int func_0063C4F0(RT_P2P *pP2p, RT_P2P_INFO *pInfo, RT_P2P_ENTRY *pEntry)
{
    char szAddress[16];
    unsigned short nPort;
    unsigned int i;

    if (pP2p == 0)
        return 0;
    if (pInfo == 0)
        return 0;
    for (i = 0; i < 32; i++) {
        if (pP2p->aSlots[i].nConn == -1)
            continue;
        if (func_0063E4A0(pP2p->pUdp, pP2p->aSlots[i].nConn, szAddress, &nPort) != 0)
            return 0;
        if (strncmp(szAddress, pInfo->szAddress, 16) == 0 && nPort == pInfo->nPort && nPort != 0) {
            char abKey[64];

            func_0063E8E8(pP2p->pUdp, pP2p->aSlots[i].nConn, pInfo->nIndex);
            KM_CreateRandomRCKey(abKey);
            KM_AddSymmetricKey(-1, pInfo->nIndex, abKey);
            func_0063F168(pP2p->pUdp, pP2p->aSlots[i].nConn);
            strncpy(pEntry->szAddress, pInfo->szAddress2, 16);
            pEntry->nPort = pInfo->nPort2;
            pEntry->nConn = pP2p->aSlots[i].nConn;
            pEntry->nState = 2;
            pP2p->aSlots[i].nConn = -1;
            return 1;
        }
        if (strncmp(szAddress, pInfo->szAddress2, 16) == 0 && nPort == pInfo->nPort2 && nPort != 0) {
            char abKey2[64];

            func_0063E8E8(pP2p->pUdp, pP2p->aSlots[i].nConn, pInfo->nIndex);
            KM_CreateRandomRCKey(abKey2);
            KM_AddSymmetricKey(-1, pInfo->nIndex, abKey2);
            func_0063F168(pP2p->pUdp, pP2p->aSlots[i].nConn);
            strncpy(pEntry->szAddress, pInfo->szAddress, 16);
            pEntry->nPort = pInfo->nPort;
            pEntry->nConn2 = pP2p->aSlots[i].nConn;
            pEntry->nState = 2;
            pP2p->aSlots[i].nConn = -1;
            return 1;
        }
    }
    return 0;
}
