/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* libnet: Sony's sample inet client library (OVL1 0x245AD8-0x247050), the EE side of the inet RPC
 * server over libmrpc (sceSifMCallRpc), prebuilt with ee-gcc 2.96 at -O2 -G0 (README "The GCC
 * front"). Every call marshals its arguments into the caller's net_buf -- words, then the data at a
 * word index -- and makes RPC `fno` through one helper that rounds both sizes to 64 and refuses a
 * buffer too small ("CallRpc() in Libnet send:%d, recv:%d, max:%d"); a failed call is -542
 * (sceINETE_RPC), a short buffer -543. A newer libnet than the demo's object 0x4223A0 (libmrpc,
 * the marshalling helpers, more calls), its functions in the same order: the names by that order
 * and the RPC numbers' (s1:order); the helpers and the calls the demo lacks keep placeholders. */

#include <stdarg.h>

typedef unsigned int u_int;

typedef struct {
	int pad[12];
	int bufsize;                    /* 0x30: net_buf's bytes */
} LIBNET_CD;                            /* sceSifMClientData and the buffer's size */

typedef struct {
	int data[4];
} sceInetAddress_t;

extern int scePrintf(const char *fmt, ...);
extern int sceSifMCallRpc(void *cd, int fno, int mode, void *send, int ssize, void *recv, int rsize,
                          void *end, void *end_arg);
extern void *memcpy(void *d, const void *s, unsigned int n);
extern unsigned int strlen(const char *s);
extern char *strcpy(char *d, const char *s);
int sceInetGetInterfaceList(LIBNET_CD *cd, u_int *buf, int *ids, int n);
int sceInetInterfaceControl(LIBNET_CD *cd, u_int *buf, int id, int code, void *ptr, int len);

int func_00245AD8(LIBNET_CD *cd, int fno, u_int *buf, int ssize, int rsize)
{
	int s = (ssize + 63) & ~63;
	int r = (rsize + 63) & ~63;

	if (cd->bufsize < s || cd->bufsize < r) {
		scePrintf("CallRpc() in Libnet send:%d, recv:%d, max:%d\n", s, r, cd->bufsize);
		return -543;
	}
	return sceSifMCallRpc(cd, fno, 0, buf, s, buf, r, 0, 0);
}

int func_00245B58(LIBNET_CD *cd, u_int *buf, int idx, void *src, int size)
{
	int room = cd->bufsize - idx * 4;

	if (src == 0)
		return 0;
	if (size < 0 || room < size)
		return -543;
	memcpy((char *)buf + idx * 4, src, size);
	return 0;
}

int func_00245BB0(LIBNET_CD *cd, void *dst, u_int *buf, int idx, int size)
{
	int room = cd->bufsize - idx * 4;

	if (dst == 0)
		return 0;
	if (size < 0 || room < size)
		return -543;
	memcpy(dst, (char *)buf + idx * 4, size);
	return 0;
}

int func_00245C08(LIBNET_CD *cd, u_int *buf, int idx, const char *str)
{
	int room = cd->bufsize - idx * 4;
	int len;

	if (str == 0)
		return 0;
	len = strlen(str) + 1;
	if (len < 0)
		return -543;
	if (room < len)
		return -543;
	strcpy((char *)buf + idx * 4, str);
	return len;
}

int func_00245C98(LIBNET_CD *cd, char *dst, u_int *buf, int idx)
{
	int room = cd->bufsize - idx * 4;      /* its siblings' check, unused here: the target's registers */

	if (dst == 0)
		return 0;
	strcpy(dst, (char *)buf + idx * 4);
	return 0;
}

int func_00245CD0(LIBNET_CD *cd, u_int *buf, void *data, int count, int arg)
{
	int size = count * 8;
	int ret;

	buf[1] = arg;
	buf[0] = count;
	ret = func_00245B58(cd, buf, 2, data, size);
	if (ret < 0)
		return ret;
	if (func_00245AD8(cd, 20, buf, size + 8, size + 4) < 0)
		return -542;
	ret = func_00245BB0(cd, data, buf, 2, size);
	if (ret < 0)
		return ret;
	return buf[0];
}

