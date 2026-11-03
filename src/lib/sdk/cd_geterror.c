/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libcdvd's sceCdGetError object (MAIN 0x18F4C0): Sony's SDK, one function as the demo's object 0x16FA90 is (RPC 4):
 * an S command over libcdvd's RPC client -- the pre-check takes the S semaphore, the reply is read
 * uncached from _sceCd_scmdrdata. Name SOCOM 1's (s1:exact). */

#include "libcdvd.h"

int sceCdGetError(void)
{
	int ret;

	if (_sceCd_scmd_prechk(3) == 0)
		return -1;
	if (sceSifCallRpc(&_sceCd_cd_scmd, 4, 0, 0, 0, _sceCd_scmdrdata, 4, 0, 0) < 0) {
		SignalSema(_sceCd_scmd_semid);
		return -1;
	}
	ret = *(int *)((u_int)_sceCd_scmdrdata | 0x20000000);
	SignalSema(_sceCd_scmd_semid);
	return ret;
}
