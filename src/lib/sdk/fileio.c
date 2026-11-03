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
 * sceChstat, sceMount, sceDevctl, sceReadlink byte-exact; sceLseek, sceLseek64, sceRead and
 * sceWrite written and OPEN (the no-wait path, diagnosed above sceLseek). Not written:
 * _sceFs_Rcv_Intr, sceFsInit (its version reply sets the unnamed .data word at 0x1CCD50),
 * sceIoctl (its queue scan and the uncached ring at 0x1DCC50 indexed by the word at 0x1CCD48).
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

int _sceFs_q[32] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
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

/* OPEN (round 17a; sceLseek64, sceRead and sceWrite with it): the no-wait descriptor
 * (SCE_NOWAIT, 0x8000 in its iob flag) queues the request's semaphore in _sceFs_q and negates
 * it, and returns 0 without waiting for the reply. The queue's loop is right (a for loop whose
 * body ends in if-break: rotated whole, its exit code duplicated before it, the copy's jump into
 * it leaving loop.c out -- the target's peeled first test and unreduced givs). Two differences
 * remain, both before the RPC:
 *  - the target tests `flag & 0x8000` twice, `andi v0,s3,0x8000` at each test; here GCSE's PRE
 *    finds the second fully redundant ("PRE: redundant insn ... reaching reg") and keeps the first
 *    result in a saved register (s4), which moves the allocation of every argument (sceRead's
 *    buffer and count s7/s8 against the target's s6/s7). No flag of this cc1 changes it
 *    (-fno-gcse loses nine matched functions, -fno-edge-lcm is 990721's PRE, edge-lm/sm,
 *    expensive-optimizations, cse-skip-blocks, rerun-*, thread-jumps ...), nor every 991111
 *    build; nor a long, unsigned short, hard-register or second-variable flag, the tail or the
 *    queue as an __inline__, a nowait variable. Making the second test another expression (a
 *    long flag, `(int)flag & 0x8000`) gives the target's first 60 words exactly: the vendor's
 *    PRE did not see the two tests as one.
 *  - the peeled test's delay slot: the target keeps `move a2,zero` (i = 0) before the load and
 *    fills the bne's slot with the `lui` PRE inserted on both of its successors; this sched2 order
 *    is the target's insn for insn, and reorg's backward scan takes i = 0 into the slot. (A label,
 *    an asm or a call between i = 0 and the test would stop that scan; none is in the source.)
 * Both point at the vendor's GCSE/reorg rather than the text (libscf's adddate/subdate, round 12a,
 * are the other place this build's GCSE does more than the target's). */
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
	if (flag & 0x8000) {
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

/* OPEN, as sceLseek (above): the no-wait test, and the queue's delay slot. Unless the
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
	if (flag & 0x8000) {
		DeleteSema(sema);
		return 0;
	}
	WaitSema(sema);
	DeleteSema(sema);
	return result;
}

/* OPEN, as sceLseek. The buffer's bytes up to its next 16-byte boundary travel in the request
 * (read uncached, after the write-back): the IOP's DMA moves whole quadwords. */
int sceWrite(int fd, const void *buf, int nbyte)
{
	struct _sceFsWrite *p = (struct _sceFsWrite *)&_send_data;
	struct _sceFsIob *io;
	int i, sema, ret, flag, head;
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
	if ((u_int)buf & 15)
		head = (((u_int)buf >> 4) << 4) + 16 - (u_int)buf;
	else
		head = 0;
	if (nbyte < head)
		head = nbyte;
	if (!(flag & 0x20000000))
		sceSifWriteBackDCache((void *)buf, nbyte);
	buf = (const void *)UNCACHED(buf);
	p->head = head;
	for (i = 0; i < head; i++)
		p->data[i] = ((const char *)buf)[i];
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
	if (flag & 0x8000) {
		DeleteSema(sema);
		return 0;
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
	if (flag & 0x8000) {
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
