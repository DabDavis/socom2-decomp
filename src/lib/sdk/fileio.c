/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libkernl's file I/O client (MAIN 0x1A7398-0x1AAA78): Sony's SDK 2.7, the demo's object 0x15ECC8
 * (C), 40 functions in the object's order -- most s1:exact; the demo's sceFsInit, sceRead, sceIoctl
 * and _sceFs_Rcv_Intr are another release's sizes. Every call takes the library's semaphore
 * (_sceFsWaitS, its argument the call's own number), initialises the client on first use, and
 * makes one RPC through _cd: the request in _send_data, the server's immediate status in
 * _rcv_data_rpc (read uncached), the result written back through the request's pointer by the
 * reply interrupt, which signals the request's own semaphore. A descriptor is an index into
 * _iob[32] (each the server's fd and a busy flag). The statics carry the demo's names (data.txt).
 * Round 16a (the GCC lane) wrote 26 of the 40, all byte-exact: the iob and semaphore helpers, the
 * power-off handler and its interrupt, _fs_version, sceFsReset, the path calls (_sceCallCode and its
 * five wrappers, sceMkdir, sceGetstat, sceSync, sceRename, sceSymlink), sceAddDrv and the descriptor
 * calls sceClose, sceDopen, sceDclose, sceDread. Round 17a: sceOpen, sceIoctl2, sceFormat,
 * sceChstat, sceMount, sceDevctl, sceReadlink byte-exact; round 46a: _sceFs_Rcv_Intr, sceLseek,
 * sceLseek64, sceRead, sceWrite and sceIoctl (the object whole, 40 of 40).
 *
 * Shapes read off the target: a request pointer initialised at its declaration (&_send_data in a
 * saved register from the entry) while the RPC call names &_send_data itself (rebuilt from the
 * %hi); each path is copied char by char with the 1024 cap and its fix-up, the call's own argument
 * stored after it; sceGetstat's request names its path unsigned char (the loop tests the loaded
 * byte, no sll 24); the descriptor calls take the iob before the library semaphore and return
 * `if (result < 0) return result; return 0;` (li -1; slt; movn). */

#include <stdarg.h>

typedef unsigned int u_int;

struct SemaParam {
	int currentCount;
	int maxCount;
	int initCount;
	int numWaitThreads;
	u_int attr;
	u_int option;
};

extern int CreateSema(struct SemaParam *);
extern int DeleteSema(int);
extern int SignalSema(int);
extern int WaitSema(int);
extern int DIntr(void);
extern int EIntr(void);

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

extern int sceSifCallRpc(sceSifClientData *, u_int, u_int, void *, int, void *, int,
			 sceSifEndFunc, void *);
extern int memcmp(const void *, const void *, unsigned int);
extern void *memset(void *, int, unsigned int);

#define	UNCACHED(p)	((u_int)(p) | 0x20000000)

/* libkernl's library stamp (another object's: "PsIIlibkernl" then the version, "2710") */
extern char __ps2_klibinfo__[];
#define	LIBVERSION	(__ps2_klibinfo__ + 12)

struct _sceFsIob {
	int i_fd;		/* the server's descriptor */
	int i_flag;		/* 0 free, 0x10000000 taken */
	int pad[2];
};

/* the request: the semaphore the reply signals, where it writes the result and its size, then the
 * call's own arguments (a path, for _sceCallCode) */
struct _sceFsCall {
	int sema;
	int *result;
	int rsize;
	union {
		char name[1024];
		int arg[256];
		void *ptr[256];
	} u;
};

/* the path calls with an argument before the path */
struct _sceFsMode {
	int sema;
	int *result;
	int rsize;
	int mode;
	char name[1024];
};

struct _sceFsSync {
	int sema;
	int *result;
	int rsize;
	int pad;
	int flag;
	char name[1024];
};

struct _sceFsStat {
	int sema;
	int *result;
	int rsize;
	void *buf;
	unsigned char name[1024];
};

struct _sceFsRename {
	int sema;
	int *result;
	int rsize;
	char oldname[1024];
	char newname[1024];
};

/* sceOpen's: the flag without its top bit, the mode (the variadic argument), the path, then the
 * iob's index */
struct _sceFsOpen {
	int sema;
	int *result;
	int rsize;
	int flag;
	int mode;
	char name[1024];
	int iob;
};

/* sceIoctl2's: the descriptor and request, the argument copied in, the reply buffer */
struct _sceFsIoctl2 {
	int sema;
	int *result;
	int rsize;
	int fd;
	int request;
	char arg[1024];
	void *bufp;
	u_int buflen;
	u_int arglen;
};

/* sceFormat's and sceMount's: two paths and an argument block */
struct _sceFsFormat {
	int sema;
	int *result;
	int rsize;
	char dev[1024];
	char blockdev[1024];
	char arg[1024];
	int arglen;
};

struct _sceFsMount {
	int sema;
	int *result;
	int rsize;
	char fsname[1024];
	char devname[1024];
	char arg[1024];
	int flag;
	int arglen;
};

/* sceChstat's: the change bits, the stat block, the path (unsigned, as sceGetstat's) */
struct _sceFsChstat {
	int sema;
	int *result;
	int rsize;
	u_int cbit;
	char stat[64];
	unsigned char name[1024];
};

/* sceDevctl's: the path, the argument copied in, the command and the reply buffer */
struct _sceFsDevctl {
	int sema;
	int *result;
	int rsize;
	char name[1024];
	char arg[1024];
	int cmd;
	u_int arglen;
	void *bufp;
	u_int buflen;
};

/* sceReadlink's: the reply buffer before the path; sent as large as sceRename's */
struct _sceFsReadlink {
	int sema;
	int *result;
	int rsize;
	u_int bufsize;
	char *buf;
	char name[1024];
};

/* sceLseek64's: the 64-bit offset (this compiler's long) on an 8-byte boundary */
struct _sceFsLseek64 {
	int sema;
	int *result;
	int rsize;
	int fd;
	long offset;
	int whence;
	int iob;
};

extern void sceSifWriteBackDCache(void *, int);
extern void *memcpy(void *, const void *, unsigned int);

volatile int _sceFs_q[32] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
static int _fs_init = 0;
static int _fs_semid = -1;
static int _fs_iob_semid = -1;
static int _fs_fsq_semid = -1;
static char *_fswildcard = "....";

extern struct _sceFsCall _send_data;
extern int _rcv_data_rpc[];
static struct _sceFsIob _iob[32];
static sceSifClientData _cd;
static char _fsversion[4];
struct _sceFsPoff {
	void (*func)(void *);
	void *addr;
};
static struct _sceFsPoff _sif_FsPoff_Data;

/* the reply side (round 17a): the reply interrupt's data (the demo's LOCAL _sif_FsRcv_Data), the
 * two 1088-byte receive buffers at _rcv_data_cmd (the demo's LOCAL, 0x40 past _rcv_data_rpc),
 * the server's double-buffering flag sceFsInit sets (the .data word at 0x1CCD50, unnamed: not in
 * the demo's release) and the buffer the last reply used (0x1CCD48, _sceFs_Rcv_Intr's, sceIoctl's) */
static int _sif_FsRcv_Data[16];
extern char _rcv_data_cmd[];
extern int data_001CCD50;
extern void _sceFs_Rcv_Intr(void *, void *);
extern int sceSifAddCmdHandler(u_int, void (*)(void *, void *), void *);
extern int sceSifInitRpc(u_int);
extern int sceSifBindRpc(sceSifClientData *, u_int, u_int);

int sceFsInit(void);

static void _sceFsIobSemaMK(void)
{
	struct SemaParam s;

	if (_fs_iob_semid == -1) {
		s.initCount = 1;
		s.maxCount = 1;
		s.option = 0;
		_fs_iob_semid = CreateSema(&s);
		_fs_fsq_semid = CreateSema(&s);
	}
}

static struct _sceFsIob *new_iob(void)
{
	struct _sceFsIob *io;

	_sceFsIobSemaMK();
	WaitSema(_fs_iob_semid);
	for (io = _iob; io < &_iob[32]; io++) {
		if (io->i_flag == 0) {
			io->i_flag = 0x10000000;
			SignalSema(_fs_iob_semid);
			return io;
		}
	}
	SignalSema(_fs_iob_semid);
	return 0;
}

static struct _sceFsIob *get_iob(int fd)
{
	struct _sceFsIob *io;

	_sceFsIobSemaMK();
	WaitSema(_fs_iob_semid);
	if ((u_int)fd >= 32) {
		SignalSema(_fs_iob_semid);
		return 0;
	}
	io = &_iob[fd];
	SignalSema(_fs_iob_semid);
	return io;
}

/* The reply interrupt: which of the two receive buffers the server filled (the packet's +12, when
 * it double-buffers), the reply's header word by word out of the uncached buffer, the reply's data
 * to its destination, the extra payload of the calls that have one, then the request's semaphore
 * -- or, for a no-wait request (negative), the slot in _sceFs_q that holds it. */
struct _sceFsRdata {
	int size1;
	int size2;
	char *dest1;
	char *dest2;
	char buf1[64];
	char buf2[64];
};

extern int data_001CCD48;	/* the receive buffer the last reply used */
extern int iSignalSema(int);

void _sceFs_Rcv_Intr(void *pkt, void *arg)
{
	int h_sema; int h_code; char *h_dest; int h_size; void *h_to; int h_len;
	char *p;
	struct _sceFsRdata *r;
	char *d, *u;
	int i;
	int n = 0;
	char *q;
	u_int len;

	if (data_001CCD50)
		n = ((int *)pkt)[3];
	q = _rcv_data_cmd + n * 1088;
	data_001CCD48 = n;
	n = 0x20000000;
	p = (char *)((u_int)q | n);
	memcpy(&h_sema, p, 4);
	memcpy(&h_code, p + 4, 4);
	memcpy(&h_dest, p + 8, 4);
	memcpy(&h_size, p + 12, 4);
	if (h_sema >= 0)
		memcpy(h_dest, p + 16, h_size);
	switch (h_code) {
	case 2:
		r = (struct _sceFsRdata *)(p + 20);
		if (r->size1 > 0) {
			d = r->dest1;
			u = (char *)UNCACHED(d);
			for (i = 0; i < r->size1; i++)
				d[i] = u[i] = r->buf1[i];
		}
		if (r->size2 > 0) {
			d = r->dest2;
			u = (char *)UNCACHED(d);
			for (i = 0; i < r->size2; i++)
				d[i] = u[i] = r->buf2[i];
		}
		break;
	case 11:
		memcpy(&h_to, p + 20, 4);
		memcpy(h_to, p + 24, 324);
		break;
	case 12:
		memcpy(&h_to, p + 20, 4);
		memcpy(h_to, p + 24, 64);
		break;
	case 23:
	case 25:
	case 26:
		memcpy(&h_to, p + 20, 4);
		memcpy(&h_len, p + 24, 4);
		len = h_len;
		if (len > 1024) {
			h_len = 1024;
			len = 1024;
		}
		memcpy(h_to, p + 28, len);
		break;
	}
	if (h_sema < 0) {
		h_sema = -h_sema;
		for (i = 0; i < 32; i++)
			if (_sceFs_q[i] == h_sema) {
				_sceFs_q[i] = -1;
				break;
			}
	} else
		iSignalSema(h_sema);
}

static void _sceFsSemInit(void)
{
	struct SemaParam s;

	if (_fs_semid == -1) {
		s.initCount = 1;
		s.maxCount = 1;
		s.option = 0;
		_fs_semid = CreateSema(&s);
	}
}

static int _sceFsWaitS(int no)
{
	_sceFsSemInit();
	WaitSema(_fs_semid);
	return 0;
}

static void _sceFsSigSema(void)
{
	SignalSema(_fs_semid);
}

void *scePowerOffHandler(void (*func)(void *), void *addr)
{
	struct _sceFsPoff *p = &_sif_FsPoff_Data;
	void *old;
	int oldei;

	_sceFsWaitS(27);
	if (!_fs_init)
		sceFsInit();
	oldei = DIntr();
	old = p->func;
	p->addr = addr;
	p->func = func;
	if (oldei)
		EIntr();
	_sceFsSigSema();
	return old;
}

static void _sceFs_Poff_Intr(void *pkt, void *arg)
{
	struct _sceFsPoff *p = arg;

	if (p->func)
		p->func(p->addr);
	__asm__ volatile ("sync.l; ei");
}

/* The client's first use: the power-off and reply handlers installed, the bind retried until the
 * server is up, every iob freed, then request 255 hands the server the two receive buffers and
 * takes back its version (kept for _fs_version) and its mode -- 2 when it double-buffers its
 * replies. The buffer pair is a function-local static (the demo's rcv_adr.30). */
int sceFsInit(void)
{
	static void *rcv_adr[2];
	struct _sceFsPoff *p = &_sif_FsPoff_Data;
	struct _sceFsIob *io;
	int oldei, i, mode;

	sceSifInitRpc(0);
	p->func = 0;
	p->addr = 0;
	oldei = DIntr();
	sceSifAddCmdHandler(0x80000011, _sceFs_Rcv_Intr, _sif_FsRcv_Data);
	sceSifAddCmdHandler(0x80000013, _sceFs_Poff_Intr, p);
	if (oldei)
		EIntr();
	while (1) {
		if (sceSifBindRpc(&_cd, 0x80000001, 0) < 0)
			return -1;
		if (_cd.serve != 0)
			break;
		i = 0x100000;
		while (i--)
			;
	}
	_sceFsIobSemaMK();
	WaitSema(_fs_iob_semid);
	for (io = _iob; io < &_iob[32]; io++)
		io->i_flag = 0;
	SignalSema(_fs_iob_semid);
	rcv_adr[0] = _rcv_data_cmd;
	rcv_adr[1] = _rcv_data_cmd + 1088;
	if (sceSifCallRpc(&_cd, 255, 0, rcv_adr, 8, _rcv_data_rpc, 8, 0, 0) < 0)
		return -0x10001;
	memcpy(_fsversion, (void *)UNCACHED(_rcv_data_rpc), 4);
	memcpy(&mode, (void *)UNCACHED(&_rcv_data_rpc[1]), 4);
	data_001CCD50 = mode == 2;
	_fs_init = 1;
	return 0;
}

static int _fs_version(void)
{
	char *ver = LIBVERSION;
	int ret = 0;

	if (memcmp(_fsversion, ver, 4) != 0 && memcmp(_fsversion, _fswildcard, 4) != 0)
		ret = memcmp(ver, _fswildcard, 4) != 0;
	return ret;
}

int sceFsReset(void)
{
	_fs_init = 0;
	memset(_fsversion, 0, 4);
	return 0;
}

int sceOpen(const char *filename, int flag, ...)
{
	struct _sceFsOpen *p = (struct _sceFsOpen *)&_send_data;
	struct _sceFsIob *io;
	int i, sema, ret, mode;
	struct SemaParam s;
	int result;
	va_list ap;

	_sceFsWaitS(0);
	if (!_fs_init)
		sceFsInit();
	if (_fs_version()) {
		_sceFsSigSema();
		return -0x10004;
	}
	if ((io = new_iob()) == 0) {
		_sceFsSigSema();
		return -19;
	}
	va_start(ap, flag);
	mode = va_arg(ap, int);
	va_end(ap);
	for (i = 0; i < 1024; i++)
		if ((p->name[i] = filename[i]) == 0)
			break;
	if (i == 1024)
		p->name[1023] = 0;
	p->flag = flag & 0x7fffffff;
	p->mode = mode;
	p->iob = io - _iob;
	s.initCount = 0;
	s.maxCount = 1;
	s.option = 0;
	sema = CreateSema(&s);
	p->result = &result;
	p->sema = sema;
	p->rsize = 4;
	if (sceSifCallRpc(&_cd, 0, 0, &_send_data, sizeof(struct _sceFsOpen), _rcv_data_rpc, 4, 0, 0) < 0) {
		DeleteSema(sema);
		_sceFsSigSema();
		return -11;
	}
	ret = *(int *)UNCACHED(_rcv_data_rpc);
	_sceFsSigSema();
	if (ret == 0) {
		DeleteSema(sema);
		return -11;
	}
	WaitSema(sema);
	DeleteSema(sema);
	if (result < 0) {
		WaitSema(_fs_iob_semid);
		io->i_flag = 0;
		SignalSema(_fs_iob_semid);
		return result;
	}
	WaitSema(_fs_iob_semid);
	io->i_fd = result;
	io->i_flag |= flag;
	ret = io - _iob;
	SignalSema(_fs_iob_semid);
	return ret;
}

int sceClose(int fd)
{
	struct _sceFsCall *p = &_send_data;
	struct _sceFsIob *io;
	int sema, ret;
	struct SemaParam s;
	int result;

	io = get_iob(fd);
	_sceFsWaitS(1);
	if (!_fs_init) {
		_sceFsSigSema();
		return -1;
	}
	if (io == 0 || io->i_flag == 0) {
		_sceFsSigSema();
		return -9;
	}
	p->u.arg[0] = io->i_fd;
	p->u.arg[1] = io - _iob;
	s.initCount = 0;
	s.maxCount = 1;
	s.option = 0;
	sema = CreateSema(&s);
	p->sema = sema;
	p->result = &result;
	p->rsize = 4;
	if (sceSifCallRpc(&_cd, 1, 0, p, 20, _rcv_data_rpc, 4, 0, 0) < 0) {
		DeleteSema(sema);
		_sceFsSigSema();
		return -11;
	}
	io->i_flag = 0;
	ret = *(int *)UNCACHED(_rcv_data_rpc);
	_sceFsSigSema();
	if (ret == 0) {
		DeleteSema(sema);
		return -11;
	}
	WaitSema(sema);
	DeleteSema(sema);
	if (result < 0)
		return result;
	return 0;
}

/* The no-wait descriptor (SCE_NOWAIT, 0x8000 in its iob flag; sceLseek64, sceRead and sceWrite with
 * it) queues the request's semaphore in _sceFs_q and negates it, and returns 0 without waiting for
 * the reply. The queue's loop: a for loop whose body ends in if-break, rotated whole, its exit code
 * duplicated before it, the copy's jump into it leaving loop.c out -- the target's peeled first test
 * and unreduced givs. Closed in round 46a, open since round 17a by two shapes:
 *  - the queue is `volatile int _sceFs_q[32]` (reorg: a volatile store never takes a delay slot, a
 *    volatile load is not moved -- so i = 0 stays above the first load and the bne's slot takes the
 *    `lui` PRE inserted; no function of the unit is lost by it);
 *  - the two `flag & 0x8000` tests are not one expression for GCSE: the second is written
 *    `(short)flag & 0x8000` (a `long` mask, `flag & m`, does the same). The narrowing goes at
 *    combine, so the code is `andi v0,s3,0x8000` both times, but PRE saw two expressions and made no
 *    reaching register in a saved register (s4 here for 200 words: every argument's allocation moved). */
int sceLseek(int fd, int offset, int whence)
{
	struct _sceFsCall *p = &_send_data;
	struct _sceFsIob *io;
	int i, sema, ret, flag;
	struct SemaParam s;
	int result;

	io = get_iob(fd);
	_sceFsWaitS(4);
	if (!_fs_init) {
		_sceFsSigSema();
		return -1;
	}
	if (io == 0 || (flag = io->i_flag) == 0) {
		_sceFsSigSema();
		return -9;
	}
	p->u.arg[0] = io->i_fd;
	p->u.arg[1] = offset;
	p->u.arg[2] = whence;
	p->u.arg[3] = io - _iob;
	s.initCount = 0;
	s.maxCount = 1;
	s.option = 0;
	sema = CreateSema(&s);
	p->sema = sema;
	p->result = &result;
	p->rsize = 4;
	if (flag & 0x8000) {
		WaitSema(_fs_fsq_semid);
		for (i = 0; i < 32; i++) {
			if (_sceFs_q[i] == -1) {
				_sceFs_q[i] = p->sema;
				p->sema = -p->sema;
				break;
			}
		}
		SignalSema(_fs_fsq_semid);
	}
	if (sceSifCallRpc(&_cd, 4, 0, &_send_data, 28, _rcv_data_rpc, 4, 0, 0) < 0) {
		DeleteSema(sema);
		_sceFsSigSema();
		return -11;
	}
	ret = *(int *)UNCACHED(_rcv_data_rpc);
	_sceFsSigSema();
	if (ret == 0) {
		DeleteSema(sema);
		return -11;
	}
	if ((short)flag & 0x8000) {
		DeleteSema(sema);
		return 0;
	}
	WaitSema(sema);
	DeleteSema(sema);
	return result;
}

/* sceRead's and sceWrite's request: the descriptor, the buffer and count, then (sceWrite) the
 * buffer's unaligned head copied in, and the iob's index last */
struct _sceFsRead {
	int sema;
	int *result;
	int rsize;
	int fd;
	void *buf;
	int nbyte;
	int pad;
	int iob;
};

struct _sceFsWrite {
	int sema;
	int *result;
	int rsize;
	int fd;
	const void *buf;
	int nbyte;
	int head;
	char data[16];
	int iob;
};

/* The no-wait path as sceLseek's (above). Unless the
 * descriptor was opened SCE_NOWBDC (0x20000000) the buffer is written back first. */
int sceRead(int fd, void *buf, int nbyte)
{
	struct _sceFsRead *p = (struct _sceFsRead *)&_send_data;
	struct _sceFsIob *io;
	int i, sema, ret, flag;
	struct SemaParam s;
	int result;

	io = get_iob(fd);
	_sceFsWaitS(2);
	if (!_fs_init) {
		_sceFsSigSema();
		return -1;
	}
	if (io == 0 || (flag = io->i_flag) == 0) {
		_sceFsSigSema();
		return -9;
	}
	p->fd = io->i_fd;
	p->buf = buf;
	p->nbyte = nbyte;
	p->iob = io - _iob;
	s.initCount = 0;
	s.maxCount = 1;
	s.option = 0;
	sema = CreateSema(&s);
	p->sema = sema;
	p->result = &result;
	p->rsize = 4;
	if (flag & 0x8000) {
		WaitSema(_fs_fsq_semid);
		for (i = 0; i < 32; i++) {
			if (_sceFs_q[i] == -1) {
				_sceFs_q[i] = p->sema;
				p->sema = -p->sema;
				break;
			}
		}
		SignalSema(_fs_fsq_semid);
	}
	if (!(flag & 0x20000000))
		sceSifWriteBackDCache(buf, nbyte);
	sceSifWriteBackDCache(p, sizeof(struct _sceFsRead));
	if (sceSifCallRpc(&_cd, 2, 0, &_send_data, sizeof(struct _sceFsRead), _rcv_data_rpc, 4, 0, 0) < 0) {
		DeleteSema(sema);
		_sceFsSigSema();
		return -11;
	}
	ret = *(int *)UNCACHED(_rcv_data_rpc);
	_sceFsSigSema();
	if (ret == 0) {
		DeleteSema(sema);
		return -11;
	}
	if ((short)flag & 0x8000) {
		DeleteSema(sema);
		return 0;
	}
	WaitSema(sema);
	DeleteSema(sema);
	return result;
}

/* The no-wait path as sceLseek's. The buffer's bytes up to its next 16-byte boundary travel in the request
 * (read uncached, after the write-back): the IOP's DMA moves whole quadwords. */
int sceWrite(int fd, const void *buf, int nbyte)
{
	struct _sceFsWrite *p = (struct _sceFsWrite *)&_send_data;
	struct _sceFsIob *io;
	int i, j, sema, ret, flag, head;
	struct SemaParam s;
	int result;

	io = get_iob(fd);
	_sceFsWaitS(3);
	if (!_fs_init) {
		_sceFsSigSema();
		return -1;
	}
	if (io == 0 || (flag = io->i_flag) == 0) {
		_sceFsSigSema();
		return -9;
	}
	p->fd = io->i_fd;
	p->nbyte = nbyte;
	p->buf = buf;
	p->iob = io - _iob;
	s.initCount = 0;
	s.maxCount = 1;
	s.option = 0;
	sema = CreateSema(&s);
	p->sema = sema;
	p->result = &result;
	p->rsize = 4;
	if (flag & 0x8000) {
		WaitSema(_fs_fsq_semid);
		for (i = 0; i < 32; i++) {
			if (_sceFs_q[i] == -1) {
				_sceFs_q[i] = p->sema;
				p->sema = -p->sema;
				break;
			}
		}
		SignalSema(_fs_fsq_semid);
	}
	if (((u_int)buf & 15) == 0)
		head = 0;
	else
		head = (((u_int)buf >> 4) << 4) + 16 - (u_int)buf;
	if (nbyte < head)
		head = nbyte;
	if (!(flag & 0x20000000))
		sceSifWriteBackDCache((void *)buf, nbyte);
	buf = (const void *)UNCACHED(buf);
	p->head = head;
	for (j = 0; j < head; j++)
		p->data[j] = ((const char *)buf)[j];
	if (sceSifCallRpc(&_cd, 3, 0, &_send_data, sizeof(struct _sceFsWrite), _rcv_data_rpc, 4, 0, 0) < 0) {
		DeleteSema(sema);
		_sceFsSigSema();
		return -11;
	}
	ret = *(int *)UNCACHED(_rcv_data_rpc);
	_sceFsSigSema();
	if (ret == 0) {
		DeleteSema(sema);
		return -11;
	}
	if ((short)flag & 0x8000) {
		DeleteSema(sema);
		return 0;
	}
	WaitSema(sema);
	DeleteSema(sema);
	return result;
}

/* sceIoctl's request: the descriptor and request, the argument copied in whole (GCC round 45a) */
struct _sceFsIoctl {
	int sema;
	int *result;
	int rsize;
	int fd;
	int request;
	char arg[1024];
	int f1044;
	int f1048;
	int arglen;
};

extern void *data_001DBF88;	/* the request's argument, for the reply */
extern int data_001CCD48;	/* the receive buffer the last reply used */

/* requests 1-3 answered here: whether a queued request's semaphore is waiting, and the last reply's
 * first word or doubleword read uncached out of its 1088-byte buffer (its +16); the rest go to the
 * IOP. The target holds the constant 1 in s5 from the top (`li s5,1` under get_iob) for one use, the
 * switch's `beql s1,s5`: CSE replaces case 1's constant by a register holding it, on its path from
 * the top (skip-blocks and follow-jumps carry the table that far), which a variable set once and
 * unused gives nowhere -- `int one = 1;` and a second `one = 1;` in the case (round 46a; round 45a
 * had the declaration alone: "not emitted"; a use of it in the arm, `*... = one`, is 3 words off). */
int sceIoctl(int fd, int request, void *arg)
{
	struct _sceFsIoctl *p = (struct _sceFsIoctl *)&_send_data;
	struct _sceFsIob *io;
	int i, sema, ret;
	struct SemaParam s;
	int result;
	int one = 1;

	io = get_iob(fd);
	_sceFsWaitS(5);
	data_001DBF88 = arg;
	if (!_fs_init)
		sceFsInit();
	if (io == 0 || io->i_flag == 0) {
		_sceFsSigSema();
		return -9;
	}
	p->f1044 = 0;
	p->f1048 = 0;
	switch (request) {
	case 1:
		one = 1;
		WaitSema(_fs_fsq_semid);
		for (i = 0; i < 32; i++)
			if (_sceFs_q[i] != -1)
				break;
		if (i == 32)
			*(int *)data_001DBF88 = 0;
		else
			*(int *)data_001DBF88 = 1;
		SignalSema(_fs_fsq_semid);
		_sceFsSigSema();
		return 0;
	case 2:
		*(int *)arg = *(int *)UNCACHED(_rcv_data_cmd + 16 + data_001CCD48 * 1088);
		_sceFsSigSema();
		return 0;
	case 3:
		*(long *)arg = *(long *)UNCACHED(_rcv_data_cmd + 16 + data_001CCD48 * 1088);
		_sceFsSigSema();
		return 0;
	}
	p->fd = io->i_fd;
	p->request = request;
	if (arg == 0)
		p->arglen = 0;
	else {
		p->arglen = 1024;
		memcpy(p->arg, arg, 1024);
	}
	s.initCount = 0;
	s.maxCount = 1;
	s.option = 0;
	sema = CreateSema(&s);
	p->sema = sema;
	p->result = &result;
	p->rsize = 4;
	sceSifWriteBackDCache(&_send_data, sizeof(struct _sceFsIoctl));
	if (sceSifCallRpc(&_cd, 5, 0, &_send_data, sizeof(struct _sceFsIoctl), _rcv_data_rpc, 4, 0, 0) < 0) {
		DeleteSema(sema);
		_sceFsSigSema();
		return -11;
	}
	ret = *(int *)UNCACHED(_rcv_data_rpc);
	_sceFsSigSema();
	if (ret == 0) {
		DeleteSema(sema);
		return -11;
	}
	WaitSema(sema);
	DeleteSema(sema);
	return result;
}

int sceIoctl2(int fd, int request, void *argp, u_int arglen, void *bufp, u_int buflen)
{
	struct _sceFsIoctl2 *p = (struct _sceFsIoctl2 *)&_send_data;
	struct _sceFsIob *io;
	int sema, ret;
	struct SemaParam s;
	int result;

	io = get_iob(fd);
	_sceFsWaitS(26);
	if (!_fs_init)
		sceFsInit();
	if (io == 0 || io->i_flag == 0) {
		_sceFsSigSema();
		return -9;
	}
	if (arglen > 1024 || buflen > 1024) {
		_sceFsSigSema();
		return -22;
	}
	if (argp == 0)
		p->arglen = 0;
	else
		memcpy(p->arg, argp, arglen);
	p->fd = io->i_fd;
	p->request = request;
	p->arglen = arglen;
	s.initCount = 0;
	s.maxCount = 1;
	s.option = 0;
	sema = CreateSema(&s);
	p->sema = sema;
	p->result = &result;
	p->rsize = 4;
	p->bufp = bufp;
	p->buflen = buflen;
	sceSifWriteBackDCache(&_send_data, sizeof(struct _sceFsIoctl2));
	if (sceSifCallRpc(&_cd, 26, 0, &_send_data, sizeof(struct _sceFsIoctl2), _rcv_data_rpc, 4, 0, 0) < 0) {
		DeleteSema(sema);
		_sceFsSigSema();
		return -11;
	}
	ret = *(int *)UNCACHED(_rcv_data_rpc);
	_sceFsSigSema();
	if (ret == 0) {
		DeleteSema(sema);
		return -11;
	}
	WaitSema(sema);
	DeleteSema(sema);
	return result;
}

static int _sceCallCode(const char *name, int code)
{
	struct _sceFsCall *p = &_send_data;
	int i, sema, ret;
	struct SemaParam s;
	int result;

	_sceFsWaitS(code);
	if (!_fs_init)
		sceFsInit();
	for (i = 0; i < 1024; i++)
		if ((p->u.name[i] = name[i]) == 0)
			break;
	if (i == 1024) {
		p->u.name[1023] = 0;
		i = 1023;
	}
	s.initCount = 0;
	s.maxCount = 1;
	s.option = 0;
	sema = CreateSema(&s);
	p->result = &result;
	p->sema = sema;
	p->rsize = 4;
	if (sceSifCallRpc(&_cd, code, 0, &_send_data, i + 13, _rcv_data_rpc, 4, 0, 0) < 0) {
		DeleteSema(sema);
		_sceFsSigSema();
		return -11;
	}
	ret = *(int *)UNCACHED(_rcv_data_rpc);
	_sceFsSigSema();
	if (ret == 0) {
		DeleteSema(sema);
		return -11;
	}
	WaitSema(sema);
	DeleteSema(sema);
	return result;
}

int sceRemove(const char *name)
{
	return _sceCallCode(name, 6);
}

int sceMkdir(const char *name, int mode)
{
	struct _sceFsMode *p = (struct _sceFsMode *)&_send_data;
	int i, sema, ret;
	struct SemaParam s;
	int result;

	_sceFsWaitS(7);
	if (!_fs_init)
		sceFsInit();
	for (i = 0; i < 1024; i++)
		if ((p->name[i] = name[i]) == 0)
			break;
	if (i == 1024) {
		p->name[1023] = 0;
		i = 1023;
	}
	p->mode = mode;
	s.initCount = 0;
	s.maxCount = 1;
	s.option = 0;
	sema = CreateSema(&s);
	p->result = &result;
	p->sema = sema;
	p->rsize = 4;
	if (sceSifCallRpc(&_cd, 7, 0, &_send_data, i + 17, _rcv_data_rpc, 4, 0, 0) < 0) {
		DeleteSema(sema);
		_sceFsSigSema();
		return -11;
	}
	ret = *(int *)UNCACHED(_rcv_data_rpc);
	_sceFsSigSema();
	if (ret == 0) {
		DeleteSema(sema);
		return -11;
	}
	WaitSema(sema);
	DeleteSema(sema);
	return result;
}

int sceRmdir(const char *name)
{
	return _sceCallCode(name, 8);
}

int sceFormat(const char *devname, const char *blockdevname, void *arg, int arglen)
{
	struct _sceFsFormat *p = (struct _sceFsFormat *)&_send_data;
	int i, sema, ret;
	struct SemaParam s;
	int result;

	_sceFsWaitS(14);
	if (!_fs_init)
		sceFsInit();
	for (i = 0; i < 1024; i++)
		if ((p->dev[i] = devname[i]) == 0)
			break;
	if (i == 1024)
		p->dev[1023] = 0;
	if (blockdevname == 0)
		p->blockdev[0] = 0;
	else {
		for (i = 0; i < 1024; i++)
			if ((p->blockdev[i] = blockdevname[i]) == 0)
				break;
		if (i == 1024)
			p->blockdev[1023] = 0;
	}
	if (arglen > 1024) {
		_sceFsSigSema();
		return -7;
	}
	for (i = 0; i < arglen; i++)
		p->arg[i] = ((char *)arg)[i];
	p->arglen = arglen;
	s.initCount = 0;
	s.maxCount = 1;
	s.option = 0;
	sema = CreateSema(&s);
	p->sema = sema;
	p->result = &result;
	p->rsize = 4;
	sceSifWriteBackDCache(&_send_data, sizeof(struct _sceFsFormat));
	if (sceSifCallRpc(&_cd, 14, 0, &_send_data, sizeof(struct _sceFsFormat), _rcv_data_rpc, 4, 0, 0) < 0) {
		DeleteSema(sema);
		_sceFsSigSema();
		return -11;
	}
	ret = *(int *)UNCACHED(_rcv_data_rpc);
	_sceFsSigSema();
	if (ret == 0) {
		DeleteSema(sema);
		return -11;
	}
	WaitSema(sema);
	DeleteSema(sema);
	return result;
}

int sceAddDrv(void *device)
{
	struct _sceFsCall *p = &_send_data;
	int sema, ret;
	struct SemaParam s;
	int result;

	_sceFsWaitS(15);
	if (!_fs_init)
		sceFsInit();
	p->u.ptr[0] = device;
	s.initCount = 0;
	s.maxCount = 1;
	s.option = 0;
	sema = CreateSema(&s);
	p->sema = sema;
	p->result = &result;
	p->rsize = 4;
	if (sceSifCallRpc(&_cd, 15, 0, p, 16, _rcv_data_rpc, 4, 0, 0) < 0) {
		DeleteSema(sema);
		_sceFsSigSema();
		return -1;
	}
	ret = *(int *)UNCACHED(_rcv_data_rpc);
	_sceFsSigSema();
	if (ret == 0) {
		DeleteSema(sema);
		return -1;
	}
	WaitSema(sema);
	DeleteSema(sema);
	return result;
}

int sceDelDrv(const char *name)
{
	return _sceCallCode(name, 16);
}

int sceDopen(const char *dirname)
{
	struct _sceFsIob *io;
	int ret;

	_sceFsWaitS(9);
	if (!_fs_init)
		sceFsInit();
	_sceFsSigSema();
	if ((io = new_iob()) == 0)
		return -19;
	ret = _sceCallCode(dirname, 9);
	if (ret < 0) {
		WaitSema(_fs_iob_semid);
		io->i_flag = 0;
		SignalSema(_fs_iob_semid);
		return ret;
	}
	WaitSema(_fs_iob_semid);
	io->i_fd = ret;
	ret = io - _iob;
	SignalSema(_fs_iob_semid);
	return ret;
}

int sceDclose(int fd)
{
	struct _sceFsCall *p = &_send_data;
	struct _sceFsIob *io;
	int sema, ret;
	struct SemaParam s;
	int result;

	io = get_iob(fd);
	_sceFsWaitS(10);
	if (!_fs_init) {
		_sceFsSigSema();
		return -1;
	}
	if (io == 0 || io->i_flag == 0) {
		_sceFsSigSema();
		return -9;
	}
	p->u.arg[0] = io->i_fd;
	s.initCount = 0;
	s.maxCount = 1;
	s.option = 0;
	sema = CreateSema(&s);
	p->sema = sema;
	p->result = &result;
	p->rsize = 4;
	if (sceSifCallRpc(&_cd, 10, 0, p, 20, _rcv_data_rpc, 4, 0, 0) < 0) {
		DeleteSema(sema);
		_sceFsSigSema();
		return -11;
	}
	io->i_flag = 0;
	ret = *(int *)UNCACHED(_rcv_data_rpc);
	_sceFsSigSema();
	if (ret == 0) {
		DeleteSema(sema);
		return -11;
	}
	WaitSema(sema);
	DeleteSema(sema);
	if (result < 0)
		return result;
	return 0;
}

int sceDread(int fd, void *buf)
{
	struct _sceFsCall *p = &_send_data;
	struct _sceFsIob *io;
	int sema, ret;
	struct SemaParam s;
	int result;

	io = get_iob(fd);
	_sceFsWaitS(11);
	if (!_fs_init) {
		_sceFsSigSema();
		return -1;
	}
	if (io == 0 || io->i_flag == 0) {
		_sceFsSigSema();
		return -9;
	}
	p->u.arg[0] = io->i_fd;
	p->u.ptr[1] = buf;
	s.initCount = 0;
	s.maxCount = 1;
	s.option = 0;
	sema = CreateSema(&s);
	p->sema = sema;
	p->result = &result;
	p->rsize = 4;
	if (sceSifCallRpc(&_cd, 11, 0, p, 32, _rcv_data_rpc, 4, 0, 0) < 0) {
		WaitSema(sema);
		_sceFsSigSema();
		return -11;
	}
	ret = *(int *)UNCACHED(_rcv_data_rpc);
	_sceFsSigSema();
	if (ret == 0) {
		DeleteSema(sema);
		return -11;
	}
	WaitSema(sema);
	DeleteSema(sema);
	return result;
}

int sceGetstat(const char *name, void *buf)
{
	struct _sceFsStat *p = (struct _sceFsStat *)&_send_data;
	int i, sema, ret;
	struct SemaParam s;
	int result;

	_sceFsWaitS(12);
	if (!_fs_init)
		sceFsInit();
	for (i = 0; i < 1024; i++)
		if ((p->name[i] = name[i]) == 0)
			break;
	if (i == 1024) {
		p->name[1023] = 0;
		i = 1023;
	}
	p->buf = buf;
	s.initCount = 0;
	s.maxCount = 1;
	s.option = 0;
	sema = CreateSema(&s);
	p->result = &result;
	p->sema = sema;
	p->rsize = 4;
	if (sceSifCallRpc(&_cd, 12, 0, &_send_data, i + 17, _rcv_data_rpc, 4, 0, 0) < 0) {
		DeleteSema(sema);
		_sceFsSigSema();
		return -11;
	}
	ret = *(int *)UNCACHED(_rcv_data_rpc);
	_sceFsSigSema();
	if (ret == 0) {
		DeleteSema(sema);
		return -11;
	}
	WaitSema(sema);
	DeleteSema(sema);
	return result;
}

int sceChstat(const char *name, void *buf, u_int cbit)
{
	struct _sceFsChstat *p = (struct _sceFsChstat *)&_send_data;
	int i, sema, ret;
	struct SemaParam s;
	int result;

	_sceFsWaitS(13);
	if (!_fs_init)
		sceFsInit();
	for (i = 0; i < 1024; i++)
		if ((p->name[i] = name[i]) == 0)
			break;
	if (i == 1024) {
		p->name[1023] = 0;
		i = 1023;
	}
	memcpy(p->stat, buf, 64);
	p->cbit = cbit;
	s.initCount = 0;
	s.maxCount = 1;
	s.option = 0;
	sema = CreateSema(&s);
	p->sema = sema;
	p->result = &result;
	p->rsize = 4;
	sceSifWriteBackDCache(&_send_data, sizeof(struct _sceFsChstat));
	if (sceSifCallRpc(&_cd, 13, 0, &_send_data, i + 81, _rcv_data_rpc, 4, 0, 0) < 0) {
		DeleteSema(sema);
		_sceFsSigSema();
		return -11;
	}
	ret = *(int *)UNCACHED(_rcv_data_rpc);
	_sceFsSigSema();
	if (ret == 0) {
		DeleteSema(sema);
		return -11;
	}
	WaitSema(sema);
	DeleteSema(sema);
	return result;
}

int sceRename(const char *oldname, const char *newname)
{
	struct _sceFsRename *p = (struct _sceFsRename *)&_send_data;
	int i, sema, ret;
	struct SemaParam s;
	int result;

	_sceFsWaitS(17);
	if (!_fs_init)
		sceFsInit();
	for (i = 0; i < 1024; i++)
		if ((p->oldname[i] = oldname[i]) == 0)
			break;
	if (i == 1024)
		p->oldname[1023] = 0;
	for (i = 0; i < 1024; i++)
		if ((p->newname[i] = newname[i]) == 0)
			break;
	if (i == 1024)
		p->newname[1023] = 0;
	s.initCount = 0;
	s.maxCount = 1;
	s.option = 0;
	sema = CreateSema(&s);
	p->sema = sema;
	p->result = &result;
	p->rsize = 4;
	sceSifWriteBackDCache(&_send_data, sizeof(struct _sceFsRename));
	if (sceSifCallRpc(&_cd, 17, 0, &_send_data, sizeof(struct _sceFsRename), _rcv_data_rpc, 4, 0, 0) < 0) {
		DeleteSema(sema);
		_sceFsSigSema();
		return -11;
	}
	ret = *(int *)UNCACHED(_rcv_data_rpc);
	_sceFsSigSema();
	if (ret == 0) {
		DeleteSema(sema);
		return -11;
	}
	WaitSema(sema);
	DeleteSema(sema);
	return result;
}

int sceChdir(const char *name)
{
	return _sceCallCode(name, 18);
}

int sceSync(const char *name, int flag)
{
	struct _sceFsSync *p = (struct _sceFsSync *)&_send_data;
	int i, sema, ret;
	struct SemaParam s;
	int result;

	_sceFsWaitS(19);
	if (!_fs_init)
		sceFsInit();
	for (i = 0; i < 1024; i++)
		if ((p->name[i] = name[i]) == 0)
			break;
	if (i == 1024)
		p->name[1023] = 0;
	p->flag = flag;
	s.initCount = 0;
	s.maxCount = 1;
	s.option = 0;
	sema = CreateSema(&s);
	p->result = &result;
	p->sema = sema;
	p->rsize = 4;
	if (sceSifCallRpc(&_cd, 19, 0, &_send_data, sizeof(struct _sceFsSync), _rcv_data_rpc, 4, 0, 0) < 0) {
		DeleteSema(sema);
		_sceFsSigSema();
		return -11;
	}
	ret = *(int *)UNCACHED(_rcv_data_rpc);
	_sceFsSigSema();
	if (ret == 0) {
		DeleteSema(sema);
		return -11;
	}
	WaitSema(sema);
	DeleteSema(sema);
	return result;
}

int sceMount(const char *fsname, const char *devname, int flag, void *arg, int arglen)
{
	struct _sceFsMount *p = (struct _sceFsMount *)&_send_data;
	int i, sema, ret;
	struct SemaParam s;
	int result;

	_sceFsWaitS(20);
	if (!_fs_init)
		sceFsInit();
	for (i = 0; i < 1024; i++)
		if ((p->fsname[i] = fsname[i]) == 0)
			break;
	if (i == 1024)
		p->fsname[1023] = 0;
	for (i = 0; i < 1024; i++)
		if ((p->devname[i] = devname[i]) == 0)
			break;
	if (i == 1024)
		p->devname[1023] = 0;
	if (arglen > 1024) {
		_sceFsSigSema();
		return -7;
	}
	for (i = 0; i < arglen; i++)
		p->arg[i] = ((char *)arg)[i];
	p->arglen = arglen;
	p->flag = flag;
	s.initCount = 0;
	s.maxCount = 1;
	s.option = 0;
	sema = CreateSema(&s);
	p->sema = sema;
	p->result = &result;
	p->rsize = 4;
	sceSifWriteBackDCache(&_send_data, sizeof(struct _sceFsMount));
	if (sceSifCallRpc(&_cd, 20, 0, &_send_data, sizeof(struct _sceFsMount), _rcv_data_rpc, 4, 0, 0) < 0) {
		DeleteSema(sema);
		_sceFsSigSema();
		return -11;
	}
	ret = *(int *)UNCACHED(_rcv_data_rpc);
	_sceFsSigSema();
	if (ret == 0) {
		DeleteSema(sema);
		return -11;
	}
	WaitSema(sema);
	DeleteSema(sema);
	return result;
}

int sceUmount(const char *name)
{
	return _sceCallCode(name, 21);
}

long sceLseek64(int fd, long offset, int whence)
{
	struct _sceFsLseek64 *p = (struct _sceFsLseek64 *)&_send_data;
	struct _sceFsIob *io;
	int i, sema, ret, flag;
	struct SemaParam s;
	long result;

	io = get_iob(fd);
	_sceFsWaitS(22);
	if (!_fs_init) {
		_sceFsSigSema();
		return -1;
	}
	if (io == 0 || (flag = io->i_flag) == 0) {
		_sceFsSigSema();
		return -9;
	}
	p->fd = io->i_fd;
	p->offset = offset;
	p->whence = whence;
	p->iob = io - _iob;
	s.initCount = 0;
	s.maxCount = 1;
	s.option = 0;
	sema = CreateSema(&s);
	p->sema = sema;
	p->result = (int *)&result;
	p->rsize = 8;
	if (flag & 0x8000) {
		WaitSema(_fs_fsq_semid);
		for (i = 0; i < 32; i++) {
			if (_sceFs_q[i] == -1) {
				_sceFs_q[i] = p->sema;
				p->sema = -p->sema;
				break;
			}
		}
		SignalSema(_fs_fsq_semid);
	}
	if (sceSifCallRpc(&_cd, 22, 0, &_send_data, sizeof(struct _sceFsLseek64), _rcv_data_rpc, 4, 0, 0) < 0) {
		DeleteSema(sema);
		_sceFsSigSema();
		return -11;
	}
	ret = *(int *)UNCACHED(_rcv_data_rpc);
	_sceFsSigSema();
	if (ret == 0) {
		DeleteSema(sema);
		return -11;
	}
	if ((short)flag & 0x8000) {
		DeleteSema(sema);
		return 0;
	}
	WaitSema(sema);
	DeleteSema(sema);
	return result;
}

int sceDevctl(const char *name, int cmd, void *arg, u_int arglen, void *bufp, u_int buflen)
{
	struct _sceFsDevctl *p;
	int i, sema, ret;
	struct SemaParam s;
	int result;

	_sceFsWaitS(23);
	p = (struct _sceFsDevctl *)&_send_data;
	if (!_fs_init)
		sceFsInit();
	for (i = 0; i < 1024; i++)
		if ((p->name[i] = name[i]) == 0)
			break;
	if (i == 1024)
		p->name[1023] = 0;
	if (arglen > 1024 || buflen > 1024) {
		_sceFsSigSema();
		return -22;
	}
	for (i = 0; i < arglen; i++)
		p->arg[i] = ((char *)arg)[i];
	p->arglen = arglen;
	p->cmd = cmd;
	s.initCount = 0;
	s.maxCount = 1;
	s.option = 0;
	sema = CreateSema(&s);
	p->sema = sema;
	p->result = &result;
	p->rsize = 4;
	p->bufp = bufp;
	p->buflen = buflen;
	sceSifWriteBackDCache(&_send_data, sizeof(struct _sceFsDevctl));
	if (sceSifCallRpc(&_cd, 23, 0, &_send_data, sizeof(struct _sceFsDevctl), _rcv_data_rpc, 4, 0, 0) < 0) {
		DeleteSema(sema);
		_sceFsSigSema();
		return -11;
	}
	ret = *(int *)UNCACHED(_rcv_data_rpc);
	_sceFsSigSema();
	if (ret == 0) {
		DeleteSema(sema);
		return -11;
	}
	WaitSema(sema);
	DeleteSema(sema);
	return result;
}

int sceSymlink(const char *existing, const char *newname)
{
	struct _sceFsRename *p = (struct _sceFsRename *)&_send_data;
	int i, sema, ret;
	struct SemaParam s;
	int result;

	_sceFsWaitS(17);
	if (!_fs_init)
		sceFsInit();
	for (i = 0; i < 1024; i++)
		if ((p->oldname[i] = existing[i]) == 0)
			break;
	if (i == 1024)
		p->oldname[1023] = 0;
	for (i = 0; i < 1024; i++)
		if ((p->newname[i] = newname[i]) == 0)
			break;
	if (i == 1024)
		p->newname[1023] = 0;
	s.initCount = 0;
	s.maxCount = 1;
	s.option = 0;
	sema = CreateSema(&s);
	p->result = &result;
	p->sema = sema;
	p->rsize = 4;
	if (sceSifCallRpc(&_cd, 24, 0, &_send_data, sizeof(struct _sceFsRename), _rcv_data_rpc, 4, 0, 0) < 0) {
		DeleteSema(sema);
		_sceFsSigSema();
		return -11;
	}
	ret = *(int *)UNCACHED(_rcv_data_rpc);
	_sceFsSigSema();
	if (ret == 0) {
		DeleteSema(sema);
		return -11;
	}
	WaitSema(sema);
	DeleteSema(sema);
	return result;
}

int sceReadlink(const char *name, char *buf, u_int bufsize)
{
	struct _sceFsReadlink *p = (struct _sceFsReadlink *)&_send_data;
	int i, sema, ret;
	struct SemaParam s;
	int result;

	_sceFsWaitS(17);
	if (!_fs_init)
		sceFsInit();
	for (i = 0; i < 1024; i++)
		if ((p->name[i] = name[i]) == 0)
			break;
	if (i == 1024)
		p->name[1023] = 0;
	if (bufsize >= 1024)
		bufsize = 1023;
	p->buf = buf;
	p->bufsize = bufsize;
	sceSifWriteBackDCache(buf, bufsize);
	s.initCount = 0;
	s.maxCount = 1;
	s.option = 0;
	sema = CreateSema(&s);
	p->result = &result;
	p->sema = sema;
	p->rsize = 4;
	if (sceSifCallRpc(&_cd, 25, 0, &_send_data, sizeof(struct _sceFsRename), _rcv_data_rpc, 4, 0, 0) < 0) {
		DeleteSema(sema);
		_sceFsSigSema();
		return -11;
	}
	ret = *(int *)UNCACHED(_rcv_data_rpc);
	_sceFsSigSema();
	if (ret == 0) {
		DeleteSema(sema);
		return -11;
	}
	WaitSema(sema);
	DeleteSema(sema);
	return result;
}
