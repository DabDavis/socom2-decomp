/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libkernl's module loader client (MAIN 0x1AAFC8-0x1ABD24): Sony's SDK 2.7, the demo's object
 * 0x1628C0 (C), 18 functions; written so far: the bind, the version check, the reset, the unload
 * and the two searches. Every call binds to the IOP loader (0x80000006) once, checks the server's
 * version against the library's ("2710", or the wildcard either side), then makes one RPC through
 * cd with _senddata both sent and received. The statics carry the demo's names (_bind_check and
 * _lfwildcard in .data; _senddata, cd, _lfversion in .bss). The error codes are the target's own
 * numbers: -0x10000 unbound, -0x10001 RPC failed, -0x10004 version mismatch. Names are SOCOM 1's
 * (s1:exact, _lf_version s1:callee).
 *
 * Not written yet (their rows stay open): _sceSifLoadModuleBuffer, sceSifLoadModuleBuffer,
 * sceSifLoadStartModuleBuffer, sceSifStopModule, _sceSifLoadModule, sceSifLoadModule,
 * sceSifLoadStartModule, _sceSifLoadElfPart, sceSifLoadElfPart, sceSifLoadElf, sceSifGetIopAddr,
 * sceSifSetIopAddr (the matched ones are drafts in src/auto/main).
 *
 * The version is libkernl's stamp, __ps2_klibinfo__'s last four bytes (data.txt), taken into a
 * local first (the target loads its address before _lfversion's: one order of two). A .data
 * static is compared by its own extent now (scdiff's gnu_static_size): _bind_check's -1 had read
 * as a 5-byte string into _lfwildcard's pointer.
 *
 * OPEN: _lf_bind -- the target lays the bind loop out with the unbound test's return after the
 *   version copy and the delay loop last, its `b` back carrying cd's address in the slot; the
 *   while/do/goto shapes tried come within the right size only as a goto loop (51 words, the
 *   blocks in another order). */

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
	int id;
	int pad;
	char name[252];
	char rest[512 - 260];
} _senddata __attribute__((aligned(64)));
static sceSifClientData cd;
static char _lfversion[4];

static int _lf_bind(void)
{
	int i;

	if (_bind_check >= 0)
		return 0;
	while (1) {
		if (sceSifBindRpc(&cd, SIF_LOADFILE, 0) < 0)
			return -1;
		if (cd.serve != 0)
			break;
		i = 0x100000;
		while (i--)
			;
	}
	_bind_check = 0;
	if (sceSifCallRpc(&cd, 255, 0, 0, 0, &_senddata, 4, 0, 0) < 0)
		return -0x10001;
	memcpy(_lfversion, &_senddata, 4);
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
