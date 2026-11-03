/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libcdvd's first object (MAIN 0x18DB90-0x18F178): Sony's SDK, the demo's object 0x16E490 (C), its
 * functions in the demo's order -- the delay thread, the N-command callback and its callback
 * thread, the power-off callback, the search, the N- and S-command pre-checks, sync, init and
 * disk-ready. Prebuilt with the SDK's ee-gcc 2.9-991111-01, the dtls13010 build (README "The GCC
 * front"). Names are SOCOM 1's (s1:exact and the demo's globals, data.txt; libcdvd.h). Not yet
 * written: sceCdLayerSearchFile, sceCdInit, sceCdDiskReady and the newer 0x18EF70. The functions
 * appear in the target's order, which is the demo's but for the accessor func_0018E9E8. */

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

/* OPEN (1 word over: 0xB4 for 0xB0): a newer call the demo lacks -- the N and S commands' semaphores,
 * buffers and clients by id. The target dispatches by a chain of equality tests (0x8001, 0x8003,
 * 0x8002, 0x8004, 0x8011, 0x8013, 0x8012, 0x8014) and lays the bodies after it in reverse, each
 * setting one value stored once: a switch lowered without a table. This build makes a jump table for
 * these eight cases (range 20) in source order or sorted, an int or u_int index alike; the if-chain
 * interleaves the bodies with the tests. */
int func_0018E9E8(int id, u_int *out)
{
	u_int v;

	if (id == 0x8001)
		v = _sceCd_ncmd_semid;
	else if (id == 0x8003)
		v = (u_int)data_001CA8C0;
	else if (id == 0x8002)
		v = (u_int)_sceCd_ncmdsdata;
	else if (id == 0x8004)
		v = (u_int)&_sceCd_cd_ncmd;
	else if (id == 0x8011)
		v = _sceCd_scmd_semid;
	else if (id == 0x8013)
		v = (u_int)_sceCd_scmdrdata;
	else if (id == 0x8012)
		v = (u_int)data_001CBE80;
	else if (id == 0x8014)
		v = (u_int)&_sceCd_cd_scmd;
	else
		v = 0;
	*out = v;
	return 1;
}
