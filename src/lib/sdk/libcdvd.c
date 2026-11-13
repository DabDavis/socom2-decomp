/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libcdvd's first object (MAIN 0x18DB90-0x18F178): Sony's SDK, the demo's object 0x16E490 (C), its
 * functions in the demo's order -- the delay thread, the N-command callback and its callback
 * thread, the power-off callback, the search, the N- and S-command pre-checks, sync, init and
 * disk-ready. Prebuilt with the SDK's ee-gcc 2.9-991111-01, the dtls13010 build (README "The GCC
 * front"). Names are SOCOM 1's (s1:exact and the demo's globals, data.txt; libcdvd.h). OPEN:
 * sceCdInit and the accessor func_0018E9E8 (each diagnosed in place). The functions appear in the target's order, which is the demo's but for the accessor. */

#include "libcdvd.h"

static void CB_DelayTh(int id, u_short time, void *arg)
{
	iSignalSema((int)arg);
	__asm__ volatile ("sync.l; ei");
}

void sceCdDelayThread(u_short usec)
{
	struct SemaParam sema;
	int sid;

	sema.initCount = 0;
	sema.maxCount = 1;
	sema.option = 0;
	sid = CreateSema(&sema);
	SetAlarm(usec, CB_DelayTh, (void *)sid);
	WaitSema(sid);
	DeleteSema(sid);
}

void *sceCdCallback(void (*func)(int))
{
	void *old;
	int oldi;

	if (sceCdSync(1) != 0)
		return 0;
	oldi = DIntr();
	old = sceCdCbfunc;
	sceCdCbfunc = func;
	__asm__ volatile ("sw $28, %0" : "=m" (data_001D5F84));
	if (oldi)
		EIntr();
	return old;
}

void _sceCd_cd_callback(int *data)
{
	sceCdCbfunc_num = data[0];
	sceCdCbfunc_number = sceCdCbfunc_num;
	if (sceCdCbfunc_num == 11) {
		sceCdCbfunc_num = 0;
		_sceCd_c_cb_sem = 0;
		return;
	}
	iSignalSema(_sceCd_ncmd_semid);
	if (data_001CA854 != 0 && sceCdCbfunc != 0)
		iSignalSema(cb_semid);
	else
		_sceCd_c_cb_sem = 0;
	sceCdCbfunc_num = 0;
}

static void _Cdvd_cbLoop(void)
{
	int old_gp;

	for (;;) {
		WaitSema(cb_semid);
		if (sceCdCbfunc_num == -1) {
			_sceCd_c_cb_sem = 0;
			sceCdCbfunc_num = 0;
			data_001CA854 = 0;
			data_001D5F94 = 0;
			ExitDeleteThread();
		}
		if (SCE_CD_debug > 0)
			scePrintf("sceCdCbfunc= %d sceCdCbfunc_num= %d\n", sceCdCbfunc, sceCdCbfunc_number);
		if (sceCdCbfunc != 0 && sceCdCbfunc_number != 0) {
			__asm__ volatile ("move %0, $28\n\tlw $28, %1" : "=r" (old_gp) : "m" (data_001D5F84));
			sceCdCbfunc(sceCdCbfunc_number);
			__asm__ volatile ("move $28, %0" : : "r" (old_gp));
		}
		_sceCd_c_cb_sem = 0;
	}
}

int sceCdInitEeCB(int prio, void *stack, int stacksize)
{
	int ret = 1;

	if (data_001CA854 == 0) {
		my_thid = GetThreadId();
		ReferThreadStatus(my_thid, &data_001D5FA0);
		data_001D5FD0.stackSize = stacksize;
		data_001D5FD0.entry = _Cdvd_cbLoop;
		data_001D5FD0.stack = stack;
		data_001D5FD0.initPriority = prio;
		data_001D5FD0.gpReg = 0;
		data_001CA854 = CreateThread(&data_001D5FD0);
		StartThread(data_001CA854, 0);
	} else {
		ChangeThreadPriority(data_001CA854, prio);
		ret = 0;
	}
	return ret;
}

