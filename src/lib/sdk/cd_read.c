/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libcdvd's sceCdRead object (MAIN 0x18F178): Sony's SDK, one function, the demo's own text
 * (s1:exact) -- N-command 1 sent without waiting: the sector, the count, the buffer and the read
 * mode, with the interrupt-side state the IOP fills (the 144-byte _sceCd_rd_intr_data and the
 * current position) and the bytes a sector holds by the data pattern (2048, 2328, 2340); the
 * buffer is written back unless the EE read mode's bit 1 says uncached, the drive's readiness is
 * asked unless its bit 0 says not to, and the read's end is the callback thread's
 * (_sceCd_cd_read_intr). GCC round 34a: two lines read off the target -- the data pattern's switch
 * names case 0 with the default (the target's case tree tests `< 2` between its two compares: {1, 2}
 * alone compares twice), and the EE read mode is volatile (the default's branch to the join keeps
 * an empty slot: a plain load of it there filled the slot and branched past it). */

#include "libcdvd.h"

typedef struct {
	u_char trycount;
	u_char spindlctrl;
	u_char datapattern;
	u_char pad;
} sceCdRMode;

typedef struct {
	u_int lbn;
	u_int sectors;
	void *buf;
	sceCdRMode mode;
	void *intr_data;
	void *cur_pos;
} CdReadCmd;

extern int sceCdNcmdDiskReady(void);
extern int _sceCd_ncmd_prechk(int cmd);
extern void _sceCd_cd_read_intr(void *data);
extern volatile int _sceCd_ee_read_mode;
extern char _sceCd_rd_intr_data[];
extern int _sceCd_Read_cur_pos[];

int sceCdRead(u_int lbn, u_int sectors, void *buf, sceCdRMode *mode)
{
	CdReadCmd *p = (CdReadCmd *)_sceCd_ncmdsdata;
	int size;

	if (!(_sceCd_ee_read_mode & 1)) {
		if (sceCdNcmdDiskReady() == 6)
			return 0;
	}
	if (_sceCd_ncmd_prechk(4) == 0)
		return 0;
	p->lbn = lbn;
	p->sectors = sectors;
	p->buf = buf;
	p->mode.trycount = mode->trycount;
	p->mode.spindlctrl = mode->spindlctrl;
	p->mode.datapattern = mode->datapattern;
	p->intr_data = _sceCd_rd_intr_data;
	p->cur_pos = _sceCd_Read_cur_pos;
	switch (mode->datapattern) {
	case 1:
		size = sectors * 2328;
		break;
	case 2:
		size = sectors * 2340;
		break;
	case 0:
	default:
		size = sectors << 11;
		break;
	}
	_sceCd_Read_cur_pos[0] = 0;
	if (!(_sceCd_ee_read_mode & 2))
		sceSifWriteBackDCache(buf, size);
	sceSifWriteBackDCache(_sceCd_rd_intr_data, 144);
	sceSifWriteBackDCache(p, 24);
	sceSifWriteBackDCache(_sceCd_Read_cur_pos, 4);
	if (SCE_CD_debug > 0)
		scePrintf("call cdread cmd\n");
	sceCdCbfunc_num = 1;
	_sceCd_c_cb_sem = 1;
	if (sceSifCallRpc(&_sceCd_cd_ncmd, 1, 1, p, 24, 0, 0, _sceCd_cd_read_intr, _sceCd_rd_intr_data) < 0) {
		sceCdCbfunc_num = 0;
		_sceCd_c_cb_sem = 0;
		SignalSema(_sceCd_ncmd_semid);
		return 0;
	}
	if (SCE_CD_debug > 0)
		scePrintf("cdread end\n");
	return 1;
}
