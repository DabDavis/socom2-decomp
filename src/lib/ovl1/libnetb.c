/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* libnetb: SCEA's network buffers and the sceBInet front end (OVL1 0x245690-0x248350), prebuilt
 * with ee-gcc 2.96 at -O2 -G0 (README "The GCC front"). Results are RT_RESULTs (2: invalid
 * parameter) or the inet library's -512. Names are SOCOM 1's (s1:exact/callee); the buffer
 * fields are named by their use here. */

extern int sprintf(char *s, const char *fmt, ...);
extern int rt_memory_align_free(void **p);
extern int func_002464A8(int, int, int, int);

struct binet_ctl {
    int a, b;
    int arg;
};

typedef struct LIBNETB {
    char pad0[16];
    void *recv_buf;                     /* +16 */
    int pad20;
    void *send_buf;                     /* +24 */
} LIBNETB;

typedef struct sceBInetAddress {
    int family;
    unsigned char addr[4];              /* +4, least significant first */
} sceBInetAddress;

int sceBInetControl(int s, int cmd, struct binet_ctl *p, int len)
{
    if (p == 0)
        return -512;
    return func_002464A8(s, cmd, p->arg, len);
}

int sceBInetAddress2IP(char *ip, sceBInetAddress *a)
{
    if (a == 0 || ip == 0)
        return -512;
    sprintf(ip, "%u.%u.%u.%u", a->addr[3], a->addr[2], a->addr[1], a->addr[0]);
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