struct cd_read_intr {
	int size1;
	int size2;
	char *dest1;
	char *dest2;
	char buf1[64];
	char buf2[64];
};

void _sceCd_cd_read_intr(void *data)
{
	struct cd_read_intr *p = (struct cd_read_intr *)((u_int)data | 0x20000000);
	char *d;
	char *du;
	int i;

	if (p->size1 > 0) {
		du = (char *)((u_int)p->dest1 | 0x20000000);
		d = p->dest1;
		for (i = 0; i < p->size1; i++)
			d[i] = du[i] = p->buf1[i];
	}
	if (p->size2 > 0) {
		du = (char *)((u_int)p->dest2 | 0x20000000);
		d = p->dest2;
		for (i = 0; i < p->size2; i++)
			d[i] = du[i] = p->buf2[i];
	}
	_sceCd_cd_callback((int *)&sceCdCbfunc_num);
}

static void cmd_sem_init(void)
{
	struct SemaParam sema;

	if (_sceCd_ncmd_semid != -1 && _sceCd_scmd_semid != -1 && data_001CA870 != -1)
		return;
	sema.initCount = 1;
	sema.maxCount = 1;
	sema.option = 0;
	_sceCd_ncmd_semid = CreateSema(&sema);
	_sceCd_scmd_semid = CreateSema(&sema);
	data_001CA870 = CreateSema(&sema);
	sema.initCount = 0;
	cb_semid = CreateSema(&sema);
	_sceCd_c_cb_sem = 0;
}

static void cdvd_exit(void)
{
	int oldi;

	if (data_001CA854 != 0) {
		sceCdCbfunc_num = -1;
		SignalSema(cb_semid);
	}
	DeleteSema(_sceCd_ncmd_semid);
	DeleteSema(_sceCd_scmd_semid);
	DeleteSema(data_001CA870);
	DeleteSema(cb_semid);
	oldi = DIntr();
	sceSifRemoveCmdHandler(0x80000012);
	if (oldi)
		EIntr();
}

static int PowerOffCB(void);

void *sceCdPOffCallback(void (*func)(void *), void *addr)
{
	void *old;
	int oldi;

	if (_icmd_bind < 0)
		PowerOffCB();
	oldi = DIntr();
	old = data_001D5F88;
	data_001D5F90 = addr;
	data_001D5F88 = func;
	__asm__ volatile ("sw $28, %0" : "=m" (data_001D5F8C));
	if (oldi)
		EIntr();
	return old;
}

static void _sceCd_Poff_Intr(void *data)
{
	int old_gp;

	if (data_001D5F88 != 0 && Init_seq == 0) {
		__asm__ volatile ("move %0, $28\n\tlw $28, %1" : "=r" (old_gp) : "m" (data_001D5F8C));
		data_001D5F88(data_001D5F90);
		__asm__ volatile ("move $28, %0" : : "r" (old_gp));
	}
}

static int PowerOffCB(void)
{
	int oldi;

	Init_seq = 1;
	oldi = DIntr();
	sceSifAddCmdHandler(0x80000012, _sceCd_Poff_Intr, 0);
	if (oldi)
		EIntr();
	Init_seq = 0;
	_icmd_bind = 1;
	return 1;
}

/* The search (GCC r35a): a file found by path on a layer, through its own RPC client (0x80000597)
 * and the older inline pre-check -- the semaphore polled, the command kept, the caller's status
 * read, sync, the bind loop with a delay on each failure (as _sceCd_ncmd_prechk's) -- the path
 * copied to the request's 256 bytes in round 17a's peeled shape (the byte stored, then tested), the
 * sceCdlFILE (36 bytes: SDK 2.7's has a flag word after the date) copied back from the uncached
 * request, three debug lines each its own test (CSE threads the first failure past all three), the
 * reply read before the semaphore is signalled. ps2sdk's sceCdSearchFile is the same call. Read off
 * the allocation (tools/ccdump.py -f): the bind flag's %hi is the first saved register after the
 * pointers (s1), which needs the flag's clear INSIDE the loop -- flow counts a reference in a loop
 * twice (4 refs over 68 insns, 1176, above the path's 1066; after the loop 3 refs, 441, s6) -- and
 * the delays count in a variable of their own (v0, the copy's index a1; one `i` for both: 9 words);
 * the nine saved registers leave `layer` on the stack, as the target's. */
