/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* libnetb: SCEA's network buffers and the sceBInet front end (OVL1 0x247040-0x248350), prebuilt
 * with ee-gcc 2.96 at -O2 -G0 (README "The GCC front"): the demo's object 0x423D38 (C), its
 * functions in the same order. A socket is a 40-byte LIBNETB over a libnet socket, with an
 * 8 KB receive and send buffer each mirrored to the IOP (rt_memory_align_alloc_init_zero gives the
 * EE's copy and the second word). Results are RT_RESULTs (2: invalid parameter) or the inet
 * library's (-512: an invalid argument, -501: no memory). Names are SOCOM 1's (s1:exact/callee);
 * the fields are named by their use here. A socket's two rings are 80-byte headers shared with the
 * IOP (LIBNETB_RING: the async receive and send calls, RPC 100/101, fill them); a datagram ring holds
 * 64-byte datagram headers (libnetb_udp.c). The newer release adds a 2-byte receive and send
 * (func_002476D0/func_00247F58, func_00247920/func_002481F8) and a connection-state check
 * (func_00247BD8). */

typedef unsigned int u_int;

extern int sprintf(char *s, const char *fmt, ...);
extern int sscanf(const char *s, const char *fmt, ...);
extern char *strstr(const char *s, const char *t);
extern char *strncpy(char *d, const char *s, unsigned int n);
extern int atoi(const char *s);
extern void *memset(void *d, int c, unsigned int n);
extern void FlushCache(int mode);
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

typedef struct LIBNETB_RING {           /* a buffer's 80-byte header, shared with the IOP */
    int pending;                        /* 0x00: a stream's bytes waiting (or an error) */
    int avail;                          /* 0x04 */
    u_int hdrsize;                      /* 0x08 */
    int state;                          /* 0x0C: 3 once the peer is gone */
    int id;                             /* 0x10: the libnet socket (sceBInetOpen) */
    int f14;                            /* 0x14 */
    int size;                           /* 0x18: the data's bytes */
    char *buf;                          /* 0x1C: the EE's copy */
    struct LIBNETB_RING *self;          /* 0x20: this header's IOP address */
    int f24;                            /* 0x24 */
    int pad28;
    char *iop_buf;                      /* 0x2C: the IOP's copy of the data */
    int pad30;
    int type;                           /* 0x34: 0 or 3 a datagram ring */
    int proto;                          /* 0x38 */
    int off;                            /* 0x3C: the read position */
    int pad40[4];
} LIBNETB_RING;

typedef struct LIBNETB_DGRAM {          /* a datagram's 64-byte header in a ring */
    int used;                           /* 0x00 */
    int valid;                          /* 0x04 */
    int size;                           /* 0x08 */
    int capacity;                       /* 0x0C */
    int pad10[5];
    int avail;                          /* 0x24 */
    int pad28[6];
} LIBNETB_DGRAM;

typedef struct LIBNETB {
    int type;                           /* +0: the inet socket's type */
    int proto;                          /* +4 */
    int id;                             /* +8: the libnet socket */
    int hdrsize;                        /* +12 */
    void *recv_buf;                     /* +16 */
    void *recv_raw;                     /* +20 */
    void *send_buf;                     /* +24 */
    void *send_raw;                     /* +28 */
    LIBNETB_RING *recv_mirror;          /* +32 */
    LIBNETB_RING *send_mirror;          /* +36 */
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
int libnetb_recv_from(LIBNETB *s, void *ptr, int count, int *pflags, sceBInetAddress *addr, int *port,
                      int *plen, int *perr);
int libnetb_send_to(LIBNETB *s, void *ptr, int count, int flags, sceBInetAddress *addr, int port,
                    int *plen, int *perr);
int libnetb_get_valid_data_header_ptr(LIBNETB_RING *r, LIBNETB_DGRAM **phdr, unsigned char *valid);
int libnetb_udp_init_buffer(char *buf, u_int size);
int func_00247F58(LIBNETB *s, void *ptr, sceBInetAddress *addr, int *pdone, int *perr);
int func_002481F8(LIBNETB *s, unsigned short val, sceBInetAddress *addr, int *pdone, int *perr);
int func_00247BD8(LIBNETB *s, int *pchk);
extern int printf(const char *fmt, ...);
extern void *memcpy(void *d, const void *s, unsigned int n);

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

static int libnetb_prepare_async_receive(void *cd, LIBNETB_RING *r);
static int libnetb_prepare_async_send(void *cd, LIBNETB_RING *r);
int libnetb_trans_data(void *src, void *dst, int size);
extern int sceInetOpen(void *cd, u_int *net_buf, int s, int ms);

/* a socket opened (GCC round 40a; the demo's sceBInetOpen 0x423F80, by its object's order between
 * Create and Close and its calls, grown here): both rings set up and handed to the IOP's async
 * receive and send, then the libnet open -- unless the socket is type 1 and opened with no timeout,
 * which waits passive (state 2) and returns at once; the rings' state then follows the open (0, or 3
 * on a failure) and each header goes back to the IOP. Byte-exact by five shapes (tools/try.py,
 * build/scratch/gcc/r40/v_open*.py, d=64 -> 0): the state tested `passive == 1` (the target's `xor`
 * with 1 and movn; `passive ? 2 : 1` gives movz); the send ring through a pointer of its own (its
 * header in v0, the receive ring's in a2); the receive ring's buf stored last and the send ring's
 * zero word last (each ring's final store in its call's delay slot); and the passive arm LAST, as an
 * else -- `ret = 0` ahead of the test, or in a first arm, is hoisted above the branch by the jump
 * optimiser (ret is dead on the open's path) and reorg fills the slot from above (`bnez`); at the
 * branch's target it is stolen into an annulled slot, the target's `bnezl`. */
int sceBInetOpen(void *cd, u_int *net_buf, LIBNETB *s, int ms)
{
    LIBNETB_RING *r;
    LIBNETB_RING *w;
    int passive = 0;
    int state;
    int ret;

    if (s == 0)
        return -512;
    if (ms == 0)
        passive = s->type == 1;
    state = passive == 1 ? 2 : 1;
    r = s->recv_mirror;
    r->state = state;
    r->type = s->type;
    r->proto = s->proto;
    r->id = s->id;
    r->hdrsize = s->hdrsize;
    r->f14 = 0;
    r->size = 8192;
    r->buf = (char *)r + 64;
    if (libnetb_prepare_async_receive(cd, r) != 0)
        return -501;
    w = s->send_mirror;
    w->state = state;
    w->buf = (char *)w + 64;
    w->type = s->type;
    w->proto = s->proto;
    w->id = s->id;
    w->hdrsize = s->hdrsize;
    w->size = 8192;
    w->f14 = 0;
    if (libnetb_prepare_async_send(cd, w) != 0)
        return -501;
    if (!passive) {
        ret = sceInetOpen(cd, net_buf, s->id, ms);
        if (ret == 0) {
            s->send_mirror->state = 0;
            s->recv_mirror->state = 0;
        } else {
            s->send_mirror->state = 3;
            s->recv_mirror->state = 3;
        }
        if (libnetb_trans_data(s->send_mirror, s->send_mirror->self, 16) != 0)
            return -501;
        if (libnetb_trans_data(s->recv_mirror, s->recv_mirror->self, 16) != 0)
            return -501;
    } else
        ret = 0;
    return ret;
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

/* a stream socket's receive from its ring (the demo's sceBInetRecv, by its object's order between
 * Close and RecvFrom, the body the demo's own grown by the peer check): the waiting bytes copied
 * out, the read position advanced, and once the ring is drained its header DMAed back to the IOP.
 * The state's load takes v0 and the descriptor's two words v0/v1 in the target's order only with FlushCache void (GCC
 * round 44a, tools/retsweep.py: open since round 31a, 6 words; the call otherwise sets v0 and keeps the next block's first
 * temporaries off it, the Medius lane's round 25b). The peer check returns its result both ways
 * (`return ret` when chk is 0: the target's v0 kept, no `move v0,zero`). The timeout `ms` is
 * unread, but its one caller passes it (rt_comm_platform2_receive 0x62E1E8: `li t2,2` before the
 * jal, the seventh argument), as sceBInetRecvFrom's (which tests it) and sceBInetSend's. */
int sceBInetRecv(void *cd, u_int *net_buf, LIBNETB *s, void *ptr, int count, int *pflags, int ms)
{
    LIBNETB_RING *r;
    struct sif_dma dma;
    int chk = 0;
    u_int id;
    int n, len, ret;

    if (s == 0)
        return -512;
    r = s->recv_mirror;
    FlushCache(0);
    if (r->state != 0) {
        ret = func_00247BD8(s, &chk);
        if (ret != 0)
            return ret;
        if (chk == 0)
            return ret;
    }
    *pflags = r->avail;
    n = r->pending;
    if (n <= 0)
        return n;
    len = count;
    if (n < len)
        len = n;
    memcpy(ptr, r->buf + r->f24, len);
    r->pending -= len;
    if (r->pending != 0) {
        r->f24 += len;
    } else {
        r->f24 = 0;
        FlushCache(0);
        dma.src = r;
        dma.dst = r->self;
        dma.size = 4;
        dma.attr = 0;
        id = sceSifSetDma(&dma, 1);
        if (id == 0)
            printf("Error DMAing data to IOP memory!!!\n");
        while (sceSifDmaStat(id) >= 0)
            ;
    }
    return len;
}

int sceBInetRecvFrom(void *cd, u_int *net_buf, LIBNETB *s, void *ptr, int count, int *pflags,
                     sceBInetAddress *addr, int *port, int ms)
{
    int len;
    int err = -501;

    if (cd == 0 || ms < -1 || net_buf == 0)
        return -501;
    if (count == 0)
        return 0;
    if (libnetb_recv_from(s, ptr, count, pflags, addr, port, &len, &err) == 0)
        return len;
    return err ? err : -501;
}

int func_002476D0(void *cd, u_int *net_buf, LIBNETB *s, void *ptr, sceBInetAddress *addr)
{
    int done = 0;
    int err = -501;

    if (cd == 0)
        return -501;
    if (func_00247F58(s, ptr, addr, &done, &err) == 0)
        return done;
    return err ? err : -501;
}

/* A stream socket's send (open since round 13a, 8 words, every order of the four field stores the same): FlushCache is
 * void -- GCC round 44a, tools/retsweep.py: as an int call it keeps v0 from the descriptors' two loaded words, each pair in
 * the other registers. An __inline__ helper per transfer gives each its own descriptor (frame 80, the target's 64), so the
 * vendor wrote both
 * out. The ring's size is an int here (slt), its header size unsigned (the prepare calls' sltu).
 * The flags and the timeout are unread, but its one caller passes both (rt_comm_platform2_send
 * 0x62DFA0: `move t1,sp` and `move t2,zero` before the jal, the sixth and seventh arguments). */
int sceBInetSend(void *cd, u_int *net_buf, LIBNETB *s, void *ptr, int count, int *pflags, int ms)
{
    LIBNETB_RING *r;
    struct sif_dma dma;
    u_int id;
    int n;

    if (s == 0)
        return -512;
    r = s->send_mirror;
    FlushCache(0);
    if (r->state != 0)
        return r->state == 3 ? -501 : 0;
    if (r->pending > 0)
        return 0;
    if (r->pending < 0)
        return r->pending;
    n = count;
    if (r->size < n)
        n = r->size;
    memcpy(r->buf, ptr, n);
    FlushCache(0);
    dma.src = r->buf;
    dma.dst = r->iop_buf;
    dma.size = n;
    dma.attr = 0;
    id = sceSifSetDma(&dma, 1);
    if (id == 0)
        printf("Error DMAing data to IOP memory!!!\n");
    while (sceSifDmaStat(id) >= 0)
        ;
    r->pending = n;
    FlushCache(0);
    dma.src = r;
    dma.dst = r->self;
    dma.size = 4;
    dma.attr = 0;
    id = sceSifSetDma(&dma, 1);
    if (id == 0)
        printf("Error DMAing data to IOP memory!!!\n");
    while (sceSifDmaStat(id) >= 0)
        ;
    return n;
}

int sceBInetSendTo(void *cd, u_int *net_buf, LIBNETB *s, void *ptr, int count, int *pflags,
                   sceBInetAddress *addr, int port, int ms)
{
    int len = 0;
    int err = -501;

    if (cd == 0 || ms < -1 || net_buf == 0)
        return -501;
    if (count == 0)
        return 0;
    if (libnetb_send_to(s, ptr, count, *pflags, addr, port, &len, &err) == 0)
        return len;
    return err ? err : -501;
}

int func_00247920(void *cd, u_int *net_buf, LIBNETB *s, unsigned short val, sceBInetAddress *addr)
{
    int done = 0;
    int err = -501;

    if (cd == 0)
        return -501;
    if (func_002481F8(s, val, addr, &done, &err) == 0)
        return done;
    return err ? err : -501;
}

int sceBInetControl(int s, int cmd, struct binet_ctl *p, int len)
{
    if (p == 0)
        return -512;
    return sceInetControl(s, cmd, p->arg, len);
}

int sceBInetGetAvailBytesToRead(LIBNETB *s, int *avail)
{
    LIBNETB_RING *r;
    unsigned char valid;
    LIBNETB_DGRAM *hdr;
    int chk;
    int ret;

    if (s == 0 || avail == 0)
        return -512;
    r = s->recv_mirror;
    FlushCache(0);
    if (r->type == 0 || r->type == 3) {
        valid = 0;
        hdr = 0;
        ret = libnetb_get_valid_data_header_ptr(r, &hdr, &valid);
        if (ret != 0)
            return ret;
        if (valid == 1) {
            *avail = hdr->avail;
            return hdr->capacity;
        }
        *avail = 0;
        return 0;
    }
    chk = 0;
    if (r->state != 0) {
        ret = func_00247BD8(s, &chk);
        if (ret != 0)
            return ret;
        if (chk == 0)
            return 0;
    }
    if (r->pending == 0) {
        *avail = r->avail;
        return 0;
    }
    return r->pending;
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

int func_00247BD8(LIBNETB *s, int *pchk)
{
    LIBNETB_RING *r;

    if (s == 0 || pchk == 0)
        return -512;
    r = s->send_mirror;
    *pchk = 0;
    FlushCache(0);
    if (r->state == 0)
        *pchk = 1;
    else if (r->state != 3)
        ;
    else
        return r->pending ? r->pending : -501;
    if (s->send_mirror->state != s->recv_mirror->state) {
        s->recv_mirror->state = s->send_mirror->state;
        if (libnetb_trans_data(s->recv_mirror, s->recv_mirror->self, 16) != 0)
            return -501;
    }
    return 0;
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

/* a datagram's 64-byte header in full, as recv_from and send_to read it (the same 64 bytes as LIBNETB_DGRAM) */
typedef struct LIBNETB_UDPHDR {
    int used;                           /* 0x00 */
    int valid;                          /* 0x04: another piece follows */
    int size;                           /* 0x08 */
    u_int capacity;                     /* 0x0C */
    sceBInetAddress addr;               /* 0x10 */
    int port;                           /* 0x20 */
    int flags;                          /* 0x24 */
    int err;                            /* 0x28 */
    int self;                           /* 0x2C: its IOP address */
    int pad30[3];
    int end;                            /* 0x3C */
} LIBNETB_UDPHDR;

extern int libnetb_is_data_waiting(LIBNETB_RING *n, unsigned char type, unsigned char peek, unsigned char *waiting);
extern int libnetb_udp_create_space(LIBNETB_RING *n, int size, int *space, unsigned char *ok);

/* a datagram taken off the receive ring: its header's fields, the data (at most count), the header
 * handed back to the IOP, the read position moved on; a header marked valid has no data of its own
 * and the next is read instead */
int libnetb_recv_from(LIBNETB *s, void *ptr, int count, int *pflags, sceBInetAddress *addr, int *port,
                      int *plen, int *perr)
{
    unsigned char waiting = 0;
    LIBNETB_RING *r;
    LIBNETB_UDPHDR *h;
    u_int n;
    int more, ret;

    if (plen == 0)
        return 2;
    *plen = 0;
    if (ptr == 0)
        return 2;
    if (count <= 0)
        return 2;
    if (pflags == 0)
        return 2;
    *pflags = 0;
    if (addr == 0)
        return 2;
    if (port == 0)
        return 2;
    if (perr == 0)
        return 2;
    *perr = 0;
    if (s == 0)
        return 2;
    r = s->recv_mirror;
    if (r == 0)
        return 2;
    FlushCache(0);
    ret = libnetb_is_data_waiting(r, 1, 0, &waiting);
    if (ret != 0)
        return ret;
    if (waiting == 0) {
        *plen = 0;
        *pflags = 0;
        *perr = 0;
        return ret;
    }
    h = (LIBNETB_UDPHDR *)(r->buf + r->off);
    more = h->valid != 0;
    if (more == 0) {
        n = h->capacity < count ? h->capacity : count;
        if (n != 0)
            memcpy(ptr, (char *)h + 64, n);
        *addr = h->addr;
        *pflags = h->flags;
        *port = h->port;
        *plen = n;
        *perr = h->err;
    }
    h->used = 0;
    h->capacity = h->size;
    h->end = 0;
    h->valid = 0;
    ret = libnetb_trans_data(h, (void *)h->self, 64);
    if (ret != 0)
        return ret;
    r->off = r->off + h->size + 64;
    if (r->off >= r->size)
        r->off = 0;
    if (more == 1) {
        ret = libnetb_recv_from(s, ptr, count, pflags, addr, port, plen, perr);
        if (ret != 0)
            return ret;
    }
    return 0;
}

/* a datagram put on the send ring: space made, the header filled, the data and the header handed to the IOP */
int libnetb_send_to(LIBNETB *s, void *ptr, int count, int flags, sceBInetAddress *addr, int port,
                    int *plen, int *perr)
{
    unsigned char ok = 0;
    int space;
    LIBNETB_RING *w;
    LIBNETB_UDPHDR *h;
    char *data;
    int off, off2, n, ret;

    if (plen == 0)
        return 2;
    *plen = 0;
    if (ptr == 0)
        return 2;
    if (count <= 0)
        return 2;
    if (addr == 0)
        return 2;
    if (perr == 0)
        return 2;
    if (s == 0)
        return 2;
    w = s->send_mirror;
    if (w == 0)
        return 2;
    if ((int)w->hdrsize < count)
        return 2;
    if (count >= w->size - 64)
        return 2;
    FlushCache(0);
    if (w->pending < 0) {
        *perr = w->pending;
        return 1;
    }
    ret = libnetb_udp_create_space(w, count, &space, &ok);
    if (ret != 0)
        return ret;
    if (ok == 1) {
        off = w->off;
        off2 = off + 64;
        h = (LIBNETB_UDPHDR *)(w->buf + off);
        n = space - 64;
        h->used = 1;
        h->valid = 0;
        data = (char *)h + 64;
        h->size = space - 64;
        h->capacity = count;
        h->addr = *addr;
        h->port = port;
        h->flags = flags;
        h->end = 1;
        h->self = (int)h;
        memcpy(data, ptr, count);
        ret = libnetb_trans_data(data, w->iop_buf + off2, n);
        if (ret != 0)
            return ret;
        ret = libnetb_trans_data(h, w->iop_buf + off, 64);
        if (ret != 0)
            return ret;
        w->off += space;
        if (w->off >= w->size)
            w->off = 0;
        *plen = count;
    } else
        *plen = 0;
    *perr = *plen;
    return 0;
}


int func_00247F58(LIBNETB *s, void *ptr, sceBInetAddress *addr, int *pdone, int *perr)
{
    int len = 0;
    int port;
    int flags = 0;
    int ret;

    if (pdone == 0)
        return 2;
    *pdone = 0;
    if (ptr == 0)
        return 2;
    if (addr == 0)
        return 2;
    if (perr == 0)
        return 2;
    *perr = 0;
    if (s == 0)
        return 2;
    ret = libnetb_recv_from(s, ptr, 2, &flags, addr, &port, &len, perr);
    if (len == 2)
        *pdone = 1;
    return ret;
}

int func_002481F8(LIBNETB *s, unsigned short val, sceBInetAddress *addr, int *pdone, int *perr)
{
    int len = 0;
    int ret;

    if (pdone == 0)
        return 2;
    *pdone = 0;
    if (addr == 0)
        return 2;
    if (perr == 0)
        return 2;
    if (s == 0)
        return 2;
    ret = libnetb_send_to(s, &val, 2, 0, addr, 0, &len, perr);
    if (len == 2)
        *pdone = 1;
    return ret;
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


static int libnetb_prepare_async_receive(void *cd, LIBNETB_RING *r)
{
    int ret;

    r->pending = 0;
    r->self = r;
    r->f24 = 0;
    r->avail = 0;
    if (sceSifMCallRpc(cd, 100, 0, r, 80, r, 80, 0, 0) < 0)
        return 1;
    if (r->type == 0 || r->type == 3) {
        if (r->size < r->hdrsize + 64)
            return 1;
        ret = libnetb_udp_init_buffer(r->buf, r->size);
        if (ret != 0)
            return ret;
    }
    return 0;
}

static int libnetb_prepare_async_send(void *cd, LIBNETB_RING *r)
{
    int ret;

    r->pending = 0;
    r->self = r;
    r->f24 = 0;
    r->avail = 0;
    if (sceSifMCallRpc(cd, 101, 0, r, 80, r, 80, 0, 0) < 0)
        return 1;
    if (r->type == 0 || r->type == 3) {
        if (r->size < r->hdrsize + 64)
            return 1;
        ret = libnetb_udp_init_buffer(r->buf, r->size);
        if (ret != 0)
            return ret;
    }
    return 0;
}
