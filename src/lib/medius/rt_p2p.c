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
 * putting nState first; every source order and a chained -1 tried). Not written: the create 0x6395B8 (its
 * record reloaded from the stack after every store, as the target does, but the defaults' order),
 * 0x639B08, 0x639D70, 0x63A038, 0x63A250, 0x63AAC0, 0x63ACA0, 0x63B0B8, 0x63B708, 0x63B9A0, 0x63BED0,
 * 0x63C4F0.
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
    char x11;
    char szAddress[16];                 /* 0x12 */
    unsigned short nPort;               /* 0x22 */
    int x24;
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
    int x19C;
    void (*pfnJoin)(int nIndex, int nLocal);            /* 0x1A0: the peer that takes a leaver's place */
    void (*pfnLeave)(int nIndex, int n);                /* 0x1A4 */
    int x1A8;
    int x1AC;
    void (*pfnAddress)(char *szAddress, unsigned short nPort, int nError); /* 0x1B0 */
    int x1B4;
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

void func_0063AC98(RT_P2P *pP2p, int nConn, int nError)
{
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

/* events 7, 8 and 9 hand over through a statement macro: a call inside a loop statement is never a
 * sibling call (README "The recogniser and Medius by hand", round 7b), and these three are jals */
#define RT_P2P_RELAY(p, c, e)   do { func_0063C170(p, c, e); } while (0)

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

typedef struct RT_P2P_PEER_ADDR {       /* a peer's two addresses, as 0x63B2F0 matches them */
    char x00[64];
    char szAddress[16];                 /* 0x40: its first connection's (nWhich 0) */
    char szAddress2[16];                /* 0x50: its second's (nWhich 1) */
    unsigned short nPort;               /* 0x60 */
    unsigned short nPort2;              /* 0x62 */
} RT_P2P_PEER_ADDR;

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

typedef struct RT_P2P_INFO {            /* a peer described (0x70 bytes) */
    unsigned int nMagic;                /* 0x00: 0xEFF0CDAB */
    char szAddress[16];                 /* 0x04: the first connection's (nWhich 0) */
    char szAddress2[16];                /* 0x14: the second's */
    unsigned short nPort;               /* 0x24 */
    unsigned short nPort2;              /* 0x26 */
    int nIndex;                         /* 0x28 */
    unsigned char bLocal;               /* 0x2C: the peer is this one */
    char x2D[3];
    char abKey[64];                     /* 0x30: its public key */
} RT_P2P_INFO;

extern int KM_GetPublicKey(int nKMID, int nClientID, void *pKey);
extern int func_0062F818(int nKMID, void *pKey);

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

int func_0063C328(RT_P2P *pP2p, int *pnIndex);
int func_0063C3D0(RT_P2P *pP2p, int nLast, int nFirst);

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