typedef struct {
	u_int lsn;
	u_int size;
	char name[16];
	u_char date[8];
	u_int flag;
} sceCdlFILE;

typedef struct {
	sceCdlFILE file;
	char name[256];
	void *dest;
	int layer;
} SearchFilePkt;

extern void *memcpy(void *, const void *, unsigned int);
extern int _sf_bind;
extern SearchFilePkt _sf_data;
extern int sfrdata[];
extern sceSifClientData cdrdsf;

int sceCdLayerSearchFile(sceCdlFILE *fp, const char *name, int layer)
{
	int i, j, ret;

	cmd_sem_init();
	if (_sceCd_ncmd_semid != PollSema(_sceCd_ncmd_semid))
		return 0;
	ncmd_sema_keep_cmd = 1;
	ReferThreadStatus(my_thid, &data_001D5FA0);
	if (sceCdSync(1)) {
		SignalSema(_sceCd_ncmd_semid);
		return 0;
	}
	sceSifInitRpc(0);
	if (_sf_bind < 0) {
		while (1) {
			if (sceSifBindRpc(&cdrdsf, 0x80000597, 0) < 0) {
				if (SCE_CD_debug > 0)
					scePrintf("Libcdvd bind err CdSearchFile\n");
				j = 0x100000;
				while (j--)
					;
				continue;
			}
			if (cdrdsf.serve != 0) {
				_sf_bind = 0;
				break;
			}
			j = 0x100000;
			while (j--)
				;
		}
	}
	for (i = 0; i < 256; i++) {
		_sf_data.name[i] = name[i];
		if (_sf_data.name[i] == 0)
			break;
	}
	if (i == 256)
		_sf_data.name[255] = 0;
	if (SCE_CD_debug > 0)
		scePrintf("ee call cmd search %s\n", _sf_data.name);
	_sf_data.layer = layer;
	_sf_data.dest = &_sf_data;
	sceSifWriteBackDCache(&_sf_data, sizeof(SearchFilePkt));
	if (sceSifCallRpc(&cdrdsf, 0, 0, &_sf_data, sizeof(SearchFilePkt), sfrdata, 4, 0, 0) < 0) {
		SignalSema(_sceCd_ncmd_semid);
		return 0;
	}
	memcpy(fp, (void *)((u_int)&_sf_data | 0x20000000), sizeof(sceCdlFILE));
	if (SCE_CD_debug > 0)
		scePrintf("search name %s\n", fp->name);
	if (SCE_CD_debug > 0)
		scePrintf("search size %d\n", fp->size);
	if (SCE_CD_debug > 0)
		scePrintf("search loc lbn %d\n", fp->lsn);
	ret = *(int *)((u_int)sfrdata | 0x20000000);
	SignalSema(_sceCd_ncmd_semid);
	return ret;
}

int sceCdSearchFile(sceCdlFILE *fp, const char *name)
{
	return sceCdLayerSearchFile(fp, name, 0);
}

int _sceCd_ncmd_prechk(int cmd)
{
	int i;

	cmd_sem_init();
	if (_sceCd_ncmd_semid != PollSema(_sceCd_ncmd_semid)) {
		if (SCE_CD_debug > 0)
			scePrintf("Ncmd fail sema cur_cmd:%d keep_cmd:%d\n", cmd, ncmd_sema_keep_cmd);
		return 0;
	}
	ncmd_sema_keep_cmd = cmd;
	ReferThreadStatus(my_thid, &data_001D5FA0);
	if (sceCdSync(1)) {
		SignalSema(_sceCd_ncmd_semid);
		return 0;
	}
	sceSifInitRpc(0);
	if (_ncmd_bind >= 0)
		return 1;
	while (1) {
		if (sceSifBindRpc(&_sceCd_cd_ncmd, 0x80000595, 0) < 0) {
			if (SCE_CD_debug > 0)
				scePrintf("Libcdvd bind err N CMD\n");
			i = 0x100000;
			while (i--)
				;
			continue;
		}
		if (_sceCd_cd_ncmd.serve != 0)
			break;
		i = 0x100000;
		while (i--)
			;
	}
	_ncmd_bind = 0;
	return 1;
}

