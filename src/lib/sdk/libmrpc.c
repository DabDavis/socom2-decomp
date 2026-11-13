/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libmrpc (MAIN 0x1BCD80-0x1BD588): Sony's SDK multi-thread SIF RPC client -- the EE side of the
 * IOP's per-client server threads: a bind names the server and the thread's three parameters, each
 * client has its own lock (a semaphore held from the bind to the unbind), and the replies come back
 * through its own command 0x80000018. The demo's object 0x379AB0 (C), its order: sceSifMInitRpc,
 * sceSifMExitRpc, the statics _sceRpcGetPacket, _sceRpcFreePacket and _request_end (sifrpc.c's
 * text, `NAME@libmrpc` in the map: sifrpc's rows hold the bare names), sceSifMBindRpc,
 * sceSifMBindRpcParam, sceSifMUnBindRpc, sceSifMCallRpc. Its state is the demo's labels
 * (_sceSifMInitCheck in .data; _packet_buffer and _data_table@libmrpc in .bss), and the packets and
 * the client are sifrpc.c's types (GCC r25a: their alias sets read off the schedules there) with
 * the multi-thread client's fields after them (the unbind packet's client void *, as sifrpc's call
 * packet's). GCC r25a, new: 7 of 9 byte-exact, sceSifMInitRpc in sifrpc's init order; 9 of 9 since
 * GCC r80, the requests' packets through a union (sceSifMPkt). */

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

/* the multi-thread client: sifrpc.c's client and its own lock, the unbind's result and the bind's
 * three server-thread parameters (sceSifMBindRpc's 0x800, 0x2000, 0x20) */
typedef struct _sif_mclient_data {
	struct _sif_rpc_data rpcd;	/* 0x00 */
	u_int command;			/* 0x10 */
	void *buff;			/* 0x14 */
	void *cbuff;			/* 0x18 */
	sceSifEndFunc func;		/* 0x1C */
	void *para;			/* 0x20 */
	void *serve;			/* 0x24 */
	int sema;			/* 0x28: held from the bind to the unbind */
	int unbind;			/* 0x2C: the unbind's reply */
	int param0;			/* 0x30 */
	int param1;			/* 0x34 */
	int param2;			/* 0x38 */
} sceSifMClientData;

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
	void *serve;			/* 0x24: the unbind's reply too */
	void *buff;			/* 0x28 */
	void *cbuff;			/* 0x2C */
} sceSifRpcRendPkt;

typedef struct {
	sceSifCmdHdr hdr;
	u_int rec_id;
	void *pkt_addr;
	u_int rpc_id;
	sceSifMClientData *client;	/* 0x1C */
	u_int sid;			/* 0x20 */
	int param0;			/* 0x24 */
	int param1;			/* 0x28 */
	int param2;			/* 0x2C */
} sceSifMBindPkt;

typedef struct {
	sceSifCmdHdr hdr;
	u_int rec_id;
	void *pkt_addr;
	u_int rpc_id;
	void *client;			/* 0x1C */
	u_int sid;			/* 0x20 */
	void *serve;			/* 0x24 */
} sceSifMUnBindPkt;

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
	void *serve;			/* 0x34 */
} sceSifMCallPkt;

/* the packet as the three requests write it: through a union, so every field store has alias set 0
 * and keeps its place against the client's (GCC round 80) */
typedef union {
	sceSifMBindPkt bind;
	sceSifMUnBindPkt unbind;
	sceSifMCallPkt call;
} sceSifMPkt;

typedef struct {
	int pid;			/* 0x00: the next request's id, never 0 */
	void *pkt_table;		/* 0x04: _packet_buffer, uncached */
	int pkt_table_len;		/* 0x08 */
	int unused1;
	int unused2;
} sceSifMRpcState;

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
extern int scePrintf(const char *fmt, ...);
extern void sceSifAddCmdHandler(u_int, void *, void *);
extern u_int sceSifSendCmd(u_int, void *, int, void *, void *, int);
extern void sceSifWriteBackDCache(void *, int);
extern int sceSifGetSreg(int);

static int _sceSifMInitCheck = 0;
static u_char _packet_buffer[2048] __attribute__((aligned(64)));
static sceSifMRpcState _data_table __attribute__((aligned(64)));

static void _request_end(sceSifRpcRendPkt *rend, void *data);

