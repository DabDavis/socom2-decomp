/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* rt_comm_platform2: the RT library's PS2 transport (OVL2 0x62DA90-0x62EA58, SOCOM 1's object at 0x405418:
 * rt_comm_platform2_startup .. rt_comm_platform2_get_local_ip_from_stack), SCEA's Medius/DME network
 * stack, prebuilt with ee-gcc 2.96 at -O2 -G0 (README "The GCC front"): sockets over SCEA's libnetb
 * (OVL1 0x245690-0x248350, its sceBInet* calls take the SIF client and the net buffer first). The
 * library is up once rt_comm_platform2_startup has found an interface; results are rt_comm's (1 the
 * inet layer refused, 2 a bad parameter, 5 not started, 13 the call would block). Names are SOCOM 1's
 * (s1:exact/calls); the rest keep placeholders. Not yet here: rt_comm_platform2_create (0x62DBD8, 0x378)
 * and the interface search 0x62E7E0.
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
extern int func_002474F8(void *cd, void *buf, int s, void *data, unsigned short len, int *flags, int ms);
extern int func_002476D0(void *cd, void *buf, int s, unsigned short *port, void *addr);
extern int func_00247920(void *cd, void *buf, int s, int port, void *addr);
extern int func_00247BD8(int s, int *pbConnected);
extern int sceSifMInitRpc(unsigned int mode);
extern int func_00246A38(void *cd, int a, int b, int c);
extern int func_00246B90(void *cd, void *buf);
extern int func_00246BD0(void *cd, void *buf);
extern int func_00246B10(void *cd);
extern int func_00247008(void *cd, void *buf, void *pIfList, int n, void *addr, int flags);
extern int func_002465E8(void *cd, void *buf, int id, int code, void *p, int len);

int func_0062E7E0(void);
int func_0062E940(char *szIP);
int func_0062E9C0(char *szIP);

typedef struct sceInetAddress {
    int reserved;
    unsigned char data[12];
} sceInetAddress;                       /* 0x10 */

typedef struct sceInetInfo {
    char pad0[36];
    sceInetAddress remote;              /* 0x24: the peer's address */
    char pad34[24];
} sceInetInfo;                          /* 0x4C: sceBInetControl's connection info (command 1) */

int rt_comm_platform2_startup(void)
{
    int result;

    if (data_006555C8 != 0)
        return 4;
    sceSifMInitRpc(0);
    func_00246A38(gSifMClientData, 2048, 8192, 32);
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
    func_00246B10(gSifMClientData);
    data_006555C8 = 0;
    data_006555FC = -1;
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
    nReceived = func_002474F8(gSifMClientData, gNetBuf, s, pData, (nLength > 4096) ? 4096 : nLength, &flags, 2);
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
    if (func_002465E8(gSifMClientData, gNetBuf, data_006555FC, 11, &addr, sizeof(addr)) != 0)
        return 1;
    if (sceBInetAddress2IP(szIP, &addr) != 0)
        return 1;
    return 0;
}
