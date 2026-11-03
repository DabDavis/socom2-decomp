/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libcdvd's sceCdGetDiskType object (MAIN 0x18F428): Sony's SDK, one function as the demo's object 0x16F9F8 is (RPC 3):
 * an S command over libcdvd's RPC client -- the pre-check takes the S semaphore, the reply is read
 * uncached from _sceCd_scmdrdata. Name SOCOM 1's (s1:exact). */

#include "libcdvd.h"

int sceCdGetDiskType(void)
{
	int ret;

	if (_sceCd_scmd_prechk(1) == 0)
		return 0;
	if (sceSifCallRpc(&_sceCd_cd_scmd, 3, 0, 0, 0, _sceCd_scmdrdata, 4, 0, 0) < 0) {
		SignalSema(_sceCd_scmd_semid);
		return 0;
	}
	ret = *(int *)((u_int)_sceCd_scmdrdata | 0x20000000);
	SignalSema(_sceCd_scmd_semid);
	return ret;
}
