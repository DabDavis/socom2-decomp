/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libcdvd's sceCdPowerOff object (MAIN 0x18F558): Sony's SDK, one function as the demo's object 0x16FB28 is (RPC 33):
 * an S command over libcdvd's RPC client -- the pre-check takes the S semaphore, the reply is read
 * uncached from _sceCd_scmdrdata. Name SOCOM 1's (s1:exact). */

#include "libcdvd.h"

int sceCdPowerOff(int *result)
{
	int ret;

	if (_sceCd_scmd_prechk(33) == 0)
		return 0;
	if (sceSifCallRpc(&_sceCd_cd_scmd, 33, 0, 0, 0, _sceCd_scmdrdata, 8, 0, 0) < 0) {
		SignalSema(_sceCd_scmd_semid);
		return 0;
	}
	*result = *(int *)((u_int)(_sceCd_scmdrdata + 4) | 0x20000000);
	ret = *(int *)((u_int)_sceCd_scmdrdata | 0x20000000);
	SignalSema(_sceCd_scmd_semid);
	return ret;
}
