/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libmpeg's decoder (MAIN 0x1B5160-0x1B9EE0): Sony's SDK, the demo's object 0x152F60 (C). This
 * unit holds the object's bitstream layer, in the demo's order -- the IPU command and wait
 * primitives (_sendIpuCommand ... _nextStartCode) and the header parsers (_sliceB ...
 * _copyrightExtension); the motion compensation before them and the picture output after them
 * are not written yet (their rows stand as drafts or open). _updateTempTackData is left to its
 * matching draft. Names are SOCOM 1's (s1:exact).
 *
 * The IPU's idle wait and the command write are macros, expanded inside the readers
 * (_ipuVdec, _peepBit, _flushBuf, _nextBit) as well as being functions of their own: the
 * readers' copies keep no constant in a saved register, the functions' do. A command's index
 * into _isDirty shifts as the command's own type does: unsigned in _flushBuf and _nextBit (srl),
 * signed in _ipuVdec (sra). _nextHeader's cases are laid out in source order (sequence, GOP,
 * picture); _pictureDisplayExtension tests progressive_sequence first; _nextBit takes its result
 * before resetting nbits.
 *
 * OPEN: _ipuVdec (0x180 against 0x178: the target reads IPU_TOP as `lui; ld 0x2030(reg)` and
 * IPU_BP as `lui; ori; lw`, and returns from the command word's register -- two words longer
 * here, in the tail); _extensionAndUserData (0x11C against 0x128: the target hoists both start
 * codes, 0x1B5 and 0x1B2, into saved registers and tests them at the loop's foot; the `while
 * (a == X || a == Y)` written here keeps only one). */

#include "libmpeg.h"

extern int _isDirty[];
extern int _dispatchMpegCbNodata(sceMpeg *mp);
extern int _dispatchMpegCallback(sceMpeg *mp, sceMpegCbData *cbdata);
extern void _Error(MpegDec *d, char *msg);
extern void _sequenceHeader(MpegDec *d);
extern void _updateTempTackData(MpegDec *d, int tr);
extern void _unknown_extension(MpegDec *d);
extern void _sequenceExtension(MpegDec *d);
extern void _sequenceDisplayExtension(MpegDec *d);
extern void _sequenceScalableExtension(MpegDec *d);
extern void _pictureSpatialScalableExtension(MpegDec *d);
extern void _pictureTemporalScalableExtension(MpegDec *d);

void _waitIpuIdle(MpegDec *d);
long _waitIpuIdle64(MpegDec *d);
u_int _peepBit(MpegDec *d, int n);
void _flushBuf(MpegDec *d, int n);
u_int _nextBit(MpegDec *d, int n);
void _nextStartCode(MpegDec *d);
void _extrainfo(MpegDec *d);
void _pictureHeader(MpegDec *d);
void _extensionAndUserData(MpegDec *d);
void _pictureCodingExtension(MpegDec *d);
void _groupOfPicturesHeader(MpegDec *d);
void _quantMatrixExtension(MpegDec *d);
void _pictureDisplayExtension(MpegDec *d);
void _copyrightExtension(MpegDec *d);

#define WAIT_IDLE(d) {								\
	int _i = 0;								\
	while ((*IPU_CTRL & 0x80004000) == 0x80000000) {			\
		if (_i++ > 5000) {						\
			_dispatchMpegCbNodata((d)->mp);				\
			_i = 0;							\
		}								\
	}									\
}

#define SEND_CMD(d, cmd) {							\
	*IPU_CMD_W = (cmd);							\
	(d)->dirty = _isDirty[(cmd) >> 28];					\
}

void _sendIpuCommand(MpegDec *d, u_int cmd)
{
	SEND_CMD(d, cmd);
}

void _waitIpuIdle(MpegDec *d)
{
	WAIT_IDLE(d);
}

long _waitIpuIdle64(MpegDec *d)
{
	long v;
	int i = 0;

	while ((v = *IPU_CMD) < 0 && !(*IPU_CTRL & 0x4000)) {
		if (i++ > 5000) {
			_dispatchMpegCbNodata(d->mp);
			i = 0;
		}
	}
	return v;
}

short _ipuVdec(MpegDec *d, int tbl)
{
	long r, top;
	u_int bp;
	int i = 0;

	WAIT_IDLE(d);
	SEND_CMD(d, (tbl << 26) | 0x30000000);
	while ((r = *IPU_CMD) < 0) {
		if (i++ > 5000) {
			_dispatchMpegCbNodata(d->mp);
			i = 0;
		}
	}
	top = *IPU_TOP;
	bp = *IPU_BP;
	d->bits = top;
	d->nbits = (top < 0) ? -(bp & 0x1F) & 0x1F : 32;
	d->ipuEnd = ((int)r == 0);
	return r & 0xFFFF;
}

u_int _peepBit(MpegDec *d, int n)
{
	if (d->dirty || d->nbits < n) {
		WAIT_IDLE(d);
		SEND_CMD(d, 0x40000000);
		d->bits = _waitIpuIdle64(d);
		d->nbits = 32;
	}
	return d->bits >> (32 - n);
}

void _flushBuf(MpegDec *d, int n)
{
	WAIT_IDLE(d);
	SEND_CMD(d, (u_int)n | 0x40000000);
	d->bits = _waitIpuIdle64(d);
	d->nbits = 32;
}

u_int _nextBit(MpegDec *d, int n)
{
	u_int v;

	WAIT_IDLE(d);
	if (d->dirty || d->nbits < n) {
		SEND_CMD(d, 0x40000000);
		d->bits = _waitIpuIdle64(d);
	}
	v = d->bits >> (32 - n);
	d->nbits = 32;
	SEND_CMD(d, (u_int)n | 0x40000000);
	d->bits = _waitIpuIdle64(d);
	return v;
}

void _nextStartCode(MpegDec *d)
{
	int n;

	_waitIpuIdle(d);
	n = -(*IPU_BP & 7) & 7;
	if (n)
		_flushBuf(d, n);
	while (_peepBit(d, 24) != 1)
		_flushBuf(d, 8);
}

int _sliceB(MpegDec *d)
{
	d->_1b4 = _nextBit(d, 5);
	if (_nextBit(d, 1)) {
		_nextBit(d, 1);
		_flushBuf(d, 7);
		_extrainfo(d);
	}
	return 0;
}

int _nextHeader(MpegDec *d)
{
	sceMpegCbDataTimeStamp cb;

	for (;;) {
		_nextStartCode(d);
		switch (_nextBit(d, 32)) {
		case 0x1B3:
			_sequenceHeader(d);
			break;
		case 0x1B8:
			_groupOfPicturesHeader(d);
			break;
		case 0x100:
			_pictureHeader(d);
			cb.type = sceMpegCbTimeStamp;
			cb.pts = -1;
			cb.dts = -1;
			_dispatchMpegCallback(d->mp, (sceMpegCbData *)&cb);
			d->pts = cb.pts;
			d->dts = cb.dts;
			return d->picture_coding_type;
		case 0x1B7:
			return 0;
		}
	}
}

void _pictureHeader(MpegDec *d)
{
	int tr = _nextBit(d, 10);

	d->picture_coding_type = _nextBit(d, 3);
	_nextBit(d, 16);
	if (d->picture_coding_type == 2 || d->picture_coding_type == 3) {
		d->full_pel_forward_vector = _nextBit(d, 1);
		d->forward_f_code = _nextBit(d, 3);
	}
	if (d->picture_coding_type == 3) {
		d->full_pel_backward_vector = _nextBit(d, 1);
		d->backward_f_code = _nextBit(d, 3);
	}
	_extrainfo(d);
	_extensionAndUserData(d);
	_updateTempTackData(d, tr);
}

void _extensionAndUserData(MpegDec *d)
{
	void (*ext[11])(MpegDec *) = {
		_unknown_extension, _sequenceExtension, _sequenceDisplayExtension,
		_quantMatrixExtension, _copyrightExtension, _sequenceScalableExtension,
		_unknown_extension, _pictureDisplayExtension, _pictureCodingExtension,
		_pictureSpatialScalableExtension, _pictureTemporalScalableExtension
	};
	u_int code, id;

	_nextStartCode(d);
	while ((code = _peepBit(d, 32)) == 0x1B5 || code == 0x1B2) {
		if (code == 0x1B5) {
			_flushBuf(d, 32);
			id = _nextBit(d, 4);
			if (id > 10)
				id = 0;
			ext[id](d);
			_nextStartCode(d);
		} else {
			_flushBuf(d, 32);
			_nextStartCode(d);
		}
	}
}

void _pictureCodingExtension(MpegDec *d)
{
	d->f_code[0][0] = _nextBit(d, 4);
	d->f_code[0][1] = _nextBit(d, 4);
	d->f_code[1][0] = _nextBit(d, 4);
	d->f_code[1][1] = _nextBit(d, 4);
	*IPU_CTRL = (*IPU_CTRL & ~0x30000) | (_nextBit(d, 2) << 16);
	d->picture_structure = _nextBit(d, 2);
	if (!d->firstStructure)
		d->firstStructure = d->picture_structure;
	d->top_field_first = _nextBit(d, 1);
	d->frame_pred_frame_dct = _nextBit(d, 1);
	d->concealment_motion_vectors = _nextBit(d, 1);
	*IPU_CTRL = (*IPU_CTRL & ~0x400000) | (_nextBit(d, 1) << 22);
	*IPU_CTRL = (*IPU_CTRL & ~0x200000) | (_nextBit(d, 1) << 21);
	*IPU_CTRL = (*IPU_CTRL & ~0x100000) | (_nextBit(d, 1) << 20);
	d->repeat_first_field = _nextBit(d, 1);
	_nextBit(d, 1);
	d->progressive_frame = _nextBit(d, 1);
	if (_nextBit(d, 1)) {
		_nextBit(d, 1);
		_nextBit(d, 3);
		_nextBit(d, 1);
		_nextBit(d, 7);
		_nextBit(d, 8);
	}
}

void _extrainfo(MpegDec *d)
{
	while (_nextBit(d, 1))
		_flushBuf(d, 8);
}

void _groupOfPicturesHeader(MpegDec *d)
{
	d->brokenLink = 0;
	d->_854 = 1;
	d->_84c = d->_850 + 1;
	_nextBit(d, 1);
	_nextBit(d, 5);
	_nextBit(d, 6);
	_nextBit(d, 1);
	_nextBit(d, 6);
	_nextBit(d, 6);
	d->closed_gop = _nextBit(d, 1);
	d->broken_link = _nextBit(d, 1);
	_extensionAndUserData(d);
}

void _quantMatrixExtension(MpegDec *d)
{
	if ((d->load_intra_quantiser_matrix = _nextBit(d, 1))) {
		_waitIpuIdle(d);
		_sendIpuCommand(d, 0x50000000);
		_waitIpuIdle(d);
	}
	if ((d->load_non_intra_quantiser_matrix = _nextBit(d, 1))) {
		_waitIpuIdle(d);
		_sendIpuCommand(d, 0x58000000);
		_waitIpuIdle(d);
	}
	if (_nextBit(d, 1))
		_Error(d, "load_chroma_intra_quantizer_matrix == 1");
	if (_nextBit(d, 1))
		_Error(d, "load_chroma_non_intra_quantizer_matrix == 1");
}

void _pictureDisplayExtension(MpegDec *d)
{
	int i, n;

	if (d->progressive_sequence) {
		if (d->repeat_first_field)
			n = d->top_field_first ? 3 : 2;
		else
			n = 1;
	} else {
		if (d->picture_structure != 3)
			n = 1;
		else
			n = d->repeat_first_field ? 3 : 2;
	}
	for (i = 0; i < n; i++) {
		d->frame_centre_horizontal_offset[i] = _nextBit(d, 16);
		_nextBit(d, 1);
		d->frame_centre_vertical_offset[i] = _nextBit(d, 16);
		_nextBit(d, 1);
	}
}

void _copyrightExtension(MpegDec *d)
{
	_nextBit(d, 1);
	_nextBit(d, 8);
	_nextBit(d, 1);
	_nextBit(d, 7);
	_nextBit(d, 1);
	_nextBit(d, 20);
	_nextBit(d, 1);
	_nextBit(d, 22);
	_nextBit(d, 1);
	_nextBit(d, 22);
}
