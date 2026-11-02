/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* lgAud, the USB headset's audio (liblgaud.a 1.08.002, "lgAudInit version 1.08.002"), OVL1
 * 0x243840-0x245690: the demo's object 0x3B5980 (C, 25 functions; SOCOM II's has four more), prebuilt
 * with ee-gcc 2.96 at -O2 -G0 (README "The GCC front"; 2.96's sibling calls keep the frame they no
 * longer need: lgAudASync). Every call is a SIF RPC to lgaud.irx through one 0x30-byte request block
 * (devRpcData) under a semaphore; each reply carries the call's result and the device-change flag,
 * which is folded into devicechanged. The failure messages are the library's own, copied between
 * calls with their names unchanged (StopRecording prints StartRecording's). Names are SOCOM 1's
 * (s1:exact/callee) or by the demo's order; the request's fields are named by their use here. Not yet
 * written: lgAudInit, Enumerate, Open, Read, Write, WriteVag, GetMixer, SetMixer, ARead, AWrite,
 * AWriteVag, AEnumHint (the generated tier holds what matched). */

typedef unsigned char u_char;

struct lgaud_rpc {
	int result;                     /* 0x00 */
	int devicechanged;              /* 0x04: the reply's device-change bits */
	int pad08;
	int handle;                     /* 0x0C */
	u_char arg[16];                 /* 0x10 */
	int size;                       /* 0x20: the bytes a read returns */
	int pad24[3];
	u_char data[1];                 /* 0x30 */
};

struct lgaud_async {
	void *dest;
	int *size;
	int *hint;
};

extern int WaitSema(int sema);
extern int SignalSema(int sema);
extern int iSignalSema(int sema);
extern int printf(const char *fmt, ...);
extern void *memcpy(void *d, const void *s, unsigned int n);
extern int sceSifCallRpc(void *cd, int fno, int mode, void *send, int ssize, void *recv, int rsize,
                         void (*end)(void *), void *end_arg);
extern int sceSifCheckStatRpc(void *cd);

extern struct lgaud_rpc *devRpcData;
extern int devicechanged;
extern int threadsafeSema;
extern char cdUA[];                             /* lgAud's sceSifClientData */

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

int lgAudClose(int handle)
{
	int ret;

	WaitSema(threadsafeSema);
	devRpcData->handle = handle;
	if (sceSifCallRpc(cdUA, 3, 0, devRpcData, 32, devRpcData, 32, 0, 0) == 0) {
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

int lgAudGetRemainingPlaybackBytes(int handle, int *bytes)
{
	int ret;

	WaitSema(threadsafeSema);
	devRpcData->handle = handle;
	if (sceSifCallRpc(cdUA, 0x13, 0, devRpcData, 32, devRpcData, 48, 0, 0) == 0) {
		ret = devRpcData->result;
		devicechanged &= ~2;
		devicechanged |= devRpcData->devicechanged;
		*bytes = devRpcData->size;
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
	if (sceSifCallRpc(cdUA, 0x12, 0, devRpcData, 32, devRpcData, 48, 0, 0) == 0) {
		ret = devRpcData->result;
		devicechanged &= ~2;
		devicechanged |= devRpcData->devicechanged;
		*bytes = devRpcData->size;
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
	if (sceSifCallRpc(cdUA, 4, 0, devRpcData, 32, devRpcData, 32, 0, 0) == 0) {
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
	if (sceSifCallRpc(cdUA, 5, 0, devRpcData, 32, devRpcData, 32, 0, 0) == 0) {
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
	if (sceSifCallRpc(cdUA, 6, 0, devRpcData, 32, devRpcData, 32, 0, 0) == 0) {
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
	if (sceSifCallRpc(cdUA, 7, 0, devRpcData, 32, devRpcData, 32, 0, 0) == 0) {
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
	if (sceSifCallRpc(cdUA, 0x11, 0, devRpcData, 32, devRpcData, 32, 0, 0) == 0) {
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
	if (sceSifCallRpc(cdUA, 0xD, 0, devRpcData, 32, devRpcData, 32, 0, 0) == 0) {
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
	if (sceSifCallRpc(cdUA, 0xE, 0, devRpcData, 32, devRpcData, 32, 0, 0) == 0) {
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

int lgAudASync(int mode)
{
	if (mode == 0) {
		while (sceSifCheckStatRpc(cdUA) != 0)
			;
		return 0;
	}
	return sceSifCheckStatRpc(cdUA);
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
	if (sceSifCallRpc(cdUA, 0x14, 0, devRpcData, 32, devRpcData, 32, 0, 0) == 0) {
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

static void fnReadEndFunc(void *arg)
{
	struct lgaud_async *a = arg;

	memcpy(a->dest, devRpcData->data, devRpcData->size);
	*a->size = devRpcData->size;
	devicechanged &= ~2;
		devicechanged |= devRpcData->devicechanged;
	iSignalSema(threadsafeSema);
}

static void fnWriteEndFunc(void *arg)
{
	struct lgaud_async *a = arg;

	*a->size = devRpcData->size;
	devicechanged &= ~2;
		devicechanged |= devRpcData->devicechanged;
	iSignalSema(threadsafeSema);
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
