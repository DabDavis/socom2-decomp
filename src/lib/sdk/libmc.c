/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libmc: the memory card client (MAIN 0x1B3970-0x1B5160): Sony's SDK 2.7, the demo's object
 * 0x171070 (C), 28 functions in the object's order -- a later release than the demo's (most
 * functions 4-16 bytes apart, a second semaphore for the delay alarm). Every call checks the
 * binding (-100) and takes the library's semaphore without waiting (-200), fills one of two
 * command blocks and makes a non-blocking RPC to mcserv (0x80000400); sceMcSync collects the
 * result and gives the semaphore back. The statics carry the demo's names (its symbol table keeps
 * the object's LOCAL labels at the same offsets: mcClientID, typeAddr/freeAddr/formAddr,
 * buffFileInfo, sifParamOrd, sifParamFname, sifParamNext, currentDir, retval in .bss;
 * mcRunCmdNo and semaidRegFunc in .data). The command blocks are the layouts ps2sdk's libmc
 * reconstructed (a descriptor of 48 bytes, a name block of 1044); the RPC numbers name the eight
 * functions the demo's order gives (s1:order): 0xC chdir, 0x10 format, 0xF delete, 0xA flush,
 * 0xE set-info (and rename, as a set-info of the name with command 0x13), 0x11 unformat, 0x12
 * the entry space.
 *
 * Shapes the target fixes: sceMcGetInfo's three flags are if/else stores; sceMcSetFileInfo masks
 * its argument in place (`valid &= 7`) and copies the 64-byte entry with a constant-size memcpy
 * from the caller's bytes (GCC's unaligned block move both sides), the table pointer stored after.
 *
 * OPEN: sceMcWrite (12 words, all in the head split): the target computes the unaligned head as
 *   ((buff - 1) & 0xFFFFFFF0) - (buff - 16), keeping buff - 16 as its own term (addiu; subu), where
 *   every spelling tried here (int, u_int, char * arithmetic, the head in a local, the three fields
 *   spelt out in ps2sdk's order) has combine reassociate it into X + 16 - buff, and the base
 *   pointer then takes a1 for a2. */

typedef unsigned int u_int;
typedef unsigned short u_short;
typedef unsigned char u_char;

typedef struct _sif_rpc_data {
	void *paddr;
	u_int pid;
	int tid;
	u_int mode;
} sceSifRpcData;

typedef void (*sceSifEndFunc)(void *);

typedef struct _sif_client_data {
	struct _sif_rpc_data rpcd;
	u_int command;
	void *buff;
	void *cbuff;
	sceSifEndFunc func;
	void *para;
	struct _sif_serve_data *serve;
} sceSifClientData;

struct SemaParam {
	int currentCount;
	int maxCount;
	int initCount;
	int numWaitThreads;
	u_int attr;
	u_int option;
};

typedef struct {
	u_char Sec, Min, Hour, Day, Month;
	u_short Year;
} sceMcStDateTime;

typedef struct {
	sceMcStDateTime _Create;
	sceMcStDateTime _Modify;
	u_int FileSizeByte;
	u_short AttrFile;
	u_short Reserve1;
	u_int Reserve2;
	u_int PdaAplNo;
	u_char EntryName[32];
} sceMcTblGetDir;

typedef struct {			/* the descriptor block: 48 bytes */
	int fd;
	int port;
	int slot;
	int size;
	int offset;
	int origin;
	void *buffer;
	void *param;
	u_char data[16];
} McDescParam;

typedef struct {			/* the name block: 1044 bytes */
	int port;
	int slot;
	int flags;
	int maxent;
	sceMcTblGetDir *table;
	char name[1024];
} McNameParam;

typedef struct {			/* the read's unaligned head and tail, written back by mcserv */
	int size1;
	int size2;
	u_char *dest1;
	u_char *dest2;
	u_char src1[64];
	u_char src2[64];
} McRfa;

extern int CreateSema(struct SemaParam *);
extern int WaitSema(int);
extern int PollSema(int);
extern int SignalSema(int);
extern int iSignalSema(int);
extern int SetAlarm(u_short, void (*)(int, u_short, void *), void *);
extern void FlushCache(int);
extern void sceSifInitRpc(u_int);
extern int sceSifBindRpc(sceSifClientData *, u_int, u_int);
extern int sceSifCallRpc(sceSifClientData *, u_int, u_int, void *, int, void *, int,
			 sceSifEndFunc, void *);
extern int sceSifCheckStatRpc(sceSifRpcData *);
extern void sceSifWriteBackDCache(void *, int);
extern int scePrintf(const char *, ...);
extern char *strncpy(char *, const char *, unsigned int);
extern unsigned int strlen(const char *);
extern void *memcpy(void *, const void *, unsigned int);

#define	UNCACHED(p)	((u_int)(p) | 0x20000000)

static int mcRunCmdNo = 0;
static int semaidRegFunc = -1;
static int semaidDelay = -1;

static sceSifClientData mcClientID __attribute__((aligned(64)));
static int *typeAddr;
static int *freeAddr;
static int *formAddr;
static sceMcTblGetDir buffFileInfo __attribute__((aligned(64)));
static McDescParam sifParamOrd __attribute__((aligned(64)));
static McNameParam sifParamFname;
static u_char sifParamNext[0xC0] __attribute__((aligned(64)));
static char currentDir[0x1000] __attribute__((aligned(64)));
static int retval[3] __attribute__((aligned(64)));

int sceMcInit(void)
{
	struct SemaParam sp;
	int ret;
	int i;

	if (semaidRegFunc < 0) {
		sp.option = 0;
		sp.initCount = 1;
		sp.maxCount = 1;
		semaidRegFunc = CreateSema(&sp);
		sp.maxCount = 1;
		sp.initCount = 0;
		sp.option = 0;
		semaidDelay = CreateSema(&sp);
	}
	sceMcSync(0, 0, 0);
	WaitSema(semaidRegFunc);
	sceSifInitRpc(0);
	while (1) {
		if (sceSifBindRpc(&mcClientID, 0x80000400, 0) < 0) {
			scePrintf("bind error libmc \n");
			while (1)
				;
		}
		if (mcClientID.serve != 0)
			break;
		i = 0x100000;
		while (--i)
			;
	}
	ret = sceSifCallRpc(&mcClientID, 254, 0, &sifParamOrd, 48, retval, 12, 0, 0);
	SignalSema(semaidRegFunc);
	if (ret < 0) {
		mcClientID.serve = 0;
		return ret - 100;
	}
	if (retval[1] < 0x20A) {
		scePrintf("libmc: too old release of mcserv.irx\n");
		mcClientID.serve = 0;
		return -120;
	}
	if (retval[2] < 0x20E) {
		scePrintf("libmc: too old release of mcman.irx\n");
		mcClientID.serve = 0;
		return -121;
	}
	return retval[0];
}

int sceMcEnd(void)
{
	if (semaidRegFunc >= 0) {
		DeleteSema(semaidRegFunc);
		semaidRegFunc = -1;
		DeleteSema(semaidDelay);
	}
	return 1;
}

sceSifClientData *_lmcGetClientPtr(int **ret, int **cmd)
{
	*ret = retval;
	*cmd = &mcRunCmdNo;
	retval[15] = semaidRegFunc;
	return &mcClientID;
}

int sceMcChangeThreadPriority(int level)
{
	int ret;

	if (mcClientID.serve == 0)
		return -100;
	if (PollSema(semaidRegFunc) < 0)
		return -200;
	sifParamOrd.origin = level;
	if ((ret = sceSifCallRpc(&mcClientID, 20, 1, &sifParamOrd, 48, retval, 4, 0, 0)) == 0)
		mcRunCmdNo = 20;
	else
		SignalSema(semaidRegFunc);
	return ret;
}

int sceMcGetSlotMax(int port)
{
	int ret;

	if (mcClientID.serve == 0)
		return -100;
	if (PollSema(semaidRegFunc) < 0)
		return -200;
	sifParamOrd.port = port;
	if ((ret = sceSifCallRpc(&mcClientID, 21, 0, &sifParamOrd, 48, retval, 4, 0, 0)) != 0) {
		SignalSema(semaidRegFunc);
		return ret;
	}
	SignalSema(semaidRegFunc);
	return retval[0];
}

int sceMcOpen(int port, int slot, const char *name, int mode)
{
	int ret;

	if (mcClientID.serve == 0)
		return -100;
	if (PollSema(semaidRegFunc) < 0)
		return -200;
	if (name == 0 || *name == 0) {
		SignalSema(semaidRegFunc);
		return -210;
	}
	strncpy(sifParamFname.name, name, 1023);
	sifParamFname.port = port;
	sifParamFname.flags = mode;
	sifParamFname.slot = slot;
	sifParamFname.name[1023] = 0;
	if ((ret = sceSifCallRpc(&mcClientID, 2, 1, &sifParamFname, 1044, retval, 4, 0, 0)) == 0)
		mcRunCmdNo = 2;
	else
		SignalSema(semaidRegFunc);
	return ret;
}

int sceMcMkdir(int port, int slot, const char *name)
{
	int ret;

	if ((ret = sceMcOpen(port, slot, name, 0x40)) == 0)
		mcRunCmdNo = 11;
	return ret;
}

int sceMcClose(int fd)
{
	int ret;

	if (mcClientID.serve == 0)
		return -100;
	if (PollSema(semaidRegFunc) < 0)
		return -200;
	sifParamOrd.fd = fd;
	if ((ret = sceSifCallRpc(&mcClientID, 3, 1, &sifParamOrd, 48, retval, 4, 0, 0)) == 0)
		mcRunCmdNo = 3;
	else
		SignalSema(semaidRegFunc);
	return ret;
}

int sceMcSeek(int fd, int offset, int origin)
{
	int ret;

	if (mcClientID.serve == 0)
		return -100;
	if (PollSema(semaidRegFunc) < 0)
		return -200;
	sifParamOrd.fd = fd;
	sifParamOrd.offset = offset;
	sifParamOrd.origin = origin;
	if ((ret = sceSifCallRpc(&mcClientID, 4, 1, &sifParamOrd, 48, retval, 4, 0, 0)) == 0)
		mcRunCmdNo = 4;
	else
		SignalSema(semaidRegFunc);
	return ret;
}

static void mceIntrReadFixAlign(void *data)
{
	McRfa *p = (McRfa *)UNCACHED(data);
	u_char *dst;
	int i;

	if (p->size1 != 0) {
		dst = p->dest1;
		for (i = 0; i < p->size1; i++)
			*dst++ = p->src1[i];
	}
	if (p->size2 != 0) {
		dst = p->dest2;
		for (i = 0; i < p->size2; i++)
			*dst++ = p->src2[i];
	}
}

int sceMcRead(int fd, void *buff, int size)
{
	int ret;

	if (mcClientID.serve == 0)
		return -100;
	if (PollSema(semaidRegFunc) < 0)
		return -200;
	sifParamOrd.fd = fd;
	sifParamOrd.param = sifParamNext;
	sifParamOrd.buffer = buff;
	sifParamOrd.size = size;
	sceSifWriteBackDCache(buff, size);
	sceSifWriteBackDCache(sifParamNext, 0xC0);
	if ((ret = sceSifCallRpc(&mcClientID, 5, 1, &sifParamOrd, 48, retval, 4,
				 mceIntrReadFixAlign, sifParamNext)) == 0)
		mcRunCmdNo = 5;
	else
		SignalSema(semaidRegFunc);
	return ret;
}

int sceMcWrite(int fd, const void *buff, int size)
{
	int ret;
	u_int i;

	if (mcClientID.serve == 0)
		return -100;
	if (PollSema(semaidRegFunc) < 0)
		return -200;
	sifParamOrd.fd = fd;
	if (size < 17) {
		sifParamOrd.origin = size;
		sifParamOrd.buffer = 0;
		sifParamOrd.size = 0;
	} else {
		sifParamOrd.origin = (((u_int)buff - 1) & 0xFFFFFFF0) - ((u_int)buff - 16);
		sifParamOrd.buffer = (char *)buff + sifParamOrd.origin;
		sifParamOrd.size = size - sifParamOrd.origin;
	}
	for (i = 0; i < sifParamOrd.origin; i++)
		sifParamOrd.data[i] = ((const u_char *)buff)[i];
	FlushCache(0);
	if ((ret = sceSifCallRpc(&mcClientID, 6, 1, &sifParamOrd, 48, retval, 4, 0, 0)) == 0)
		mcRunCmdNo = 6;
	else
		SignalSema(semaidRegFunc);
	return ret;
}

static void mcHearAlarm(int id, u_short time, void *sema)
{
	iSignalSema((int)sema);
}

static void mcDelayThread(int time)
{
	SetAlarm(time, mcHearAlarm, (void *)semaidDelay);
	WaitSema(semaidDelay);
}

int sceMcSync(int mode, int *cmd, int *result)
{
	int ret;

	if (mcRunCmdNo == 0)
		return -1;
	ret = sceSifCheckStatRpc(&mcClientID.rpcd);
	if (mode == 0 && ret != 0) {
		while (sceSifCheckStatRpc(&mcClientID.rpcd) != 0)
			mcDelayThread(60);
		ret = 0;
	}
	ret = ret == 0;
	if (cmd != 0)
		*cmd = mcRunCmdNo;
	if (ret) {
		mcRunCmdNo = 0;
		if (result != 0)
			*result = retval[0];
		SignalSema(semaidRegFunc);
	}
	return ret;
}

static void mceGetInfoApdx(void *data)
{
	int *p = (int *)UNCACHED(data);

	if (typeAddr != 0)
		*typeAddr = p[0];
	if (freeAddr != 0)
		*freeAddr = p[1];
	if (formAddr != 0)
		*formAddr = p[36];
}

int sceMcGetInfo(int port, int slot, int *type, int *free, int *format)
{
	int ret;

	if (mcClientID.serve == 0)
		return -100;
	if (PollSema(semaidRegFunc) < 0)
		return -200;
	sifParamOrd.port = port;
	sifParamOrd.slot = slot;
	sifParamOrd.param = sifParamNext;
	if (type != 0)
		sifParamOrd.origin = 1;
	else
		sifParamOrd.origin = 0;
	if (free != 0)
		sifParamOrd.offset = 1;
	else
		sifParamOrd.offset = 0;
	if (format != 0)
		sifParamOrd.size = 1;
	else
		sifParamOrd.size = 0;
	typeAddr = type;
	freeAddr = free;
	formAddr = format;
	sceSifWriteBackDCache(sifParamNext, 0xC0);
	if ((ret = sceSifCallRpc(&mcClientID, 1, 1, &sifParamOrd, 48, retval, 4,
				 mceGetInfoApdx, sifParamNext)) == 0)
		mcRunCmdNo = 1;
	else
		SignalSema(semaidRegFunc);
	return ret;
}

int sceMcUdCheckNewCard(void)
{
	if (sceSifCallRpc(&mcClientID, 0x35, 0, &sifParamOrd, 48, retval, 4, 0, 0) != 0) {
		scePrintf("sceMcUdCheckNewCard RPC faild\n");
		return -1;
	}
	return retval[0];
}

int sceMcGetDir(int port, int slot, const char *name, u_int mode, int maxent, sceMcTblGetDir *table)
{
	int ret;

	if (mcClientID.serve == 0)
		return -100;
	if (PollSema(semaidRegFunc) < 0)
		return -200;
	if (name == 0 || *name == 0) {
		SignalSema(semaidRegFunc);
		return -210;
	}
	sifParamFname.port = port;
	sifParamFname.slot = slot;
	sifParamFname.flags = mode;
	sifParamFname.maxent = maxent;
	sifParamFname.table = table;
	strncpy(sifParamFname.name, name, 1023);
	sifParamFname.name[1023] = 0;
	if (maxent >= 0)
		sceSifWriteBackDCache(table, maxent << 6);
	if ((ret = sceSifCallRpc(&mcClientID, 13, 1, &sifParamFname, 1044, retval, 4, 0, 0)) == 0)
		mcRunCmdNo = 13;
	else
		SignalSema(semaidRegFunc);
	return ret;
}

static void mceStorePwd(void *pwd)
{
	int len;

	if (pwd != 0) {
		len = strlen((char *)UNCACHED(currentDir)) > 1023 ? 1023
			: strlen((char *)UNCACHED(currentDir));
		memcpy(pwd, (char *)UNCACHED(currentDir), len);
		((char *)pwd)[len] = 0;
	}
}

int sceMcChdir(int port, int slot, const char *newdir, char *olddir)
{
	int ret;

	if (mcClientID.serve == 0)
		return -100;
	if (PollSema(semaidRegFunc) < 0)
		return -200;
	if (newdir == 0 || *newdir == 0) {
		SignalSema(semaidRegFunc);
		return -210;
	}
	sifParamFname.port = port;
	sifParamFname.table = (sceMcTblGetDir *)currentDir;
	sifParamFname.slot = slot;
	strncpy(sifParamFname.name, newdir, 1023);
	sifParamFname.name[1023] = 0;
	sceSifWriteBackDCache(currentDir, 0x400);
	if ((ret = sceSifCallRpc(&mcClientID, 12, 1, &sifParamFname, 1044, retval, 4,
				 mceStorePwd, olddir)) == 0)
		mcRunCmdNo = 12;
	else
		SignalSema(semaidRegFunc);
	return ret;
}

int sceMcFormat(int port, int slot)
{
	int ret;

	if (mcClientID.serve == 0)
		return -100;
	if (PollSema(semaidRegFunc) < 0)
		return -200;
	sifParamOrd.port = port;
	sifParamOrd.slot = slot;
	if ((ret = sceSifCallRpc(&mcClientID, 16, 1, &sifParamOrd, 48, retval, 4, 0, 0)) == 0)
		mcRunCmdNo = 16;
	else
		SignalSema(semaidRegFunc);
	return ret;
}

int sceMcDelete(int port, int slot, const char *name)
{
	int ret;

	if (mcClientID.serve == 0)
		return -100;
	if (PollSema(semaidRegFunc) < 0)
		return -200;
	if (name == 0 || *name == 0) {
		SignalSema(semaidRegFunc);
		return -210;
	}
	strncpy(sifParamFname.name, name, 1023);
	sifParamFname.port = port;
	sifParamFname.slot = slot;
	sifParamFname.name[1023] = 0;
	sifParamFname.flags = 0;
	if ((ret = sceSifCallRpc(&mcClientID, 15, 1, &sifParamFname, 1044, retval, 4, 0, 0)) == 0)
		mcRunCmdNo = 15;
	else
		SignalSema(semaidRegFunc);
	return ret;
}

int sceMcFlush(int fd)
{
	int ret;

	if (mcClientID.serve == 0)
		return -100;
	if (PollSema(semaidRegFunc) < 0)
		return -200;
	sifParamOrd.fd = fd;
	if ((ret = sceSifCallRpc(&mcClientID, 10, 1, &sifParamOrd, 48, retval, 4, 0, 0)) == 0)
		mcRunCmdNo = 10;
	else
		SignalSema(semaidRegFunc);
	return ret;
}

int sceMcSetFileInfo(int port, int slot, const char *name, const char *info, u_int valid)
{
	int ret;

	if (mcClientID.serve == 0)
		return -100;
	if (PollSema(semaidRegFunc) < 0)
		return -200;
	if (name == 0 || *name == 0) {
		SignalSema(semaidRegFunc);
		return -210;
	}
	valid &= 7;
	sifParamFname.port = port;
	sifParamFname.slot = slot;
	sifParamFname.flags = valid;
	memcpy(&buffFileInfo, info, sizeof(sceMcTblGetDir));
	sifParamFname.table = &buffFileInfo;
	strncpy(sifParamFname.name, name, 1023);
	sifParamFname.name[1023] = 0;
	FlushCache(0);
	if ((ret = sceSifCallRpc(&mcClientID, 14, 1, &sifParamFname, 1044, retval, 4, 0, 0)) == 0)
		mcRunCmdNo = 14;
	else
		SignalSema(semaidRegFunc);
	return ret;
}

int sceMcRename(int port, int slot, const char *oldname, const char *newname)
{
	int ret;

	if (mcClientID.serve == 0)
		return -100;
	if (PollSema(semaidRegFunc) < 0)
		return -200;
	if (oldname == 0 || *oldname == 0 || newname == 0) {
		SignalSema(semaidRegFunc);
		return -210;
	}
	sifParamFname.port = port;
	sifParamFname.slot = slot;
	sifParamFname.flags = 0x10;
	strncpy(sifParamFname.name, oldname, 1023);
	sifParamFname.name[1023] = 0;
	strncpy(buffFileInfo.EntryName, newname, 32);
	buffFileInfo.EntryName[31] = 0;
	sifParamFname.table = &buffFileInfo;
	FlushCache(0);
	if ((ret = sceSifCallRpc(&mcClientID, 14, 1, &sifParamFname, 1044, retval, 4, 0, 0)) == 0)
		mcRunCmdNo = 19;
	else
		SignalSema(semaidRegFunc);
	return ret;
}

int sceMcUnformat(int port, int slot)
{
	int ret;

	if (mcClientID.serve == 0)
		return -100;
	if (PollSema(semaidRegFunc) < 0)
		return -200;
	sifParamOrd.port = port;
	sifParamOrd.slot = slot;
	if ((ret = sceSifCallRpc(&mcClientID, 17, 1, &sifParamOrd, 48, retval, 4, 0, 0)) == 0)
		mcRunCmdNo = 17;
	else
		SignalSema(semaidRegFunc);
	return ret;
}

int sceMcGetEntSpace(int port, int slot, const char *path)
{
	int ret;

	if (mcClientID.serve == 0)
		return -100;
	if (PollSema(semaidRegFunc) < 0)
		return -200;
	if (path == 0 || *path == 0) {
		SignalSema(semaidRegFunc);
		return -210;
	}
	sifParamFname.port = port;
	sifParamFname.slot = slot;
	strncpy(sifParamFname.name, path, 1023);
	sifParamFname.name[1023] = 0;
	if ((ret = sceSifCallRpc(&mcClientID, 18, 1, &sifParamFname, 1044, retval, 4, 0, 0)) == 0)
		mcRunCmdNo = 18;
	else
		SignalSema(semaidRegFunc);
	return ret;
}