int sceCdNcmdDiskReady(void)
{
	int ret;

	if (_sceCd_ncmd_prechk(2) == 0)
		return 0;
	if (sceSifCallRpc(&_sceCd_cd_ncmd, 14, 0, 0, 0, data_001CA8C0, 4, 0, 0) < 0) {
		SignalSema(_sceCd_ncmd_semid);
		return 0;
	}
	ret = *(int *)((u_int)data_001CA8C0 | 0x20000000);
	SignalSema(_sceCd_ncmd_semid);
	return ret;
}

int sceCdSync(int mode)
{
	if (mode == 0) {
		if (SCE_CD_debug > 0)
			scePrintf("N cmd wait\n");
		while (_sceCd_c_cb_sem || sceSifCheckStatRpc(&_sceCd_cd_ncmd))
			sceCdDelayThread(60);
		return 0;
	}
	if (_sceCd_c_cb_sem || sceSifCheckStatRpc(&_sceCd_cd_ncmd))
		return 1;
	return 0;
}

int sceCdSyncS(int mode)
{
	if (mode == 0) {
		if (SCE_CD_debug > 0)
			scePrintf("S cmd wait\n");
		while (sceSifCheckStatRpc(&_sceCd_cd_scmd))
			sceCdDelayThread(60);
		return 0;
	}
	return sceSifCheckStatRpc(&_sceCd_cd_scmd);
}

int _sceCd_scmd_prechk(int cmd)
{
	int i;

	cmd_sem_init();
	if (_sceCd_scmd_semid != PollSema(_sceCd_scmd_semid)) {
		if (SCE_CD_debug > 0)
			scePrintf("Scmd fail sema cur_cmd:%d keep_cmd:%d\n", cmd, scmd_sema_keep_cmd);
		return 0;
	}
	scmd_sema_keep_cmd = cmd;
	ReferThreadStatus(my_thid, &data_001D5FA0);
	if (sceCdSyncS(1)) {
		SignalSema(_sceCd_scmd_semid);
		return 0;
	}
	sceSifInitRpc(0);
	if (_scmd_bind >= 0)
		return 1;
	while (1) {
		if (sceSifBindRpc(&_sceCd_cd_scmd, 0x80000593, 0) < 0) {
			if (SCE_CD_debug > 0)
				scePrintf("Libcdvd bind err S cmd\n");
			i = 0x100000;
			while (i--)
				;
			continue;
		}
		if (_sceCd_cd_scmd.serve != 0)
			break;
		i = 0x100000;
		while (i--)
			;
	}
	_scmd_bind = 0;
	return 1;
}

/* A newer call the demo lacks -- the N and S commands' semaphores, buffers and clients by id (GCC
 * round 37a; open since round 13a). The target tests the eight ids in an order no switch keeps
 * (0x8001, 0x8003, 0x8002, 0x8004, 0x8011, 0x8013, 0x8012, 0x8014: a switch's case list is sorted,
 * and this build makes a jump table of it anyway, eight cases in a range of 20), every test first,
 * each a beq to its arm, the arms after in reverse and the value stored once: ONE conditional
 * expression, nested -- expand_expr emits each COND_EXPR's test with a jump to its arm and the arms
 * as the nesting unwinds. An if-else chain interleaves the arms with the tests (0xB4), and gotos to
 * the arms in either order are 31 and 43 words off. */
int func_0018E9E8(int id, u_int *out)
{
	*out = (id == 0x8001) ? _sceCd_ncmd_semid
	     : (id == 0x8003) ? (u_int)data_001CA8C0
	     : (id == 0x8002) ? (u_int)_sceCd_ncmdsdata
	     : (id == 0x8004) ? (u_int)&_sceCd_cd_ncmd
	     : (id == 0x8011) ? _sceCd_scmd_semid
	     : (id == 0x8013) ? (u_int)_sceCd_scmdrdata
	     : (id == 0x8012) ? (u_int)data_001CBE80
	     : (id == 0x8014) ? (u_int)&_sceCd_cd_scmd
	     : 0;
	return 1;
}

