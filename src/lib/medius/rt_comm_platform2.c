/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* rt_comm_platform2: the RT library's PS2 transport (OVL2 0x62DA90-0x62EA58, SOCOM 1's object at 0x405418:
 * rt_comm_platform2_startup .. rt_comm_platform2_get_local_ip_from_stack), SCEA's Medius/DME network
 * stack, prebuilt with ee-gcc 2.96 at -O2 -G0 (README "The GCC front"): sockets over SCEA's libnetb
 * (OVL1 0x245690-0x248350, its sceBInet* calls take the SIF client and the net buffer first). The
 * library is up once rt_comm_platform2_startup has found an interface; results are rt_comm's (1 the
 * inet layer refused, 2 a bad parameter, 5 not started, 13 the call would block). Names are SOCOM 1's
 * (s1:exact/calls); the rest keep placeholders. Round 8b added rt_comm_platform2_create (0x62DBD8: its
 * timeout a conditional expression -- an `if` keeps the constant 1 across the name lookup in a saved
 * register and branches on it) and the interface search 0x62E7E0.
 *
 * Shapes, each measured: a receive's length clamp is passed as the ternary itself to a libnetb
 * parameter that is an unsigned short (2.96's `xori v0,v0,0` before the movz, the clamp in the
 * argument register; a clamped local, int or short, re-extends); the started flag's 1 is reused
 * for every `= 1`/`return 1` that follows its test. OPEN: rt_comm_platform2_receive_from (41s: the
 * target's flags sit below the port in the frame, 32 against 36, and the count is kept in a0 before
 * s0 -- declaring the port first, the count short, or flags an array all miss). */

extern unsigned char data_006555C8;     /* started */
extern char data_006555D0[];            /* the interface's address, dotted */
extern char data_006555E8[];            /* the second address, dotted */
extern int data_006555FC;               /* the interface id, -1 without one */
extern char gSifMClientData[];
extern char gNetBuf[];

extern void *memset(void *p, int c, unsigned int n);
extern char *strcpy(char *dst, const char *src);
extern int sceBInetSend(void *cd, void *buf, int s, void *data, int len, int *flags, int ms);
extern int sceBInetSendTo(void *cd, void *buf, int s, void *data, int len, int *flags, void *addr,
                          int port, int ms);
extern int sceBInetRecvFrom(void *cd, void *buf, int s, void *data, unsigned short len, int *flags, void *addr,
                            unsigned short *port, int ms);
extern int sceBInetGetAvailBytesToRead(int s, int *flags);
extern int sceBInetIP2Address(void *addr, char *ip);
extern int sceBInetAddress2IP(char *ip, void *addr);
extern int sceBInetClose(void *cd, void *buf, int s, int ms);
extern int sceBInetControl(void *cd, void *buf, int s, int cmd, void *p, int len);
extern int sceBInetDisplayLibnetbState(void *cd);
extern int sceInetName2Address(void *cd, void *buf, int flags, void *addr, const char *name, int ms,
                               int nretry);
extern int sceInetAddress2String(void *cd, void *buf, char *name, int len, void *addr);
extern int sceBInetRecv(void *cd, void *buf, int s, void *data, unsigned short len, int *flags, int ms);
extern int func_002476D0(void *cd, void *buf, int s, unsigned short *port, void *addr);
extern int func_00247920(void *cd, void *buf, int s, int port, void *addr);
extern int func_00247BD8(int s, int *pbConnected);
extern int sceSifMInitRpc(unsigned int mode);
extern int sceLibnetInitialize(void *cd, int a, int b, int c);
extern int func_00246B90(void *cd, void *buf);
extern int func_00246BD0(void *cd, void *buf);
extern int sceLibnetTerminate(void *cd);
extern int func_00247008(void *cd, void *buf, void *pIfList, int n, void *addr, int flags);
extern int sceInetInterfaceControl(void *cd, void *buf, int id, int code, void *p, int len);
extern int sceInetGetInterfaceList(void *cd, void *buf, int *ids, int n);
extern int data_00672180[32];           /* the interfaces' ids */