int sceInetName2Address(LIBNET_CD *cd, u_int *buf, int flags, sceInetAddress_t *paddr, const char *name,
                        int ms, int nretry, ...)
{
	va_list ap;
	int len;
	int ret;

	va_start(ap, nretry);
	buf[1] = ms;
	buf[2] = nretry;
	buf[3] = 0;
	buf[0] = flags;
	if (flags & 0x80)
		buf[3] = va_arg(ap, int);
	va_end(ap);
	len = func_00245C08(cd, buf, 5, name);
	if (len < 0)
		return len;
	buf[4] = len;
	if (func_00245AD8(cd, 6, buf, len + 20, 20) < 0)
		return -542;
	ret = func_00245BB0(cd, paddr, buf, 1, 16);
	if (ret < 0)
		return ret;
	return buf[0];
}

int sceInetAddress2String(LIBNET_CD *cd, u_int *buf, char *name, int len, sceInetAddress_t *paddr)
{
	int ret;

	buf[0] = len;
	ret = func_00245B58(cd, buf, 1, paddr, 16);
	if (ret < 0)
		return ret;
	if (func_00245AD8(cd, 7, buf, 20, len + 20) < 0)
		return -542;
	ret = func_00245C98(cd, name, buf, 5);
	if (ret < 0)
		return ret;
	return buf[0];
}

int sceInetAddress2Name(LIBNET_CD *cd, u_int *buf, int flags, char *name, int len, sceInetAddress_t *paddr,
                        int ms, int nretry, ...)
{
	va_list ap;
	int ret;

	va_start(ap, nretry);
	buf[2] = ms;
	buf[3] = nretry;
	buf[8] = 0;
	buf[0] = flags;
	buf[1] = len;
	if (flags & 0x80)
		buf[8] = va_arg(ap, int);
	va_end(ap);
	ret = func_00245B58(cd, buf, 4, paddr, 16);
	if (ret < 0)
		return ret;
	if (func_00245AD8(cd, 18, buf, 36, len + 32) < 0)
		return -542;
	ret = func_00245C98(cd, name, buf, 8);
	if (ret < 0)
		return ret;
	return buf[0];
}

int sceInetCreate(LIBNET_CD *cd, u_int *buf, void *param)
{
	int ret;

	ret = func_00245B58(cd, buf, 1, param, 64);
	if (ret < 0)
		return ret;
	if (func_00245AD8(cd, 1, buf, 68, 4) < 0)
		return -542;
	return buf[0] ? buf[0] : -540;
}

int sceInetOpen(LIBNET_CD *cd, u_int *buf, int s, int ms)
{
	buf[0] = s;
	buf[1] = ms;
	if (func_00245AD8(cd, 2, buf, 8, 4) < 0)
		return -542;
	return buf[0];
}

int sceInetClose(LIBNET_CD *cd, u_int *buf, int s, int ms)
{
	buf[0] = s;
	buf[1] = ms;
	if (func_00245AD8(cd, 3, buf, 8, 4) < 0)
		return -542;
	return buf[0];
}

int sceInetAbort(LIBNET_CD *cd, u_int *buf, int s, int flags)
{
	buf[0] = s;
	buf[1] = flags;
	if (func_00245AD8(cd, 15, buf, 8, 4) < 0)
		return -542;
	return buf[0];
}

int sceInetChangeThreadPriority(LIBNET_CD *cd, u_int *buf, int prio)
{
	buf[0] = prio;
	if (func_00245AD8(cd, 12, buf, 4, 4) < 0)
		return -542;
	return buf[0];
}

int sceInetAbortLog(LIBNET_CD *cd, u_int *buf)
{
	if (func_00245AD8(cd, 16, buf, 0, 4) < 0)
		return -542;
	return buf[0];
}

int func_002468E8(LIBNET_CD *cd, u_int *buf, int arg)
{
	buf[0] = arg;
	if (func_00245AD8(cd, 50, buf, 4, 4) < 0)
		return -542;
	return buf[0];
}

int func_00246930(LIBNET_CD *cd, u_int *buf, int arg)
{
	buf[0] = arg;
	if (func_00245AD8(cd, 51, buf, 4, 4) < 0)
		return -542;
	return buf[0];
}

int func_00246978(LIBNET_CD *cd, u_int *buf, int arg)
{
	buf[0] = arg;
	if (func_00245AD8(cd, 52, buf, 4, 4) < 0)
		return -542;
	return buf[0];
}

int func_00246B90(LIBNET_CD *cd, u_int *buf)
{
	if (func_00245AD8(cd, 30, buf, 0, 4) < 0)
		return -542;
	return buf[0];
}

