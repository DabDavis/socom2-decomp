/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* rt_comm: the RT library's transport layer (OVL2 0x62C958-0x62DA90, SOCOM 1's object at 0x4044D0:
 * rt_comm_get_version_string_ptr .. rt_comm_metric_print), SCEA's Medius/DME network stack,
 * prebuilt with ee-gcc 2.96 at -O2 -G0 (README "The GCC front"); built with sibling calls (the
 * platform forwarders end in `j`). A channel is a block rt_comm_get_new_channel_id allocates, its
 * address the channel's id; the platform layer (rt_comm_platform -> rt_comm_platform2, SCEA's
 * libnetb socket) does the work, and every transfer is counted for the metrics. SOCOM II's TCP
 * channels learn they are connected (the flag at 0x16) before the first send or receive.
 * Names are SOCOM 1's (most s1:exact: the demo's copy is these bytes; the rest by position). */

typedef int RT_RESULT;

typedef struct RT_TIME {
    unsigned int secs;
    unsigned int usecs;
} RT_TIME;

typedef struct rt_comm_options {
    int nType;                          /* 0x00: 0 a TCP client, 1 UDP, 2 TCP accepted, 3 the third kind */
    int nOption1;                       /* 0x04 */
    int nOption2;                       /* 0x08 */
} rt_comm_options;                      /* 0x0C */

typedef struct rt_comm_channel {
    unsigned char bUsed;                /* 0x00 */
    unsigned char bCreated;             /* 0x01 */
    int hPlatform;                      /* 0x04: the platform layer's socket */
    rt_comm_options options;            /* 0x08 */
    unsigned short nPort;               /* 0x14 */
    unsigned char bConnected;           /* 0x16 */
} rt_comm_channel;                      /* 0x18 */

typedef struct rt_comm_metric {
    unsigned int nCount;                /* 0x00: since the last update */
    float fRate;                        /* 0x04: per second */
    float fMaxRate;                     /* 0x08 */
} rt_comm_metric;                       /* 0x0C */

typedef struct rt_comm_metric_data {
    RT_TIME tStart;                     /* 0x00 */
    rt_comm_metric sends;               /* 0x08 */
    rt_comm_metric bytesSent;           /* 0x14 */
    rt_comm_metric recvs;               /* 0x20 */
    rt_comm_metric bytesReceived;       /* 0x2C */
} rt_comm_metric_data;                  /* 0x38 */

#define RT_COMM_OK                  0
#define RT_COMM_ERR_GENERAL         1
#define RT_COMM_ERR_INVALID_PARAM   2
#define RT_COMM_ERR_NOT_STARTED     4
#define RT_COMM_ERR_MEMORY          9
#define RT_COMM_ERR_NO_CHANNEL      10
#define RT_COMM_ERR_INVALID_CHANNEL 11
#define RT_COMM_ERR_WRONG_TYPE      12

#define RT_COMM_TCP_OVERHEAD        40
#define RT_COMM_UDP_OVERHEAD        28

extern char *data_006555C0;             /* the version string */
extern int data_006555C4;               /* the startup count */
extern rt_comm_metric_data gMetricData;
extern RT_TIME gLastMetricPrintTime;

extern int sprintf(char *buf, const char *fmt, ...);
extern char *strstr(const char *s, const char *find);
extern char *strncpy(char *dst, const char *src, unsigned int n);
extern int atoi(const char *s);
extern int printf(const char *fmt, ...);
extern void *memset(void *p, int c, unsigned int n);
extern int rt_memory_allocate_init_zero(void **ppMemory, unsigned long nSize);
extern int rt_memory_free(void **ppMemory);
extern RT_RESULT rt_time_start(RT_TIME *start);
extern RT_RESULT rt_time_elapsed_msecs(RT_TIME *start, unsigned int *msecs);

extern int rt_comm_platform_startup(void);
extern int rt_comm_platform_shutdown(void);
extern int rt_comm_platform_create(int *phPlatform, char *szAddress, unsigned short *pnPort, rt_comm_options *pOptions,
                         unsigned char *pbConnected);
