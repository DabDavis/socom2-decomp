/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libmpeg's sequence layer (MAIN 0x1BC228-0x1BCD30): Sony's SDK, the demo's object 0x159F10
 * (C), its functions in the same order -- the decoder's resets (_initSeqAgain, _lastFrame,
 * _clearOnce, _clearEach), the error reporting (_ErrMessage, _Error1, _Error), the bitstream
 * feed (_sendDataToIPU: a chain of REF tags to the IPU's input DMA), the sequence header and its
 * extensions, and the reference pictures' allocation (_initSeq). Names are SOCOM 1's
 * (s1:exact; the four extension stubs by the demo's order and their own messages).
 *
 * Shapes the target settled: a value fold would distribute into a ternary is computed in its own
 * statement first -- _sendDataToIPU's tag id (`id = size ? 3 : 0`, a movn; inline in the tag,
 * fold makes `size ? 3<<28 : 0` and a branch) and its address before it (the loop's hoisted
 * constants come out in the order the body first uses them); _sequenceExtension extracts the
 * extension's fields into locals from the low bits up, before the chroma check; _clearOnce's
 * 0x70000000 is a local set before the call (it lives in s1 across _ipuSetMPEG1); _lastFrame
 * reads frameNo before its tests; _RefImageInit stores width, mbw, height, mbh.
 *
 * OPEN: _initSeq (right size, 109 words: the scheduling of the reallocation block -- the target
 * multiplies height by 384 first and stores mp->height before mp->width; no spelling of the
 * product or order of the two stores tried closes it); _initRefImages (0xD8 against 0xE0: the
 * target multiplies the field offset by 384 three times, once per picture -- GCC CSEs every
 * spelling tried into one multiply). */

#include "libmpeg.h"

extern int DIntr(void);
extern int EIntr(void);
extern void FlushCache(int);
extern void sceIpuSync(int mode, u_short timeout);
extern int scePrintf(const char *fmt, ...);
extern int sprintf(char *buf, const char *fmt, ...);
extern void _ipuSetMPEG1(int mpeg1);
extern void _dispRefImage(MpegDec *d, MpegRef *ref, int frameNo);
extern void _dispRefImageField(MpegDec *d, MpegRef *top, MpegRef *bottom, int frameNo);
extern int _dispatchMpegCallback(sceMpeg *mp, sceMpegCbData *cbdata);
extern u_int _nextBit(MpegDec *d, int n);
extern void _waitIpuIdle(MpegDec *d);
extern void _sendIpuCommand(MpegDec *d, u_int cmd);
extern void _extensionAndUserData(MpegDec *d);
extern void _alalcFree(MpegAlloc *a);
extern u_int _alalcAlloc(MpegDec *d, MpegAlloc *a, u_int size, u_int align);
extern u_long128 _defIQM[4];
extern u_long128 _defNIQM[4];

void _Error(MpegDec *d, char *msg);
void _initSeq(sceMpeg *mp);
static int _RefImageInit(MpegRef *r, int width, int height);
static void _initRefImages(MpegRef *r0, MpegRef *r1, MpegRef *r2, MpegRef *r3, MpegRef *r4,
			   MpegRef *r5, MpegRef *r6, MpegRef *r7, MpegRef *r8,
			   u_int buf0, u_int buf1, u_int buf2, int width, int height);
static void _setDefaultQM(MpegDec *d, u_int cmd, u_long128 *table);

void _initSeqAgain(MpegDec *d)
{
	d->mpeg2 = 0;
	_ipuSetMPEG1(1);
}

void _lastFrame(MpegDec *d)
{
	int frameNo = d->frameNo;

	if (d->secondField)
		_Error(d, "the second field is missing");
	else if (d->picture_structure == 3)
		_dispRefImage(d, d->ref[0][1], frameNo - 1);
	else
		_dispRefImageField(d, d->ref[1][1], d->ref[2][1], frameNo - 1);
	d->secondField = 0;
}

void _clearOnce(MpegDec *d)
{
	u_int csc = 0x70000000;

	_ipuSetMPEG1(1);
	d->_590 = csc;
	d->_594 = 0x70001800;
	d->_6d0 = 0x70001B00;
	d->_6d4 = 0x70003300;
	d->_810 = 0;
}

void _clearEach(MpegDec *d)
{
	int ie;

	d->dirty = 1;
	d->_1b0 = 0;
	ie = DIntr();
	*D_ENABLEW = *D_ENABLER | 0x10000;
	*D3_CHCR = 0;
	*D4_CHCR = 0;
	*D9_CHCR = 0;
	*D_ENABLEW = *D_ENABLER & ~0x10000;
	if (ie)
		EIntr();
	*D3_QWC = 0;
	*D4_QWC = 0;
	*D9_QWC = 0;
	*IPU_CTRL = 0x40000000;
	sceIpuSync(0, 0);
}

void _ErrMessage(char *msg)
{
	scePrintf("[MPEG ERROR]%s\n", msg);
}

void _Error1(MpegDec *d, char *fmt, int arg)
{
	char buf[256];

	sprintf(buf, fmt, arg);
	_Error(d, buf);
}

void _Error(MpegDec *d, char *msg)
{
	sceMpegCbDataError cbdata;

	if (d->mp && d && d->cb[sceMpegCbError].func) {
		cbdata.type = sceMpegCbError;
		cbdata.errMessage = msg;
		_dispatchMpegCallback(d->mp, (sceMpegCbData *)&cbdata);
	} else
		_ErrMessage(msg);
}

void _sendDataToIPU(MpegDec *d, u_char *data, int size)
{
	u_long *tags = (u_long *)(((u_int)d->tags + 15) / 16 * 16);
	u_long *t = tags;
	int ie, n, id;
	u_int addr;

	while (size > 0) {
		n = (size > 0xFFF40) ? 0xFFF40 : size;
		size -= n;
		addr = (u_int)data & 0x0FFFFFFF;
		id = size ? 3 : 0;
		*t = ((u_long)addr << 32) | ((u_long)id << 28) | (u_int)((n + 15) / 16);
		asm volatile ("sync.l");
		data += n;
		t += 2;
	}
	FlushCache(0);
	ie = DIntr();
	*D4_TADR = (u_int)tags & 0x0FFFFFFF;
	*D4_QWC = 0;
	*D4_CHCR = 0x105;
	if (ie)
		EIntr();
}

static int _RefImageInit(MpegRef *r, int width, int height)
{
	r->width = width;
	r->mbw = width >> 4;
	r->height = height;
	r->mbh = height >> 4;
	return 1;
}

void _sequenceHeader(MpegDec *d)
{
	u_int v;

	d->firstStructure = 0;
	v = _nextBit(d, 32);
	d->horizontal_size = v >> 20;
	d->vertical_size = (v >> 8) & 0xFFF;
	if (d->vertical_size > 2800)
		_Error(d, "vertical size > 2800");
	v = _nextBit(d, 30);
	d->bit_rate = v >> 12;
	d->vbv_buffer_size = (v >> 1) & 0x3FF;
	if ((d->load_intra_quantiser_matrix = _nextBit(d, 1))) {
		_waitIpuIdle(d);
		_sendIpuCommand(d, 0x50000000);
		_waitIpuIdle(d);
	} else
		_setDefaultQM(d, 0x50000000, _defIQM);
	if ((d->load_non_intra_quantiser_matrix = _nextBit(d, 1))) {
		_waitIpuIdle(d);
		_sendIpuCommand(d, 0x58000000);
		_waitIpuIdle(d);
	} else
		_setDefaultQM(d, 0x58000000, _defNIQM);
	_extensionAndUserData(d);
	_initSeq(d->mp);
}

void _initSeq(sceMpeg *mp)
{
	MpegDec *d = mp->sys;
	int width, height, half;
	u_int size;

	if (!d->mpeg2) {
		d->picture_structure = 3;
		d->frame_pred_frame_dct = 1;
		d->matrix_coefficients = 5;
		d->progressive_sequence = 1;
		d->chroma_format = 1;
		d->progressive_frame = 1;
	}
	d->mb_width = (d->horizontal_size + 15) >> 4;
	d->mb_height = (d->mpeg2 && !d->progressive_sequence)
		? ((d->vertical_size + 31) >> 5) << 1 : (d->vertical_size + 15) >> 4;
	height = d->mb_height << 4;
	width = d->mb_width << 4;
	if (width != mp->width || height != mp->height) {
		mp->height = height;
		mp->width = width;
		half = height >> 1;
		size = (u_int)(width * (height * 384)) >> 8;
		_alalcFree(&d->alloc);
		d->frameBuf[0] = _alalcAlloc(d, &d->alloc, size, 64);
		d->frameBuf[1] = _alalcAlloc(d, &d->alloc, size, 64);
		d->frameBuf[2] = _alalcAlloc(d, &d->alloc, size, 64);
		_initRefImages(&d->refs[0], &d->refs[1], &d->refs[2], &d->refs[3], &d->refs[4],
			       &d->refs[5], &d->refs[6], &d->refs[7], &d->refs[8],
			       d->frameBuf[0], d->frameBuf[1], d->frameBuf[2], width, height);
		_RefImageInit(&d->refs[0], width, height);
		_RefImageInit(&d->refs[1], width, height);
		_RefImageInit(&d->refs[2], width, height);
		_RefImageInit(&d->refs[3], width, half);
		_RefImageInit(&d->refs[4], width, half);
		_RefImageInit(&d->refs[5], width, half);
		_RefImageInit(&d->refs[6], width, half);
		_RefImageInit(&d->refs[7], width, half);
		_RefImageInit(&d->refs[8], width, half);
	}
}

static void _initRefImages(MpegRef *r0, MpegRef *r1, MpegRef *r2, MpegRef *r3, MpegRef *r4,
			   MpegRef *r5, MpegRef *r6, MpegRef *r7, MpegRef *r8,
			   u_int buf0, u_int buf1, u_int buf2, int width, int height)
{
	int mbs = width * height / 512;
	u_int a0 = (buf0 & 0x0FFFFFFF) | 0x20000000;
	u_int a1 = (buf1 & 0x0FFFFFFF) | 0x20000000;
	u_int a2 = (buf2 & 0x0FFFFFFF) | 0x20000000;

	r0->buf = a0;
	r1->buf = a1;
	r2->buf = a2;
	r3->buf = a0;
	r4->buf = a1;
	r5->buf = a2;
	r6->buf = ((buf0 + mbs * 384) & 0x0FFFFFFF) | 0x20000000;
	r7->buf = ((buf1 + mbs * 384) & 0x0FFFFFFF) | 0x20000000;
	r8->buf = ((buf2 + mbs * 384) & 0x0FFFFFFF) | 0x20000000;
}

static void _setDefaultQM(MpegDec *d, u_int cmd, u_long128 *table)
{
	sceMpegCbData cbdata;
	int ie;

	cbdata.type = sceMpegCbStopDMA;
	_dispatchMpegCallback(d->mp, &cbdata);
	_waitIpuIdle(d);
	*IPU_CMD_W = 0;
	_waitIpuIdle(d);
	ie = DIntr();
	*D4_MADR = (u_int)table & 0x0FFFFFFF;
	*D4_QWC = 4;
	*D4_CHCR = 0x101;
	if (ie)
		EIntr();
	_sendIpuCommand(d, cmd);
	_waitIpuIdle(d);
	cbdata.type = sceMpegCbRestartDMA;
	_dispatchMpegCallback(d->mp, &cbdata);
}

void _sequenceExtension(MpegDec *d)
{
	u_int v, pl, hext, vext, brext, vbvext;

	d->mpeg2 = 1;
	_ipuSetMPEG1(0);
	v = _nextBit(d, 28);
	brext = (v >> 1) & 0xFFF;
	vext = (v >> 13) & 3;
	hext = (v >> 15) & 3;
	d->chroma_format = (v >> 17) & 3;
	if (d->chroma_format != 1)
		_Error(d, "_chroma_format needs to be 1: 420");
	d->progressive_sequence = (v >> 19) & 1;
	pl = v >> 20;
	vbvext = _nextBit(d, 16) >> 8;
	if (pl != 0x48 && pl != 0x58 && pl != 0x44)
		_Error(d, "Unsupported profile/level");
	d->horizontal_size = (hext << 12) | (d->horizontal_size & 0xFFF);
	d->vertical_size = (vext << 12) | (d->vertical_size & 0xFFF);
	d->bit_rate += brext << 18;
	d->vbv_buffer_size += vbvext << 10;
}

void _sequenceDisplayExtension(MpegDec *d)
{
	_nextBit(d, 3);
	if (_nextBit(d, 1)) {
		_nextBit(d, 8);
		_nextBit(d, 8);
		d->matrix_coefficients = _nextBit(d, 8);
	}
	d->display_horizontal_size = _nextBit(d, 14);
	_nextBit(d, 1);
	d->display_vertical_size = _nextBit(d, 14);
}

void _sequenceScalableExtension(MpegDec *d)
{
	_Error(d, "_sequenceScalableExtension() is not implemented");
}

void _unknown_extension(MpegDec *d)
{
	_Error(d, "Unknown Extension");
}

void _pictureSpatialScalableExtension(MpegDec *d)
{
	_Error(d, "_pictureSpatialScalableExtension is not supported");
}

void _pictureTemporalScalableExtension(MpegDec *d)
{
	_Error(d, "_pictureTemporalScalableExtension is not supported");
}
