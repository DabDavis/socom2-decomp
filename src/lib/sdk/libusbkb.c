/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* Sony's libusbkb (MAIN 0x1BE6A0-0x1BFC00), the USB keyboard library's EE side over SIF RPC
 * 0x80000211: not in SOCOM 1 (its demo links no USB keyboard), so its rows are named only where a
 * function prints its own name. The RPC calls are asynchronous: each checks the library is up
 * (the semaphore made at init), takes the semaphore, sends its command (the keyboard number
 * first in the 16-byte send buffer) and records which command is pending, for which keyboard and
 * where its result goes; the RPC's end function signals the semaphore, and sceUsbKbSync (wait or
 * poll) runs the pending command's result handler from a table (0x1CE5C0: GetInfo's 0x1BEFA0,
 * Read's 0x1BEFF8, GetLocation's 0x1BF570, the LED setters' 0x1BF5C0 and 0x1BF5E0). The setters
 * without RPC keep a 56-byte state per keyboard. Written: the init and end (0x1BE6A0, 0x1BE898:
 * the library's sceUsbKbInit(int *max_connect) and sceUsbKbEnd() by Sony's API, but they print
 * only "libusbkb : ..." and "libusbkb.a : ...", so their rows keep placeholders), the RPC calls,
 * the sync and its dispatcher, GetInfo's result, the state setters; not written: the read
 * result's key-code conversion (0x1BEFF8 and its helpers 0x1BF600-0x1BFA38: 0x1BF600 is the
 * INTC 3 handler the init installs) and the other result handlers. The library's data has no names
 * (data_<VA>: the state at 0x1E41C0 -- keyboards, the per-keyboard array, the pending command --
 * the client 0x1E4260, the buffers 0x1E42C0 and 0x1E4300, the semaphore 0x1CE5D8).
 *
 * Shapes, measured: the RPC calls' keyboard numbers are u_int for Read (and 0x1BFB18, command 6,
 * which clears the keyboard's read state and prints Read's message: one string, two callers) and
 * int elsewhere (sltu against slt); the poll case of the sync tests for completion and returns 1
 * last (an early `return 1` lays the case out apart: 31s); the dispatcher clears the command
 * before the result pointer; 0x1BFB18's clears run in field order (one of 720,
 * tools/ccperm.py); GetInfo's result reads the receive buffer through a pointer of its own and
 * the read-mode setter addresses its keyboard's state straight (index first).
 *
 * The init (round 15a, build/scratch/gcc/kb/): its bind delay and its keyboard loop share one
 * u_int (both in a2; the delay's `while (i--)` compares with lui/ori 0xFFFFFFFF), the semaphore's
 * initial count is set before its maximum, the keyboard count is tested in two statements (one
 * `<= 0 || >= 128` folds to a range check), and each failure after the semaphore has its own
 * `DeleteSema; sema = -1; return -1` -- CSE gives each copy the -1 still live from the CreateSema
 * test (s0), cross-jumping then merges the copies into the one after the last test, and the copy
 * after the loop, where the -1 is no longer known, stays apart. The end matched as written. */

typedef unsigned int u_int;
typedef unsigned short u_short;
typedef unsigned char u_char;

typedef void (*sceSifEndFunc)(void *);

typedef struct {
	void *paddr;
	u_int pid;
	int tid;
	u_int mode;
	u_int command;
	void *buff;
	void *cbuff;
	sceSifEndFunc func;
	void *para;
	void *serve;
} sceSifClientData;

typedef struct {
	int ledmode;			/* 0x00: SetLEDMode's */
	int f04;			/* 0x04 */
	int f08;			/* 0x08 */
	int f0c;			/* 0x0C */
	int f10;			/* 0x10 */
	int f14;			/* 0x14 */
	int f18;			/* 0x18 */
	int f1c;			/* 0x1C */
	u_short f20;			/* 0x20 */
	u_short pad22;
	int f24[2];			/* 0x24 */
	int f2c;			/* 0x2C */
	int f30;			/* 0x30 */
	int f34;			/* 0x34 */
} UsbKbDev;

typedef struct {
	int max;			/* 0x00: keyboards the IOP side serves */
	UsbKbDev *dev;			/* 0x04 */
	int f08[2];
	int f10;			/* 0x10 */
	int f14[31];
	int cmd;			/* 0x90: the pending command */
	int no;				/* 0x94 */
	void *result;			/* 0x98 */
	int intc;			/* 0x9C */
} UsbKbState;

typedef struct {
	int max_connect;
	int now_connect;
	u_char status[127];
} UsbKbInfo;

extern UsbKbState data_001E41C0;
extern sceSifClientData data_001E4260;
extern u_char data_001E42C0[16];
extern u_char data_001E4300[144];
extern int data_001CE5D8;
extern int (*data_001CE5C0[])(void);

extern int WaitSema(int);
extern int SignalSema(int);
extern int iSignalSema(int);
extern int scePrintf(const char *, ...);
extern int sceSifCallRpc(sceSifClientData *, u_int, u_int, void *, int, void *, int, sceSifEndFunc, void *);
extern int sceSifCheckStatRpc(sceSifClientData *);


struct SemaParam {
	int currentCount;
	int maxCount;
	int initCount;
	int numWaitThreads;
	u_int attr;
	u_int option;
};

extern int sceSifInitRpc(int);
extern int sceSifBindRpc(sceSifClientData *, u_int, int);
extern int CreateSema(struct SemaParam *);
extern int DeleteSema(int);
extern int AddIntcHandler(int, int (*)(int), int);
extern int RemoveIntcHandler(int, int);
extern int EnableIntc(int);
extern int DisableIntc(int);
extern int DIntr(void);
extern int EIntr(void);
extern int func_001BEE88(int mode, int *result);
extern int func_001BF600(int);
extern UsbKbDev data_001E4390[];

static void func_001BEF28(void *);
static int func_001BEF38(void);

int func_001BE6A0(int *max)
{
	UsbKbInfo info;
	struct SemaParam sema;
	int result;
	u_int i;

	sceSifInitRpc(0);
	while (1) {
		if (sceSifBindRpc(&data_001E4260, 0x80000211, 0) < 0) {
			scePrintf("libusbkb : bind errr\n");
			return -1;
		}
		if (data_001E4260.serve != 0)
			break;
		i = 0x10000;
		while (i--)
			;
	}
	sema.initCount = 1;
	sema.maxCount = 1;
	if ((data_001CE5D8 = CreateSema(&sema)) == -1) {
		scePrintf("libusbkb : CreateSema Error\n");
		return -1;
	}
	if (sceUsbKbGetInfo(&info) != 0) {
		DeleteSema(data_001CE5D8);
		data_001CE5D8 = -1;
		return -1;
	}
	func_001BEE88(0, &result);
	if (result != 0) {
		DeleteSema(data_001CE5D8);
		data_001CE5D8 = -1;
		return -1;
	}
	if (info.max_connect <= 0) {
		DeleteSema(data_001CE5D8);
		data_001CE5D8 = -1;
		return -1;
	}
	if (info.max_connect >= 128) {
		DeleteSema(data_001CE5D8);
		data_001CE5D8 = -1;
		return -1;
	}
	data_001E41C0.max = info.max_connect;
	*max = info.max_connect;
	data_001E41C0.dev = data_001E4390;
	if (data_001E41C0.dev == 0) {
		DeleteSema(data_001CE5D8);
		data_001CE5D8 = -1;
		return -1;
	}
	for (i = 0; i < info.max_connect; i++) {
		data_001E41C0.dev[i].ledmode = 1;
		data_001E41C0.dev[i].f04 = 1;
		data_001E41C0.dev[i].f08 = 1;
		data_001E41C0.dev[i].f0c = 0;
		data_001E41C0.dev[i].f10 = 0;
		data_001E41C0.dev[i].f14 = 0;
		data_001E41C0.dev[i].f18 = 0;
		data_001E41C0.dev[i].f1c = 0;
		data_001E41C0.dev[i].f20 = 0;
		data_001E41C0.dev[i].f2c = 0;
		data_001E41C0.dev[i].f30 = 0;
		data_001E41C0.dev[i].f34 = 0;
	}
	data_001E41C0.f10 = 0;
	if ((data_001E41C0.intc = AddIntcHandler(3, func_001BF600, -1)) < 0) {
		DeleteSema(data_001CE5D8);
		data_001CE5D8 = -1;
		return -1;
	}
	EnableIntc(3);
	return 0;
}

int func_001BE898(void)
{
	int err = 0;
	int intr, ret;

	intr = DIntr();
	ret = RemoveIntcHandler(3, data_001E41C0.intc);
	if (ret == 0)
		DisableIntc(3);
	if (intr)
		EIntr();
	if (ret < 0) {
		err = 1;
		scePrintf("libusbkb.a : RemoveIntcHandler() --> error\n");
	}
	if (data_001CE5D8 != -1) {
		if (DeleteSema(data_001CE5D8) < 0) {
			err++;
			scePrintf("libusbkb.a : DeleteSema() --> error\n");
		} else
			data_001CE5D8 = -1;
	}
	if (err)
		return -1;
	return 0;
}

int sceUsbKbGetInfo(UsbKbInfo *info)
{
	int ret;

	if (data_001CE5D8 == -1)
		return -1;
	WaitSema(data_001CE5D8);
	if ((ret = sceSifCallRpc(&data_001E4260, 1, 1, data_001E42C0, 16, data_001E4300, 144,
	                         func_001BEF28, 0)) != 0) {
		scePrintf("sceUsbKbGetInfo: sceSifCallRpc -> %d\n", ret);
		return -2;
	}
	data_001E41C0.cmd = 1;
	data_001E41C0.result = info;
	return 0;
}

int sceUsbKbRead(u_int no, void *data)
{
	int ret;

	if (data_001CE5D8 == -1)
		return -1;
	if (no >= data_001E41C0.max)
		return -11;
	data_001E42C0[0] = no;
	WaitSema(data_001CE5D8);
	if ((ret = sceSifCallRpc(&data_001E4260, 2, 1, data_001E42C0, 16, data_001E4300, 80,
	                         func_001BEF28, 0)) != 0) {
		scePrintf("sceUsbKbRead: sceSifCallRpc -> %d\n", ret);
		return -2;
	}
	data_001E41C0.cmd = 2;
	data_001E41C0.no = no;
	data_001E41C0.result = data;
	return 0;
}

int sceUsbKbGetLocation(int no, void *location)
{
	int ret;

	if (data_001CE5D8 == -1)
		return -1;
	if (no >= data_001E41C0.max)
		return -11;
	data_001E42C0[0] = no;
	WaitSema(data_001CE5D8);
	if ((ret = sceSifCallRpc(&data_001E4260, 3, 1, data_001E42C0, 16, data_001E4300, 16,
	                         func_001BEF28, 0)) != 0) {
		scePrintf("sceUsbKbGetLocation: sceSifCallRpc -> %d\n", ret);
		return -2;
	}
	data_001E41C0.cmd = 3;
	data_001E41C0.no = no;
	data_001E41C0.result = location;
	return 0;
}

int sceUsbKbSetLEDStatus(int no, u_char led)
{
	int ret;

	if (data_001CE5D8 == -1)
		return -1;
	if (no >= data_001E41C0.max)
		return -11;
	data_001E42C0[0] = no;
	data_001E42C0[1] = led;
	WaitSema(data_001CE5D8);
	if ((ret = sceSifCallRpc(&data_001E4260, 4, 1, data_001E42C0, 16, data_001E4300, 16,
	                         func_001BEF28, 0)) != 0) {
		scePrintf("sceUsbKbSetLEDStatus: sceSifCallRpc -> %d\n", ret);
		return -2;
	}
	data_001E41C0.cmd = 4;
	data_001E41C0.no = no;
	data_001E41C0.result = 0;
	return 0;
}

int sceUsbKbSetLEDMode(int no, u_int mode)
{
	int ret;

	if (data_001CE5D8 == -1)
		return -1;
	if (no >= data_001E41C0.max)
		return -11;
	if (mode >= 3)
		return -12;
	data_001E41C0.dev[no].ledmode = mode;
	data_001E42C0[0] = no;
	data_001E42C0[1] = mode;
	WaitSema(data_001CE5D8);
	if ((ret = sceSifCallRpc(&data_001E4260, 5, 1, data_001E42C0, 16, data_001E4300, 16,
	                         func_001BEF28, 0)) != 0) {
		scePrintf("sceUsbKbSetLEDMode: sceSifCallRpc -> %d\n", ret);
		return -2;
	}
	data_001E41C0.cmd = 5;
	data_001E41C0.no = no;
	data_001E41C0.result = 0;
	return 0;
}

int func_001BEDA0(int no, int sta, int interval)
{
	if (sta < 0)
		return -11;
	if (interval < 0)
		return -12;
	data_001E41C0.dev[no].f1c = interval;
	data_001E41C0.dev[no].f18 = sta;
	data_001E41C0.dev[no].f14 = 0;
	return 0;
}

int func_001BEDE8(int no, u_int type)
{
	if (no >= data_001E41C0.max)
		return -11;
	if (type >= 2)
		return -12;
	data_001E41C0.dev[no].f04 = type;
	return 0;
}

int func_001BEE38(int no, u_int arrange)
{
	if (no >= data_001E41C0.max)
		return -11;
	if (arrange >= 3)
		return -12;
	data_001E41C0.dev[no].f08 = arrange;
	return 0;
}

int func_001BEE88(int mode, int *result)
{
	if (data_001CE5D8 == -1)
		return -1;
	switch (mode) {
	case 0:
		WaitSema(data_001CE5D8);
		*result = func_001BEF38();
		SignalSema(data_001CE5D8);
		return 0;
	case 1:
		if (sceSifCheckStatRpc(&data_001E4260) == 0) {
			*result = func_001BEF38();
			return 0;
		}
		return 1;
	}
	return -11;
}

static void func_001BEF28(void *p)
{
	iSignalSema(data_001CE5D8);
}

static int func_001BEF38(void)
{
	int ret;

	ret = 0;
	if (data_001E41C0.cmd >= 1 && data_001E41C0.cmd <= 5)
		ret = data_001CE5C0[data_001E41C0.cmd]();
	data_001E41C0.cmd = 0;
	data_001E41C0.result = 0;
	return ret;
}

int func_001BEFA0(void)
{
	UsbKbInfo *info;
	u_char *rb;
	int i;

	info = data_001E41C0.result;
	rb = data_001E4300;
	info->max_connect = rb[0];
	info->now_connect = rb[1];
	for (i = 0; i < info->max_connect; i++)
		info->status[i] = rb[2 + i];
	return 0;
}

int func_001BFAA8(int no, u_int rmode)
{
	if (no >= data_001E41C0.max)
		return -11;
	if (rmode >= 2)
		return -12;
	if (rmode != data_001E41C0.dev[no].f0c) {
		data_001E41C0.dev[no].f14 = 0;
		data_001E41C0.dev[no].f2c = 0;
		data_001E41C0.dev[no].f30 = 0;
		data_001E41C0.f10 = 0;
		data_001E41C0.dev[no].f20 = 0;
	}
	data_001E41C0.dev[no].f0c = rmode;
	return 0;
}

int func_001BFB18(u_int no)
{
	UsbKbDev *d;
	int ret;

	if (data_001CE5D8 == -1)
		return -1;
	if (no >= data_001E41C0.max)
		return -11;
	data_001E42C0[0] = no;
	WaitSema(data_001CE5D8);
	if ((ret = sceSifCallRpc(&data_001E4260, 6, 1, data_001E42C0, 16, data_001E4300, 0,
	                         func_001BEF28, 0)) != 0) {
		scePrintf("sceUsbKbRead: sceSifCallRpc -> %d\n", ret);
		return -2;
	}
	d = &data_001E41C0.dev[no];
	d->f10 = 0;
	d->f14 = 0;
	d->f20 = 0;
	d->f2c = 0;
	d->f30 = 0;
	d->f34 = 0;
	return 0;
}
