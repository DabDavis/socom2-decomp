/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libdbc (MAIN 0x18FCB8-0x190BC8): Sony's SDK DBC client -- the EE side of dbcman.irx, the
 * controller-socket manager libpad2 sits on. Not in SOCOM 1's demo (it links libpad); each RPC
 * wrapper names itself in its error message ("sceDbcSetWorkAddr: rpc error"), which named the rows
 * (round 12a). Every call is one sceSifCallRpc on the first client with the same 640-byte buffer
 * both ways, its function number 0x80001300 + n; the reply's words are read back out of it. The
 * debug printf is compiled out: an empty varargs function (func_00190BA0, the object's last).
 * The client data and the buffer are the object's own .bss, spelled by address here (data_<VA>).
 * Its init (0x18FD10: the three binds and the version check), end (0x18FF50) and version query
 * (0x18FCB8) keep placeholders (they print no name; sceDbcInit/sceDbcEnd by the API, not proven).
 * Not written: sceDbcGetDepNumber and 0x1902B0 (interrupts off around a cache sync).
 * GCC r24a: a copy loop's base is assigned inside the loop (loop.c hoists it into the preheader,
 * where combine cannot fold it into the store), and the create request's two buffers are pointers
 * (their stores alias no int load under strict aliasing): CreateSocket, SendData and ReceiveData,
 * open since r12a/r22a, are byte-exact so. */

typedef struct { int a[10]; } sceSifClientData;
extern int sceSifCallRpc(sceSifClientData *cd, unsigned int fno, unsigned int mode, void *send, int ssize,
			 void *recv, int rsize, void (*end)(void *), void *ea);

extern sceSifClientData data_001D6248;          /* the client of server 0x80001300 */
extern unsigned int data_001D62C0[160];         /* the send and receive buffer */
extern int data_001CC410;                       /* nonzero once sceDbcInit has bound */

typedef struct {
	int f0, f4, f8, f12, f16;
	char name[16];
} sceDbcSocketParam;

void func_00190BA0(const char *fmt, ...);

struct SemaParam {
	int currentCount;
	int maxCount;
	int initCount;
	int numWaitThreads;
	unsigned int attr;
	unsigned int option;
};

extern int sceSifInitRpc(unsigned int mode);
extern int sceSifBindRpc(sceSifClientData *cd, unsigned int request, unsigned int mode);
extern int CreateSema(struct SemaParam *p);
extern int DeleteSema(int sema);
extern int scePrintf(const char *fmt, ...);
int sceDbcSetWorkAddr(int addr);

extern sceSifClientData data_001D6270;          /* the client of server 0x8000131B */
extern sceSifClientData data_001D6298;          /* the client of server 0x8000131C */
extern int data_001D9800;                       /* the library's semaphore */
extern int data_001D97C0[16];
extern int data_001D9740[32];                   /* the work area handed to dbcman */

/* dbcman's version: (major << 8) | minor, the server's reply word (RPC 0x80001363) */
int func_0018FCB8(void)
{
	sceSifCallRpc(&data_001D6248, 0x80001363, 0, data_001D62C0, 640, data_001D62C0, 640, 0, 0);
	return data_001D62C0[0];
}

/* the library's init: the three servers bound (each retried after a short delay until it serves),
 * dbcman's major version checked against the library's 3, the semaphore made, the work area
 * cleared and handed over; each bind's failure prints and returns on its own (cross-jumping keeps
 * one copy, laid out after the semaphore's failure: as one goto label at the end it sits last) */
