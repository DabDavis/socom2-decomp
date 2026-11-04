/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libgraph's display set-up (MAIN 0x1A15D8-0x1A19FC): the demo's object 0x14E160 (C), its three
 * functions in the same order -- the statics checkModelVersion (the ROM's version date, the last
 * eight digits of rom0:ROMVER, after 2001-06-08) and _GetGsDxDyOffset (syscall 128: the display
 * offsets a newer ROM keeps for the video modes beyond NTSC and PAL), and sceGsSetDefDispEnv.
 * Names: SOCOM 1's (s1:exact). checkModelVersion counts its bytes in an unsigned int (sltiu).
 *
 * OPEN: sceGsSetDefDispEnv (0x3B8 against 0x374): the mode tests are an if-chain
 *   (2, 3, 0x50, else the message, as written) and dx/dy take the target's s5/s4, but the DISPLAY
 *   word is not yet the vendor's spelling -- the target sums DX in 64 bits (`daddiu off0,636;
 *   daddu dx*magh`) and masks DX and DY with 0xfff while MAGH, DW and DH go unmasked, multiplies
 *   magh*w before dx*magh, and the NTSC and PAL interlaced arms share one tail (cross-jumped);
 *   smode2 is a branch per constant into one store (five spellings tried: 168-192 words). */
#include "libgraph.h"

extern int sceOpen(const char *name, int mode, ...);
extern int sceRead(int fd, void *buf, int n);
extern int sceClose(int fd);
extern int atoi(const char *s);
extern int scePrintf(const char *fmt, ...);

static int checkModelVersion(void)
{
	int fd;
	unsigned int i;
	char buf[256];
	char *p;

	if ((fd = sceOpen("rom0:ROMVER", 1)) < 0)
		return -1;
	for (i = 0, p = buf; i < 256; i++) {
		sceRead(fd, p, 1);
		if (*p++ == 0)
			break;
	}
	sceClose(fd);
	return atoi(&buf[i - 9]) > 20010608;
}

static void _GetGsDxDyOffset(int mode, int *dx, int *dy, int *dw, int *dh)
{
	__asm__ __volatile__("li $3, 128\n\tsyscall");
}

void sceGsSetDefDispEnv(sceGsDispEnv *disp, short psm, short w, short h, short dx, short dy)
{
	sceGsGParam *gp;
	int off[4];
	int magh;

	gp = sceGsGetGParam();
	if ((gp->sceGsOutMode != 2 && gp->sceGsOutMode != 3) && checkModelVersion()) {
		_GetGsDxDyOffset(gp->sceGsOutMode, &off[0], &off[1], &off[2], &off[3]);
	} else {
		off[3] = 0;
		off[2] = 0;
		off[1] = 0;
		off[0] = 0;
	}
	disp->pmode = 0x66;
	if (gp->sceGsInterMode != 0) {
		if (gp->sceGsFFMode != 0)
			disp->smode2 = 3;
		else
			disp->smode2 = 1;
	} else
		disp->smode2 = 2;
	disp->dispfb = ((u_long)(psm & 0xf) << 15) | ((u_long)(((w + 63) >> 6) & 0x3f) << 9);
	if (gp->sceGsOutMode == 2) {
		if (gp->sceGsInterMode == 1) {
			magh = (w + 2559) / w;
			disp->display = (((u_long)off[0] + 636 + dx * magh) & 0xfff)
				| ((u_long)((dy + off[1] + 50) & 0xfff) << 12)
				| ((u_long)(magh - 1) << 23)
				| ((u_long)(magh * w - 1) << 32)
				| ((u_long)(gp->sceGsFFMode ? h * 2 - 1 : h - 1) << 44);
		} else {
			magh = (w + 2559) / w;
			disp->display = (((u_long)off[0] + 636 + dx * magh) & 0xfff)
				| ((u_long)((dy + off[1] + 25) & 0xfff) << 12)
				| ((u_long)(magh - 1) << 23)
				| ((u_long)(magh * w - 1) << 32)
				| ((u_long)(h - 1) << 44);
		}
	} else if (gp->sceGsOutMode == 3) {
		if (gp->sceGsInterMode == 1) {
			magh = (w + 2559) / w;
			disp->display = (((u_long)off[0] + 656 + dx * magh) & 0xfff)
				| ((u_long)((dy + off[1] + 72) & 0xfff) << 12)
				| ((u_long)(magh - 1) << 23)
				| ((u_long)(magh * w - 1) << 32)
				| ((u_long)(gp->sceGsFFMode ? h * 2 - 1 : h - 1) << 44);
		} else {
			magh = (w + 2559) / w;
			disp->display = (((u_long)off[0] + 656 + dx * magh) & 0xfff)
				| ((u_long)((dy + off[1] + 36) & 0xfff) << 12)
				| ((u_long)(magh - 1) << 23)
				| ((u_long)(magh * w - 1) << 32)
				| ((u_long)(h - 1) << 44);
		}
	} else if (gp->sceGsOutMode == 0x50) {
		disp->display = (((u_long)((720 - w) / 2 * 2) + off[0] + ((u_long)(dx * 2) + 232)) & 0xfff)
			| ((u_long)((dy + off[1] + 35) & 0xfff) << 12)
			| ((u_long)1 << 23)
			| ((u_long)(w * 2 - 1) << 32)
			| ((u_long)(h - 1) << 44);
	} else {
		scePrintf("sceGsDefDispEnv:Not support displaymode for 0x%x\n", gp->sceGsOutMode);
	}
	disp->bgcolor = 0;
}