void sceSifMInitRpc(u_int mode)
{
	u_int *cmd;

	if (_sceSifMInitCheck)
		return;
	_sceSifMInitCheck = 1;
	DIntr();
	_data_table.pkt_table = (void *)((u_int)_packet_buffer | 0x20000000);
	_data_table.pkt_table_len = 32;
	_data_table.unused1 = 0;
	_data_table.unused2 = 0;
	_data_table.pid = 1;
	sceSifAddCmdHandler(0x80000018, (void *)_request_end, &_data_table);
	EIntr();
	cmd = (u_int *)&_packet_buffer[64];
	cmd[4] = 1;
	cmd[5] = 1;
	sceSifSendCmd(0x80000001, cmd, 24, 0, 0, 0);
	while (sceSifGetSreg(1) == 0)
		;
}

void sceSifMExitRpc(void)
{
	_sceSifMInitCheck = 0;
}

static void *_sceRpcGetPacket(sceSifMRpcState *d)
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

static void _request_end(sceSifRpcRendPkt *rend, void *data)
{
	sceSifMClientData *cd;

	switch (rend->cid) {
	case 0x8000001A:
		cd = (sceSifMClientData *)rend->client;
		if (cd->func)
			cd->func(cd->para);
		iSignalSema(cd->sema);
		break;
	case 0x80000019:
		cd = (sceSifMClientData *)rend->client;
		cd->serve = rend->serve;
		cd->buff = rend->buff;
		cd->cbuff = rend->cbuff;
		iSignalSema(cd->sema);
		break;
	case 0x8000001D:
		cd = (sceSifMClientData *)rend->client;
		cd->unbind = (int)rend->serve;
		iSignalSema(cd->sema);
		break;
	case 0x8000001C:
		break;
	}
	cd = (sceSifMClientData *)rend->client;
	if (cd->rpcd.tid >= 0)
		iSignalSema(cd->rpcd.tid);
	_sceRpcFreePacket(cd->rpcd.paddr);
	cd->rpcd.paddr = 0;
}

int sceSifMBindRpcParam(sceSifMClientData *cd, u_int command, u_int mode, int param0, int param1, int param2);

int sceSifMBindRpc(sceSifMClientData *cd, u_int command, u_int mode)
{
	return sceSifMBindRpcParam(cd, command, mode, 0x800, 0x2000, 0x20);
}

int sceSifMBindRpcParam(sceSifMClientData *cd, u_int command, u_int mode, int param0, int param1, int param2)
{
	struct SemaParam sema;
	sceSifMPkt *bind;
	int s;

	if (mode != 0) {
		scePrintf("mode must be 0 in this version.\n");
		return -1;
	}
	sema.initCount = 1;
	sema.maxCount = 1;
	sema.option = 0;
	cd->sema = CreateSema(&sema);
	if (cd->sema < 0)
		return -3;
	WaitSema(cd->sema);
	cd->command = command;
	cd->serve = 0;
	cd->param0 = param0;
	cd->param1 = param1;
	cd->param2 = param2;
	bind = _sceRpcGetPacket(&_data_table);
	if (bind == 0) {
		DeleteSema(cd->sema);
		return -1;
	}
	cd->rpcd.pid = bind->bind.rpc_id;
	cd->rpcd.paddr = bind;
	bind->bind.sid = command;
	bind->bind.param0 = param0;
	bind->bind.param1 = param1;
	bind->bind.param2 = param2;
	bind->bind.pkt_addr = bind;
	bind->bind.client = cd;
	sema.maxCount = 1;
	sema.initCount = 0;
	sema.option = 0;
	s = cd->rpcd.tid = CreateSema(&sema);
	if (s < 0) {
		_sceRpcFreePacket((sceSifRpcPkt *)bind);
		DeleteSema(cd->sema);
		return -3;
	}
	if (sceSifSendCmd(0x80000019, bind, 64, 0, 0, 0) == 0) {
		_sceRpcFreePacket((sceSifRpcPkt *)bind);
		DeleteSema(s);
		DeleteSema(cd->sema);
		return -2;
	}
	WaitSema(s);
	DeleteSema(s);
	if (cd->serve == 0)
		DeleteSema(cd->sema);
	return 0;
}