extern int rt_comm_platform_destroy(int hPlatform);
extern int rt_comm_platform_send(int hPlatform, void *pData, unsigned short nLength, unsigned short *pnSent);
extern int rt_comm_platform_send_to(int hPlatform, char *szAddress, unsigned short nPort, void *pData,
                                    unsigned short nLength, unsigned short *pnSent);
extern int func_0062EAF0(int hPlatform, unsigned char *pbSent, void *pData, unsigned short nPort);
extern int rt_comm_platform_receive(int hPlatform, void *pData, unsigned short nLength, unsigned short *pnReceived);
extern int rt_comm_platform_receive_from(int hPlatform, char *szAddress, unsigned short *pnPort, void *pData,
                                         unsigned short nLength, unsigned short *pnReceived);
extern int func_0062EB38(int hPlatform, unsigned char *pbReceived, void *pData, void *pFrom);
extern int rt_comm_platform_get_avail_bytes_to_read(int hPlatform, unsigned short *pnBytes);
extern int rt_comm_platform_get_local_ip(void *p);
extern int func_0062EB80(void *p);
extern int rt_comm_platform_get_host_by_name(char *szName, char *szAddress);
extern int rt_comm_platform_get_remote_ip(int hPlatform, char *szAddress);
extern int rt_comm_platform_display_state(void);
extern int func_0062EBE0(int hPlatform, unsigned char *pbConnected);

int rt_comm_get_channel(int nChannelId, rt_comm_channel **ppChannel);
int rt_comm_get_valid_channel(int nChannelId, rt_comm_channel **ppChannel);
int rt_comm_is_valid_channel(rt_comm_channel *pChannel);
int rt_comm_get_new_channel_id(int *pnChannelId);
int rt_comm_release_channel_id(int nChannelId);
int func_0062CB68(rt_comm_options *pOptions);
int func_0062D538(int nChannelId, unsigned char *pbConnected);
int rt_comm_metric_init(void);
int rt_comm_metric_add_data(unsigned int nSent, unsigned int nReceived, unsigned int nOverhead);
int rt_comm_metric_update(void);
int rt_comm_metric_update_single(rt_comm_metric *pMetric, unsigned int nElapsed);

char *rt_comm_get_version_string_ptr(void)
{
    return data_006555C0;
}

/* rt_comm.c's own __DATE__ and __TIME__ (Oct 3 2003, 15:45:15), spelled out; the shape is
 * rt_msg_client_get_build_timestamp's */
#define RT_COMM_BUILD_DATE  "Oct  3 2003"
#define RT_COMM_BUILD_TIME  "15:45:15"

int rt_comm_get_build_timestamp(char *szTimeStamp, short nLength)
{
    char szMonths[] = "JanFebMarAprMayJunJulAugSepOctNovDec\0";
    char szMonth[8];
    char szBuf[32];
    char *szDate;
    char *szTime;
    int nMonth;

    if (szTimeStamp == 0)
        return RT_COMM_ERR_INVALID_PARAM;
    szDate = RT_COMM_BUILD_DATE;
    szTime = RT_COMM_BUILD_TIME;
    sprintf(szMonth, "%.3s", szDate);
    nMonth = (strstr(szMonths, szMonth) - szMonths) / 3 + 1;
    sprintf(szBuf, "%.2d.%.2d.%.4s.%.2s.%.2s.%.2s", nMonth, atoi(szDate + 4), szDate + 7, szTime,
            szTime + 3, szTime + 6);
    strncpy(szTimeStamp, szBuf, nLength);
    return RT_COMM_OK;
}

int rt_comm_startup(void)
{
    int result;

    if (data_006555C4 == 0) {
        result = rt_comm_platform_startup();
        if (result != RT_COMM_OK)
            return result;
        result = rt_comm_metric_init();
        if (result != RT_COMM_OK)
            return result;
    }
    data_006555C4++;
    return RT_COMM_OK;
}

