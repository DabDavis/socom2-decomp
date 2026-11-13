/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libkernl's SIF RPC (MAIN 0x1A6368-0x1A7398): Sony's SDK, the demo's object 0x15DD68 (C), in its
 * order -- the client calls (bind, call, other data), the server queue (register, the request
 * loop) and the command handlers the IOP's replies arrive through. One release on from the
 * demo's: the data request's reply (`_request_rdata`, cmd 0x8000000C) answers a client's own
 * reply packet when the request names one (rec_id bit 2) and retries a refused send through an
 * alarm (func_001A6720, new). Names are SOCOM 1's (s1:exact, and the demo's labels of the
 * object's .bss: _packet_buffer, _free_buffer, _free_buffer2, _data_table); the layouts are
 * Sony's sifrpc.h, checked by the code.
 *
 * GCC r25a, round 24a's lead: strict aliasing reads the vendor's field types off the schedule. Under
 * it GCC moves a load above a store only when their types' alias sets differ (signed and unsigned
 * share one, every pointer type has its own), and the target keeps some pairs in source order and
 * hoists others -- so the types below are the ones whose sets give the target's order, swept field
 * by field (build/scratch/gcc/v_sifrpc.py: a variants file of every combination, the functions
 * scored under each): the reply packets' words are u_int (their client, pkt_addr, receive, src
 * and dest), the client's serve and the queue's end are void *, the serve data's paddr and
 * receive u_int, the call packet's client void *. They are not Sony's sifrpc.h types where that
 * header is public -- another assignment with the same partition into sets schedules the same --
 * but they are the partition the vendor's code had: nine of round 11a's ten open close
 * with them (_request_end, _request_bind, _request_call, _request_rdata at once), with the layout
 * read off the target -- the wait path first in the clients (`if (!(mode & 1)) {...}`, the
 * no-wait one laid out last), sceSifExecRequest and sceSifCallRpc a newer release's (the reply
 * DMA'd with sceSifSetDma in two descriptors and retried in a do-while; one write-back when the
 * send and receive buffers are one), sceSifInitRpc's early return (991111's void-context sibling
 * call into sceSifSetReg), _sceRpcGetPacket's counter tested in memory (`d->pid++; if (d->pid ==
 * 1)`: one of fifteen spellings). The packet buffers have data rows (`_packet_buffer@sifrpc`, the
 * demo's labels). OPEN: sceSifCallRpc (17 words, right size): the client's four stores and the
 * packet's seven schedule otherwise (the packet's in every order, and jointly with the client's,
 * miss: the nearest 16). GCC r26a, the field types swept over the function's own view of the
 * client and the packet (build/scratch/gcc/v_calltypes.py, v_ct3.py: 2,916 + 3,456 + 5,184
 * variants): 17 -> 2 words with the client's paddr, func, para, serve and buff ints (buff's load
 * then waits two cycles behind the rmode store -- a true dependence, so int-set -- as the target's
 * does) and `pid = rpc_id` stored first; what is left is the client's paddr and pid stores in
 * the other order, and sched1 decides it: pid's store is the last use of the loaded rpc_id, so it
 * outranks paddr's on register pressure as soon as the load's latency allows, in every order and
 * type tried. Not adopted (the unit's types are the other functions' too). GCC r80: 17 -> 2 with
 * the unit's types kept, the packet written through a one-member union (alias set 0, as libmrpc's
 * requests, byte-exact that way): every packet store in source order, serve's load before the
 * client store, pid read before paddr's store; the same two words left (paddr and pid swapped). */

typedef unsigned int u_int;
typedef unsigned short u_short;
typedef unsigned char u_char;

typedef struct {
	u_int psize:8;
	u_int dsize:24;
	u_int dest;
	u_int fno;
	u_int opt;
} sceSifCmdHdr;

typedef struct _sif_rpc_data {
	void *paddr;
	u_int pid;
	int tid;
	u_int mode;
} sceSifRpcData;

typedef void (*sceSifEndFunc)(void *);
typedef void *(*sceSifRpcFunc)(u_int, void *, int);

typedef struct _sif_client_data {
	struct _sif_rpc_data rpcd;	/* 0x00 */
	u_int command;			/* 0x10 */
	void *buff;			/* 0x14 */
	void *cbuff;			/* 0x18 */
	sceSifEndFunc func;		/* 0x1C */
	void *para;			/* 0x20 */
	void *serve;			/* 0x24 */
} sceSifClientData;

typedef struct _sif_receive_data {
	struct _sif_rpc_data rpcd;
	void *src;
	void *dest;
	int size;
} sceSifReceiveData;

typedef struct _sif_serve_data {
	u_int command;			/* 0x00 */
	sceSifRpcFunc func;		/* 0x04 */
	void *buff;			/* 0x08 */
	int size;			/* 0x0C */
	sceSifRpcFunc cfunc;		/* 0x10 */
	void *cbuff;			/* 0x14 */
	int csize;			/* 0x18 */
	sceSifClientData *client;	/* 0x1C */
	u_int paddr;			/* 0x20 */
	u_int fno;			/* 0x24 */
	u_int receive;			/* 0x28 */
	int rsize;			/* 0x2C */
	int rmode;			/* 0x30 */
	u_int rid;			/* 0x34 */
	struct _sif_serve_data *link;	/* 0x38 */
	struct _sif_serve_data *next;	/* 0x3C */
	struct _sif_queue_data *base;	/* 0x40 */
} sceSifServeData;

typedef struct _sif_queue_data {
	int key;			/* 0x00: the server thread */
	int active;			/* 0x04 */
	struct _sif_serve_data *link;	/* 0x08 */
	struct _sif_serve_data *start;	/* 0x0C */
	void *end;			/* 0x10 */
	struct _sif_queue_data *next;	/* 0x14 */
} sceSifQueueData;

/* the packets (64 bytes each, a command header first) */
typedef struct {
	sceSifCmdHdr hdr;
	u_int rec_id;			/* 0x10: the slot << 16, bit 2 a client packet, bit 0 in use */
	void *pkt_addr;			/* 0x14 */
	u_int rpc_id;			/* 0x18 */
} sceSifRpcPkt;

typedef struct {
	sceSifCmdHdr hdr;
	u_int rec_id;
	u_int pkt_addr;
	u_int rpc_id;
	u_int client;			/* 0x1C */
	u_int cid;			/* 0x20: the command answered */
	void *serve;			/* 0x24 */
	void *buff;			/* 0x28 */
	void *cbuff;			/* 0x2C */
} sceSifRpcRendPkt;

typedef struct {
	sceSifCmdHdr hdr;
	u_int rec_id;
	void *pkt_addr;
	u_int rpc_id;
	sceSifReceiveData *receive;	/* 0x1C */
	u_int src;			/* 0x20 */
	u_int dest;			/* 0x24 */
	int size;			/* 0x28 */
} sceSifRpcOtherDataPkt;

typedef struct {
	sceSifCmdHdr hdr;
	u_int rec_id;
	void *pkt_addr;
	u_int rpc_id;
	sceSifClientData *client;	/* 0x1C */
	u_int sid;			/* 0x20 */
} sceSifRpcBindPkt;

/* the reply to a data request: the other data's place in the reply's own slots, so that a refused
 * send can be repeated from the packet alone */
typedef struct {
	sceSifCmdHdr hdr;
	u_int rec_id;
	u_int pkt_addr;
	u_int rpc_id;
	u_int receive;			/* 0x1C */
	u_int cid;			/* 0x20 */
	u_int src;			/* 0x24 */
	u_int dest;			/* 0x28 */
	int size;			/* 0x2C */
} sceSifRpcRdataRendPkt;

typedef struct {
	sceSifCmdHdr hdr;
	u_int rec_id;
	void *pkt_addr;
	u_int rpc_id;
	void *client;			/* 0x1C */
	u_int fno;			/* 0x20 */
	int size;			/* 0x24 */
	u_int receive;			/* 0x28 */
	int rsize;			/* 0x2C */
	int rmode;			/* 0x30 */
	sceSifServeData *serve;		/* 0x34 */
} sceSifRpcCallPkt;

/* the object's state */
typedef struct {
	int pid;			/* 0x00: the next request's id, never 0 */
	void *pkt_table;		/* 0x04: _packet_buffer, uncached */
	int pkt_table_len;		/* 0x08 */
	int unused1;
	int unused2;
	void *rdata_table;		/* 0x14: _free_buffer, a ring */
	int rdata_table_len;		/* 0x18 */
	void *client_table;		/* 0x1C: _free_buffer2, by the client's slot */
	int client_table_len;		/* 0x20 */
	int rdata_table_idx;		/* 0x24 */
	sceSifQueueData *active_queue;	/* 0x28 */
} sceSifRpcState;

struct SemaParam {
	int currentCount;
	int maxCount;
	int initCount;
	int numWaitThreads;
	u_int attr;
	u_int option;
};

extern int DIntr(void);
extern int EIntr(void);
extern int CreateSema(struct SemaParam *);
extern int DeleteSema(int);
extern int WaitSema(int);
extern int iSignalSema(int);
extern int iWakeupThread(int);
extern int SleepThread(void);
extern int iSetAlarm(u_short, void (*)(int, u_short, void *), void *);
extern void sceSifInitCmd(void);
extern void sceSifExitCmd(void);
extern void sceSifAddCmdHandler(u_int, void *, void *);
extern u_int sceSifSendCmd(u_int, void *, int, void *, void *, int);
extern u_int isceSifSendCmd(u_int, void *, int, void *, void *, int);

typedef struct {
	u_int data;
	u_int addr;
	int size;
	int mode;
} sceSifDmaData;

extern u_int sceSifSetDma(sceSifDmaData *, int);
extern void sceSifWriteBackDCache(void *, int);
extern int sceSifGetSreg(int);
extern u_int sceSifGetReg(u_int);
extern u_int sceSifSetReg(u_int, u_int);

extern int _sceSifInitCheck;

static u_char _packet_buffer[2048] __attribute__((aligned(64)));
static u_char _free_buffer[2048] __attribute__((aligned(64)));
static u_char _free_buffer2[2048] __attribute__((aligned(64)));
static sceSifRpcState _data_table __attribute__((aligned(64)));

static void _request_end(sceSifRpcRendPkt *rend, void *data);
static void _request_rdata(sceSifRpcOtherDataPkt *rdata, void *data);
static void _request_bind(sceSifRpcBindPkt *bind, void *data);
static void _request_call(sceSifRpcCallPkt *call, void *data);

void sceSifInitRpc(u_int mode)
{
	u_int *cmd;

	DIntr();
	if (_sceSifInitCheck) {
		EIntr();
		return;
	}
	_sceSifInitCheck = 1;
	EIntr();
	sceSifInitCmd();
	DIntr();
	_data_table.pkt_table = (void *)((u_int)_packet_buffer | 0x20000000);
	_data_table.pkt_table_len = 32;
	_data_table.unused1 = 0;
	_data_table.unused2 = 0;
	_data_table.rdata_table = (void *)((u_int)_free_buffer | 0x20000000);
	_data_table.rdata_table_len = 32;
	_data_table.client_table = (void *)((u_int)_free_buffer2 | 0x20000000);
	_data_table.client_table_len = 32;
	_data_table.rdata_table_idx = 0;
	_data_table.pid = 1;
	sceSifAddCmdHandler(0x80000008, (void *)_request_end, &_data_table);
	sceSifAddCmdHandler(0x80000009, (void *)_request_bind, &_data_table);
	sceSifAddCmdHandler(0x8000000A, (void *)_request_call, &_data_table);
	sceSifAddCmdHandler(0x8000000C, (void *)_request_rdata, &_data_table);
	EIntr();
	if (sceSifGetReg(0x80000002))
		return;
	cmd = (u_int *)&_packet_buffer[64];
	cmd[3] = 1;
	sceSifSendCmd(0x80000002, cmd, 16, 0, 0, 0);
	while (sceSifGetSreg(0) == 0)
		;
	sceSifSetReg(0x80000002, 1);
}

void sceSifExitRpc(void)
{
	sceSifExitCmd();
	_sceSifInitCheck = 0;
}

static void *_sceRpcGetPacket(sceSifRpcState *d)
{
	sceSifRpcPkt *p;
	int i, pid;

	DIntr();
	p = d->pkt_table;
	for (i = 0; i < d->pkt_table_len; i++, p = (sceSifRpcPkt *)((char *)p + 64)) {
		if (!(p->rec_id & 1)) {
			p->rec_id = (i << 16) | 5;
			d->pid++;
			if (d->pid == 1) {
				d->pid++;
				pid = 1;
			} else
				pid = d->pid;
			p->pkt_addr = p;
			p->rpc_id = pid;
			EIntr();
			return p;
		}
	}
	EIntr();
	return 0;
}

static void _sceRpcFreePacket(sceSifRpcPkt *p)
{
	p->rpc_id = 0;
	p->rec_id &= ~1;
}

static void *_sceRpcGetFPacket(sceSifRpcState *d)
{
	void *p;
	int i;

	i = d->rdata_table_idx % d->rdata_table_len;
	p = (char *)d->rdata_table + i * 64;
	d->rdata_table_idx = i + 1;
	return p;
}

static void *_sceRpcGetFPacket2(sceSifRpcState *d, int rid)
{
	if (rid < 0 || rid >= d->client_table_len)
		return _sceRpcGetFPacket(d);
	return (char *)d->client_table + rid * 64;
}

/* Byte-exact as written (checked GCC r80: the note below predates the match). Was OPEN, 9 words:
 * the bind case's three copies are scheduled here as the loads first (buff's above
 * serve's store), and the switch's two branches to the join are plain where the target's are
 * likely ones; the target keeps every load after the store before it. Not strict aliasing (without
 * it, 22 words and the wrong size), not the reply packet's field types (void, int, a struct
 * pointer: 9-10); written with `cd` assigned in each case and again after the switch -- one local
 * the target loads on every path and keeps across the two calls (assigned once at the top, or an
 * if-chain: the wrong size). The third, empty case gives the target's case tree (0x8000000A at
 * its root). */
static void _request_end(sceSifRpcRendPkt *rend, void *data)
{
	sceSifClientData *cd;

	switch (rend->cid) {
	case 0x8000000A:
		cd = (sceSifClientData *)rend->client;
		if (cd->func)
			cd->func(cd->para);
		break;
	case 0x80000009:
		cd = (sceSifClientData *)rend->client;
		cd->serve = rend->serve;
		cd->buff = rend->buff;
		cd->cbuff = rend->cbuff;
		break;
	case 0x8000000C:
		break;
	}
	cd = (sceSifClientData *)rend->client;
	if (cd->rpcd.tid >= 0)
		iSignalSema(cd->rpcd.tid);
	_sceRpcFreePacket(cd->rpcd.paddr);
	cd->rpcd.paddr = 0;
}

/* a refused reply, sent again from an alarm until the IOP takes it */
static void func_001A6720(int id, u_short time, void *p)
{
	sceSifRpcRdataRendPkt *rend = p;

	if (isceSifSendCmd(0x80000008, rend, 64, rend->src, rend->dest, rend->size) == 0)
		iSetAlarm(10, func_001A6720, rend);
}

static void _request_rdata(sceSifRpcOtherDataPkt *rdata, void *data)
{
	sceSifRpcRdataRendPkt *rend;

	if (rdata->rec_id & 4)
		rend = _sceRpcGetFPacket2(data, rdata->rec_id >> 16);
	else
		rend = _sceRpcGetFPacket(data);
	rend->pkt_addr = rdata->pkt_addr;
	rend->receive = rdata->receive;
	rend->cid = 0x8000000C;
	rend->src = rdata->src;
	rend->dest = rdata->dest;
	rend->size = rdata->size;
	if (isceSifSendCmd(0x80000008, rend, 64, rdata->src, rdata->dest, rdata->size) == 0)
		iSetAlarm(10, func_001A6720, rend);
}

int sceSifGetOtherData(sceSifReceiveData *rd, void *src, void *dest, int size, u_int mode)
{
	struct SemaParam sema;
	sceSifRpcOtherDataPkt *other;

	other = _sceRpcGetPacket(&_data_table);
	if (other == 0)
		return -1;
	rd->rpcd.paddr = other;
	rd->rpcd.pid = other->rpc_id;
	other->src = src;
	other->dest = dest;
	other->size = size;
	other->pkt_addr = other;
	other->receive = rd;
	if (!(mode & 1)) {
		sema.initCount = 0;
		sema.maxCount = 1;
		rd->rpcd.tid = CreateSema(&sema);
		if (rd->rpcd.tid < 0) {
			_sceRpcFreePacket((sceSifRpcPkt *)other);
			return -3;
		}
		if (sceSifSendCmd(0x8000000C, other, 64, 0, 0, 0) == 0) {
			_sceRpcFreePacket((sceSifRpcPkt *)other);
			DeleteSema(rd->rpcd.tid);
			return -2;
		}
		WaitSema(rd->rpcd.tid);
		DeleteSema(rd->rpcd.tid);
		return 0;
	}
	rd->rpcd.tid = -1;
	if (sceSifSendCmd(0x8000000C, other, 64, 0, 0, 0) == 0) {
		_sceRpcFreePacket((sceSifRpcPkt *)other);
		return -2;
	}
	return 0;
}

static sceSifServeData *_search_svdata(u_int command, sceSifRpcState *d)
{
	sceSifQueueData *q;
	sceSifServeData *s;

	for (q = d->active_queue; q; q = q->next) {
		for (s = q->link; s; s = s->link) {
			if (s->command == command)
				return s;
		}
	}
	return 0;
}

static void _request_bind(sceSifRpcBindPkt *bind, void *data)
{
	sceSifRpcRendPkt *rend;
	sceSifServeData *sd;

	rend = _sceRpcGetFPacket(data);
	rend->pkt_addr = (u_int)bind->pkt_addr;
	rend->client = (u_int)bind->client;
	rend->cid = 0x80000009;
	sd = _search_svdata(bind->sid, data);
	if (sd == 0) {
		rend->serve = 0;
		rend->buff = 0;
		rend->cbuff = 0;
	} else {
		rend->serve = sd;
		rend->buff = sd->buff;
		rend->cbuff = sd->cbuff;
	}
	isceSifSendCmd(0x80000008, rend, 64, 0, 0, 0);
}

int sceSifBindRpc(sceSifClientData *bd, u_int command, u_int mode)
{
	struct SemaParam sema;
	sceSifRpcBindPkt *bind;

	bd->command = 0;
	bd->serve = 0;
	bind = _sceRpcGetPacket(&_data_table);
	if (bind == 0)
		return -1;
	bd->rpcd.paddr = bind;
	bd->rpcd.pid = bind->rpc_id;
	bind->sid = command;
	bind->pkt_addr = bind;
	bind->client = bd;
	if (!(mode & 1)) {
		sema.initCount = 0;
		sema.maxCount = 1;
		bd->rpcd.tid = CreateSema(&sema);
		if (bd->rpcd.tid < 0) {
			_sceRpcFreePacket((sceSifRpcPkt *)bind);
			return -3;
		}
		if (sceSifSendCmd(0x80000009, bind, 64, 0, 0, 0) == 0) {
			_sceRpcFreePacket((sceSifRpcPkt *)bind);
			DeleteSema(bd->rpcd.tid);
			return -2;
		}
		WaitSema(bd->rpcd.tid);
		DeleteSema(bd->rpcd.tid);
		return 0;
	}
	bd->rpcd.tid = -1;
	if (sceSifSendCmd(0x80000009, bind, 64, 0, 0, 0) == 0) {
		_sceRpcFreePacket((sceSifRpcPkt *)bind);
		return -2;
	}
	return 0;
}

static void _request_call(sceSifRpcCallPkt *call, void *data)
{
	sceSifServeData *sd = call->serve;
	sceSifQueueData *base = sd->base;

	if (base->start == 0)
		base->start = sd;
	else
		((sceSifServeData *)base->end)->next = sd;
	base->end = sd;
	sd->paddr = call->pkt_addr;
	sd->client = call->client;
	sd->fno = call->fno;
	sd->size = call->size;
	sd->receive = call->receive;
	sd->rsize = call->rsize;
	sd->rmode = call->rmode;
	sd->rid = call->rec_id;
	if (base->key >= 0 && base->active == 0)
		iWakeupThread(base->key);
}

int sceSifCallRpc(sceSifClientData *bd, u_int fno, u_int mode, void *send, int ssize, void *receive,
		  int rsize, sceSifEndFunc end_function, void *end_param)
{
	struct SemaParam sema;
	union { sceSifRpcCallPkt c; } *call;

	call = _sceRpcGetPacket(&_data_table);
	if (call == 0)
		return -1;
	bd->rpcd.pid = call->c.rpc_id;
	bd->rpcd.paddr = call;
	bd->func = end_function;
	bd->para = end_param;
	call->c.fno = fno;
	call->c.size = ssize;
	call->c.receive = (u_int)receive;
	call->c.rsize = rsize;
	call->c.pkt_addr = call;
	call->c.serve = bd->serve;
	call->c.client = bd;
	if (!(mode & 2)) {
		if (send == receive) {
			sceSifWriteBackDCache(send, ssize < rsize ? rsize : ssize);
		} else {
			if (ssize > 0)
				sceSifWriteBackDCache(send, ssize);
			if (rsize > 0)
				sceSifWriteBackDCache(receive, rsize);
		}
	}
	if (mode & 1) {
		if (end_function == 0)
			call->c.rmode = 0;
		else
			call->c.rmode = 1;
		bd->rpcd.tid = -1;
		if (sceSifSendCmd(0x8000000A, call, 64, send, bd->buff, ssize) != 0)
			return 0;
		_sceRpcFreePacket((sceSifRpcPkt *)call);
		return -2;
	}
	sema.initCount = 0;
	sema.maxCount = 1;
	bd->rpcd.tid = CreateSema(&sema);
	if (bd->rpcd.tid < 0) {
		_sceRpcFreePacket((sceSifRpcPkt *)call);
		return -3;
	}
	call->c.rmode = 1;
	if (sceSifSendCmd(0x8000000A, call, 64, send, bd->buff, ssize) == 0) {
		DeleteSema(bd->rpcd.tid);
		_sceRpcFreePacket((sceSifRpcPkt *)call);
		return -2;
	}
	WaitSema(bd->rpcd.tid);
	DeleteSema(bd->rpcd.tid);
	return 0;
}

int sceSifCheckStatRpc(sceSifRpcData *cd)
{
	sceSifRpcPkt *p = cd->paddr;

	if (p == 0 || cd->pid != p->rpc_id || !(p->rec_id & 1))
		return 0;
	return 1;
}

void sceSifSetRpcQueue(sceSifQueueData *qd, int key)
{
	sceSifQueueData *q;

	DIntr();
	qd->key = key;
	qd->active = 0;
	qd->link = 0;
	qd->start = 0;
	qd->end = 0;
	qd->next = 0;
	if (_data_table.active_queue == 0) {
		_data_table.active_queue = qd;
	} else {
		for (q = _data_table.active_queue; q->next; q = q->next)
			;
		q->next = qd;
	}
	EIntr();
}

void sceSifRegisterRpc(sceSifServeData *sd, u_int command, sceSifRpcFunc func, void *buff,
		       sceSifRpcFunc cfunc, void *cbuff, sceSifQueueData *qd)
{
	sceSifServeData *s;

	DIntr();
	sd->command = command;
	sd->func = func;
	sd->buff = buff;
	sd->next = 0;
	sd->link = 0;
	sd->cfunc = cfunc;
	sd->cbuff = cbuff;
	sd->base = qd;
	if (qd->link == 0) {
		qd->link = sd;
	} else {
		for (s = qd->link; s->link; s = s->link)
			;
		s->link = sd;
	}
	EIntr();
}

sceSifServeData *sceSifRemoveRpc(sceSifServeData *sd, sceSifQueueData *qd)
{
	sceSifServeData *s;

	DIntr();
	if ((s = qd->link) == sd) {
		qd->link = s->link;
	} else {
		for (; s; s = s->link) {
			if (s->link == sd) {
				s->link = sd->link;
				break;
			}
		}
	}
	EIntr();
	return s;
}

sceSifQueueData *sceSifRemoveRpcQueue(sceSifQueueData *qd)
{
	sceSifQueueData *q;

	DIntr();
	if ((q = _data_table.active_queue) == qd) {
		_data_table.active_queue = q->next;
	} else {
		for (; q; q = q->next) {
			if (q->next == qd) {
				q->next = qd->next;
				break;
			}
		}
	}
	EIntr();
	return q;
}

sceSifServeData *sceSifGetNextRequest(sceSifQueueData *qd)
{
	sceSifServeData *sd;

	DIntr();
	sd = qd->start;
	if (sd == 0) {
		qd->active = 0;
	} else {
		qd->active = 1;
		qd->start = sd->next;
	}
	EIntr();
	return sd;
}

void sceSifExecRequest(sceSifServeData *sd)
{
	sceSifDmaData dmat[2];
	sceSifRpcRendPkt *rend;
	void *rec;
	int size, n, i;
	u_int id;

	size = 0;
	rec = sd->func(sd->fno, sd->buff, sd->size);
	if (rec)
		size = sd->rsize;
	if (sd->size > 0)
		sceSifWriteBackDCache(sd->buff, sd->size);
	if (size > 0)
		sceSifWriteBackDCache(rec, size);
	DIntr();
	if (sd->rid & 4)
		rend = _sceRpcGetFPacket2(&_data_table, sd->rid >> 16);
	else
		rend = _sceRpcGetFPacket(&_data_table);
	EIntr();
	rend->cid = 0x8000000A;
	rend->client = (u_int)sd->client;
	if (sd->rmode) {
		while (sceSifSendCmd(0x80000008, rend, 64, rec, (void *)sd->receive, size) == 0)
			;
		return;
	}
	rend->rpc_id = 0;
	rend->rec_id = 0;
	n = 0;
	if (size > 0) {
		dmat[0].data = (u_int)rec;
		dmat[0].addr = (u_int)sd->receive;
		dmat[0].size = size;
		dmat[0].mode = 0;
		n = 1;
	}
	dmat[n].data = (u_int)rend;
	dmat[n].addr = (u_int)sd->paddr;
	dmat[n].size = 64;
	dmat[n].mode = 0;
	n++;
	do {
		id = sceSifSetDma(dmat, n);
		if (id == 0) {
			i = 0x100000;
			while (i--)
				;
		}
	} while (id == 0);
}

void sceSifRpcLoop(sceSifQueueData *qd)
{
	sceSifServeData *sd;

	for (;;) {
		while ((sd = sceSifGetNextRequest(qd)) != 0)
			sceSifExecRequest(sd);
		SleepThread();
	}
}



