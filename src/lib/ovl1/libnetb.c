/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* libnetb: SCEA's network buffers and the sceBInet front end (OVL1 0x247040-0x248350), prebuilt
 * with ee-gcc 2.96 at -O2 -G0 (README "The GCC front"): the demo's object 0x423D38 (C), its
 * functions in the same order. A socket is a 40-byte LIBNETB over a libnet socket, with an
 * 8 KB receive and send buffer each mirrored to the IOP (rt_memory_align_alloc_init_zero gives the
 * EE's copy and the second word). Results are RT_RESULTs (2: invalid parameter) or the inet
 * library's (-512: an invalid argument, -501: no memory). Names are SOCOM 1's (s1:exact/callee);
 * the fields are named by their use here. Not yet written: Open, Recv, RecvFrom, Send, SendTo,
 * GetAvailBytesToRead, recv_from, send_to and the three unnamed helpers. */

typedef unsigned int u_int;

extern int sprintf(char *s, const char *fmt, ...);
extern int sscanf(const char *s, const char *fmt, ...);
extern char *strstr(const char *s, const char *t);
extern char *strncpy(char *d, const char *s, unsigned int n);
extern int atoi(const char *s);
extern void *memset(void *d, int c, unsigned int n);
extern int FlushCache(int mode);
extern u_int sceSifSetDma(void *dma, int n);
extern int sceSifDmaStat(u_int id);
extern int sceSifMCallRpc(void *cd, int fno, int mode, void *send, int ssize, void *recv, int rsize,
                          void *end, void *end_arg);
extern int rt_memory_allocate_init_zero(void *p, int size);
extern int rt_memory_align_alloc_init_zero(void *p, int size, int align);
extern int rt_memory_align_free(void *p);
extern int rt_memory_free(void *p);
extern int sceInetCreate(void *cd, u_int *net_buf, void *param);
extern int sceInetControl(int s, int cmd, int arg, int len);       /* libnet.c's, its own way */

extern char *data_003DCFC0;                     /* the library's version string */

struct binet_ctl {
    int a, b;
    int arg;
};

typedef struct LIBNETB {
    int type;                           /* +0: the inet socket's type */
    int proto;                          /* +4 */
    int id;                             /* +8: the libnet socket */
    int hdrsize;                        /* +12 */
    void *recv_buf;                     /* +16 */
    void *recv_raw;                     /* +20 */
    void *send_buf;                     /* +24 */
    void *send_raw;                     /* +28 */
    void *recv_mirror;                  /* +32 */
    void *send_mirror;                  /* +36 */
} LIBNETB;

typedef struct sceBInetParam {
    int hdrsize;                        /* +0: a type-0 socket's */
    int pad04;
    int type;                           /* +8: the sceInetParam libnet takes starts here */
    int pad0c[5];
    int proto;                          /* +32 */
} sceBInetParam;

typedef struct sceBInetAddress {
    int family;
    unsigned char addr[4];              /* +4, least significant first */
    int pad[2];
} sceBInetAddress;

struct sif_dma {
    void *src;
    void *dst;
    int size;
    int attr;
};

int libnetb_free_bufs(LIBNETB *n);
static int libnetb_alloc_bufs(LIBNETB *n);

char *sceBInetGetVersionStringPtr(void)
{
    return data_003DCFC0;
}

/* OPEN (83 words, the frame 144 for 160): the target keeps six registers (the stamp's address
 * too) and copies the month table first; the order of the work is the target's. */
int sceBInetGetBuildTimestamp(char *buf, int len)
{
    char months[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
    char mon[16];
    char stamp[32];
    const char *date = "Aug 20 2003";                   /* __DATE__ and __TIME__ when it was built */
    const char *time = "15:15:35";
    int m;

    if (buf == 0)
        return -512;
    sprintf(mon, "%.3s", date);
    m = (strstr(months, mon) - months) / 3 + 1;
    sprintf(stamp, "%.2d.%.2d.%.4s.%.2s.%.2s.%.2s", m, atoi(date + 4), date + 7, time, time + 3, time + 6);
    strncpy(buf, stamp, len);
    return 0;
}

int sceBInetCreate(void *cd, u_int *net_buf, int *ps, sceBInetParam *param)
{
    LIBNETB *n;
    int s;

    if (ps == 0 || param == 0)
        return -512;
    *ps = 0;
    n = 0;
    if (rt_memory_allocate_init_zero(&n, sizeof(LIBNETB)) != 0 || n == 0)
        return -501;
    n->type = param->type;
    n->proto = param->proto;
    if (n->type == 0)
        n->hdrsize = param->hdrsize;
    else if (n->type == 3)
        n->hdrsize = 4;
    else
        n->hdrsize = 0;
    if (libnetb_alloc_bufs(n) != 0)
        return -501;
    s = sceInetCreate(cd, net_buf, &param->type);
    if (s < 0) {
        libnetb_free_bufs(n);
        rt_memory_free(&n);
        return s;
    }
    n->id = s;
    *ps = (int)n;
    return 0;
}

int sceBInetClose(void *cd, u_int *net_buf, LIBNETB *s, int ms)
{
    LIBNETB *n;
    int ret;

    if (s == 0)
        return -512;
    net_buf[0] = s->id;
    net_buf[1] = ms;
    n = s;
    ret = sceSifMCallRpc(cd, 102, 0, net_buf, 16, net_buf, 16, 0, 0);
    if (libnetb_free_bufs(n) != 0)
        return -501;
    if (rt_memory_free(&n) != 0)
        return -501;
    return (ret < 0) ? -501 : 0;
}

int sceBInetControl(int s, int cmd, struct binet_ctl *p, int len)
{
    if (p == 0)
        return -512;
    return sceInetControl(s, cmd, p->arg, len);
}

int sceBInetIP2Address(sceBInetAddress *a, char *ip)
{
    u_int b[4];

    if (a == 0 || ip == 0)
        return -512;
    memset(a, 0, sizeof *a);
    if (sscanf(ip, "%u.%u.%u.%u", &b[0], &b[1], &b[2], &b[3]) != 4)
        return -512;
    a->family = 0;
    a->addr[0] = b[3];
    a->addr[1] = b[2];
    a->addr[2] = b[1];
    a->addr[3] = b[0];
    return 0;
}

int sceBInetAddress2IP(char *ip, sceBInetAddress *a)
{
    if (a == 0 || ip == 0)
        return -512;
    sprintf(ip, "%u.%u.%u.%u", a->addr[3], a->addr[2], a->addr[1], a->addr[0]);
    return 0;
}

void sceBInetDisplayLibnetbState(void *cd)
{
    FlushCache(0);
    sceSifMCallRpc(cd, 103, 0, 0, 0, 0, 0, 0, 0);
}

int libnetb_trans_data(void *src, void *dst, int size)
{
    struct sif_dma dma;
    u_int id;

    if (src == 0)
        return 2;
    if (dst == 0)
        return 2;
    if (size == 0)
        return 2;
    FlushCache(0);
    dma.src = src;
    dma.dst = dst;
    dma.size = size;
    dma.attr = 0;
    id = sceSifSetDma(&dma, 1);
    if (id == 0)
        return 4;
    while (sceSifDmaStat(id) >= 0)
        ;
    return 0;
}

static int libnetb_alloc_bufs(LIBNETB *n)
{
    if (n == 0)
        return 2;
    if (n->hdrsize + 64 > 8192)
        return 1;
    if (rt_memory_align_alloc_init_zero(&n->recv_buf, 8256, 64) != 0)
        return 3;
    n->recv_mirror = n->recv_raw;
    if (rt_memory_align_alloc_init_zero(&n->send_buf, 8256, 64) != 0)
        return 3;
    n->send_mirror = n->send_raw;
    return 0;
}

int libnetb_free_bufs(LIBNETB *n)
{
    if (n == 0)
        return 2;
    if (rt_memory_align_free(&n->recv_buf) != 0)
        return 1;
    return rt_memory_align_free(&n->send_buf) != 0;
}