int func_0018FD10(void)
{
	struct SemaParam sp;
	int i, ver;

	if (data_001CC410 == 1)
		return 1;
	data_001D9800 = 0;
	sceSifInitRpc(0);
	for (;;) {
		if (sceSifBindRpc(&data_001D6248, 0x80001300, 0) < 0)
			{ func_00190BA0("libdbc: bind failed\n"); return 0; }
		if (data_001D6248.a[9] != 0)
			break;
		i = 0x10000;
		while (i--)
			;
	}
	for (;;) {
		if (sceSifBindRpc(&data_001D6270, 0x8000131B, 0) < 0)
			{ func_00190BA0("libdbc: bind failed\n"); return 0; }
		if (data_001D6270.a[9] != 0)
			break;
		i = 0x10000;
		while (i--)
			;
	}
	for (;;) {
		if (sceSifBindRpc(&data_001D6298, 0x8000131C, 0) < 0)
			{ func_00190BA0("libdbc: bind failed\n"); return 0; }
		if (data_001D6298.a[9] != 0)
			break;
		i = 0x10000;
		while (i--)
			;
	}
	ver = func_0018FCB8();
	if ((ver >> 8) != 3) {
		scePrintf("libdbc: Module version mismatch ");
		scePrintf("[libdbc.a = %d.%d, dbcman.irx = %d.%d]\n", 3, 0, ver >> 8, ver & 0xff);
		return 0;
	}
	sp.initCount = 1;
	sp.maxCount = 127;
	if ((data_001D9800 = CreateSema(&sp)) < 0)
		return 0;
	for (i = 15; i >= 0; i--)
		data_001D97C0[i] = 0;
	func_00190BA0("libdbc: SifDmaAddr %08x\n", data_001D9740);
	sceDbcSetWorkAddr((int)data_001D9740);
	data_001CC410 = 1;
	return 1;
}

/* the library's end: the semaphore deleted once bound */
int func_0018FF50(void)
{
	if (data_001CC410 == 0)
		return 0;
	data_001CC410 = 0;
	if (data_001D9800 >= 0)
		DeleteSema(data_001D9800);
	return 1;
}

int sceDbcSetWorkAddr(int addr)
{
	data_001D62C0[1] = addr;
	if (sceSifCallRpc(&data_001D6248, 0x80001304, 0, data_001D62C0, 640, data_001D62C0, 640, 0, 0) < 0) {
		func_00190BA0("sceDbcSetWorkAddr: rpc error\n");
		return 0;
	}
	return data_001D62C0[0];
}

/* Function 0x80001301's request: the socket's parameters, the reply word, and the caller's two
 * buffers -- DMA addresses (libpad2's 0x1BD630 passes one it checks for 64-byte alignment, and that
 * one + 128). */
typedef struct {
	sceDbcSocketParam param;	/* 0: copied in, its name byte by byte */
	int result;			/* 36: the reply */
	void *buf0;			/* 40 */
	void *buf1;			/* 44 */
} sceDbcCreateRequest;

/* The request built in the buffer through a pointer taken after the init test, which also reads
 * the reply (it holds the base in s0 from its first store, the name copied through bases loop.c
 * hoists). GCC r24a, open since r12a (40 words, then 2): the two buffers are POINTERS -- under strict
 * aliasing their stores alias no int load, so the parameter's second word is loaded right after the
 * first buffer's store (an int store there makes that load wait two cycles behind it, and i = 0
 * takes its place). */
int sceDbcCreateSocket(sceDbcSocketParam *p, void *buf0, void *buf1)
{
	sceDbcCreateRequest *r;
	int i;

	if (data_001CC410 == 0)
		return 0;
	r = (sceDbcCreateRequest *)data_001D62C0;
	r->param.f0 = p->f0;
	r->buf0 = buf0;
	r->buf1 = buf1;
	r->param.f4 = p->f4;
	r->param.f8 = p->f8;
	r->param.f12 = p->f12;
	r->param.f16 = p->f16;
	for (i = 0; i < 16; i++)
		r->param.name[i] = p->name[i];
	if (sceSifCallRpc(&data_001D6248, 0x80001301, 0, data_001D62C0, 640, data_001D62C0, 640, 0, 0) < 0) {
		func_00190BA0("sceDbcCreateSocket: rpc error\n");
		return 0;
	}
	return r->result;
}

int sceDbcDeleteSocket(int s)
{
	if (data_001CC410 == 0)
		return 0;
	data_001D62C0[0] = s;
	if (sceSifCallRpc(&data_001D6248, 0x80001302, 0, data_001D62C0, 640, data_001D62C0, 640, 0, 0) < 0) {
		func_00190BA0("sceDbcDeleteSocket: rpc error\n");
		return 0;
	}
	return data_001D62C0[1];
}

