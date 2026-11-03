/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libmpeg's API (MAIN 0x1BB688-0x1BC228): Sony's SDK, the demo's object 0x1593B8 (C), its
 * functions in the same order -- sceMpegInit ... _sceMpegFlush, the work-area allocator
 * (_alalc*) and the picture loop (_getpic, _decodeOrSkip*). SOCOM II's release adds one
 * function after sceMpegGetPictureRAW8xy (0x1BBA70, the flag at 0x10A0) and grows a few by a
 * word. Names are SOCOM 1's (s1:exact, s1:calls); the unnamed rows by the demo's order.
 *
 * Shapes the target settled: the picture loop is `for (;;) { ... if (a || b) break; }` (a
 * do-while's second test is rotated to the top); _decodeOrSkipFrame's `ret = ret && _decPicture()`
 * after `ret = _updateRefImage()` (the result through a temporary: `ret` in its own expression);
 * sceMpegGetPicture's image pointer is its parameter reassigned in place; the stores in the order
 * ccperm found (the last source store is scheduled first).
 *
 * OPEN: _decodeOrSkipField (size 0x158 against 0x15C): the target keeps the constant 1 in s1
 * across the calls and builds the second field's expected structure (`li 2; xori; movn`) and both
 * booleans with movn from it, its result in s4; every spelling tried (a ternary compared inline --
 * which fold distributes into branches --, an `expect` local, `?1:0`, `&&` chains) gives sltu or
 * branches and one saved register fewer. */

#include "libmpeg.h"

extern int DIntr(void);
extern int EIntr(void);
extern void sceIpuInit(void);
extern void *memset(void *, int, unsigned int);
extern void _Error(MpegDec *d, char *msg);
extern void _Error1(MpegDec *d, char *fmt, int arg);
extern void _clearOnce(MpegDec *d);
extern void _clearEach(MpegDec *d);
extern void _initSeqAgain(MpegDec *d);
extern int _nextHeader(MpegDec *d);
extern int _updateRefImage(MpegDec *d, int second);
extern int _decPicture(MpegDec *d);
extern void _outputFrame(MpegDec *d, int frameNo, int n);
extern void _lastFrame(MpegDec *d);
extern void _sendDataToIPU(MpegDec *d, u_char *data, int size);
extern int _defStopDMA(sceMpeg *mp, sceMpegCbData *cbdata, void *anyData);
extern int _defRestartDMA(sceMpeg *mp, sceMpegCbData *cbdata, void *anyData);

int sceMpegReset(sceMpeg *mp);
int sceMpegClearRefBuff(sceMpeg *mp);
int _dispatchMpegCallback(sceMpeg *mp, sceMpegCbData *cbdata);
void _alalcInit(MpegAlloc *a, u_int base, u_int size);
void _alalcSetDynamic(MpegAlloc *a);
u_int _alalcAlloc(MpegDec *d, MpegAlloc *a, u_int size, u_int align);
int _sceMpegFlush(sceMpeg *mp);

int sceMpegInit(void)
{
	int ie = DIntr();

	*D_ENABLEW = *D_ENABLER | 0x10000;
	*D3_CHCR &= ~0x100;
	*D4_CHCR &= ~0x100;
	*D_ENABLEW = *D_ENABLER & ~0x10000;
	if (ie)
		EIntr();
	*D3_QWC = 0;
	*D4_QWC = 0;
	sceIpuInit();
	return 1;
}

int sceMpegCreate(sceMpeg *mp, u_char *work, int size)
{
	MpegDec *d;
	u_int rest;

	memset(work, 0, size);
	d = (MpegDec *)(((u_int)work + 3) / 4 * 4);
	rest = size - ((u_char *)d - work);
	if (rest < sizeof(MpegDec)) {
		_Error(d, "The size of work area is too small");
		return 0;
	}
	mp->sys = d;
	_alalcInit(&d->alloc, (u_int)d + sizeof(MpegDec), rest - sizeof(MpegDec));
	mp->width = 0;
	mp->height = 0;
	mp->frameCount = 0;
	mp->pts = -1;
	mp->dts = -1;
	mp->flags = 0;
	mp->pts2nd = -1;
	mp->dts2nd = -1;
	mp->flags2nd = 0;
	d->_0b4 = 0;
	d->_0b8 = 0;
	d->_0bc = 0;
	d->_0c0 = 0;
	d->_0c4 = 0;
	d->_0c8 = 0;
	d->dispWidth = 0;
	d->dispHeight = 0;
	d->firstStructure = 0;
	d->imageBuf = 0;
	d->imageX = 0;
	d->imageY = 0;
	d->imageSize = 0;
	d->brokenLink = 0;
	d->ptmSet = 0;
	d->cb[sceMpegCbError].func = 0;
	d->cb[sceMpegCbNodata].func = 0;
	d->cb[sceMpegCbBackground].func = 0;
	d->cb[sceMpegCbTimeStamp].func = 0;
	d->cb[sceMpegCbStr].func = 0;
	d->ptm = -1;
	d->cb[sceMpegCbStopDMA].func = _defStopDMA;
	d->cb[sceMpegCbRestartDMA].func = _defRestartDMA;
	d->work = (void *)_alalcAlloc(d, &d->alloc, 0x600, 8);
	d->_048 = 0;
	d->frameBuf[0] = 0;
	d->frameBuf[1] = 0;
	d->frameBuf[2] = 0;
	d->ptsGapSet = 0;
	d->ptsGap = 0;
	d->_080 = -1;
	d->_088 = 0;
	d->_090 = 0;
	d->frameBase = 0;
	d->modeI = -1;
	d->modeP = -1;
	d->modeB = -1;
	d->mp = mp;
	d->csc = 1;
	_clearOnce(d);
	sceMpegReset(mp);
	sceMpegClearRefBuff(mp);
	d->ref[0][0] = &d->refs[0];
	d->ref[0][1] = &d->refs[1];
	d->ref[0][3] = &d->refs[2];
	d->ref[1][0] = &d->refs[3];
	d->ref[1][1] = &d->refs[4];
	d->ref[1][3] = &d->refs[5];
	d->ref[2][0] = &d->refs[6];
	d->ref[2][1] = &d->refs[7];
	d->ref[2][3] = &d->refs[8];
	_alalcSetDynamic(&d->alloc);
	d->_850 = -1;
	d->_854 = 0;
	d->_84c = 0;
	d->_81c = 0x70003600;
	return 1;
}

int sceMpegDelete(sceMpeg *mp)
{
	return 1;
}

int sceMpegAddBs(sceMpeg *mp, u_long128 *bs, int size)
{
	_sendDataToIPU(mp->sys, (u_char *)bs, (size + 19) / 16 * 16);
	return 1;
}

static int _getpic(sceMpeg *mp);
static int _decodeOrSkip(sceMpeg *mp, int n, int mode);
static int _decodeOrSkipFrame(sceMpeg *mp, int n, int mode);
static int _decodeOrSkipField(sceMpeg *mp, int n, int mode);

int sceMpegGetPicture(sceMpeg *mp, u_long128 *imageBuf, int mbcount)
{
	MpegDec *d = mp->sys;

	imageBuf = (u_long128 *)(((u_int)imageBuf & 0x0FFFFFFF) | 0x20000000);
	d->csc = 1;
	d->imageBuf = (u_int)imageBuf;
	d->imageSize = mbcount;
	d->imageY = 0;
	d->imageX = 0;
	return _getpic(mp);
}

int sceMpegGetPictureRAW8(sceMpeg *mp, u_long128 *imageBuf, int mbcount)
{
	MpegDec *d = mp->sys;

	imageBuf = (u_long128 *)(((u_int)imageBuf & 0x0FFFFFFF) | 0x20000000);
	d->csc = 0;
	d->imageBuf = (u_int)imageBuf;
	d->imageSize = mbcount;
	d->imageY = 0;
	d->imageX = 0;
	return _getpic(mp);
}

int sceMpegGetPictureRAW8xy(sceMpeg *mp, u_long128 *imageBuf, int mbx, int mby)
{
	MpegDec *d = mp->sys;

	imageBuf = (u_long128 *)(((u_int)imageBuf & 0x0FFFFFFF) | 0x20000000);
	d->csc = 0;
	d->imageBuf = (u_int)imageBuf;
	d->imageSize = mbx * mby;
	d->imageX = mbx * 16;
	d->imageY = mby * 16;
	return _getpic(mp);
}

void func_001BBA70(sceMpeg *mp)
{
	((MpegDec *)mp->sys)->_10a0 = 1;
}

void sceMpegSetDecodeMode(sceMpeg *mp, int ni, int np, int nb)
{
	MpegDec *d = mp->sys;

	d->modeI = ni;
	d->modeP = np;
	d->modeB = nb;
}

void sceMpegGetDecodeMode(sceMpeg *mp, int *ni, int *np, int *nb)
{
	MpegDec *d = mp->sys;

	*ni = d->modeI;
	*np = d->modeP;
	*nb = d->modeB;
}

int sceMpegIsEnd(sceMpeg *mp)
{
	return ((MpegDec *)mp->sys)->isEnd;
}

int sceMpegIsRefBuffEmpty(sceMpeg *mp)
{
	return ((MpegDec *)mp->sys)->nFrames == 0;
}

int sceMpegReset(sceMpeg *mp)
{
	MpegDec *d = mp->sys;

	d->isEnd = 0;
	d->nFrames = 0;
	d->started = 0;
	mp->frameCount = 0;
	d->frameBase = 0;
	d->_080 = -1;
	_clearEach(d);
	d->frameNo = 0;
	_initSeqAgain(d);
	return 1;
}

int sceMpegClearRefBuff(sceMpeg *mp)
{
	MpegDec *d = mp->sys;

	if (d->ref[0][0])
		d->ref[0][0]->valid = 0;
	if (d->ref[1][0])
		d->ref[1][0]->valid = 0;
	if (d->ref[2][0])
		d->ref[2][0]->valid = 0;
	if (d->ref[0][1])
		d->ref[0][1]->valid = 0;
	if (d->ref[1][1])
		d->ref[1][1]->valid = 0;
	if (d->ref[2][1])
		d->ref[2][1]->valid = 0;
	return 1;
}

sceMpegCallback sceMpegAddCallback(sceMpeg *mp, sceMpegCbType type, sceMpegCallback func, void *anyData)
{
	MpegDec *d = mp->sys;
	sceMpegCallback old;

	d->cb[type].data = anyData;
	old = d->cb[type].func;
	d->cb[type].func = func;
	return old;
}

int _dispatchMpegCallback(sceMpeg *mp, sceMpegCbData *cbdata)
{
	MpegDec *d;
	int ret = 0;

	if (mp && (d = mp->sys) && d->cb[cbdata->type].func)
		ret = d->cb[cbdata->type].func(mp, cbdata, d->cb[cbdata->type].data);
	return ret;
}

int _dispatchMpegCbNodata(sceMpeg *mp)
{
	sceMpegCbData cbdata;

	cbdata.type = sceMpegCbNodata;
	_dispatchMpegCallback(mp, &cbdata);
	return 1;
}

int sceMpegSetDefaultPtsGap(sceMpeg *mp, long gap)
{
	MpegDec *d = mp->sys;

	d->ptsGapSet = 1;
	d->ptsGap = gap;
	return 1;
}

int sceMpegResetDefaultPtsGap(sceMpeg *mp)
{
	MpegDec *d = mp->sys;

	d->ptsGapSet = 0;
	d->ptsGap = 0;
	return 1;
}

int sceMpegSetImageBuff(sceMpeg *mp, u_int buf)
{
	((MpegDec *)mp->sys)->imageBuf = buf;
	return 1;
}

int sceMpegDispWidth(sceMpeg *mp)
{
	return ((MpegDec *)mp->sys)->dispWidth;
}

int sceMpegDispHeight(sceMpeg *mp)
{
	return ((MpegDec *)mp->sys)->dispHeight;
}

int *sceMpegDispCenterOffX(sceMpeg *mp)
{
	return &((MpegDec *)mp->sys)->_0b4;
}

int *sceMpegDispCenterOffY(sceMpeg *mp)
{
	return &((MpegDec *)mp->sys)->_0b4;
}

int sceSetBrokenLink(sceMpeg *mp, int brokenLink)
{
	MpegDec *d = mp->sys;
	int old = d->brokenLink;

	d->brokenLink = brokenLink;
	return old;
}

int sceSetPtm(sceMpeg *mp, long ptm)
{
	MpegDec *d = mp->sys;

	d->ptm = ptm;
	d->ptmSet = 1;
	return 1;
}

void _alalcInit(MpegAlloc *a, u_int base, u_int size)
{
	a->size = size;
	a->base = base;
	a->cur = base;
	a->mark = base;
}

void _alalcSetDynamic(MpegAlloc *a)
{
	a->mark = a->cur;
}

void _alalcFree(MpegAlloc *a)
{
	a->cur = a->mark;
}

u_int _alalcAlloc(MpegDec *d, MpegAlloc *a, u_int size, u_int align)
{
	u_int p = (a->cur + align - 1) / align * align;

	if (p + size <= a->base + a->size) {
		a->cur = p + size;
		return p;
	}
	_Error(d, "work area size is too small");
	return 0;
}

int _alalcRest(MpegAlloc *a)
{
	return a->base + a->size - a->cur;
}

static int _getpic(sceMpeg *mp)
{
	MpegDec *d = mp->sys;
	int type = 1;
	int ret = 0;

	d->isEnd = 0;
	if (d->imageBuf & 0x3F) {
		_Error1(d, "image buffer needs to be aligned to 64byte boundary(0x%08x)", d->imageBuf);
		return -1;
	}
	d->_820 = 0;
	for (;;) {
		if (ret != -1) {
			do {
				type = _nextHeader(d);
			} while (type && d->picture_structure != d->firstStructure && d->mpeg2);
		}
		switch (type) {
		case 0:
			_sceMpegFlush(mp);
			d->isEnd = 1;
			break;
		case 1:
			d->nB = 0;
			d->nP = 0;
			d->nI = 0;
			ret = _decodeOrSkip(mp, 0, d->modeI);
			d->nI++;
			break;
		case 2:
			ret = _decodeOrSkip(mp, d->nP, d->modeP);
			d->nP++;
			break;
		case 3:
		case 4:
			ret = _decodeOrSkip(mp, d->nB, d->modeB);
			d->nB++;
			break;
		}
		if (d->_820 || d->isEnd)
			break;
	}
	return 1;
}

static int _decodeOrSkipFrame(sceMpeg *mp, int n, int mode)
{
	MpegDec *d = mp->sys;
	int ret;
	int skip = 0;

	if (mode == -1 || n < mode) {
		if (!d->started) {
			mp->frameCount = 0;
			d->started = 1;
		}
		ret = _updateRefImage(d, 0);
		ret = ret && _decPicture(d);
	} else {
		ret = _updateRefImage(d, 0);
		skip = 1;
		_dispatchMpegCbNodata(mp);
	}
	_outputFrame(d, d->frameNo, d->nFrames);
	if (d->picture_structure != 3 && !skip)
		d->secondField = !d->secondField;
	mp->frameCount = d->frameNo - d->frameBase;
	if (!d->secondField) {
		d->frameNo++;
		d->nFrames++;
	}
	return ret;
}

static int _decodeOrSkip(sceMpeg *mp, int n, int mode)
{
	MpegDec *d = mp->sys;
	int ret;

	if (d->picture_structure != 3)
		ret = _decodeOrSkipField(mp, n, mode);
	else
		ret = _decodeOrSkipFrame(mp, n, mode);
	return ret;
}

static int _decodeOrSkipField(sceMpeg *mp, int n, int mode)
{
	MpegDec *d = mp->sys;
	int decode = 0;
	int ok, ret;

	d->secondField = 0;
	if (mode == -1 || n < mode)
		decode = 1;
	if (!d->started) {
		mp->frameCount = 0;
		d->started = 1;
	}
	if (_updateRefImage(d, 0) && decode)
		_decPicture(d);
	d->secondField = 1;
	if (!_nextHeader(d)) {
		_sceMpegFlush(mp);
		d->isEnd = 1;
		return 0;
	}
	if (d->picture_structure != (d->firstStructure == 1 ? 2 : 1))
		return -1;
	ok = _updateRefImage(d, 1) ? 1 : 0;
	ret = 0;
	if (ok && decode)
		ret = _decPicture(d) ? 1 : 0;
	_outputFrame(d, d->frameNo, d->nFrames);
	d->secondField = 0;
	mp->frameCount = d->frameNo - d->frameBase;
	d->frameNo++;
	d->nFrames++;
	if (!decode)
		_dispatchMpegCbNodata(mp);
	return ret;
}

int _sceMpegFlush(sceMpeg *mp)
{
	MpegDec *d = mp->sys;
	int ret = 0;

	if (d->nFrames && d->started) {
		_lastFrame(d);
		ret = 1;
		mp->frameCount = d->frameNo - d->frameBase;
		d->nFrames = 0;
	}
	return ret;
}