typedef struct RT_TIME {
    unsigned int secs;
    unsigned int usecs;
} RT_TIME;

extern int rt_time_start(RT_TIME *start);
extern int rt_time_elapsed(RT_TIME *start, unsigned int *msecs);
extern int rt_time_delay(unsigned int msecs);

int func_0062E7E0(void);
int func_0062E940(char *szIP);
int func_0062E9C0(char *szIP);

typedef struct sceInetAddress {
    int reserved;
    unsigned char data[12];
} sceInetAddress;                       /* 0x10 */

typedef struct sceInetInfo {
    char pad0[32];
    int local_port;                     /* 0x20 */
    sceInetAddress remote;              /* 0x24: the peer's address */
    char pad34[24];
} sceInetInfo;                          /* 0x4C: sceBInetControl's connection info (command 1) */

/* sceBInetCreate's parameters (libnetb's; the first word a long: stored with sd) */
typedef struct sceBInetParam {
    long bufsize;                       /* 0x00: a UDP socket's buffer */
    int type;                           /* 0x08: 1 connect, 0 datagram, 2 listen, 3 the third kind */
    int local_port;                     /* 0x0C: -1 any */
    sceInetAddress remote_addr;         /* 0x10 */
    int remote_port;                    /* 0x20 */
    char reserved[36];
} sceBInetParam;                        /* 0x48 */

/* a channel's options (rt_comm.c's rt_comm_options, as this layer reads them) */
typedef struct rt_comm_options {
    int nType;                          /* 0x00: 0 TCP client, 1 UDP, 2 TCP accepting, 3 the third kind */
    int nBufSize;                       /* 0x04: a UDP socket's buffer */
    unsigned char bNoBlock;             /* 0x08: a TCP client connects without waiting */
} rt_comm_options;

extern int sceBInetCreate(void *cd, void *buf, int *ps, sceBInetParam *param);
extern int sceBInetOpen(void *cd, void *buf, int s, int ms);

int rt_comm_platform2_startup(void)
{
    int result;

    if (data_006555C8 != 0)
        return 4;
    sceSifMInitRpc(0);
    sceLibnetInitialize(gSifMClientData, 2048, 8192, 32);
    if (func_00246B90(gSifMClientData, gNetBuf) < 0)
        return 1;
    result = func_0062E7E0();
    if (result != 0)
        return result;
    if (data_006555FC < 0)
        return 1;
    result = func_0062E940(data_006555D0);
    if (result != 0)
        return result;
    result = func_0062E9C0(data_006555E8);
    if (result != 0)
        return result;
    data_006555C8 = 1;
    return 0;
}

int rt_comm_platform2_shutdown(void)
{
    if (data_006555C8 != 1)
        return 5;
    if (func_00246BD0(gSifMClientData, gNetBuf) < 0)
        return 1;
    sceLibnetTerminate(gSifMClientData);
    data_006555C8 = 0;
    data_006555FC = -1;
    return 0;
}

/* a socket made and opened for a channel's kind (round 8b; SOCOM 1's with the third kind): a TCP
 * client connects to the address (blocking unless the options say not), a UDP socket binds the port
 * given (or any, the port it got told back), the accepting kind listens on the port, the third kind
 * takes local port 1; the TCP kinds set the socket option 3 */
