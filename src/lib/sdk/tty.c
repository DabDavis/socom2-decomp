/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libkernl's DECI2 console (MAIN 0x1A4D60-0x1A5280): Sony's SDK, the demo's object 0x15C760 (C), in
 * its order -- the read queue (QueueInit, and the peek-then-done pair the handler and the reader
 * advance it with), the protocol handler and the blocking write and read over DECI2 protocol
 * 0x210 ('E' to 'H'). Names are SOCOM 1's (s1:exact; tinfo, wbuf, rbuf and QueueInit's own q
 * the demo's labels). tinfo's socket is volatile (sceTtyInit reads it back after storing it) and so
 * is its writing flag (sceTtyWrite spins on it), and so are the two lengths: sceTtyInit's success
 * path keeps its stores in source order -- writing, wlen, rlen, the volatile three -- with the
 * pointers' lui/addiu/or between them (plain lengths: 16 words; every field volatile puts a store
 * first in the block, and reorg then fills the branch's slot from the failure path); the read queue
 * is plain (sceTtyRead, byte-exact only so). The packet is set in field order, `reserved` first (GCC
 * round 28a: sceTtyInit closed, open since round 11a).
 *
 * OPEN: sceTtyHandler (97s) and sceTtyWrite (78s) are ps2sdk's reconstruction adapted to Sony's
 * queue, not yet fitted. */

typedef unsigned int u_int;
typedef unsigned short u_short;
typedef unsigned char u_char;

typedef struct {
	u_short length;
	u_short reserved;
	u_short protocol;
	u_char source;
	u_char destination;
	u_int ttyp_reserved;
	char buf[256];
} tty_packet;

typedef struct {
	int size;
	volatile int count;
	char *rp;
	char *wp;
	char buf[256];
} tty_queue;

struct tinfo {
	volatile int socket;
	volatile int wlen;
	volatile int rlen;
	volatile int writing;
	char *wptr;
	char *rptr;
	tty_queue *read_queue;
};

extern int sceDeci2Open(u_short, void *, void (*)(int, int, void *));
extern int sceDeci2ReqSend(int, char);
extern void sceDeci2Poll(int);
extern int sceDeci2ExRecv(int, void *, u_short);
extern int sceDeci2ExSend(int, void *, u_short);
extern int DIntr(void);
extern int EIntr(void);
extern void FlushCache(int);

extern tty_packet wbuf, rbuf;
extern struct tinfo tinfo;

static tty_queue *QueueInit(int size)
{
	static tty_queue q;

	q.size = size;
	q.count = 0;
	q.wp = q.buf;
	q.rp = q.buf;
	return &q;
}

static void QueuePeekWriteDone(tty_queue *q)
{
	q->count++;
	if (++q->wp == q->buf + q->size)
		q->wp = q->buf;
}

static void QueuePeekReadDone(tty_queue *q)
{
	q->count--;
	if (++q->rp == q->buf + q->size)
		q->rp = q->buf;
}

static void sceTtyHandler(int event, int param, void *opt)
{
	struct tinfo *info = opt;
	tty_packet *pkt;
	int ret, i;

	switch (event) {
	case 1:
	case 2:
		if (param) {
			ret = sceDeci2ExRecv(info->socket, info->rptr + info->rlen, param);
			info->rlen += ret;
			break;
		}
		pkt = (tty_packet *)info->rptr;
		for (i = 0; i < pkt->length - 12; i++) {
			if (info->read_queue->count >= info->read_queue->size)
				break;
			*info->read_queue->wp = pkt->buf[i];
			QueuePeekWriteDone(info->read_queue);
		}
		info->rlen = 0;
		break;
	case 3:
		ret = sceDeci2ExSend(info->socket, info->wptr, info->wlen);
		if (ret < 0) {
			info->writing = 0;
			break;
		}
		info->wptr += ret;
		info->wlen -= ret;
		break;
	case 4:
		info->writing = 0;
		break;
	}
}

int sceTtyWrite(char *buf, int len)
{
	tty_packet *pkt;
	int oldintr, i, n;
	char *wp;

	if (tinfo.writing)
		return -1;
	oldintr = DIntr();
	tinfo.writing = 1;
	tinfo.wptr = (char *)((u_int)&wbuf | 0x20000000);
	pkt = (tty_packet *)((u_int)&wbuf | 0x20000000);
	n = 0;
	wp = pkt->buf;
	for (i = 0; i < len; i++) {
		if (*buf == '\n') {
			*wp++ = '\r';
			if (++n >= 256)
				break;
		}
		*wp++ = *buf++;
		if (++n >= 256)
			break;
	}
	tinfo.wlen = n + 12;
	pkt->length = tinfo.wlen;
	if (sceDeci2ReqSend(tinfo.socket, pkt->destination) < 0) {
		tinfo.writing = 0;
		if (oldintr)
			EIntr();
		return -1;
	}
	while (tinfo.writing)
		sceDeci2Poll(tinfo.socket);
	if (oldintr)
		EIntr();
	return i;
}

int sceTtyRead(char *buf, int len)
{
	int i;

	for (i = 0; i < len; i++) {
		while (tinfo.read_queue->count == 0)
			;
		buf[i] = *tinfo.read_queue->rp;
		QueuePeekReadDone(tinfo.read_queue);
		if (buf[i] == '\n' || buf[i] == '\r')
			return i + 1;
	}
	return i;
}

int sceTtyInit(void)
{
	tty_packet *pkt;
	int ret;

	FlushCache(0);
	tinfo.socket = sceDeci2Open(0x210, (void *)&tinfo, sceTtyHandler);
	if (tinfo.socket < 0) {
		ret = 0;
	} else {
	tinfo.writing = 0;
	tinfo.wlen = 0;
	tinfo.rlen = 0;
	tinfo.rptr = (char *)((u_int)&rbuf | 0x20000000);
	tinfo.wptr = (char *)((u_int)&wbuf | 0x20000000);
	pkt = (tty_packet *)((u_int)&wbuf | 0x20000000);
	pkt->reserved = 0;
	pkt->protocol = 0x210;
	pkt->source = 'E';
	pkt->destination = 'H';
	pkt->ttyp_reserved = 0;
	tinfo.read_queue = QueueInit(256);
		ret = 1;
	}
	return ret;
}
