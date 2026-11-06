/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libkernl's IOP heap client (MAIN 0x1AAA78-0x1AAFC4): Sony's SDK 2.7, the demo's object 0x162370
 * (C), its eleven functions in the same order. Each call is one RPC to the IOP heap server
 * (0x80000003) through the static client data cd, its argument words in sdata and the reply in
 * rdata (64-byte aligned, for the SIF DMA); nothing is sent before sceSifInitIopHeap has bound
 * (_bind, -1 until then). The statics carry the demo's names (its symbol table keeps them:
 * _bind in .data; cd, rdata, sdata, _lih_data in .bss). Names are SOCOM 1's (s1:exact). The query
 * calls return unsigned values (their failure is lui/ori 0xffffffff, an int's -1 would be li).
 *
 * sceSifInitIopHeap (GCC round 42a; open since round 11a, 3 words) stores _bind inside the serve
 * test's arm and breaks: `if (cd.serve != 0) { _bind = 0; break; }`, `return 0` after the loop. With
 * the store after the loop, the return's clear sat in the store's block and sched1 issued it with
 * the %hi (no dependence, ISSUE_RATE 2), so the %hi took v1; in the arm the clear is the loop exit's
 * own block and follows the store, the %hi in v0 as the target's. Round 18a read this (byte-exact
 * as written only under -fno-schedule-insns) as the vendor's cc1 differing from dtls13010's, with
 * fileio's no-wait PRE and libscf's adddate GCSE -- adddate was a source shape too (round 42a: the
 * year read into a local), so of the three only fileio's is left to explain.
 * cd and rdata are data.txt's cd@iopheap and rdata@iopheap (round 17a: the IOP reset and loadfile
 * objects label statics of the same names). */

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

#define	SIF_IOPHEAP	0x80000003

static int _bind = -1;
static sceSifClientData cd __attribute__((aligned(64)));
static u_int rdata[16] __attribute__((aligned(64)));
static u_int sdata[16] __attribute__((aligned(64)));
static struct {
	void *addr;
	char name[252];
} _lih_data __attribute__((aligned(64)));

int sceSifInitIopHeap(void)
{
	int i;

	while (1) {
		if (sceSifBindRpc(&cd, SIF_IOPHEAP, 0) < 0)
			return -1;
		if (cd.serve != 0) {
			_bind = 0;
			break;
		}
		i = 0x100000;
		while (i--)
			;
	}
	return 0;
}

void *sceSifAllocIopHeap(int size)
{
	if (_bind < 0)
		return 0;
	sdata[0] = size;
	if (sceSifCallRpc(&cd, 1, 0, sdata, 4, rdata, 4, 0, 0) < 0)
		return 0;
	return (void *)rdata[0];
}

void *sceSifAllocSysMemory(int type, int size, void *addr)
{
	if (_bind < 0)
		return 0;
	sdata[0] = size;
	sdata[1] = type;
	sdata[2] = (u_int)addr;
	if (sceSifCallRpc(&cd, 4, 0, sdata, 12, rdata, 4, 0, 0) < 0)
		return 0;
	return (void *)rdata[0];
}

int sceSifFreeSysMemory(void *addr)
{
	if (_bind < 0)
		return 0;
	sdata[0] = (u_int)addr;
	if (sceSifCallRpc(&cd, 2, 0, sdata, 4, rdata, 4, 0, 0) < 0)
		return -1;
	return rdata[0];
}

int sceSifFreeIopHeap(void *addr)
{
	return sceSifFreeSysMemory(addr);
}

int sceSifLoadIopHeap(const char *path, void *addr)
{
	int i;

	if (_bind < 0)
		return 0;
	for (i = 0; i < 252; i++) {
		_lih_data.name[i] = path[i];
		if (_lih_data.name[i] == 0)
			break;
	}
	if (i == 252) {
		i = 251;
		_lih_data.name[i] = 0;
	}
	_lih_data.addr = addr;
	_lih_data.name[251] = 0;
	if (sceSifCallRpc(&cd, 3, 0, &_lih_data, i + 5, rdata, 4, 0, 0) < 0)
		return -1;
	return rdata[0];
}

u_int sceSifQueryMemSize(void)
{
	if (_bind < 0)
		return 0;
	if (sceSifCallRpc(&cd, 5, 0, 0, 0, rdata, 4, 0, 0) < 0)
		return -1;
	return rdata[0];
}

u_int sceSifQueryMaxFreeMemSize(void)
{
	if (_bind < 0)
		return 0;
	if (sceSifCallRpc(&cd, 6, 0, 0, 0, rdata, 4, 0, 0) < 0)
		return -1;
	return rdata[0];
}

u_int sceSifQueryTotalFreeMemSize(void)
{
	if (_bind < 0)
		return 0;
	if (sceSifCallRpc(&cd, 7, 0, 0, 0, rdata, 4, 0, 0) < 0)
		return -1;
	return rdata[0];
}

void *sceSifQueryBlockTopAddress(void *addr)
{
	if (_bind < 0)
		return 0;
	sdata[0] = (u_int)addr;
	if (sceSifCallRpc(&cd, 8, 0, sdata, 4, rdata, 4, 0, 0) < 0)
		return (void *)-1;
	return (void *)rdata[0];
}

u_int sceSifQueryBlockSize(void *addr)
{
	if (_bind < 0)
		return 0;
	sdata[0] = (u_int)addr;
	if (sceSifCallRpc(&cd, 9, 0, sdata, 4, rdata, 4, 0, 0) < 0)
		return -1;
	return rdata[0];
}