int func_00246BD0(LIBNET_CD *cd, u_int *buf)
{
	if (func_00245AD8(cd, 31, buf, 0, 4) < 0)
		return -542;
	return buf[0];
}

int func_00246C10(LIBNET_CD *cd, u_int *buf, int arg)
{
	buf[0] = arg;
	if (func_00245AD8(cd, 32, buf, 4, 4) < 0)
		return -542;
	return buf[0];
}

int sceInetGetLog(LIBNET_CD *cd, u_int *buf, void *log, int len, int ms)
{
	int ret;

	buf[1] = ms;
	buf[0] = len;
	if (func_00245AD8(cd, 17, buf, 8, len + 4) < 0)
		return -542;
	ret = func_00245BB0(cd, log, buf, 1, len);
	if (ret < 0)
		return ret;
	return buf[0];
}

int func_002469C0(LIBNET_CD *cd, u_int *buf, int arg, int *inout)
{
	buf[0] = arg;
	if (inout != 0)
		buf[1] = *inout;
	else
		buf[1] = 0;
	if (func_00245AD8(cd, 53, buf, 8, 8) < 0)
		return -542;
	if (inout != 0)
		*inout = buf[1];
	return buf[0];
}

int func_00246C58(LIBNET_CD *cd, u_int *buf, int *out, int n, int fno)
{
	int i;

	if (func_00245AD8(cd, fno, buf, 0, n * 4 + 4) < 0)
		return -542;
	for (i = 0; i < n; i++)
		out[i] = buf[i + 1];
	return buf[0];
}

int func_00246CE0(LIBNET_CD *cd, u_int *buf, int *out, int n)
{
	return func_00246C58(cd, buf, out, n, 33);
}

int func_00246CF8(LIBNET_CD *cd, u_int *buf, int *out, int n)
{
	return func_00246C58(cd, buf, out, n, 34);
}

int func_00246D10(LIBNET_CD *cd, u_int *buf, int *id)
{
	int ids[256];
	int flags;
	int n;
	int i;

	n = sceInetGetInterfaceList(cd, buf, ids, 256);
	if (n < 0)
		return -1;
	if (n == 0)
		return -2;
	for (i = 0; i < n; i++) {
		if (sceInetInterfaceControl(cd, buf, ids[i], 8, &flags, 4) < 0)
			return -1;
		if (flags & 2)
			break;
	}
	if (i == n)
		return -2;
	*id = ids[i];
	return 0;
}

int func_00246DF0(LIBNET_CD *cd, u_int *buf, int id, void *ptr)
{
	return sceInetInterfaceControl(cd, buf, id, 9, ptr, 16);
}

int func_00246E10(LIBNET_CD *cd, u_int *buf, int *id, int n, void *ptr)
{
	int ret;

	if (func_00246D10(cd, buf, id) < 0) {
		ret = func_00246CF8(cd, buf, id, n);
		if (ret < 0)
			return ret;
	}
	ret = func_00246DF0(cd, buf, *id, ptr);
	if (ret < 0)
		return ret;
	return 0;
}

int func_00246EA8(LIBNET_CD *cd, u_int *buf, int *id, int n, void *ptr)
{
	int ret;

	ret = func_00246CF8(cd, buf, id, n);
	if (ret < 0)
		return ret;
	ret = func_00246DF0(cd, buf, *id, ptr);
	if (ret < 0)
		return ret;
	return 0;
}

int func_00246F10(LIBNET_CD *cd, u_int *buf, int *id, int n)
{
	int ret;

	if (id == 0)
		return -540;
	if (func_00246D10(cd, buf, id) < 0) {
		ret = func_00246CE0(cd, buf, id, n);
		if (ret < 0)
			return ret;
	}
	return 0;
}

int func_00246F90(LIBNET_CD *cd, u_int *buf, int *a, int *b)
{
	if (func_00245AD8(cd, 35, buf, 0, 12) < 0)
		return -542;
	if (a != 0)
		*a = buf[1];
	if (b != 0)
		*b = buf[2];
	return buf[0];
}

int func_00247008(LIBNET_CD *cd, u_int *buf, int *id, int n, void *ptr, int flags)
{
	int ret;

	if (flags & 2)
		ret = func_00246E10(cd, buf, id, n, ptr);
	else
		ret = func_00246EA8(cd, buf, id, n, ptr);
	return ret;
}