/* Byte-exact since GCC round 80: the packet written through sceSifMPkt -- a union, so each field
 * store has alias set 0 and keeps its source place against the client's loads and stores (round 79's
 * 720 orders all ran under the struct's own alias sets). */
int sceSifMUnBindRpc(sceSifMClientData *cd, u_int mode)
{
	struct SemaParam sema;
	sceSifMPkt *ub;
	int s;

	if (mode != 0) {
		scePrintf("mode must be 0 in this version.\n");
		return -1;
	}
	WaitSema(cd->sema);
	ub = _sceRpcGetPacket(&_data_table);
	if (ub == 0)
		return -1;
	cd->rpcd.pid = ub->unbind.rpc_id;
	cd->rpcd.paddr = ub;
	ub->unbind.sid = cd->command;
	ub->unbind.pkt_addr = ub;
	ub->unbind.client = cd;
	ub->unbind.serve = cd->serve;
	sema.maxCount = 1;
	sema.initCount = 0;
	sema.option = 0;
	s = cd->rpcd.tid = CreateSema(&sema);
	if (s < 0) {
		_sceRpcFreePacket((sceSifRpcPkt *)ub);
		return -3;
	}
	if (sceSifSendCmd(0x8000001D, ub, 64, 0, 0, 0) == 0) {
		_sceRpcFreePacket((sceSifRpcPkt *)ub);
		DeleteSema(s);
		return -2;
	}
	WaitSema(s);
	DeleteSema(s);
	DeleteSema(cd->sema);
	return cd->unbind;
}

/* sifrpc.c's sceSifCallRpc with the lock and the alignment checks. Byte-exact since GCC round 80:
 * the packet through sceSifMPkt's union (every store in source order: fno, pkt_addr, the sizes, the
 * client), the client's pid read before its paddr store. */
int sceSifMCallRpc(sceSifMClientData *cd, u_int fno, u_int mode, void *send, int ssize, void *receive,
		   int rsize, sceSifEndFunc end_function, void *end_param)
{
	struct SemaParam sema;
	sceSifMPkt *call;
	int s;

	if (mode != 0) {
		scePrintf("mode must be 0 in this version.\n");
		return -1;
	}
	WaitSema(cd->sema);
	if ((u_int)send & 0x3f) {
		scePrintf("%s: send buffer addr is not 64 byte align. %08x=n", "sceSifMCallRpc", send);
		return -1;
	}
	if ((u_int)receive & 0x3f) {
		scePrintf("%s: receive buffer addr is not 64 byte align. %08x=n", "sceSifMCallRpc", receive);
		return -1;
	}
	call = _sceRpcGetPacket(&_data_table);
	if (call == 0)
		return -1;
	cd->rpcd.pid = call->call.rpc_id;
	cd->rpcd.paddr = call;
	cd->func = end_function;
	cd->para = end_param;
	call->call.fno = fno;
	call->call.pkt_addr = call;
	call->call.size = ssize;
	call->call.receive = (u_int)receive;
	call->call.rsize = rsize;
	call->call.client = cd;
	call->call.serve = cd->serve;
	if (send == receive) {
		sceSifWriteBackDCache(send, ssize < rsize ? rsize : ssize);
	} else {
		if (ssize > 0)
			sceSifWriteBackDCache(send, ssize);
		if (rsize > 0)
			sceSifWriteBackDCache(receive, rsize);
	}
	if (mode & 1) {
		if (end_function == 0)
			call->call.rmode = 0;
		else
			call->call.rmode = 1;
		cd->rpcd.tid = -1;
		if (sceSifSendCmd(0x8000001A, call, 64, send, cd->buff, ssize) != 0)
			return 0;
		_sceRpcFreePacket((sceSifRpcPkt *)call);
		return -2;
	}
	sema.maxCount = 1;
	sema.initCount = 0;
	sema.option = 0;
	s = cd->rpcd.tid = CreateSema(&sema);
	if (s < 0) {
		_sceRpcFreePacket((sceSifRpcPkt *)call);
		return -3;
	}
	call->call.rmode = 1;
	if (sceSifSendCmd(0x8000001A, call, 64, send, cd->buff, ssize) == 0) {
		_sceRpcFreePacket((sceSifRpcPkt *)call);
		DeleteSema(s);
		return -2;
	}
	WaitSema(s);
	DeleteSema(s);
	return 0;
}
