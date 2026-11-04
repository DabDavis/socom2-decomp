/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libcdvd's streaming object (MAIN 0x18F7D8-0x18FCB4): the sceCdSt* calls, each one stream command
 * (the IOP's N-command RPC 9 with a 20-byte request: sector, count, buffer, command, read mode)
 * through func_0018FB50 -- INIT 5, START 1, SEEKF 9, SEEK 4, STOP 3, READ 2, PAUSE 7, RESUME 8,
 * STAT 6 (the numbers ps2sdk's reconstruction gives the same calls). Not in SOCOM 1's demo (a newer
 * libcdvd); the four that print their names are named by them (sceCdStRead, sceCdStPause,
 * sceCdStResume, sceCdStStat), the rest keep placeholders. The started flag (0x1CC3F8) and the
 * default read mode (0x1D6218) are this object's own data. The N-command semaphore is read plainly
 * here (its load fills SignalSema's delay slot; libcdvd.h's volatile declaration keeps it out). */

typedef unsigned int u_int;
typedef unsigned char u_char;

typedef struct {
	u_char trycount;
	u_char spindlctrl;
	u_char datapattern;
	u_char pad;
} sceCdRMode;

typedef struct {
	u_int lbn;
	u_int nsectors;
	void *buf;
	u_int cmd;
	sceCdRMode mode;
} CdStCmd;

typedef struct { int a[10]; } sceSifClientData;

extern int sceSifCallRpc(sceSifClientData *cd, u_int fno, u_int mode, void *send, int ssize, void *recv,
			 int rsize, void (*end)(void *), void *ea);
extern void sceSifWriteBackDCache(void *ptr, int size);
extern int SignalSema(int sema_id);
extern int scePrintf(const char *fmt, ...);
extern int _sceCd_ncmd_prechk(int cmd);
extern int sceCdDelayThread(int ticks);

extern int SCE_CD_debug;
extern int _sceCd_ncmd_semid;
extern sceSifClientData _sceCd_cd_ncmd;
extern CdStCmd _sceCd_ncmdsdata;
extern int data_001CA8C0;			/* the N-command reply */
extern int data_001CC3F8;			/* streaming started */
extern sceCdRMode data_001D6218;

int func_0018FB50(u_int lbn, u_int nsectors, void *buf, u_int cmd, sceCdRMode *rm);

int func_0018F7D8(u_int bufmax, u_int bankmax, void *buffer)
{
	data_001CC3F8 = 0;
	return func_0018FB50(bufmax, bankmax, buffer, 5, &data_001D6218);
}

int func_0018F808(u_int lbn, sceCdRMode *mode)
{
	data_001CC3F8 = 1;
	return func_0018FB50(lbn, 0, 0, 1, mode);
}

int func_0018F840(u_int lbn)
{
	return func_0018FB50(lbn, 0, 0, 9, &data_001D6218);
}

int func_0018F870(u_int lbn)
{
	return func_0018FB50(lbn, 0, 0, 4, &data_001D6218);
}

int func_0018F8A0(void)
{
	data_001CC3F8 = 0;
	return func_0018FB50(0, 0, 0, 3, &data_001D6218);
}

/* OPEN (0x190 against 0x188, 65 words): laid out as the target's -- the blocking loop inline, the
 * single read last -- but each stream call's result is copied out of v0 (`move a2,v0`) before
 * it is split, where the target splits v0 itself (i into s1, the error half into s0, mode's
 * register), and the hoisted %hi's and `last` take s5-s7 in another order; seven spellings of the
 * split, the declarations and last's assignment alike (build/scratch/gcc/vf/v_st.c, GCC r22a). */
int sceCdStRead(u_int size, u_int *buf, u_int mode, u_int *err)
{
	u_int rbuff, i, e, ret, last;

	if (SCE_CD_debug > 0)
		scePrintf("sceCdStRead call read size= %d mode= %d\n", size, mode);
	if (data_001CC3F8 == 0)
		return 0;
	last = 0;
	rbuff = 0;
	sceSifWriteBackDCache(buf, size << 11);
	if (mode != 0) {
		do {
			ret = func_0018FB50(0, size - rbuff, (char *)buf + (rbuff << 11), 2, &data_001D6218);
			i = ret & 0xffff;
			e = ret >> 16;
			rbuff += i;
			if (e) {
				last = e;
				if (SCE_CD_debug > 0)
					scePrintf("sceCdStRead BLK Read cur_size= %d read_size= %d req_size= %d err 0x%x\n",
						  rbuff, i, size, e);
			} else if (i == 0)
				sceCdDelayThread(8);
		} while (rbuff != size && (e == 0 || i != 0));
		if (SCE_CD_debug > 0)
			scePrintf("sceCdStRead BLK Read Ended\n");
		*err = last;
	} else {
		ret = func_0018FB50(0, size, buf, 2, &data_001D6218);
		rbuff = ret & 0xffff;
		*err = ret >> 16;
	}
	return rbuff;
}

int sceCdStPause(void)
{
	data_001CC3F8 = 0;
	if (SCE_CD_debug > 0)
		scePrintf("sceCdStPause call\n");
	return func_0018FB50(0, 0, 0, 7, &data_001D6218);
}

int sceCdStResume(void)
{
	data_001CC3F8 = 1;
	if (SCE_CD_debug > 0)
		scePrintf("sceCdStResume call\n");
	return func_0018FB50(0, 0, 0, 8, &data_001D6218);
}

int sceCdStStat(void)
{
	if (SCE_CD_debug > 0)
		scePrintf("sceCdStStat call\n");
	return func_0018FB50(0, 0, 0, 6, &data_001D6218);
}

int func_0018FB50(u_int lbn, u_int nsectors, void *buf, u_int cmd, sceCdRMode *rm)
{
	CdStCmd *p = &_sceCd_ncmdsdata;
	int ret;

	if (_sceCd_ncmd_prechk(15) == 0)
		return 0;
	if (SCE_CD_debug > 0)
		scePrintf("call cdreadstm call\n");
	p->lbn = lbn;
	p->nsectors = nsectors;
	p->buf = buf;
	p->cmd = cmd;
	if (rm) {
		p->mode.trycount = rm->trycount;
		p->mode.spindlctrl = rm->spindlctrl;
		p->mode.datapattern = rm->datapattern;
	}
	if (SCE_CD_debug > 0)
		scePrintf("call cdreadstm cmd\n");
	sceSifWriteBackDCache(p, 20);
	if (sceSifCallRpc(&_sceCd_cd_ncmd, 9, 0, p, 20, &data_001CA8C0, 4, 0, 0) < 0) {
		SignalSema(_sceCd_ncmd_semid);
		return 0;
	}
	if (SCE_CD_debug > 0)
		scePrintf("cdread end\n");
	ret = *(int *)((u_int)&data_001CA8C0 | 0x20000000);
	SignalSema(_sceCd_ncmd_semid);
	return ret;
}