int sceDbcInitSocket(int s)
{
	data_001D62C0[0] = s;
	if (sceSifCallRpc(&data_001D6248, 0x80001315, 0, data_001D62C0, 640, data_001D62C0, 640, 0, 0) < 0) {
		func_00190BA0("sceDbcInitSocket: rpc error\n");
		return 0;
	}
	return data_001D62C0[1];
}

int sceDbcResetSocket(int s)
{
	data_001D62C0[0] = s;
	if (sceSifCallRpc(&data_001D6248, 0x80001316, 0, data_001D62C0, 640, data_001D62C0, 640, 0, 0) < 0) {
		func_00190BA0("sceDbcResetSocket: rpc error\n");
		return 0;
	}
	return data_001D62C0[1];
}

int sceDbcGetDeviceStatus(int s)
{
	data_001D62C0[0] = s;
	if (sceSifCallRpc(&data_001D6248, 0x80001317, 0, data_001D62C0, 640, data_001D62C0, 640, 0, 0) < 0) {
		func_00190BA0("sceDbcGetDeviceStatus: rpc error\n");
		return 0;
	}
	return data_001D62C0[1];
}

/* Function 0x8000131D: a second status query that prints GetDeviceStatus's message (the vendor's
 * copy), -1 on failure; the map keeps its placeholder. */
int func_001904F8(int *p)
{
	data_001D62C0[2] = 2;
	data_001D62C0[4] = p[2];
	data_001D62C0[5] = p[3];
	if (sceSifCallRpc(&data_001D6248, 0x8000131D, 0, data_001D62C0, 640, data_001D62C0, 640, 0, 0) < 0) {
		func_00190BA0("sceDbcGetDeviceStatus: rpc error\n");
		return -1;
	}
	return data_001D62C0[1];
}

extern int WaitSema(int sema);
extern int SignalSema(int sema);

/* Function 0x80001318: a send and a receive in one call -- both sizes in, the data copied in after
 * them, then on a result that is not negative both sizes and the reply's data (at word 132) copied
 * back out; ReceiveData's shape, the bases assigned in the loops (GCC r24a). */
int sceDbcSRData(int s, int a1, int *ssize, char *sbuf, int *rsize, char *rbuf)
{
	unsigned int *p;
	char *d;
	int i;

	p = data_001D62C0;
	p[0] = s;
	p[1] = a1;
	p[2] = *ssize;
	p[3] = *rsize;
	for (i = 0; i < *ssize; i++) {
		d = (char *)&p[4];
		d[i] = sbuf[i];
	}
	if (sceSifCallRpc(&data_001D6248, 0x80001318, 0, data_001D62C0, 640, data_001D62C0, 640, 0, 0) < 0) {
		func_00190BA0("sceDbcSRData: rpc error\n");
		return 0;
	}
	if ((int)p[260] >= 0) {
		*ssize = p[2];
		*rsize = p[3];
		for (i = 0; i < (int)p[3]; i++) {
			d = (char *)&p[132];
			rbuf[i] = d[i];
		}
	}
	return p[260];
}

/* Function 0x80001319, under the library's semaphore: the data copied in after the size; the size
 * written back from the reply when its result is not negative. The buffer's address is taken
 * after the wait (its first store goes through the %lo, the rest through the register: as a local
 * initialised at its declaration it is held across WaitSema). The copy's base is assigned inside
 * the loop (GCC r24a; open since r22a, 21 words): loop.c hoists it into the preheader -- the
 * target's `addiu a2,s2,12` after the size test, whose delay slot then takes `i = 0` -- where
 * combine, which runs after loop.c, cannot fold it into the store; assigned before the loop it is
 * the test's delay slot and the buffer's base and the size pointer swap s2/s3, and copying
 * through `((char *)&p[3])[i]` folds the 12 into the store (`sb v1,12(a0)`, 0x124). */