int rt_comm_platform2_create(int *phSocket, char *szAddress, unsigned short *pnPort,
                             rt_comm_options *pOptions, unsigned char *pbConnected)
{
    sceBInetParam param;
    sceInetInfo info;
    int hSocket = 0;
    int nTimeout;
    int nResult;
    int one;

    if (phSocket == 0)
        return 2;
    *phSocket = 0;
    if (data_006555C8 != 1)
        return 5;
    if (pOptions == 0)
        return 2;
    if (pbConnected == 0)
        return 2;
    *pbConnected = 0;
    if (pOptions->nType == 0 && szAddress == 0)
        return 2;
    if (pOptions->nType != 3 && *pnPort == 0)
        return 2;
    memset(&param, 0, sizeof(param));
    if (pOptions->nType == 0) {
        if (szAddress == 0)
            return 2;
        param.local_port = -1;
        param.remote_port = *pnPort;
        param.type = 1;
        if (sceInetName2Address(gSifMClientData, gNetBuf, 0, &param.remote_addr, szAddress, 0, 0) < 0)
            return 1;
        nTimeout = (pOptions->bNoBlock == 1) ? 0 : 10000;
    } else if (pOptions->nType == 1) {
        if (szAddress != 0)
            return 2;
        if (pOptions->nBufSize <= 0)
            return 2;
        if (*pnPort == 0)
            param.local_port = -1;
        else
            param.local_port = *pnPort;
        param.bufsize = pOptions->nBufSize;
        nTimeout = 10000;
        param.remote_port = 0;
        param.type = 0;
    } else if (pOptions->nType == 3) {
        if (szAddress != 0)
            return 2;
        param.local_port = 1;
        param.type = 3;
        nTimeout = 10000;
        param.remote_port = 0;
        if (sceInetName2Address(gSifMClientData, gNetBuf, 0, &param.remote_addr, 0, 0, 0) < 0)
            return 1;
    } else if (pOptions->nType == 2) {
        if (szAddress != 0)
            return 2;
        param.local_port = *pnPort;
        nTimeout = 0;
        param.type = 2;
        param.remote_port = 0;
        if (sceInetName2Address(gSifMClientData, gNetBuf, 0, &param.remote_addr, 0, 0, 0) < 0)
            return 1;
    } else {
        return 3;
    }
    if (sceBInetCreate(gSifMClientData, gNetBuf, &hSocket, &param) != 0)
        return 1;
    nResult = sceBInetOpen(gSifMClientData, gNetBuf, hSocket, nTimeout);
    if (nResult != 0) {
        if (nResult != -500 || pOptions->nType != 2) {
            sceBInetClose(gSifMClientData, gNetBuf, hSocket, 500);
            return 1;
        }
    }
    if (pOptions->nType == 0 || pOptions->nType == 2) {
        one = 1;
        if (sceBInetControl(gSifMClientData, gNetBuf, hSocket, 3, &one, sizeof(one)) != 0)
            return 1;
    }
    if (pOptions->nType == 1 && *pnPort == 0) {
        memset(&info, 0, sizeof(info));
        if (sceBInetControl(gSifMClientData, gNetBuf, hSocket, 1, &info, sizeof(info)) != 0)
            return 1;
        *pnPort = info.local_port;
    }
    if (pOptions->nType == 0 && pOptions->bNoBlock == 1)
        *pbConnected = 0;
    else
        *pbConnected = 1;
    *phSocket = hSocket;
    return 0;
}

int rt_comm_platform2_destroy(int s)
{
    if (data_006555C8 != 1)
        return 5;
    if (s < 0)
        return 2;
    if (sceBInetClose(gSifMClientData, gNetBuf, s, 500) != 0)
        return 1;
    return 0;
}

int rt_comm_platform2_send(int s, void *pData, unsigned short nLength, unsigned short *pnSent)
{
    int flags = 0;
    int nSent;

    if (data_006555C8 != 1)
        return 5;
    if (s < 0)
        return 2;
    if (pData == 0)
        return 2;
    if (pnSent == 0)
        return 2;
    nSent = sceBInetSend(gSifMClientData, gNetBuf, s, pData, nLength, &flags, 0);
    if (nSent < 0)
        return 1;
    *pnSent = nSent;
    return 0;
}

int rt_comm_platform2_send_to(int s, char *szAddress, unsigned short nPort, void *pData, unsigned short nLength,
                              unsigned short *pnSent)
{
    sceInetAddress addr;
    int flags = 0;
    int nSent;

    if (data_006555C8 != 1)
        return 5;
    if (s < 0)
        return 2;
    if (szAddress == 0)
        return 2;
    if (nPort == 0)
        return 2;
    if (pData == 0)
        return 2;
    if (pnSent == 0)
        return 2;
    *pnSent = 0;
    if (sceBInetIP2Address(&addr, szAddress) < 0)
        return 1;
    nSent = sceBInetSendTo(gSifMClientData, gNetBuf, s, pData, nLength, &flags, &addr, nPort, 0);
    if (nSent < 0)
        return 1;
    *pnSent = nSent;
    return 0;
}

