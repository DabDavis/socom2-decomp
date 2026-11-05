/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libcdvd's seek object (MAIN 0x18F358, between sceCdRead's and sceCdGetDiskType's objects): Sony's
 * SDK, one function, not in SOCOM 1 (the demo's libcdvd has no such object, so the row keeps its
 * placeholder: N-command 5 is ps2sdk's reconstruction's CD_NCMD_SEEK, sceCdSeek(lbn), whose shape
 * this is -- the drive's readiness, the N pre-check, the sector number sent without waiting, the
 * completion left to the callback thread, which is told the command (sceCdCbfunc_num 4) before the
 * call and cleared again when the call fails). GCC round 34a. */

#include "libcdvd.h"

extern int sceCdNcmdDiskReady(void);
extern int _sceCd_ncmd_prechk(int cmd);
extern void _sceCd_cd_callback(void *data);

int func_0018F358(u_int lbn)
{
	u_int *sdata = (u_int *)_sceCd_ncmdsdata;

	if (sceCdNcmdDiskReady() == 6)
		return 0;
	if (_sceCd_ncmd_prechk(9) == 0)
		return 0;
	*sdata = lbn;
	sceSifWriteBackDCache(sdata, 4);
	sceCdCbfunc_num = 4;
	_sceCd_c_cb_sem = 1;
	if (sceSifCallRpc(&_sceCd_cd_ncmd, 5, 1, sdata, 4, 0, 0, _sceCd_cd_callback, (void *)&sceCdCbfunc_num) < 0) {
		sceCdCbfunc_num = 0;
		_sceCd_c_cb_sem = 0;
		SignalSema(_sceCd_ncmd_semid);
		return 0;
	}
	return 1;
}
