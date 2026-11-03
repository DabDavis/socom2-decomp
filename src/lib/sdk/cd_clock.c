/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libcdvd's sceCdReadClock object (MAIN 0x18F6E0): Sony's SDK, one function as the demo's object 0x16FBE0 is (RPC 1):
 * an S command over libcdvd's RPC client -- the pre-check takes the S semaphore, the reply is read
 * uncached from _sceCd_scmdrdata. Name SOCOM 1's (s1:exact). */

#include "libcdvd.h"

int sceCdReadClock(sceCdCLOCK *rtc)
{
	int ret;

	if (_sceCd_scmd_prechk(15) == 0)
		return 0;
	if (SCE_CD_debug > 0)
		scePrintf("Libcdvd call Clock read 1\n");
	if (sceSifCallRpc(&_sceCd_cd_scmd, 1, 0, 0, 0, _sceCd_scmdrdata, 16, 0, 0) < 0) {
		SignalSema(_sceCd_scmd_semid);
		return 0;
	}
	*rtc = *(sceCdCLOCK *)((u_int)(_sceCd_scmdrdata + 4) | 0x20000000);
	if (SCE_CD_debug > 0)
		scePrintf("Libcdvd call Clock read 2\n");
	ret = *(int *)((u_int)_sceCd_scmdrdata | 0x20000000);
	SignalSema(_sceCd_scmd_semid);
	return ret;
}
