/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libmpeg's colour-space conversion (MAIN 0x1B9EE0-0x1BA670): Sony's SDK, the demo's object
 * 0x157C20 (C) -- _doCSC, _ch3dmaCSC, _doCSC2, _ch4dma, _csc_storeRefImage, in that order. Written:
 * _ch4dma, DMA channel 4's handler (the IPU's input), which _csc_storeRefImage
 * registers with a {qwc, addr} pair on its stack and which sends the rest of a picture in pieces
 * of at most 0xFFFF quadwords, each kick acknowledged in D_STAT first; _doCSC, the single
 * conversion (its wait on channel 3 reads STR as (CHCR >> 8) & 1: a bitfield is an lbu, a mask
 * an andi 0x100). Names are SOCOM 1's (_doCSC s1:exact; _ch4dma s1:ref, its address taken where
 * the demo takes _ch4dma's).
 *
 * OPEN: _ch3dmaCSC (right size, 15 words): the pass count is volatile (the target reloads it after
 * every access while it reuses npass - 1: every field volatile is 0x158, none 0x13C); what is left
 * is the plain QWC/CHCR stores' schedule (`sw -0x4FE0($at)` before `-0x5000($at)` in both arms) --
 * neither store order nor the nmb update's place gives it (build/scratch/gcc/v_ch3.py, v_ch3b.py).
 * _doCSC2 (0x1B8 against 0x1C0): the frame block's stores and the division's check scheduled
 * otherwise -- the waiting loop reads err, npass and count through a volatile view as the target
 * does, the rest not yet measured. */

#include "libmpeg.h"

#define D_STAT		((volatile u_int *)0x1000E010)

typedef struct {
	u_int qwc;		/* quadwords still to send */
	u_int addr;		/* where they start */
} CscDma;

extern int DIntr(void);
extern int EIntr(void);
extern int _dispatchMpegCallback(sceMpeg *mp, sceMpegCbData *cbdata);
extern void _sendIpuCommand(MpegDec *d, u_int cmd);

/* CSC of nmb macroblocks from the IPU's output into buf (channel 3, 64 quadwords a macroblock),
 * the background callback run while it converts */
static void _doCSC(MpegDec *d, u_long128 *buf, int nmb)
{
	sceMpegCbData cbdata;
	int ie;

	while ((int)*IPU_CTRL < 0)
		;
	ie = DIntr();
	*D3_MADR = (u_int)buf & 0x0fffffff;
	*D3_QWC = nmb << 6;
	*D3_CHCR = 0x100;
	if (ie)
		EIntr();
	_sendIpuCommand(d, 0x70000000 | nmb);
	cbdata.type = sceMpegCbBackground;
	_dispatchMpegCallback(d->mp, &cbdata);
	while ((*D3_CHCR >> 8) & 1)
		;
	while ((int)*IPU_CTRL < 0)
		;
}

/* a conversion of more than 1023 macroblocks, in passes: _doCSC2 starts the first, _ch3dmaCSC
 * (channel 3's handler) each next one, the last of what is left */
typedef struct {
	volatile int count;	/* passes done (the waiting loop reads it) */
	int err;		/* the channel or the IPU still busy at a pass's end */
	int nmb;		/* macroblocks */
	u_int addr;		/* the next pass's output */
	int npass;
} CscData;

/* the handler's QWC and CHCR stores are plain (the target's `sw x,-0x4FE0($at)`), its MADR and
 * the IPU command volatile */
#define D3_QWC_NV	((u_int *)0x1000B020)
#define D3_CHCR_NV	((u_int *)0x1000B000)

extern int AddDmacHandler2(int ch, int (*handler)(int, void *, void *), int next, void *arg);
extern int RemoveDmacHandler(int ch, int id);
extern int EnableDmac(int ch);
extern int DisableDmac(int ch);
extern void _Error(MpegDec *d, char *msg);

static int _ch3dmaCSC(int ch, void *arg)
{
	CscData *cd = (CscData *)arg;

	*D_STAT = 8;
	cd->count++;
	if (*D3_QWC != 0 || (*D3_CHCR & 0x100)) {
		cd->err = 1;
		asm volatile ("sync.l; ei");
		return 0;
	}
	if (cd->count < cd->npass - 1) {
		*D3_MADR = cd->addr;
		*D3_QWC_NV = 0xffc0;
		*D3_CHCR_NV = 0x100;
		*IPU_CMD_W = 0x700003ff;
		cd->addr = (cd->addr + 0xffc00) & 0x0fffffff;
	} else if (cd->count == cd->npass - 1) {
		cd->nmb -= cd->count * 1023;
		*D3_MADR = cd->addr;
		*D3_QWC_NV = cd->nmb << 6;
		*D3_CHCR_NV = 0x100;
		*IPU_CMD_W = cd->nmb | 0x70000000;
	}
	asm volatile ("sync.l; ei");
	return 0;
}

