/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libkernl's file I/O client (MAIN 0x1A7398-0x1AAA78): Sony's SDK 2.7, the demo's object 0x15ECC8
 * (C), 40 functions in the object's order -- most s1:exact; the demo's sceFsInit, sceRead, sceIoctl
 * and _sceFs_Rcv_Intr are another release's sizes. Every call takes the library's semaphore
 * (_sceFsWaitS, its argument the call's own number), initialises the client on first use, and
 * makes one RPC through _cd: the request in _send_data, the server's immediate status in
 * _rcv_data_rpc (read uncached), the result written back through the request's pointer by the
 * reply interrupt, which signals the request's own semaphore. A descriptor is an index into
 * _iob[32] (each the server's fd and a busy flag). The statics carry the demo's names (data.txt).
 * Written in round 16a (the GCC lane): see each function's note; the ones not here are not written. */

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

int sceUmount(const char *name)
{
	return _sceCallCode(name, 21);
}