int sceDbcSendData(int s, int a1, int *size, char *buf)
{
	unsigned int *p;
	char *d;
	int i;

	WaitSema(data_001D9800);
	p = data_001D62C0;
	p[0] = s;
	p[1] = a1;
	p[2] = *size;
	for (i = 0; i < *size; i++) {
		d = (char *)&p[3];
		d[i] = buf[i];
	}
	if (sceSifCallRpc(&data_001D6248, 0x80001319, 0, data_001D62C0, 640, data_001D62C0, 640, 0, 0) < 0) {
		func_00190BA0("sceDbcSendData: rpc error\n");
		SignalSema(data_001D9800);
		return 0;
	}
	if ((int)p[131] >= 0)
		*size = p[2];
	SignalSema(data_001D9800);
	return p[131];
}

extern unsigned int data_001D6540[1060];	/* SendData2's buffer (4240 bytes) */
extern int sceSifCheckStatRpc(sceSifClientData *cd);

/* Function 0x8000131B on the second client, no-wait: the data (at most 4224 bytes) copied in after
 * the size as SendData's, refused while the last send is still in flight (GCC r24a). */
int sceDbcSendData2(int s, int a1, int *size, char *buf)
{
	unsigned int *p;
	char *d;
	int i;

	if (*size > 4224) {
		func_00190BA0("libdbc: SendData2 data length too long [%d]\n", *size);
		return 0;
	}
	WaitSema(data_001D9800);
	p = data_001D6540;
	p[0] = s;
	p[1] = a1;
	p[2] = *size;
	for (i = 0; i < *size; i++) {
		d = (char *)&p[3];
		d[i] = buf[i];
	}
	if (sceSifCheckStatRpc(&data_001D6270) == 1) {
		func_00190BA0("dbcman : SendData2 BUSY\n");
		SignalSema(data_001D9800);
		return 0;
	}
	if (sceSifCallRpc(&data_001D6270, 0x8000131B, 1, data_001D6540, 4240, data_001D6540, 4240, 0, 0) < 0) {
		func_00190BA0("sceDbcSendData2: rpc error\n");
		SignalSema(data_001D9800);
		return 0;
	}
	SignalSema(data_001D9800);
	return 1;
}

extern unsigned int data_001D7640[2084];	/* SendData3's buffer (8336 bytes) */

/* Function 0x8000131C on the third client, no-wait, at most 8320 bytes: SendData2's shape, a failed
 * call returning without a message (GCC r24a). */
int sceDbcSendData3(int s, int a1, int *size, char *buf)
{
	unsigned int *p;
	char *d;
	int i;

	if (*size > 8320) {
		func_00190BA0("libdbc: SendData3 data length too long\n");
		return 0;
	}
	WaitSema(data_001D9800);
	p = data_001D7640;
	p[0] = s;
	p[1] = a1;
	p[2] = *size;
	for (i = 0; i < *size; i++) {
		d = (char *)&p[3];
		d[i] = buf[i];
	}
	if (sceSifCheckStatRpc(&data_001D6298) == 1) {
		func_00190BA0("dbcman : SendData3 BUSY\n");
		SignalSema(data_001D9800);
		return 0;
	}
	if (sceSifCallRpc(&data_001D6298, 0x8000131C, 1, data_001D7640, 8336, data_001D7640, 8336, 0, 0) < 0) {
		SignalSema(data_001D9800);
		return 0;
	}
	SignalSema(data_001D9800);
	return 1;
}

/* OPEN (61 words, a frame 16 bytes smaller in the target): the target holds the buffer's address
 * in s0, size and buf in s1/s2 and copies through a base made once (`addiu a2,s0,12`); this build
 * keeps the buffer's %hi in a fourth saved register. */
int sceDbcReceiveData(int s, int a1, int *size, char *buf)
{
	unsigned int *p;
	char *d;
	int i;

	p = data_001D62C0;
	p[0] = s;
	p[1] = a1;
	p[2] = *size;
	if (sceSifCallRpc(&data_001D6248, 0x8000131A, 0, p, 640, p, 640, 0, 0) < 0) {
		func_00190BA0("sceDbcReceiveData: rpc error\n");
		return 0;
	}
	if ((int)p[131] >= 0) {
		*size = p[2];
		for (i = 0; i < (int)p[2]; i++) {
			d = (char *)&p[3];
			buf[i] = d[i];
		}
	}
	return p[131];
}

void func_00190BA0(const char *fmt, ...)
{
}