static void _doCSC2(MpegDec *d, u_long128 *buf, int nmb)
{
	sceMpegCbData cbdata;
	CscData cd;
	int id, ie, err;

	cd.count = 0;
	cd.err = 0;
	cd.nmb = nmb;
	cd.addr = ((u_int)buf + 0xffc00) & 0x0fffffff;
	cd.npass = nmb / 1023 + 1;
	while ((int)*IPU_CTRL < 0)
		;
	id = AddDmacHandler2(3, (int (*)(int, void *, void *))_ch3dmaCSC, 0, &cd);
	*D_STAT = 8;
	EnableDmac(3);
	ie = DIntr();
	*D3_MADR = (u_int)buf & 0x0fffffff;
	*D3_QWC = 0xffc0;
	*D3_CHCR = 0x100;
	if (ie)
		EIntr();
	*IPU_CMD_W = 0x700003ff;
	cbdata.type = sceMpegCbBackground;
	_dispatchMpegCallback(d->mp, &cbdata);
	do
		err = ((volatile CscData *)&cd)->err;
	while (((volatile CscData *)&cd)->npass > ((volatile CscData *)&cd)->count);
	if (err)
		_Error(d, "CSC handler error\n");
	while ((int)*IPU_CTRL < 0)
		;
	DisableDmac(3);
	RemoveDmacHandler(3, id);
}

static int _ch4dma(int ch, void *arg)
{
	CscDma *d = (CscDma *)arg;

	*D_STAT = 0x10;
	if (d->qwc == 0) {
		asm volatile ("sync.l; ei");
		return 1;
	}
	if (d->qwc > 0xffff) {
		*D4_MADR = d->addr;
		*D4_QWC = 0xffff;
		*D4_CHCR = 0x101;
		d->addr = (d->addr + 0xffff0) & 0x0fffffff;
		d->qwc -= 0xffff;
	} else {
		*D4_MADR = d->addr;
		*D4_QWC = d->qwc;
		*D4_CHCR = 0x101;
		d->qwc = 0;
	}
	asm volatile ("sync.l; ei");
	return 0;
}

/* GCC round 72a: a picture stored colour-converted -- the stream's DMA stopped, the IPU reset if it
 * stalled on an error, its FIFO cleared, the picture sent back into the IPU by channel 4 (in pieces
 * by _ch4dma when it passes 0xFFFF quadwords) and converted into the caller's buffer, the stream
 * restarted */
void _csc_storeRefImage(MpegDec *d, MpegRef *ref)
{
	sceMpegCbData cbdata;
	CscDma dma;
	int nmb = ref->mbw * ref->mbh;
	int id, ie;

	cbdata.type = sceMpegCbStopDMA;
	_dispatchMpegCallback(d->mp, &cbdata);
	if (*IPU_CTRL & 0x4000)
		*IPU_CTRL_NV = 0x40000000;
	while ((int)*IPU_CTRL < 0)
		;
	_sendIpuCommand(d, 0);
	while ((int)*IPU_CTRL < 0)
		;
	dma.qwc = nmb * 24;
	dma.addr = ref->buf & 0x0fffffff;
	if (dma.qwc > 0xffff) {
		id = AddDmacHandler2(4, (int (*)(int, void *, void *))_ch4dma, 0, &dma);
		*D_STAT = 0x10;
		EnableDmac(4);
		ie = DIntr();
		*D4_MADR = dma.addr;
		*D4_QWC = 0xffff;
		*D4_CHCR = 0x101;
		if (ie)
			EIntr();
		dma.addr = (dma.addr + 0xffff0) & 0x0fffffff;
		dma.qwc -= 0xffff;
		if (nmb < 1024)
			_doCSC(d, (u_long128 *)d->imageBuf, nmb);
		else
			_doCSC2(d, (u_long128 *)d->imageBuf, nmb);
		DisableDmac(4);
		RemoveDmacHandler(4, id);
	} else {
		ie = DIntr();
		*D4_MADR = ref->buf & 0x0fffffff;
		*D4_QWC = dma.qwc;
		*D4_CHCR = 0x101;
		if (ie)
			EIntr();
		dma.qwc = 0;
		if (nmb < 1024)
			_doCSC(d, (u_long128 *)d->imageBuf, nmb);
		else
			_doCSC2(d, (u_long128 *)d->imageBuf, nmb);
	}
	cbdata.type = sceMpegCbRestartDMA;
	_dispatchMpegCallback(d->mp, &cbdata);
}
