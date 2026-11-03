/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* lgAud, the USB headset's audio (liblgaud.a 1.08.002, "lgAudInit version 1.08.002"), OVL1
 * 0x243840-0x245690: the demo's object 0x3B5980 (C, 25 functions; SOCOM II's has four more), prebuilt
 * with ee-gcc 2.96 at -O2 -G0 (README "The GCC front"; 2.96's sibling calls keep the frame they no
 * longer need: lgAudASync). Every call is a SIF RPC to lgaud.irx through one request block
 * (devRpcData, carved 64-aligned out of the caller's allocator by lgAudInit) under a semaphore; each
 * reply carries the call's result and the device-change flag, which is folded into devicechanged.
 * The failure messages are the library's own, copied between calls with their names unchanged
 * (StopRecording prints StartRecording's, SetMixer GetMixer's). Names are SOCOM 1's (s1:exact/callee)
 * or by the demo's order and the functions' own messages; the request's fields are named by their use
 * here. The functions in the image's order. */

typedef unsigned char u_char;
typedef unsigned short u_short;

typedef struct {
	u_char Mode;                    /* 0x00: nonzero */
	u_char pad01[11];
	short pad0c;
} lgAudOpenParam;                       /* 14 bytes */

typedef struct {
	u_short pad00[6];
	u_short value;                  /* 0x0C: what GetMixer returns */
	u_short pad0e;
} lgAudMixer;                           /* 16 bytes */

typedef struct {
	int pad[83];
} lgAudDeviceDesc;                      /* 332 bytes */

struct lgaud_rpc {
	int result;                     /* 0x00 */
	int devicechanged;              /* 0x04: the reply's device-change bits */
	int index;                      /* 0x08: the device (Enumerate, Open); the irx's version (Init) */
	int handle;                     /* 0x0C */
	u_char arg[16];                 /* 0x10: [0]/[1] the volumes, [2] the read/write's channel */
	union {
		struct {
			int size;       /* 0x20: the bytes a read returns; Init's: the irx's buffer */
			int pad24[3];
			u_char data[1]; /* 0x30 */
		} buf;
		lgAudMixer mixer;
		lgAudOpenParam open;
		lgAudDeviceDesc desc;
	} u;
};

struct lgaud_async {
	void *dest;
	int *size;
	int *hint;
};

struct sif_client_data {                /* sceSifClientData */
	int pad[9];
	void *serve;                    /* 0x24: set once the server is bound */
	int pad28[2];
};

struct SemaParam {
	int currentCount;
	int maxCount;
	int initCount;
	int numWaitThreads;
	unsigned int attr;
	unsigned int option;
};

extern int WaitSema(int sema);
extern int SignalSema(int sema);
extern int iSignalSema(int sema);
extern int CreateSema(struct SemaParam *param);
extern int printf(const char *fmt, ...);
extern void *memcpy(void *d, const void *s, unsigned int n);
extern void *memset(void *d, int c, unsigned int n);
extern void *malloc(unsigned int n);
extern void free(void *p);
extern void sceSifInitRpc(unsigned int mode);
extern int sceSifBindRpc(struct sif_client_data *cd, unsigned int id, unsigned int mode);
extern int sceSifCallRpc(struct sif_client_data *cd, int fno, int mode, void *send, int ssize, void *recv,
                         int rsize, void (*end)(void *), void *end_arg);
extern int sceSifCheckStatRpc(struct sif_client_data *cd);

extern int data_003DCFA8;                       /* the irx's buffer size devRpcData was carved for */
extern int data_003DCFAC;                       /* the streaming maximum: that size less the header */
extern void *data_003DCFB0;                     /* the allocation devRpcData is carved from */
extern struct lgaud_rpc *devRpcData;
extern int devicechanged;
extern int threadsafeSema;                      /* -1 until lgAudInit */
extern struct sif_client_data cdUA;
extern struct lgaud_rpc data_004149C0;          /* lgAudInit's own request */
extern struct lgaud_async data_00414B30;        /* the async call's destination */

#define HALT()	for (;;)			/* GNU as pads the R5900's short loop with nops */

/* OPEN (32 words, every one a register): the shape is the target's -- the bind loop resets its delay
 * count before the bind (the count lives across the call, in s0, as the target's), the halts are
 * `for (;;);` and the delay `while (i--);`, the R5900's short loops GNU as pads with five nops; the
 * allocator gives &cdUA (the serve test's hoisted address) and dealloc the other two of s2/s3, and
 * the cascade moves the request's base, the size and the allocation's address (the target keeps the
 * address in s0 for the store, here its %lo rides the store). Declaration order, the default
 * assignments' order, nested ifs, the size's spelling and the two size stores' order change nothing
 * (-fno-strict-aliasing loses 25 of lgAud's matched functions: lgAud keeps strict aliasing). */
int lgAudInit(void *(*alloc)(unsigned int), void (*dealloc)(void *))
{
	struct SemaParam sp;
	int size;
	int i;

	sceSifInitRpc(0);
	while (1) {
		i = 0x10000;
		if (sceSifBindRpc(&cdUA, 0x50494C42, 0) < 0) {
			printf("lgAudInit: rpc binding error\n");
			printf("lgAudInit: system halted.\n");
			HALT();
		}
		if (cdUA.serve != 0)
			break;
		while (i--)
			;
	}
	data_004149C0.index = 0;
	if (sceSifCallRpc(&cdUA, 16, 0, &data_004149C0, 32, &data_004149C0, 48, 0, 0) != 0) {
		printf("lgAudInit: rpc error\n");
		printf("lgAudInit: system halted.\n");
		HALT();
	}
	devicechanged &= ~2;
	devicechanged |= data_004149C0.devicechanged;
	if (data_004149C0.index != 264) {
		printf("lgAudInit: wrong version of lgAud.irx!\n");
		printf("lgAudInit:  liblgaud.a is version %d.%02d\n", 1, 8);
		printf("lgAudInit:  lgaud.irx is version %d.%02d\n", data_004149C0.index / 256,
		       data_004149C0.index % 256);
		printf("lgAudInit: system halted.\n");
		HALT();
	}
	if (alloc == 0)
		alloc = malloc;
	if (dealloc == 0)
		dealloc = free;
	if (data_003DCFB0 != 0 && data_004149C0.u.buf.size != data_003DCFA8) {
		dealloc(data_003DCFB0);
		data_003DCFB0 = 0;
	}
	if (data_003DCFB0 == 0) {
		size = ((data_004149C0.u.buf.size + 63) & ~63) + 64;
		data_003DCFB0 = alloc(size);
		if (data_003DCFB0 == 0) {
			printf("lgAudInit: unable to allocate %d bytes!\n", size);
			printf("lgAudInit: system halted.\n");
			HALT();
		}
		devRpcData = (struct lgaud_rpc *)(((int)data_003DCFB0 + 63) & ~63);
		data_003DCFA8 = data_004149C0.u.buf.size;
		data_003DCFAC = data_004149C0.u.buf.size - 48;
	}
	if (threadsafeSema == -1) {
		memset(&sp, 0, sizeof sp);
		sp.initCount = 1;
		sp.maxCount = 1;
		sp.option = 0;
		threadsafeSema = CreateSema(&sp);
		if (threadsafeSema == -1) {
			printf("lgAudInit: unable to create semaphore!\n");
			printf("lgAudInit: system halted.\n");
			HALT();
		}
	}
	printf("lgAudInit version 1.08.002: max streaming data size: %d\n", data_003DCFAC);
	return 0;
}

int lgAudEnumHint(int *hint)
{
	if (hint == 0) {
		printf("lgAudEnumHint: invalid parameter (hint)\n");
		return 0x80000004;
	}
	WaitSema(threadsafeSema);
	if ((*hint = devicechanged) == 1)
		devicechanged = 0;
	SignalSema(threadsafeSema);
	return 0;
}

int lgAudEnumerate(int index, lgAudDeviceDesc *description)
{
	int ret;

	if (description == 0) {
		printf("lgAudEnumerate: invalid parameter (description)\n");
		return 0x80000004;
	}
	WaitSema(threadsafeSema);
	devRpcData->index = index;
	if (sceSifCallRpc(&cdUA, 1, 0, devRpcData, 32, devRpcData, 368, 0, 0) == 0) {
		ret = devRpcData->result;
		*description = devRpcData->u.desc;
		devicechanged &= ~2;
		devicechanged |= devRpcData->devicechanged;
		SignalSema(threadsafeSema);
		return ret;
	}
	printf("lgAudEnumerate: rpc call failed\n");
	SignalSema(threadsafeSema);
	return 0x80000000;
}

int lgAudOpen(int index, lgAudOpenParam *openparam, int *handle)
{
	int ret;

	if (handle == 0) {
		printf("lgAudOpen: invalid parameter (device)\n");
		return 0x80000004;
	}
	if (openparam == 0) {
		printf("lgAudOpen: invalid parameter (openparam)\n");
		return 0x80000004;
	}
	if (openparam->Mode == 0) {
		printf("lgAudOpen: invalid parameter (openparam->Mode)\n");
		return 0x80000004;
	}
	WaitSema(threadsafeSema);
	devRpcData->index = index;
	devRpcData->u.open = *openparam;
	if (sceSifCallRpc(&cdUA, 2, 0, devRpcData, 48, devRpcData, 32, 0, 0) == 0) {
		ret = devRpcData->result;
		*handle = devRpcData->handle;
		devicechanged &= ~2;
		devicechanged |= devRpcData->devicechanged;
		SignalSema(threadsafeSema);
		return ret;
	}
	printf("lgAudOpen: rpc call failed\n");
	SignalSema(threadsafeSema);
	return 0x80000000;
}

int lgAudClose(int handle)
{
	int ret;

	WaitSema(threadsafeSema);
	devRpcData->handle = handle;
	if (sceSifCallRpc(&cdUA, 3, 0, devRpcData, 32, devRpcData, 32, 0, 0) == 0) {
		ret = devRpcData->result;
		devicechanged &= ~2;
		devicechanged |= devRpcData->devicechanged;
		SignalSema(threadsafeSema);
		return ret;
	}
	printf("lgAudClose: rpc call failed\n");
	SignalSema(threadsafeSema);
	return 0x80000000;
}

int lgAudRead(int handle, int channel, void *dest, int *size)
{
	int ret;

	if (dest == 0) {
		printf("lgAudRead: invalid parameter (dest)\n");
		return 0x80000004;
	}
	if (size == 0 || *size > data_003DCFAC) {
		printf("lgAudRead: invalid parameter (size too big)\n");
		return 0x80000004;
	}
	WaitSema(threadsafeSema);
	devRpcData->handle = handle;
	devRpcData->u.buf.size = *size;
	devRpcData->arg[2] = channel;
	if (sceSifCallRpc(&cdUA, 8, 0, devRpcData, 48, devRpcData, (*size + 63) >> 4 << 4, 0, 0) == 0) {
		ret = devRpcData->result;
		memcpy(dest, devRpcData->u.buf.data, devRpcData->u.buf.size);
		*size = devRpcData->u.buf.size;
		devicechanged &= ~2;
		devicechanged |= devRpcData->devicechanged;
		SignalSema(threadsafeSema);
		return ret;
	}
	*size = 0;
	printf("lgAudRead: rpc call failed\n");
	SignalSema(threadsafeSema);
	return 0x80000000;
}

int lgAudWrite(int handle, int channel, void *src, int *size)
{
	int ret;

	if (src == 0) {
		printf("lgAudWrite: invalid parameter (src)\n");
		return 0x80000004;
	}
	if (size == 0 || *size > data_003DCFAC) {
		printf("lgAudWrite: invalid parameter (size too big)\n");
		return 0x80000004;
	}
	WaitSema(threadsafeSema);
	devRpcData->handle = handle;
	devRpcData->u.buf.size = *size;
	memcpy(devRpcData->u.buf.data, src, *size);
	devRpcData->arg[2] = channel;
	if (sceSifCallRpc(&cdUA, 9, 0, devRpcData, (*size + 63) >> 4 << 4, devRpcData, 48, 0, 0) == 0) {
		ret = devRpcData->result;
		*size = devRpcData->u.buf.size;
		devicechanged &= ~2;
		devicechanged |= devRpcData->devicechanged;
		SignalSema(threadsafeSema);
		return ret;
	}
	*size = 0;
	printf("lgAudWrite: rpc call failed\n");
	SignalSema(threadsafeSema);
	return 0x80000000;
}

int lgAudWriteVag(int handle, int channel, void *src, int *size)
{
	int ret;

	if (src == 0) {
		printf("lgAudWriteVag: invalid parameter (src)\n");
		return 0x80000004;
	}
	if (size == 0 || *size > data_003DCFAC) {
		printf("lgAudWriteVag: invalid parameter (size too big)\n");
		return 0x80000004;
	}
	WaitSema(threadsafeSema);
	devRpcData->handle = handle;
	devRpcData->u.buf.size = *size;
	memcpy(devRpcData->u.buf.data, src, *size);
	devRpcData->arg[2] = channel;
	if (sceSifCallRpc(&cdUA, 21, 0, devRpcData, (*size + 63) >> 4 << 4, devRpcData, 48, 0, 0) == 0) {
		ret = devRpcData->result;
		*size = devRpcData->u.buf.size;
		devicechanged &= ~2;
		devicechanged |= devRpcData->devicechanged;
		SignalSema(threadsafeSema);
		return ret;
	}
	*size = 0;
	printf("lgAudWriteVag: rpc call failed\n");
	SignalSema(threadsafeSema);
	return 0x80000000;
}

int lgAudGetMixer(int handle, lgAudMixer *mr)
{
	int ret;

	if (mr == 0) {
		printf("lgAudGetMixer: invalid parameter (mr)\n");
		return 0x80000004;
	}
	WaitSema(threadsafeSema);
	devRpcData->handle = handle;
	devRpcData->u.mixer = *mr;
	if (sceSifCallRpc(&cdUA, 11, 0, devRpcData, 48, devRpcData, 48, 0, 0) == 0) {
		ret = devRpcData->result;
		mr->value = devRpcData->u.mixer.value;
		devicechanged &= ~2;
		devicechanged |= devRpcData->devicechanged;
		SignalSema(threadsafeSema);
		return ret;
	}
	printf("lgAudGetMixer: rpc call failed\n");
	SignalSema(threadsafeSema);
	return 0x80000000;
}

int lgAudSetMixer(int handle, lgAudMixer *mr)
{
	int ret;

	if (mr == 0) {
		printf("lgAudSetMixer: invalid parameter (mr)\n");
		return 0x80000004;
	}
	WaitSema(threadsafeSema);
	devRpcData->handle = handle;
	devRpcData->u.mixer = *mr;
	if (sceSifCallRpc(&cdUA, 12, 0, devRpcData, 48, devRpcData, 48, 0, 0) == 0) {
		ret = devRpcData->result;
		devicechanged &= ~2;
		devicechanged |= devRpcData->devicechanged;
		SignalSema(threadsafeSema);
		return ret;
	}
	printf("lgAudGetMixer: rpc call failed\n");
	SignalSema(threadsafeSema);
	return 0x80000000;
}

int lgAudGetRemainingPlaybackBytes(int handle, int *bytes)
{
	int ret;

	WaitSema(threadsafeSema);
	devRpcData->handle = handle;
	if (sceSifCallRpc(&cdUA, 0x13, 0, devRpcData, 32, devRpcData, 48, 0, 0) == 0) {
		ret = devRpcData->result;
		devicechanged &= ~2;
		devicechanged |= devRpcData->devicechanged;
		*bytes = devRpcData->u.buf.size;
		SignalSema(threadsafeSema);
		return ret;
	}
	printf("lgAudGetRemainingPlaybackBytes: rpc call failed\n");
	SignalSema(threadsafeSema);
	return 0x80000000;
}

int lgAudGetAvailableRecordingBytes(int handle, int *bytes)
{
	int ret;

	WaitSema(threadsafeSema);
	devRpcData->handle = handle;
	if (sceSifCallRpc(&cdUA, 0x12, 0, devRpcData, 32, devRpcData, 48, 0, 0) == 0) {
		ret = devRpcData->result;
		devicechanged &= ~2;
		devicechanged |= devRpcData->devicechanged;
		*bytes = devRpcData->u.buf.size;
		SignalSema(threadsafeSema);
		return ret;
	}
	printf("lgAudGetAvailableRecordingBytes: rpc call failed\n");
	SignalSema(threadsafeSema);
	return 0x80000000;
}

int lgAudStartRecording(int handle)
{
	int ret;

	WaitSema(threadsafeSema);
	devRpcData->handle = handle;
	if (sceSifCallRpc(&cdUA, 4, 0, devRpcData, 32, devRpcData, 32, 0, 0) == 0) {
		ret = devRpcData->result;
		devicechanged &= ~2;
		devicechanged |= devRpcData->devicechanged;
		SignalSema(threadsafeSema);
		return ret;
	}
	printf("lgAudStartRecording: rpc call failed\n");
	SignalSema(threadsafeSema);
	return 0x80000000;
}

int lgAudStopRecording(int handle)
{
	int ret;

	WaitSema(threadsafeSema);
	devRpcData->handle = handle;
	if (sceSifCallRpc(&cdUA, 5, 0, devRpcData, 32, devRpcData, 32, 0, 0) == 0) {
		ret = devRpcData->result;
		devicechanged &= ~2;
		devicechanged |= devRpcData->devicechanged;
		SignalSema(threadsafeSema);
		return ret;
	}
	printf("lgAudStartRecording: rpc call failed\n");
	SignalSema(threadsafeSema);
	return 0x80000000;
}

int lgAudStartPlayback(int handle)
{
	int ret;

	WaitSema(threadsafeSema);
	devRpcData->handle = handle;
	if (sceSifCallRpc(&cdUA, 6, 0, devRpcData, 32, devRpcData, 32, 0, 0) == 0) {
		ret = devRpcData->result;
		devicechanged &= ~2;
		devicechanged |= devRpcData->devicechanged;
		SignalSema(threadsafeSema);
		return ret;
	}
	printf("lgAudStartPlayback: rpc call failed\n");
	SignalSema(threadsafeSema);
	return 0x80000000;
}

int lgAudStopPlayback(int handle)
{
	int ret;

	WaitSema(threadsafeSema);
	devRpcData->handle = handle;
	if (sceSifCallRpc(&cdUA, 7, 0, devRpcData, 32, devRpcData, 32, 0, 0) == 0) {
		ret = devRpcData->result;
		devicechanged &= ~2;
		devicechanged |= devRpcData->devicechanged;
		SignalSema(threadsafeSema);
		return ret;
	}
	printf("lgAudStopPlayback: rpc call failed\n");
	SignalSema(threadsafeSema);
	return 0x80000000;
}

int lgAudResumePlayback(int handle)
{
	int ret;

	WaitSema(threadsafeSema);
	devRpcData->handle = handle;
	if (sceSifCallRpc(&cdUA, 0x11, 0, devRpcData, 32, devRpcData, 32, 0, 0) == 0) {
		ret = devRpcData->result;
		devicechanged &= ~2;
		devicechanged |= devRpcData->devicechanged;
		SignalSema(threadsafeSema);
		return ret;
	}
	printf("lgAudResumePlayback: rpc call failed\n");
	SignalSema(threadsafeSema);
	return 0x80000000;
}

int lgAudSetPlaybackVolume(int handle, u_char left, u_char right)
{
	int ret;

	WaitSema(threadsafeSema);
	devRpcData->handle = handle;
	devRpcData->arg[0] = left;
	devRpcData->arg[1] = right;
	if (sceSifCallRpc(&cdUA, 0xD, 0, devRpcData, 32, devRpcData, 32, 0, 0) == 0) {
		ret = devRpcData->result;
		devicechanged &= ~2;
		devicechanged |= devRpcData->devicechanged;
		SignalSema(threadsafeSema);
		return ret;
	}
	printf("lgAudStopPlayback: rpc call failed\n");
	SignalSema(threadsafeSema);
	return 0x80000000;
}

int lgAudSetRecordingVolume(int handle, u_char left, u_char right)
{
	int ret;

	WaitSema(threadsafeSema);
	devRpcData->handle = handle;
	devRpcData->arg[0] = left;
	devRpcData->arg[1] = right;
	if (sceSifCallRpc(&cdUA, 0xE, 0, devRpcData, 32, devRpcData, 32, 0, 0) == 0) {
		ret = devRpcData->result;
		devicechanged &= ~2;
		devicechanged |= devRpcData->devicechanged;
		SignalSema(threadsafeSema);
		return ret;
	}
	printf("lgAudStopPlayback: rpc call failed\n");
	SignalSema(threadsafeSema);
	return 0x80000000;
}

int lgAudPrepareForReboot(void)
{
	int ret;

	if (lgAudASync(1) != 0) {
		printf("lgAudPrepareForReboot: error: an async call is in progress...\n");
		return 0x80000008;
	}
	WaitSema(threadsafeSema);
	devRpcData->handle = -1;
	if (sceSifCallRpc(&cdUA, 0x14, 0, devRpcData, 32, devRpcData, 32, 0, 0) == 0) {
		ret = devRpcData->result;
		devicechanged &= ~2;
		devicechanged |= devRpcData->devicechanged;
		SignalSema(threadsafeSema);
		return ret;
	}
	printf("lgAudPrepareForReboot: rpc call failed\n");
	SignalSema(threadsafeSema);
	return 0x80000000;
}

int lgAudASync(int mode)
{
	if (mode == 0) {
		while (sceSifCheckStatRpc(&cdUA) != 0)
			;
		return 0;
	}
	return sceSifCheckStatRpc(&cdUA);
}

static void fnReadEndFunc(void *arg)
{
	struct lgaud_async *a = arg;

	memcpy(a->dest, devRpcData->u.buf.data, devRpcData->u.buf.size);
	*a->size = devRpcData->u.buf.size;
	devicechanged &= ~2;
	devicechanged |= devRpcData->devicechanged;
	iSignalSema(threadsafeSema);
}

int lgAudARead(int handle, int channel, void *dest, int *size)
{
	if (dest == 0) {
		printf("lgAudARead: invalid parameter (dest)\n");
		return 0x80000004;
	}
	if (size == 0 || *size > data_003DCFAC) {
		printf("lgAudARead: invalid parameter (*size too big)\n");
		return 0x80000004;
	}
	if (lgAudASync(1) != 0) {
		printf("lgAudARead: error: another async call is in progress...\n");
		return 0x80000008;
	}
	WaitSema(threadsafeSema);
	devRpcData->handle = handle;
	devRpcData->u.buf.size = *size;
	devRpcData->arg[2] = channel;
	data_00414B30.dest = dest;
	data_00414B30.size = size;
	if (sceSifCallRpc(&cdUA, 8, 1, devRpcData, 48, devRpcData, (*size + 63) >> 4 << 4, fnReadEndFunc,
	                  &data_00414B30) != 0) {
		*size = 0;
		printf("lgAudARead: rpc call failed\n");
		return 0x80000000;
	}
	return 0;
}

static void fnWriteEndFunc(void *arg)
{
	struct lgaud_async *a = arg;

	*a->size = devRpcData->u.buf.size;
	devicechanged &= ~2;
	devicechanged |= devRpcData->devicechanged;
	iSignalSema(threadsafeSema);
}

int lgAudAWrite(int handle, int channel, void *src, int *size)
{
	if (src == 0) {
		printf("lgAudAWrite: invalid parameter (src)\n");
		return 0x80000004;
	}
	if (size == 0 || *size > data_003DCFAC) {
		printf("lgAudAWrite: invalid parameter (*size too big)\n");
		return 0x80000004;
	}
	if (lgAudASync(1) != 0) {
		printf("lgAudAWrite: error: another async call is in progress...\n");
		return 0x80000008;
	}
	WaitSema(threadsafeSema);
	devRpcData->handle = handle;
	devRpcData->u.buf.size = *size;
	devRpcData->arg[2] = channel;
	data_00414B30.dest = src;
	data_00414B30.size = size;
	memcpy(devRpcData->u.buf.data, src, devRpcData->u.buf.size);
	if (sceSifCallRpc(&cdUA, 9, 1, devRpcData, (*size + 63) >> 4 << 4, devRpcData, 48, fnWriteEndFunc,
	                  &data_00414B30) != 0) {
		*size = 0;
		printf("lgAudAWrite: rpc call failed\n");
		return 0x80000000;
	}
	return 0;
}

int lgAudAWriteVag(int handle, int channel, void *src, int *size)
{
	if (src == 0) {
		printf("lgAudAWriteVag: invalid parameter (src)\n");
		return 0x80000004;
	}
	if (size == 0 || *size > data_003DCFAC) {
		printf("lgAudAWriteVag: invalid parameter (*size too big)\n");
		return 0x80000004;
	}
	if (lgAudASync(1) != 0) {
		printf("lgAudAWriteVag: error: another async call is in progress...\n");
		return 0x80000008;
	}
	WaitSema(threadsafeSema);
	devRpcData->handle = handle;
	devRpcData->u.buf.size = *size;
	devRpcData->arg[2] = channel;
	data_00414B30.dest = src;
	data_00414B30.size = size;
	memcpy(devRpcData->u.buf.data, src, devRpcData->u.buf.size);
	if (sceSifCallRpc(&cdUA, 21, 1, devRpcData, (*size + 63) >> 4 << 4, devRpcData, 48, fnWriteEndFunc,
	                  &data_00414B30) != 0) {
		*size = 0;
		printf("lgAudAWriteVag: rpc call failed\n");
		return 0x80000000;
	}
	return 0;
}

static void fnCheckDeviceChangeEndFunc(void *arg)
{
	struct lgaud_async *a = arg;

	devicechanged &= ~2;
	devicechanged |= devRpcData->devicechanged;
	if ((*a->hint = devicechanged) == 1)
		devicechanged = 0;
	iSignalSema(threadsafeSema);
}

int lgAudAEnumHint(int *hint)
{
	if (hint == 0) {
		printf("lgAudEnumHint: invalid parameter (hint)\n");
		return 0x80000004;
	}
	if (lgAudASync(1) != 0) {
		printf("lgAudAEnumHint: error: another async call is in progress...\n");
		return 0x80000008;
	}
	WaitSema(threadsafeSema);
	data_00414B30.hint = hint;
	if (sceSifCallRpc(&cdUA, 15, 1, devRpcData, 32, devRpcData, 32, fnCheckDeviceChangeEndFunc,
	                  &data_00414B30) != 0) {
		*hint = 2;
		printf("lgAudAEnumHint: rpc call failed.\n");
		return 0x80000000;
	}
	return 0;
}