int rt_comm_shutdown(void)
{
    int result;

    if (data_006555C4 <= 0)
        return RT_COMM_ERR_NOT_STARTED;
    data_006555C4--;
    if (data_006555C4 == 0) {
        result = rt_comm_platform_shutdown();
        if (result != RT_COMM_OK)
            return result;
    }
    return RT_COMM_OK;
}

int func_0062CB68(rt_comm_options *pOptions)
{
    if (pOptions == 0)
        return RT_COMM_ERR_INVALID_PARAM;
    memset(pOptions, 0, sizeof(*pOptions));
    pOptions->nType = 0;
    return RT_COMM_OK;
}

int rt_comm_create(int *pnChannelId, char *szAddress, unsigned short nPort, rt_comm_options *pOptions)
{
    int nChannelId;
    rt_comm_channel *pChannel = 0;
    int result;

    if (pnChannelId == 0)
        return RT_COMM_ERR_INVALID_PARAM;
    *pnChannelId = 0;
    result = rt_comm_get_new_channel_id(&nChannelId);
    if (result != RT_COMM_OK)
        return result;
    result = rt_comm_get_channel(nChannelId, &pChannel);
    if (result != RT_COMM_OK)
        return result;
    if (pChannel == 0)
        return RT_COMM_ERR_NO_CHANNEL;
    if (pOptions != 0) {
        pChannel->options = *pOptions;
    } else {
        result = func_0062CB68(&pChannel->options);
        if (result != RT_COMM_OK) {
            rt_comm_release_channel_id(nChannelId);
            return result;
        }
    }
    result = rt_comm_platform_create(&pChannel->hPlatform, szAddress, &nPort, &pChannel->options, &pChannel->bConnected);
    if (result != RT_COMM_OK) {
        rt_comm_release_channel_id(nChannelId);
        return result;
    }
    pChannel->nPort = nPort;
    pChannel->bCreated = 1;
    *pnChannelId = nChannelId;
    return RT_COMM_OK;
}

int rt_comm_destroy(int nChannelId)
{
    rt_comm_channel *pChannel = 0;
    int result;

    result = rt_comm_get_valid_channel(nChannelId, &pChannel);
    if (result != RT_COMM_OK)
        return result;
    if (pChannel == 0)
        return RT_COMM_ERR_NO_CHANNEL;
    rt_comm_platform_destroy(pChannel->hPlatform);
    pChannel->bCreated = 0;
    pChannel->bUsed = 0;
    return rt_comm_release_channel_id(nChannelId);
}

int rt_comm_send(int nChannelId, void *pData, unsigned short nLength, unsigned short *pnSent)
{
    unsigned char bConnected;
    rt_comm_channel *pChannel = 0;
    int result;

    if (pnSent == 0)
        return RT_COMM_ERR_INVALID_PARAM;
    *pnSent = 0;
    if (pData == 0)
        return RT_COMM_ERR_INVALID_PARAM;
    result = rt_comm_get_valid_channel(nChannelId, &pChannel);
    if (result != RT_COMM_OK)
        return result;
    if (pChannel == 0)
        return RT_COMM_ERR_NO_CHANNEL;
    if (pChannel->options.nType != 0 && pChannel->options.nType != 2)
        return RT_COMM_ERR_WRONG_TYPE;
    if (pChannel->bConnected == 0 && pChannel->options.nType == 0) {
        bConnected = 0;
        result = func_0062D538(nChannelId, &bConnected);
        if (result != RT_COMM_OK)
            return result;
        if (bConnected == 0) {
            *pnSent = 0;
            return RT_COMM_OK;
        }
        pChannel->bConnected = 1;
    }
    result = rt_comm_platform_send(pChannel->hPlatform, pData, nLength, pnSent);
    if (result != RT_COMM_OK)
        return result;
    result = rt_comm_metric_add_data(*pnSent, 0, RT_COMM_TCP_OVERHEAD);
    return result;
}