extern int _sf_bind;                            /* the demo's labels (data.txt) */
extern int _it_bind;
extern int _scmd_dr_bind;
extern int _sceCd_ee_read_mode;
extern int c_cnt asm("c_cnt.60");               /* sceCdInit's static count, the demo's label */
extern sceSifClientData cdit;                   /* the init and disk-ready clients, their requests */
extern sceSifClientData cddr;
extern int _i_mode;
extern int _dr_mode;
extern char data_001CC2C0[];                    /* the newer disk-ready call's reply */

/* OPEN (right size since GCC round 83, 36 words, registers and the prologue's store schedule): the
 * call made INSIDE the bind loop, under `if (cdit.serve != 0)`, its break after the failure test --
 * loop.c hoists _i_mode's address, the reply's %hi and _it_bind's %hi out (s2, s8, s4), which the
 * old after-the-loop shape never had; the reply's words read through the pointer passed to the call
 * (CSE's related value, `addiu v0,s0,12`), in offset order; `v1 / 256 < 2 || v2 / 256 < 2` one
 * assignment (bnezl into it); Init_seq volatile (libcdvd.h), so GCSE stops carrying its %hi from the
 * first store into a callee-saved register. Left: c_cnt's %hi in s4 where the target has s5 (its
 * priority 833 against _it_bind's 677), the six -1 stores' bases; my_thid volatile takes 8 more but
 * loses sceCdInitEeCB. */
int sceCdInit(int mode)
{
	int ret;
	int r;
	int i;
	int v1, v2, v3;
	char *rd;

	if (sceCdSyncS(1))
		return 0;
	sceSifInitRpc(0);
	my_thid = GetThreadId();
	Init_seq = 1;
	_icmd_bind = -1;
	_sf_bind = -1;
	_ncmd_bind = -1;
	_scmd_bind = -1;
	_scmd_dr_bind = -1;
	_sceCd_ee_read_mode = 0;
	c_cnt++;
	_it_bind = -1;
	while (1) {
		if ((r = sceSifBindRpc(&cdit, 0x80000592, 0)) < 0) {
			if (SCE_CD_debug > 0)
				scePrintf("Libcdvd bind err %d CD_Init %d\n", r, c_cnt);
			i = 0x100000;
			while (i--)
				;
			continue;
		}
		if (cdit.serve != 0) {
			_i_mode = mode;
			_it_bind = 0;
			sceSifWriteBackDCache(&_i_mode, 4);
			if (sceSifCallRpc(&cdit, 0, 0, &_i_mode, 4, rd = _sceCd_scmdrdata, 16, 0, 0) < 0) {
				Init_seq = 0;
				return 0;
			}
			break;
		}
		i = 0x100000;
		while (i--)
			;
	}
	v1 = *(int *)((u_int)&rd[4] | 0x20000000);
	v2 = *(int *)((u_int)&rd[8] | 0x20000000);
	v3 = *(int *)((u_int)&rd[12] | 0x20000000);
	ret = 1;
	if (v3 != 255) {
		if (v3 == 254)
			SCE_CD_debug = 1;
		else if (v1 / 256 < 2 || v2 / 256 < 2)
			ret = 2;
	}
	Init_seq = 0;
	switch (mode) {
	case 5:
		if (SCE_CD_debug > 0)
			scePrintf("Libcdvd Exit\n");
		cdvd_exit();
		_sceCd_ncmd_semid = -1;
		_sceCd_scmd_semid = -1;
		cb_semid = -1;
		return ret;
	case 0:
	case 1:
		break;
	}
	cmd_sem_init();
	PowerOffCB();
	return ret;
}

