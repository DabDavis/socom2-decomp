/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libdbc (MAIN 0x18FCB8-0x190BC8): Sony's SDK DBC client -- the EE side of dbcman.irx, the
 * controller-socket manager libpad2 sits on. Not in SOCOM 1's demo (it links libpad); each RPC
 * wrapper names itself in its error message ("sceDbcSetWorkAddr: rpc error"), which named the rows
 * (round 12a). Every call is one sceSifCallRpc on the first client with the same 640-byte buffer
 * both ways, its function number 0x80001300 + n; the reply's words are read back out of it. The
 * debug printf is compiled out: an empty varargs function (func_00190BA0, the object's last).
 * The client data and the buffer are the object's own .bss, spelled by address here (data_<VA>).
 * Not written: sceDbcInit (0x18FD10, the three binds and the version check), the end
 * (0x18FF50), sceDbcGetDepNumber and 0x1902B0 (interrupts off around a cache sync), the send and
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