int rt_comm_send_to(int nChannelId, char *szAddress, unsigned short nPort, void *pData, unsigned short nLength,
                    unsigned short *pnSent)
{
    rt_comm_channel *pChannel = 0;
    int result;

    if (szAddress == 0)
        return RT_COMM_ERR_INVALID_PARAM;
    if (pnSent == 0)
        return RT_COMM_ERR_INVALID_PARAM;
    *pnSent = 0;
    if (pData == 0)
        return RT_COMM_ERR_INVALID_PARAM;
    result = rt_comm_get_valid_channel(nChannelId, &pChannel);
    if (result != RT_COMM_OK)
        return result;
    if (pChannel == 0)
        return RT_COMM_ERR_NO_CHANNEL;
    if (pChannel->options.nType != 1)
        return RT_COMM_ERR_WRONG_TYPE;
    result = rt_comm_platform_send_to(pChannel->hPlatform, szAddress, nPort, pData, nLength, pnSent);
    if (result != RT_COMM_OK)
        return result;
    result = rt_comm_metric_add_data(*pnSent, 0, RT_COMM_UDP_OVERHEAD);
    return result;
}

int func_0062CEF8(int nChannelId, unsigned char *pbSent, void *pData, unsigned short nPort)
{
    rt_comm_channel *pChannel = 0;
    int result;

    if (pbSent == 0)
        return RT_COMM_ERR_INVALID_PARAM;
    *pbSent = 0;
    if (pData == 0)
        return RT_COMM_ERR_INVALID_PARAM;
    result = rt_comm_get_valid_channel(nChannelId, &pChannel);
    if (result != RT_COMM_OK)
        return result;
    if (pChannel == 0)
        return RT_COMM_ERR_NO_CHANNEL;
    if (pChannel->options.nType != 3)
        return RT_COMM_ERR_WRONG_TYPE;
    result = func_0062EAF0(pChannel->hPlatform, pbSent, pData, nPort);
    if (result != RT_COMM_OK)
        return result;
    return rt_comm_metric_add_data(RT_COMM_UDP_OVERHEAD, 0, 0);
}

int rt_comm_receive(int nChannelId, void *pData, unsigned short nLength, unsigned short *pnReceived)
{
    unsigned char bConnected;
    rt_comm_channel *pChannel = 0;
    int result;

    if (pnReceived == 0)
        return RT_COMM_ERR_INVALID_PARAM;
    *pnReceived = 0;
    if (pData == 0)
        return RT_COMM_ERR_INVALID_PARAM;
    result = rt_comm_get_valid_channel(nChannelId, &pChannel);
    if (result != RT_COMM_OK)
        return result;
    if (pChannel == 0)
        return RT_COMM_ERR_NO_CHANNEL;
    if (pChannel->options.nType != 0 && pChannel->options.nType != 2)
        return RT_COMM_ERR_WRONG_TYPE;
    if (pChannel->bConnected == 0 && pChannel->options.nType == 0) {
        bConnected = 0;
        result = func_0062D538(nChannelId, &bConnected);
        if (result != RT_COMM_OK)
            return result;
        if (bConnected == 0) {
            *pnReceived = 0;
            return RT_COMM_OK;
        }
        pChannel->bConnected = 1;
    }
    result = rt_comm_platform_receive(pChannel->hPlatform, pData, nLength, pnReceived);
    if (result != RT_COMM_OK)
        return result;
    result = rt_comm_metric_add_data(0, *pnReceived, RT_COMM_TCP_OVERHEAD);
    return result;
}

int rt_comm_receive_from(int nChannelId, char *szAddress, unsigned short *pnPort, void *pData,
                         unsigned short nLength, unsigned short *pnReceived)
{
    rt_comm_channel *pChannel = 0;
    int result;

    if (szAddress == 0)
        return RT_COMM_ERR_INVALID_PARAM;
    if (pnReceived == 0)
        return RT_COMM_ERR_INVALID_PARAM;
    *pnReceived = 0;
    if (pData == 0)
        return RT_COMM_ERR_INVALID_PARAM;
    result = rt_comm_get_valid_channel(nChannelId, &pChannel);
    if (result != RT_COMM_OK)
        return result;
    if (pChannel == 0)
        return RT_COMM_ERR_NO_CHANNEL;
    if (pChannel->options.nType != 1)
        return RT_COMM_ERR_WRONG_TYPE;
    result = rt_comm_platform_receive_from(pChannel->hPlatform, szAddress, pnPort, pData, nLength, pnReceived);
    if (result != RT_COMM_OK)
        return result;
    result = rt_comm_metric_add_data(0, *pnReceived, RT_COMM_UDP_OVERHEAD);
    return result;
}

