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
 * Not written: sceDbcGetDepNumber and 0x1902B0 (interrupts off around a cache sync), the send and
 * receive-and-send calls (sceDbcSRData, sceDbcSendData, SendData2, SendData3). */

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

/* OPEN (40 words, the size short): the target keeps the buffer's base in s0 from its first store,
 * copies the parameter's five words with a1 and a2 stored between and the name byte by byte through
 * base pointers made before the loop; neither the buffer indexed nor a struct pointer over it (as
 * here) gives that allocation. */
int sceDbcCreateSocket(sceDbcSocketParam *p, int a1, int a2)
{
	sceDbcSocketParam *s = (sceDbcSocketParam *)data_001D62C0;
	int i;

	if (data_001CC410 == 0)
		return 0;
	s->f0 = p->f0;
	data_001D62C0[10] = a1;
	data_001D62C0[11] = a2;
	s->f4 = p->f4;
	s->f8 = p->f8;
	s->f12 = p->f12;
	s->f16 = p->f16;
	for (i = 0; i < 16; i++)
		s->name[i] = p->name[i];
	if (sceSifCallRpc(&data_001D6248, 0x80001301, 0, data_001D62C0, 640, data_001D62C0, 640, 0, 0) < 0) {
		func_00190BA0("sceDbcCreateSocket: rpc error\n");
		return 0;
	}
	return data_001D62C0[9];
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

/* Function 0x80001319, under the library's semaphore: the data copied in after the size; the size
 * written back from the reply when its result is not negative. The buffer's address is taken
 * after the wait (its first store goes through the %lo, the rest through the register: as a local
 * initialised at its declaration it is held across WaitSema).
 * OPEN (right size, 21 words; GCC r22a): the buffer's base takes s3 and the size pointer s2 here,
 * the other way round in the target, whose `i = 0` sits in the size test's delay slot and whose
 * copy runs through a base made in the loop's preheader (`addiu a2,s2,12`); copying through
 * `((char *)&p[3])[i]` instead folds the 12 into the store (`sb v1,12(a0)`, 0x124). */
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
	d = (char *)&p[3];
	for (i = 0; i < *size; i++)
		d[i] = buf[i];
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

/* OPEN (61 words, a frame 16 bytes smaller in the target): the target holds the buffer's address
 * in s0, size and buf in s1/s2 and copies through a base made once (`addiu a2,s0,12`); this build
 * keeps the buffer's %hi in a fourth saved register. */
int sceDbcReceiveData(int s, int a1, int *size, char *buf)
{
	int i;

	data_001D62C0[0] = s;
	data_001D62C0[1] = a1;
	data_001D62C0[2] = *size;
	if (sceSifCallRpc(&data_001D6248, 0x8000131A, 0, data_001D62C0, 640, data_001D62C0, 640, 0, 0) < 0) {
		func_00190BA0("sceDbcReceiveData: rpc error\n");
		return 0;
	}
	if ((int)data_001D62C0[131] >= 0) {
		*size = data_001D62C0[2];
		for (i = 0; i < (int)data_001D62C0[2]; i++)
			buf[i] = ((char *)&data_001D62C0[3])[i];
	}
	return data_001D62C0[131];
}

void func_00190BA0(const char *fmt, ...)
{
}