/* The target gives the bind flag's %hi s1, mode s2, the semaphore's s3, the request's s4, the
 * client's s5, the debug flag's s6, the reply's s7 -- global-alloc's order, which two shapes give
 * (GCC r35a; r32a had the structure, 20 words in their registers): the flag cleared INSIDE the bind
 * loop (a reference in a loop counts twice: 4 refs over 66 insns, 1212) and the failure written in
 * both places, each copy a reference to mode and to the semaphore's %hi (4 and 6: 1081 and 833, over
 * the request's 757) -- cross-jumping merges the copies after reload into the second, after the
 * call, the SyncS test branching into it (one copy reached by a goto has the layout and not the
 * references; two copies without the in-loop clear are 31 words off). */
int sceCdDiskReady(int mode)
{
	int i;
	int ret;

	if (SCE_CD_debug > 0)
		scePrintf("OLD DiskReady Call\n");
	cmd_sem_init();
	if (_sceCd_scmd_semid != PollSema(_sceCd_scmd_semid))
		return 6;
	if (sceCdSyncS(1)) {
		SignalSema(_sceCd_scmd_semid);
		return mode != 8 ? 6 : -1;
	}
	sceSifInitRpc(0);
	if (_scmd_dr_bind < 0) {
		while (1) {
			if (sceSifBindRpc(&cddr, 0x8000059A, 0) < 0) {
				if (SCE_CD_debug > 0)
					scePrintf("Libcdvd bind err CdDiskReady\n");
				i = 0x100000;
				while (i--)
					;
				continue;
			}
			if (cddr.serve != 0) {
				_scmd_dr_bind = 0;
				break;
			}
			i = 0x100000;
			while (i--)
				;
		}
	}
	_dr_mode = mode;
	sceSifWriteBackDCache(&_dr_mode, 4);
	if (sceSifCallRpc(&cddr, 0, 0, &_dr_mode, 4, _sceCd_scmdrdata, 4, 0, 0) < 0) {
		SignalSema(_sceCd_scmd_semid);
		return mode != 8 ? 6 : -1;
	}
	if (SCE_CD_debug > 0)
		scePrintf("DiskReady ended\n");
	ret = *(int *)((u_int)_sceCd_scmdrdata | 0x20000000);
	SignalSema(_sceCd_scmd_semid);
	return ret;
}

/* The newer disk-ready call (the demo lacks it; strfile.cpp calls it): its own semaphore and server
 * 0x8000059C over the old call's client and request, falling back to the old call after 17 binds
 * that find no server. Byte-exact since GCC round 83 (open: "lazy code motion's placement" of the
 * client's %hi): the count cleared BEFORE the bind-flag test (the target's `bgez; move s1,zero`) and
 * the flag cleared inside the loop at its break, as sceCdDiskReady does -- a reference in a loop
 * counts twice, lifting the flag's %hi from s6 to s3 and the rest after it. */
int func_0018EF70(int mode)
{
	int i;
	int n;
	int ret;

	if (SCE_CD_debug > 0)
		scePrintf("NEW DiskReady Call\n");
	cmd_sem_init();
	if (data_001CA870 != PollSema(data_001CA870))
		return mode != 8 ? 6 : -1;
	sceSifInitRpc(0);
	n = 0;
	if (_scmd_dr_bind < 0) {
		while (1) {
			if (sceSifBindRpc(&cddr, 0x8000059C, 0) < 0) {
				if (SCE_CD_debug > 0)
					scePrintf("Libcdvd bind err CdDiskReady\n");
				i = 0x100000;
				while (i--)
					;
				continue;
			}
			if (cddr.serve != 0) {
				_scmd_dr_bind = 0;
				break;
			}
			if (n >= 17) {
				SignalSema(data_001CA870);
				return sceCdDiskReady(mode);
			}
			i = 0x100000;
			while (i--)
				;
			n++;
		}
	}
	_dr_mode = mode;
	sceSifWriteBackDCache(&_dr_mode, 4);
	if (sceSifCallRpc(&cddr, 0, 0, &_dr_mode, 4, data_001CC2C0, 4, 0, 0) < 0) {
		SignalSema(data_001CA870);
		return mode != 8 ? 6 : -1;
	}
	if (SCE_CD_debug > 0)
		scePrintf("DiskReady ended\n");
	ret = *(int *)((u_int)data_001CC2C0 | 0x20000000);
	SignalSema(data_001CA870);
	return ret;
}