int func_0062E128(int s, unsigned char *pbSent, char *szAddress, unsigned short nPort)
{
    sceInetAddress addr;
    int nSent;

    if (pbSent == 0)
        return 2;
    *pbSent = 0;
    if (data_006555C8 != 1)
        return 5;
    if (s < 0)
        return 2;
    if (szAddress == 0)
        return 2;
    if (sceBInetIP2Address(&addr, szAddress) < 0)
        return 1;
    nSent = func_00247920(gSifMClientData, gNetBuf, s, nPort, &addr);
    if (nSent < 0)
        return 1;
    if (nSent > 0)
        *pbSent = 1;
    return 0;
}

int rt_comm_platform2_receive(int s, void *pData, unsigned short nLength, unsigned short *pnReceived)
{
    int flags = 0;
    int nReceived;

    if (data_006555C8 != 1)
        return 5;
    if (s < 0)
        return 2;
    if (pData == 0)
        return 2;
    if (pnReceived == 0)
        return 2;
    nReceived = sceBInetRecv(gSifMClientData, gNetBuf, s, pData, (nLength > 4096) ? 4096 : nLength, &flags, 2);
    if (nReceived < 0) {
        if (flags & 4)
            return 13;
        return 1;
    }
    if (flags & 4)
        return 13;
    *pnReceived = nReceived;
    return 0;
}

int rt_comm_platform2_receive_from(int s, char *szAddress, unsigned short *pnPort, void *pData,
                                   unsigned short nLength, unsigned short *pnReceived)
{
    sceInetAddress addr;
    int flags = 0;
    unsigned short nPort;
    int nReceived;

    if (data_006555C8 != 1)
        return 5;
    if (s < 0)
        return 2;
    if (szAddress == 0)
        return 2;
    if (pnPort == 0)
        return 2;
    if (pData == 0)
        return 2;
    if (pnReceived == 0)
        return 2;
    *pnReceived = 0;
    nReceived = sceBInetRecvFrom(gSifMClientData, gNetBuf, s, pData, (nLength > 4096) ? 4096 : nLength, &flags,
                                 &addr, &nPort, 2);
    if (nReceived < 0) {
        if (nReceived != -500)
            return 1;
        nReceived = 0;
    }
    if (nReceived > 0) {
        *pnPort = nPort;
        if (sceBInetAddress2IP(szAddress, &addr) != 0)
            return 1;
    }
    *pnReceived = nReceived;
    return 0;
}

int func_0062E3D0(int s, unsigned char *pbReceived, char *szAddress, unsigned short *pnPort)
{
    sceInetAddress addr;
    unsigned short nPort;
    int nReceived;

    if (pbReceived == 0)
        return 2;
    *pbReceived = 0;
    if (data_006555C8 != 1)
        return 5;
    if (s < 0)
        return 2;
    if (szAddress == 0)
        return 2;
    nReceived = func_002476D0(gSifMClientData, gNetBuf, s, &nPort, &addr);
    if (nReceived < 0) {
        if (nReceived != -500)
            return 1;
        nReceived = 0;
    }
    if (nReceived > 0) {
        *pbReceived = 1;
        *pnPort = nPort;
        if (sceBInetAddress2IP(szAddress, &addr) != 0)
            return 1;
    }
    return 0;
}

int rt_comm_platform2_get_avail_bytes_to_read(int s, unsigned short *pnBytes)
{
    int flags = 0;
    int nBytes;

    if (data_006555C8 != 1)
        return 5;
    if (s < 0)
        return 2;
    if (pnBytes == 0)
        return 2;
    *pnBytes = 0;
    nBytes = sceBInetGetAvailBytesToRead(s, &flags);
    if (nBytes < 0) {
        if (nBytes >= -510 && nBytes <= -508)
            return 13;
        return 1;
    }
    if (flags & 4)
        return 13;
    *pnBytes = nBytes;
    return 0;
}

int rt_comm_platform2_get_local_ip(char *szIP)
{
    if (data_006555C8 != 1)
        return 5;
    if (szIP == 0)
        return 2;
    strcpy(szIP, data_006555D0);
    return 0;
}