int func_0062D178(int nChannelId, unsigned char *pbReceived, void *pData, void *pFrom)
{
    rt_comm_channel *pChannel = 0;
    int result;

    if (pbReceived == 0)
        return RT_COMM_ERR_INVALID_PARAM;
    *pbReceived = 0;
    if (pData == 0)
        return RT_COMM_ERR_INVALID_PARAM;
    if (pFrom == 0)
        return RT_COMM_ERR_INVALID_PARAM;
    result = rt_comm_get_valid_channel(nChannelId, &pChannel);
    if (result != RT_COMM_OK)
        return result;
    if (pChannel == 0)
        return RT_COMM_ERR_NO_CHANNEL;
    if (pChannel->options.nType != 3)
        return RT_COMM_ERR_WRONG_TYPE;
    result = func_0062EB38(pChannel->hPlatform, pbReceived, pData, pFrom);
    if (result != RT_COMM_OK)
        return result;
    if (*pbReceived == 1) {
        result = rt_comm_metric_add_data(0, RT_COMM_UDP_OVERHEAD, 0);
        if (result != RT_COMM_OK)
            return result;
    }
    return RT_COMM_OK;
}

int rt_comm_get_avail_bytes_to_read(int nChannelId, unsigned short *pnBytes)
{
    unsigned char bConnected;
    rt_comm_channel *pChannel = 0;
    int result;

    if (pnBytes == 0)
        return RT_COMM_ERR_INVALID_PARAM;
    *pnBytes = 0;
    result = rt_comm_get_valid_channel(nChannelId, &pChannel);
    if (result != RT_COMM_OK)
        return result;
    if (pChannel == 0)
        return RT_COMM_ERR_NO_CHANNEL;
    if (pChannel->bConnected == 0 && pChannel->options.nType == 0) {
        bConnected = 0;
        result = func_0062D538(nChannelId, &bConnected);
        if (result != RT_COMM_OK)
            return result;
        if (bConnected == 0) {
            *pnBytes = 0;
            return RT_COMM_OK;
        }
        pChannel->bConnected = 1;
    }
    return rt_comm_platform_get_avail_bytes_to_read(pChannel->hPlatform, pnBytes);
}

int rt_comm_get_local_ip(void *p)
{
    return rt_comm_platform_get_local_ip(p);
}

int func_0062D310(void *p)
{
    return func_0062EB80(p);
}

int rt_comm_get_host_by_name(char *szName, char *szAddress)
{
    return rt_comm_platform_get_host_by_name(szName, szAddress);
}

int rt_comm_get_remote_ip(int nChannelId, char *szAddress)
{
    rt_comm_channel *pChannel = 0;
    int result;

    if (szAddress == 0)
        return RT_COMM_ERR_INVALID_PARAM;
    result = rt_comm_get_valid_channel(nChannelId, &pChannel);
    if (result != RT_COMM_OK)
        return result;
    if (pChannel == 0)
        return RT_COMM_ERR_NO_CHANNEL;
    if (pChannel->options.nType != 0 && pChannel->options.nType != 2)
        return RT_COMM_ERR_WRONG_TYPE;
    return rt_comm_platform_get_remote_ip(pChannel->hPlatform, szAddress);
}

int rt_comm_get_metrics(rt_comm_metric_data *pMetrics)
{
    int result;

    if (pMetrics == 0)
        return RT_COMM_ERR_INVALID_PARAM;
    result = rt_comm_metric_update();
    if (result != RT_COMM_OK)
        return result;
    *pMetrics = gMetricData;
    return RT_COMM_OK;
}

int rt_comm_display_state(void)
{
    return rt_comm_platform_display_state();
}

