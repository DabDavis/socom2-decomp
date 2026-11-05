/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libmpeg's PSS demultiplexer (MAIN 0x1BA880-0x1BB688): Sony's SDK, the demo's object 0x1585B0
 * (C), eight functions in this order -- _type2id, _id2type, sceMpegDemuxPssRing, sceMpegDemuxPss,
 * sceMpegAddStrCallback, _pack_header, _system_header, _PES_packet. The stream map `_strmap` (ten
 * 16-byte entries: a stream id and the mask of its channel bits, the demo's label) turns a stream
 * type and channel into the id the packets carry. Written in the GCC lane's round 19a:
 * _system_header (its entry loop a while, laid out by 991111's 8-aligned loop head), _type2id
 * (OPEN); GCC round 30a: _type2id, _id2type, sceMpegAddStrCallback and _pack_header (5 of the
 * object's 8; sceMpegDemuxPss stands as its draft, sceMpegDemuxPssRing and _PES_packet are not
 * written). Names are SOCOM 1's (s1:exact). What they showed: a switch's case tree reads its
 * cases off the masks the table holds (both case trees here have the 0xFFFF<<24 mask at the root,
 * a bare `sltu` for a node whose body cross-jumps with the fall-through's); a chain of `bne`s each
 * falling into its body is an if/else-if, not a two-case switch; a value AND-ed in each case into
 * one variable keeps the AND in the loop and only the constant hoisted (a separate test per case
 * hoists the AND, a variable mask set in each case hoists nothing); the switched-on id held in a
 * variable is compared after the switch without a reload; a default spelled with its case's
 * constant reads the hoisted constant's register (with the mask variable a3 stays live and the
 * dispatch's beq becomes a beql); the callback search is round 17a's peeled loop (its body ends in
 * the break), its `old = 0` before the call (the saved register from the entry) and `id ==
 * t[i].id` in that order. */

#include "libmpeg.h"

typedef struct {
	u_long id;
	u_long mask;		/* the channel's bits in the id */
} MpegStrMap;

extern MpegStrMap _strmap[10];

/* a stream callback (sceMpegAddStrCallback's): the decoder's work area holds 64 of them (0x600 bytes,
 * MpegDec.work) and their count (MpegDec._048) */
typedef struct {
	u_long id;		/* 0x00: _type2id's */
	u_long mask;		/* 0x08: the type's channel bits (_strmap's) */
	sceMpegCallback cb;	/* 0x10 */
	void *data;		/* 0x14 */
} MpegStrCb;

/* what _pack_header reads: the SCR's extension, its low 32 bits and its 33rd, and whether a system
 * header follows (the names ours) */
typedef struct {
	int scr_ext;		/* 0x0 */
	u_int scr;		/* 0x4 */
	int scr32;		/* 0x8 */
	int sysheader;		/* 0xC */
} MpegPackHeader;

typedef struct sceMpegSysBit sceMpegSysBit;
extern int _sysbitNext(sceMpegSysBit *sb, int n);
extern int _sysbitGet(sceMpegSysBit *sb, int n);
extern int _sysbitMarker(sceMpegSysBit *sb);

/* a stream type's id with its channel shifted into the masked bits. GCC round 30a (open since
 * round 19a: 0x68 against 0x88): the switch names the three masks _strmap holds, 0xFFFFFFFFFF
 * first, with no default -- its case tree has 0xFFFF<<24 at the root and 0xFFFFFFFFFF's shift-0
 * body cross-jumped with the fall-through's, so that node is a bare `sltu`; and with it comes the
 * 16-byte frame the code never uses. Two cases became an xor and a movn; a default added a node;
 * the masks in the other order lay the bodies out otherwise (0x74-0x94). */
u_long _type2id(int type, int ch)
{
	u_long id = 0;
	int shift = 0;

	if ((u_int)type < 10) {
		switch (_strmap[type].mask) {
		case 0xFFFFFFFFFF:
			shift = 0;
			break;
		case 0xFFFF000000:
			shift = 24;
			break;
		case 0xFF00000000:
			shift = 32;
			break;
		}
		id = _strmap[type].id | ((u_long)ch << shift);
	}
	return id;
}

/* an id's stream type and channel: the first entry of _strmap whose id the packet's matches under
 * the type's mask (a 16-bit channel for the PSS types; for private stream 1 and the MPEG streams the
 * sub-stream's channel bits); found or not */
int _id2type(int *type, int *ch, u_long id)
{
	u_int i;
	int found = 0;
	u_long mask, sid, v, cm;

	for (i = 0; i < 10 && !found; i++) {
		mask = _strmap[i].mask;
		switch (mask) {
		case 0xFFFFFFFFFF:
			v = id & 0xFFFFFF0000;
			if (v == _strmap[i].id) {
				*type = i;
				*ch = id & 0xFFFF;
				found = 1;
			}
			break;
		case 0xFFFF000000:
			sid = _strmap[i].id;
			switch (sid) {
			case 0xBD20000000:
				v = id & 0xFFE0000000;
				cm = 0x1F;
				break;
			case 0xBD80000000:
			case 0xBD88000000:
			case 0xBD90000000:
			case 0xBDA0000000:
				v = id & 0xFFF8000000;
				cm = 7;
				break;
			default:
				v = id & 0xFFFF000000;
				cm = 0;
				break;
			}
			if (v == sid) {
				*type = i;
				*ch = (id >> 24) & cm;
				found = 1;
			}
			break;
		case 0xFF00000000:
			sid = _strmap[i].id;
			if (sid == 0xE000000000) {
				v = id & 0xF000000000;
				cm = 0xF;
			} else if (sid == 0xC000000000) {
				v = id & 0xE000000000;
				cm = 0x1F;
			} else {
				v = id & mask;
				cm = 0;
			}
			if (v == sid) {
				*type = i;
				*ch = (id >> 32) & cm;
				found = 1;
			}
			break;
		}
	}
	return found;
}

/* a stream's callback set or replaced: the old one's returned */
sceMpegCallback sceMpegAddStrCallback(sceMpeg *mp, int type, int ch, sceMpegCallback cb, void *data)
{
	MpegDec *d = mp->sys;
	MpegStrCb *t = d->work;
	sceMpegCallback old = 0;
	u_long id = _type2id(type, ch);
	int i;

	for (i = 0; i < d->_048; i++) {
		if (id == t[i].id) {
			old = t[i].cb;
			break;
		}
	}
	if (i < 64) {
		t[i].id = id;
		t[i].mask = _strmap[type].mask;
		t[i].cb = cb;
		t[i].data = data;
		d->_048++;
	}
	return old;
}

int _system_header(sceMpegSysBit *sb, MpegPackHeader *h);

/* the pack header: the SCR in its three parts, its extension, the mux rate skipped, the stuffing
 * bytes, and a system header if one follows */
int _pack_header(sceMpegSysBit *sb, MpegPackHeader *h)
{
	u_int hi, mid, lo;
	u_int n, i;

	_sysbitGet(sb, 34);
	hi = _sysbitGet(sb, 3);
	_sysbitMarker(sb);
	mid = _sysbitGet(sb, 15);
	_sysbitMarker(sb);
	lo = _sysbitGet(sb, 15);
	_sysbitMarker(sb);
	h->scr_ext = _sysbitGet(sb, 9);
	_sysbitGet(sb, 30);
	n = _sysbitGet(sb, 3);
	h->scr = (hi << 30) | (mid << 15) | lo;
	h->scr32 = (hi >> 2) & 1;
	for (i = 0; i < n; i++)
		_sysbitGet(sb, 8);
	if (_sysbitNext(sb, 32) == 0x1BB) {
		h->sysheader = 1;
		_system_header(sb, h);
	} else
		h->sysheader = 0;
	return 1;
}

/* the system header skipped: its fixed fields, then a stream entry while the next bit is set */
int _system_header(sceMpegSysBit *sb, MpegPackHeader *h)
{
	_sysbitGet(sb, 56);
	_sysbitGet(sb, 40);
	while (_sysbitNext(sb, 1) == 1)
		_sysbitGet(sb, 24);
	return 1;
}
