/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libmpeg's PSS demultiplexer (MAIN 0x1BA880-0x1BB688): Sony's SDK, the demo's object 0x1585B0
 * (C), eight functions in this order -- _type2id, _id2type, sceMpegDemuxPssRing, sceMpegDemuxPss,
 * sceMpegAddStrCallback, _pack_header, _system_header, _PES_packet. The stream map `_strmap` (ten
 * 16-byte entries: a stream id and the mask of its channel bits, the demo's label) turns a stream
 * type and channel into the id the packets carry. Written in the GCC lane's round 19a:
 * _system_header (its entry loop a while, laid out by 991111's 8-aligned loop head), _type2id
 * (OPEN); GCC round 30a: _type2id, _id2type, sceMpegAddStrCallback and _pack_header (5 of the
 * object's 8); GCC round 45a: sceMpegDemuxPssRing, sceMpegDemuxPss (over its draft) and _PES_packet,
 * the object whole. Names are SOCOM 1's (s1:exact). What they showed: a switch's case tree reads its
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

/* mpeg_sysbit.c's reader; pos is compared unsigned here */
typedef struct {
	u_long bits;		/* 0x00 */
	u_char *start;		/* 0x08 */
	u_char *ptr;		/* 0x0C */
	u_int count;		/* 0x10 */
	u_long pos;		/* 0x18: bits consumed */
	u_char *bufStart;	/* 0x20 */
	u_char *bufEnd;		/* 0x24 */
	int bufSize;		/* 0x28 */
} sceMpegSysBit;

/* what _PES_packet reads: the stream id, the packet's bit positions (the names ours) */
typedef struct {
	u_long id;		/* 0x00 */
	int size;		/* 0x08 */
	int scrambling;		/* 0x0C */
	long pts;		/* 0x10 */
	long dts;		/* 0x18 */
	int data;		/* 0x20 */
	int len;		/* 0x24 */
	int header;		/* 0x28 */
	char pad2c[0x38 - 0x2C];
} MpegPes;

typedef struct {
	MpegPackHeader pack;	/* 0x00 */
	long _10;		/* 0x10 */
	MpegPes pes;		/* 0x18 */
} MpegPssHeaders;

extern void _sysbitInit(sceMpegSysBit *sb, u_char *start, u_char *buf, int size);
extern u_char *_sysbitPtr(sceMpegSysBit *sb, int bits);
extern void _sysbitJump(sceMpegSysBit *sb, int n);
extern void _Error(MpegDec *d, char *msg);
extern int _sysbitNext(sceMpegSysBit *sb, int n);
extern int _sysbitGet(sceMpegSysBit *sb, int n);
extern void _sysbitMarker(sceMpegSysBit *sb);

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

int _pack_header(sceMpegSysBit *sb, MpegPackHeader *h);
int _PES_packet(MpegDec *d, sceMpegSysBit *sb, MpegPes *p);

/* a program stream's packs walked and each PES packet handed to its stream's callback, or to the
 * catch-all's (id 0xBDFF<<24), until a callback says stop: the bytes consumed to the last one that
 * did not. GCC round 45a. The four locals at 160-172(sp) are SPILLED PSEUDOS, not a struct (slots in
 * pseudo order: nine saved registers are taken), which is why they are read back in loops the
 * optimizer would otherwise hoist from; `!allcb` in the first loop's condition, where the first
 * test folds and only the count is tested (`if (allcb) break` leaves that loop unrotated); and the
 * outer test split, `if (pos > n) break; } while (next == 0x1BA)`, not `while (pos <= n && ...)`:
 * this compiler's expand_end_loop scans the whole loop body for a conditional exit and rolls up to
 * it, so an `&&` there moves the second test above the loop's head (a `goto` loop has no notes at
 * all, 0x2D8). The order of the first block's assignments is sched1's: alldata = 0 after the call
 * and ahead of cont, so it is issued with the call, its spill store then stays above the count's
 * load at sched2 and ret's stays below. */
int sceMpegDemuxPssRing(sceMpeg *mp, u_char *pss, int size, u_char *buf, int bufsize)
{
	sceMpegSysBit sbuf, *sb;
	MpegPssHeaders hbuf, *h;
	sceMpegCbDataStr cbstr;
	MpegDec *d;
	sceMpegCallback allcb;
	void *alldata;
	MpegStrCb *t;
	int ret, cont, i;

	d = mp->sys;
	t = d->work;
	allcb = 0;
	h = &hbuf;
	_sysbitInit(&sbuf, pss, buf, bufsize);
	alldata = 0;
	sb = &sbuf;
	cont = 1;
	ret = 0;
	for (i = 0; !allcb && i < d->_048; i++) {
		if (t[i].id == 0xBDFF000000) {
			allcb = t[i].cb;
			alldata = t[i].data;
		}
	}
	do {
		if (_sysbitNext(sb, 32) == 0x1BA)
			_pack_header(sb, &h->pack);
		while (_sysbitNext(sb, 24) == 1 && _sysbitNext(sb, 32) != 0x1BA && _sysbitNext(sb, 32) != 0x1B9
		       && sb->pos < size * 8 && cont) {
			_PES_packet(d, sb, &h->pes);
			if (sb->pos > size * 8)
				continue;
			for (i = 0; i < d->_048; i++) {
				if (t[i].id == (h->pes.id & t[i].mask)) {
					cbstr.type = sceMpegCbStr;
					cbstr.header = _sysbitPtr(sb, h->pes.header);
					cbstr.data = _sysbitPtr(sb, h->pes.data);
					cbstr.len = h->pes.len;
					cbstr.pts = h->pes.pts;
					cbstr.dts = h->pes.dts;
					cont = t[i].cb(mp, (sceMpegCbData *)&cbstr, t[i].data);
					break;
				}
			}
			if (i == d->_048 && allcb) {
				cbstr.type = sceMpegCbStr;
				cbstr.header = _sysbitPtr(sb, h->pes.header);
				cbstr.data = _sysbitPtr(sb, h->pes.data);
				cbstr.len = h->pes.len;
				cbstr.pts = h->pes.pts;
				cbstr.dts = h->pes.dts;
				cont = allcb(mp, (sceMpegCbData *)&cbstr, alldata);
			}
			if (cont)
				ret = sb->pos >> 3;
		}
		if (sb->pos > size * 8)
			break;
	} while (_sysbitNext(sb, 32) == 0x1BA);
	return ret;
}

/* a whole stream in memory: no ring (the bound is a buffer of size -1) */
int sceMpegDemuxPss(sceMpeg *mp, u_char *pss, int size)
{
	return sceMpegDemuxPssRing(mp, pss, size, 0, -1);
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

/* a PES packet's header read as ISO 13818-1's PES_packet() lays it out: the stream id, the length,
 * the two timestamps, and what remains jumped over, its data's bit position left in p->data for the
 * callback. Three branches in the standard's own order; a private stream's sub-stream id is or-ed
 * into the id. SKIP's index is the four flags ES_rate, DSM_trick_mode, additional_copy_info and
 * PES_CRC; its value the bits they add. GCC round 45a: written from the standard, byte-exact after
 * four readings: a, b and c scoped to each timestamp's block (block-local, local-alloc's s0-s2,
 * shared across the function they are global and sb moves); dts = -1 before pts = -1; one `if (m)`
 * after the private stream's arm, whose join label keeps the store out of the branch's slot (each
 * arm with its own call gives the slot the store, 0x5AC); and _sysbitMarker void in this unit: as an int
 * the low word's temporary misses v0 (12 words; a named temporary for it closes the same 12, so which
 * of the two the source had is not settled). */
int _PES_packet(MpegDec *d, sceMpegSysBit *sb, MpegPes *p)
{
	unsigned char skip[16] = { 0, 16, 8, 24, 8, 24, 16, 32, 24, 40, 32, 48, 32, 48, 40, 56 };
	int ptsdts, escr, flags, ext, hlen, pos0, n, m;

	p->header = sb->pos;
	_sysbitGet(sb, 24);
	p->id = (u_long)_sysbitGet(sb, 8) << 32;
	p->size = _sysbitGet(sb, 16);
	p->dts = -1;
	p->pts = -1;
	if (p->id != 0xBC00000000 && p->id != 0xBE00000000 && p->id != 0xBF00000000 && p->id != 0xF000000000
	    && p->id != 0xF100000000 && p->id != 0xFF00000000 && p->id != 0xF200000000 && p->id != 0xF800000000) {
		_sysbitGet(sb, 2);
		p->scrambling = _sysbitGet(sb, 2);
		_sysbitGet(sb, 4);
		ptsdts = _sysbitGet(sb, 2);
		escr = _sysbitGet(sb, 1);
		flags = _sysbitGet(sb, 4);
		ext = _sysbitGet(sb, 1);
		hlen = _sysbitGet(sb, 8);
		pos0 = sb->pos;
		if (ptsdts & 2) {
			u_int a, b, c;

			_sysbitGet(sb, 4);
			a = _sysbitGet(sb, 3);
			_sysbitMarker(sb);
			b = _sysbitGet(sb, 15);
			_sysbitMarker(sb);
			c = _sysbitGet(sb, 15);
			_sysbitMarker(sb);
			p->pts = ((long)((a >> 2) & 1) << 32) | (u_int)((a << 30) | (b << 15) | c);
		}
		if (ptsdts == 3) {
			u_int a, b, c;

			_sysbitGet(sb, 4);
			a = _sysbitGet(sb, 3);
			_sysbitMarker(sb);
			b = _sysbitGet(sb, 15);
			_sysbitMarker(sb);
			c = _sysbitGet(sb, 15);
			_sysbitMarker(sb);
			p->dts = ((long)((a >> 2) & 1) << 32) | (u_int)((a << 30) | (b << 15) | c);
		}
		if (escr == 1)
			_sysbitGet(sb, 48);
		if (flags)
			_sysbitGet(sb, skip[flags]);
		if (ext == 1) {
			int priv, pack, seq, pstd, ext2;

			priv = _sysbitGet(sb, 1);
			pack = _sysbitGet(sb, 1);
			seq = _sysbitGet(sb, 1);
			pstd = _sysbitGet(sb, 1);
			_sysbitGet(sb, 3);
			ext2 = _sysbitGet(sb, 1);
			if (priv == 1) {
				_sysbitGet(sb, 48);
				_sysbitGet(sb, 48);
				_sysbitGet(sb, 32);
			}
			if (pack == 1) {
				_Error(d, "pack_header_field_flag needs to be '0' in PS\n");
				return 0;
			}
			if (seq == 1)
				_sysbitGet(sb, 16);
			if (pstd == 1)
				_sysbitGet(sb, 16);
			if (ext2 == 1) {
				u_int i, m;

				_sysbitMarker(sb);
				m = _sysbitGet(sb, 7);
				for (i = 0; i < m; i++)
					_sysbitGet(sb, 8);
			}
		}
		m = hlen - (int)((sb->pos - pos0) >> 3);
		if (m)
			_sysbitJump(sb, m);
		n = p->size - hlen;
		m = n - 3;
		p->len = m;
		p->data = sb->pos;
		if (p->id == 0xBD00000000) {
			p->id |= (u_int)_sysbitGet(sb, 32);
			m = n - 7;
		}
		if (m)
			_sysbitJump(sb, m);
	} else if (p->id == 0xBC00000000 || p->id == 0xBF00000000 || p->id == 0xF000000000 || p->id == 0xF100000000
		   || p->id == 0xFF00000000 || p->id == 0xF200000000 || p->id == 0xF800000000) {
		n = p->size;
		if (p->id == 0xBF00000000) {
			p->id |= (u_int)_sysbitGet(sb, 32);
			n -= 4;
		}
		if (n)
			_sysbitJump(sb, n);
	} else if (p->id == 0xBE00000000) {
		if (p->size)
			_sysbitJump(sb, p->size);
	}
	return 1;
}