int func_0062D478(int nChannelId, int **pphPlatform)
{
    rt_comm_channel *pChannel = 0;
    int result;

    if (pphPlatform == 0)
        return RT_COMM_ERR_INVALID_PARAM;
    *pphPlatform = 0;
    result = rt_comm_get_valid_channel(nChannelId, &pChannel);
    if (result != RT_COMM_OK)
        return result;
    if (pChannel == 0)
        return RT_COMM_ERR_NO_CHANNEL;
    *pphPlatform = &pChannel->hPlatform;
    return RT_COMM_OK;
}

int func_0062D4D8(int nChannelId, unsigned short *pnPort)
{
    rt_comm_channel *pChannel = 0;
    int result;

    if (pnPort == 0)
        return RT_COMM_ERR_INVALID_PARAM;
    *pnPort = 0;
    result = rt_comm_get_valid_channel(nChannelId, &pChannel);
    if (result != RT_COMM_OK)
        return result;
    if (pChannel == 0)
        return RT_COMM_ERR_NO_CHANNEL;
    *pnPort = pChannel->nPort;
    return RT_COMM_OK;
}

int func_0062D538(int nChannelId, unsigned char *pbConnected)
{
    rt_comm_channel *pChannel = 0;
    int result;

    if (pbConnected == 0)
        return RT_COMM_ERR_INVALID_PARAM;
    result = rt_comm_get_valid_channel(nChannelId, &pChannel);
    if (result != RT_COMM_OK)
        return result;
    if (pChannel == 0)
        return RT_COMM_ERR_NO_CHANNEL;
    if (pChannel->options.nType != 0)
        return RT_COMM_ERR_WRONG_TYPE;
    if (pChannel->bConnected == 1) {
        *pbConnected = pChannel->bConnected;
    } else {
        result = func_0062EBE0(pChannel->hPlatform, pbConnected);
        if (result != RT_COMM_OK)
            return result;
        pChannel->bConnected = *pbConnected;
    }
    return RT_COMM_OK;
}

int rt_comm_get_new_channel_id(int *pnChannelId)
{
    rt_comm_channel *pChannel = 0;

    if (pnChannelId == 0)
        return RT_COMM_ERR_INVALID_PARAM;
    *pnChannelId = 0;
    if (rt_memory_allocate_init_zero((void **)&pChannel, sizeof(rt_comm_channel)) != 0)
        return RT_COMM_ERR_MEMORY;
    if (pChannel == 0)
        return RT_COMM_ERR_NO_CHANNEL;
    pChannel->bUsed = 1;
    *pnChannelId = (int)pChannel;
    return RT_COMM_OK;
}

int rt_comm_release_channel_id(int nChannelId)
{
    if (nChannelId == 0)
        return RT_COMM_ERR_INVALID_PARAM;
    if (rt_memory_free((void **)&nChannelId) != 0)
        return RT_COMM_ERR_GENERAL;
    return RT_COMM_OK;
}

int rt_comm_get_channel(int nChannelId, rt_comm_channel **ppChannel)
{
    if (ppChannel == 0)
        return RT_COMM_ERR_INVALID_PARAM;
    *ppChannel = (rt_comm_channel *)nChannelId;
    return RT_COMM_OK;
}

int rt_comm_get_valid_channel(int nChannelId, rt_comm_channel **ppChannel)
{
    if (ppChannel == 0)
        return RT_COMM_ERR_INVALID_PARAM;
    if (rt_comm_get_channel(nChannelId, ppChannel) != RT_COMM_OK)
        return RT_COMM_ERR_INVALID_CHANNEL;
    if (!rt_comm_is_valid_channel(*ppChannel))
        return RT_COMM_ERR_INVALID_CHANNEL;
    return RT_COMM_OK;
}

int rt_comm_is_valid_channel(rt_comm_channel *pChannel)
{
    if (pChannel == 0)
        return 0;
    if (pChannel->bUsed == 0)
        return 0;
    if (pChannel->bCreated == 0)
        return 0;
    return (unsigned int)pChannel->options.nType < 4;
}

