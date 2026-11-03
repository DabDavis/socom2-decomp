/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libcdvd's sceCdMmode object (MAIN 0x18F610): Sony's SDK, one function as the demo's object 0x16F750's function is (RPC 34, its number and sizes read from an initialised request where the demo's used immediates):
 * an S command over libcdvd's RPC client -- the pre-check takes the S semaphore, the reply is read
 * uncached from _sceCd_scmdrdata. Name SOCOM 1's (s1:calls). */

#include "libcdvd.h"

int sceCdMmode(int media)
{
	int *sdata = (int *)data_001CBE80;
	int ret;

	if (_sceCd_scmd_prechk(34) == 0)
		return 0;
	*sdata = media;
	sceSifWriteBackDCache(sdata, 4);
	if (sceSifCallRpc(&_sceCd_cd_scmd, data_001CC3E8, 0, sdata, data_001CC3EC, _sceCd_scmdrdata, data_001CC3F0, 0, 0) < 0) {
		SignalSema(_sceCd_scmd_semid);
		return 0;
	}
	ret = *(int *)((u_int)_sceCd_scmdrdata | 0x20000000);
	SignalSema(_sceCd_scmd_semid);
	return ret;
}