int func_0062E580(char *szIP)
{
    if (data_006555C8 != 1)
        return 5;
    if (szIP == 0)
        return 2;
    strcpy(szIP, data_006555E8);
    return 0;
}

int rt_comm_platform2_get_host_by_name(char *szName, char *szAddress)
{
    sceInetAddress addr;

    if (data_006555C8 != 1)
        return 5;
    if (szName == 0)
        return 2;
    if (szAddress == 0)
        return 2;
    if (sceInetName2Address(gSifMClientData, gNetBuf, 0, &addr, szName, 0, 0) < 0)
        return 1;
    if (sceInetAddress2String(gSifMClientData, gNetBuf, szAddress, 16, &addr) < 0)
        return 1;
    return 0;
}

int rt_comm_platform2_get_remote_ip(int s, char *szAddress)
{
    sceInetInfo info;

    if (data_006555C8 != 1)
        return 5;
    if (s < 0)
        return 2;
    if (szAddress == 0)
        return 2;
    if (sceBInetControl(gSifMClientData, gNetBuf, s, 1, &info, sizeof(info)) != 1)
        return 1;
    if (sceInetAddress2String(gSifMClientData, gNetBuf, szAddress, 16, &info.remote) != 0)
        return 1;
    return 0;
}

int rt_comm_platform2_display_state(void)
{
    if (data_006555C8 != 1)
        return 5;
    if (sceBInetDisplayLibnetbState(gSifMClientData) != 0)
        return 1;
    return 0;
}

int func_0062E760(int s, unsigned char *pbConnected)
{
    int bConnected = 0;

    if (data_006555C8 != 1)
        return 5;
    if (s < 0)
        return 2;
    if (pbConnected == 0)
        return 2;
    *pbConnected = 0;
    if (func_00247BD8(s, &bConnected) != 0)
        return 1;
    if (bConnected != 0)
        *pbConnected = 1;
    return 0;
}

/* the interface search (round 8b): the inet layer's interfaces listed, each one's flags read until
 * one is up and running (both low bits) -- its id kept -- for at most 95 seconds, 2 ms apart */
int func_0062E7E0(void)
{
    RT_TIME start;
    int flags;
    unsigned int elapsed = 0;
    int nInterfaces;
    int i;
    int bFound = 0;

    nInterfaces = sceInetGetInterfaceList(gSifMClientData, gNetBuf, data_00672180, 32);
    if (nInterfaces <= 0)
        return 1;
    if (rt_time_start(&start) != 0)
        return 1;
    while (!bFound) {
        for (i = 0; i < nInterfaces; i++) {
            int id = data_00672180[i];

            if (sceInetInterfaceControl(gSifMClientData, gNetBuf, id, 8, &flags, 4) != 0)
                return 1;
            if ((flags & 3) == 3) {
                bFound = 1;
                data_006555FC = id;
                break;
            }
        }
        if (!bFound) {
            if (rt_time_elapsed(&start, &elapsed) != 0)
                return 1;
            if (elapsed > 95000)
                return 1;
            if (rt_time_delay(2) != 0)
                return 1;
        }
    }
    return 0;
}

int func_0062E940(char *szIP)
{
    sceInetAddress addr;
    char ifList[16];

    if (szIP == 0)
        return 2;
    memset(&addr, 0, sizeof(addr));
    if (func_00247008(gSifMClientData, gNetBuf, ifList, 1, &addr, 2) < 0)
        return 1;
    if (sceBInetAddress2IP(szIP, &addr) != 0)
        return 1;
    return 0;
}

int func_0062E9C0(char *szIP)
{
    sceInetAddress addr;

    if (szIP == 0)
        return 2;
    if (data_006555FC < 0)
        return 1;
    memset(&addr, 0, sizeof(addr));
    if (sceInetInterfaceControl(gSifMClientData, gNetBuf, data_006555FC, 11, &addr, sizeof(addr)) != 0)
        return 1;
    if (sceBInetAddress2IP(szIP, &addr) != 0)
        return 1;
    return 0;
}
