/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libmpeg's default DMA callbacks (MAIN 0x1BCD30-0x1BCD7C): Sony's SDK, the demo's object 0x15AA18
 * (C), its two functions in the same order -- sceMpegCreate installs them for sceMpegCbStopDMA and
 * sceMpegCbRestartDMA, around the decoder's own IPU transfers. SOCOM II's release returns 1 where
 * the demo's tail-called libipu. Names by the demo's order (s1:order). */

#include "libmpeg.h"

extern void sceIpuStopDMA(sceIpuDmaEnv *env);
extern void sceIpuRestartDMA(sceIpuDmaEnv *env);

int _defStopDMA(sceMpeg *mp, sceMpegCbData *cbdata, void *anyData)
{
	sceIpuStopDMA(&((MpegDec *)mp->sys)->dmaEnv);
	return 1;
}

int _defRestartDMA(sceMpeg *mp, sceMpegCbData *cbdata, void *anyData)
{
	sceIpuRestartDMA(&((MpegDec *)mp->sys)->dmaEnv);
	return 1;
}
