/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libkernl's module loader client (MAIN 0x1AAFC8-0x1ABD24): Sony's SDK 2.7, the demo's object
 * 0x1628C0 (C), 18 functions, all written, in the object's order. Every call binds to the IOP
 * loader (0x80000006) once, checks the server's version against the library's ("2710", or the
 * wildcard either side) -- but the IOP memory calls, which skip the check -- then makes one RPC
 * through cd with _senddata both sent and received: the module calls lay it out as two words (the
 * address, module id or argument length; the argument length, then the module's result), the path
 * (252) and the arguments (252, copied whole by a constant-size memcpy, which GCC expands into its
 * block move with the run-time alignment test); the IOP memory calls as address, width and value.
 * The statics carry the demo's names (_bind_check and _lfwildcard in .data; _senddata, cd,
 * _lfversion in .bss). The error codes are the target's own numbers: -0x10000 unbound, -0x10001
 * RPC failed, -0x10002 a bad width, -0x10003 no entry point, -0x10004 version mismatch. Names
 * are SOCOM 1's (s1:exact, _lf_version s1:callee).
 *
 * The version is libkernl's stamp, __ps2_klibinfo__'s last four bytes (data.txt), taken into a
 * local first (the target loads its address before _lfversion's: one order of two). A .data
 * static is compared by its own extent (scdiff's local_extent): _bind_check's -1 had read as a
 * 5-byte string into _lfwildcard's pointer. Two shapes the target fixes: _lf_bind's bind loop sits
 * inside `if (_bind_check < 0)` with the success path returning from inside it (the delay loop is
 * laid out last, the first test jumps to the final `return 0`); and a call returning the module's
 * result reads _senddata.id into a local before it stores through `result`, which may alias it. */

typedef unsigned int u_int;

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

extern int sceSifBindRpc(sceSifClientData *, u_int, u_int);
extern int sceSifCallRpc(sceSifClientData *, u_int, u_int, void *, int, void *, int,
			 sceSifEndFunc, void *);
extern int memcmp(const void *, const void *, unsigned int);
extern void *memset(void *, int, unsigned int);
extern void *memcpy(void *, const void *, unsigned int);
extern char *strncpy(char *, const char *, unsigned int);

#define	SIF_LOADFILE	0x80000006

/* libkernl's library stamp (another object's: "PsIIlibkernl" then the version, "2710") */
extern char __ps2_klibinfo__[];
#define	LIBVERSION	(__ps2_klibinfo__ + 12)

static int _bind_check = -1;
static char *_lfwildcard = "....";
static struct {
	int id;			/* the address, module id or result; LoadModule's argument length */
	int arglen;		/* the argument length; back, the module's result */
	char name[252];		/* the path */
	char args[252];		/* the module's arguments; LoadElfPart's section name */
} _senddata __attribute__((aligned(64)));
static sceSifClientData cd;
static char _lfversion[4];

static int _lf_bind(void)
{
	int i;

	if (_bind_check < 0) {
		while (1) {
			if (sceSifBindRpc(&cd, SIF_LOADFILE, 0) < 0)
				return -1;
			if (cd.serve != 0) {
				_bind_check = 0;
				if (sceSifCallRpc(&cd, 255, 0, 0, 0, &_senddata, 4, 0, 0) < 0)
					return -0x10001;
				memcpy(_lfversion, &_senddata, 4);
				return 0;
			}
			i = 0x100000;
			while (i--)
				;
		}
	}
	return 0;
}

static int _lf_version(void)
{
	char *ver = LIBVERSION;
	int ret = 0;

	if (memcmp(_lfversion, ver, 4) != 0 && memcmp(_lfversion, _lfwildcard, 4) != 0)
		ret = memcmp(ver, _lfwildcard, 4) != 0;
	return ret;
}

int sceSifLoadFileReset(void)
{
	_bind_check = -1;
	memset(_lfversion, 0, 4);
	return 0;
}

int _sceSifLoadModuleBuffer(const void *addr, int args, const char *argp, int *result)
{
	int ret;

	if (_lf_bind() < 0)
		return -0x10000;
	if (_lf_version())
		return -0x10004;
	_senddata.id = (int)addr;
	if (argp != 0) {
		if (args > 252) {
			memcpy(_senddata.args, argp, 252);
			_senddata.arglen = 252;
		} else {
			memcpy(_senddata.args, argp, args);
			_senddata.arglen = args;
		}
	} else
		_senddata.arglen = 0;
	if (sceSifCallRpc(&cd, 6, 0, &_senddata, 512, &_senddata, 8, 0, 0) < 0)
		return -0x10001;
	ret = _senddata.id;
	*result = _senddata.arglen;
	return ret;
}

int sceSifStopModule(int modid, int args, const char *argp, int *result)
{
	int ret;

	if (_lf_bind() < 0)
		return -0x10000;
	if (_lf_version())
		return -0x10004;
	_senddata.id = modid;
	if (argp != 0) {
		if (args > 252) {
			memcpy(_senddata.args, argp, 252);
			_senddata.arglen = 252;
		} else {
			memcpy(_senddata.args, argp, args);
			_senddata.arglen = args;
		}
	} else
		_senddata.arglen = 0;
	if (sceSifCallRpc(&cd, 7, 0, &_senddata, 512, &_senddata, 8, 0, 0) < 0)
		return -0x10001;
	ret = _senddata.id;
	*result = _senddata.arglen;
	return ret;
}

int sceSifUnloadModule(int modid)
{
	if (_lf_bind() < 0)
		return -0x10000;
	if (_lf_version())
		return -0x10004;
	_senddata.id = modid;
	if (sceSifCallRpc(&cd, 8, 0, &_senddata, 4, &_senddata, 4, 0, 0) < 0)
		return -0x10001;
	return _senddata.id;
}

int sceSifSearchModuleByName(const char *modulename)
{
	if (_lf_bind() < 0)
		return -0x10000;
	if (_lf_version())
		return -0x10004;
	strncpy(_senddata.name, modulename, 252);
	_senddata.name[251] = 0;
	if (sceSifCallRpc(&cd, 9, 0, &_senddata, 512, &_senddata, 4, 0, 0) < 0)
		return -0x10001;
	return _senddata.id;
}

int sceSifSearchModuleByAddress(const void *addr)
{
	if (_lf_bind() < 0)
		return -0x10000;
	if (_lf_version())
		return -0x10004;
	_senddata.id = (int)addr;
	if (sceSifCallRpc(&cd, 10, 0, &_senddata, 4, &_senddata, 4, 0, 0) < 0)
		return -0x10001;
	return _senddata.id;
}

int sceSifLoadModuleBuffer(const void *addr, int args, const char *argp)
{
	int result;

	return _sceSifLoadModuleBuffer(addr, args, argp, &result);
}

int sceSifLoadStartModuleBuffer(const void *addr, int args, const char *argp, int *result)
{
	return _sceSifLoadModuleBuffer(addr, args, argp, result);
}

int _sceSifLoadModule(const char *filename, int args, const char *argp, int *result, int fno)
{
	int ret;

	if (_lf_bind() < 0)
		return -0x10000;
	if (_lf_version())
		return -0x10004;
	strncpy(_senddata.name, filename, 252);
	_senddata.name[251] = 0;
	if (argp != 0) {
		if (args > 252) {
			memcpy(_senddata.args, argp, 252);
			_senddata.id = 252;
		} else {
			memcpy(_senddata.args, argp, args);
			_senddata.id = args;
		}
	} else {
		_senddata.args[0] = 0;
		_senddata.id = 0;
	}
	if (sceSifCallRpc(&cd, fno, 0, &_senddata, 512, &_senddata, 8, 0, 0) < 0)
		return -0x10001;
	ret = _senddata.id;
	*result = _senddata.arglen;
	return ret;
}

int sceSifLoadModule(const char *filename, int args, const char *argp)
{
	int result;

	return _sceSifLoadModule(filename, args, argp, &result, 0);
}

int sceSifLoadStartModule(const char *filename, int args, const char *argp, int *result)
{
	return _sceSifLoadModule(filename, args, argp, result, 0);
}

typedef struct {
	u_int epc;
	u_int gp;
	u_int sp;
	u_int dummy;
} sceExecData;

int _sceSifLoadElfPart(const char *name, const char *secname, sceExecData *data, int fno)
{
	if (_lf_bind() < 0)
		return -0x10000;
	if (_lf_version())
		return -0x10004;
	strncpy(_senddata.name, name, 252);
	_senddata.name[251] = 0;
	strncpy(_senddata.args, secname, 252);
	_senddata.args[251] = 0;
	if (sceSifCallRpc(&cd, fno, 0, &_senddata, 512, &_senddata, 16, 0, 0) < 0)
		return -0x10001;
	if (_senddata.id == 0)
		return -0x10003;
	data->epc = _senddata.id;
	data->gp = _senddata.arglen;
	return 0;
}

int sceSifLoadElfPart(const char *name, const char *secname, sceExecData *data)
{
	return _sceSifLoadElfPart(name, secname, data, 1);
}

int sceSifLoadElf(const char *name, sceExecData *data)
{
	return _sceSifLoadElfPart(name, "all", data, 1);
}

int sceSifGetIopAddr(u_int addr, void *val, u_int type)
{
	if (_lf_bind() < 0)
		return -0x10000;
	if (type > 2)
		return -0x10002;
	_senddata.id = addr;
	_senddata.arglen = type;
	if (sceSifCallRpc(&cd, 3, 0, &_senddata, 32, &_senddata, 32, 0, 0) < 0)
		return -0x10001;
	if (type == 0)
		*(unsigned char *)val = *(unsigned char *)&_senddata.id;
	else if (type == 1)
		*(unsigned short *)val = *(unsigned short *)&_senddata.id;
	else if (type == 2)
		*(u_int *)val = _senddata.id;
	else
		return -0x10002;
	return 0;
}

int sceSifSetIopAddr(u_int addr, void *val, u_int type)
{
	if (_lf_bind() < 0)
		return -0x10000;
	_senddata.id = addr;
	_senddata.arglen = type;
	if (type == 0)
		*(unsigned char *)_senddata.name = *(unsigned char *)val;
	else if (type == 1)
		*(unsigned short *)_senddata.name = *(unsigned short *)val;
	else if (type == 2)
		*(u_int *)_senddata.name = *(u_int *)val;
	else
		return -0x10002;
	if (sceSifCallRpc(&cd, 2, 0, &_senddata, 32, &_senddata, 16, 0, 0) < 0)
		return -0x10001;
	return 0;
}