int rt_comm_metric_init(void)
{
    gMetricData.sends.nCount = 0;
    gMetricData.sends.fRate = 0;
    gMetricData.sends.fMaxRate = 0;
    gMetricData.bytesSent.nCount = 0;
    gMetricData.bytesSent.fRate = 0;
    gMetricData.bytesSent.fMaxRate = 0;
    gMetricData.recvs.nCount = 0;
    gMetricData.recvs.fRate = 0;
    gMetricData.recvs.fMaxRate = 0;
    gMetricData.bytesReceived.nCount = 0;
    gMetricData.bytesReceived.fRate = 0;
    gMetricData.bytesReceived.fMaxRate = 0;
    if (rt_time_start(&gMetricData.tStart) != 0)
        return RT_COMM_ERR_GENERAL;
    gLastMetricPrintTime = gMetricData.tStart;
    return RT_COMM_OK;
}

int rt_comm_metric_add_data(unsigned int nSent, unsigned int nReceived, unsigned int nOverhead)
{
    int result;
    if (nSent != 0) {
        gMetricData.sends.nCount++;
        gMetricData.bytesSent.nCount += nSent + nOverhead;
    }
    if (nReceived != 0) {
        gMetricData.recvs.nCount++;
        gMetricData.bytesReceived.nCount += nReceived + nOverhead;
    }
    result = rt_comm_metric_update();
    if (result != 0)
        return result;
    return 0;
}

int rt_comm_metric_update(void)
{
    unsigned int nElapsed;
    int result;

    if (rt_time_elapsed_msecs(&gMetricData.tStart, &nElapsed) != 0)
        return RT_COMM_ERR_GENERAL;
    if (nElapsed > 1000) {
        result = rt_comm_metric_update_single(&gMetricData.sends, nElapsed);
        if (result != RT_COMM_OK)
            return result;
        result = rt_comm_metric_update_single(&gMetricData.bytesSent, nElapsed);
        if (result != RT_COMM_OK)
            return result;
        result = rt_comm_metric_update_single(&gMetricData.recvs, nElapsed);
        if (result != RT_COMM_OK)
            return result;
        result = rt_comm_metric_update_single(&gMetricData.bytesReceived, nElapsed);
        if (result != RT_COMM_OK)
            return result;
        if (rt_time_start(&gMetricData.tStart) != 0)
            return RT_COMM_ERR_GENERAL;
    }
    return RT_COMM_OK;
}

int rt_comm_metric_update_single(rt_comm_metric *pMetric, unsigned int nElapsed)
{
    if (pMetric == 0)
        return RT_COMM_ERR_INVALID_PARAM;
    if (nElapsed != 0)
        pMetric->fRate = (float)pMetric->nCount / ((float)nElapsed / 1000.0f);
    else
        pMetric->fRate = 0;
    if (pMetric->fRate > pMetric->fMaxRate)
        pMetric->fMaxRate = pMetric->fRate;
    pMetric->nCount = 0;
    return RT_COMM_OK;
}

int rt_comm_metric_print(void)
{
    unsigned int nElapsed;

    if (rt_time_elapsed_msecs(&gLastMetricPrintTime, &nElapsed) != 0)
        return RT_COMM_ERR_GENERAL;
    if (nElapsed > 1000) {
    printf("     sends/sec: %9.2f, %9.2f max\n", gMetricData.sends.fRate, gMetricData.sends.fMaxRate);
    printf("bytes sent/sec: %9.2f, %9.2f max\n", gMetricData.bytesSent.fRate, gMetricData.bytesSent.fMaxRate);
    printf("     recvs/sec: %9.2f, %9.2f max\n", gMetricData.recvs.fRate, gMetricData.recvs.fMaxRate);
    printf("bytes recv/sec: %9.2f, %9.2f max\n", gMetricData.bytesReceived.fRate,
           gMetricData.bytesReceived.fMaxRate);
    printf("\n");
    if (rt_time_start(&gLastMetricPrintTime) != 0)
        return RT_COMM_ERR_GENERAL;
    }
    return RT_COMM_OK;
}
