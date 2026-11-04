/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libmpeg's PSS demultiplexer (MAIN 0x1BA880-0x1BB688): Sony's SDK, the demo's object 0x1585B0
 * (C), eight functions in this order -- _type2id, _id2type, sceMpegDemuxPssRing, sceMpegDemuxPss,
 * sceMpegAddStrCallback, _pack_header, _system_header, _PES_packet. The stream map `_strmap` (ten
 * 16-byte entries: a stream id and the mask of its channel bits, the demo's label) turns a stream
 * type and channel into the id the packets carry. Written in the GCC lane's round 19a:
 * _system_header (its entry loop a while, laid out by 991111's 8-aligned loop head), _type2id
 * (OPEN); the rest stand open or as drafts. Names are SOCOM 1's (s1:exact). */

typedef unsigned int u_int;
typedef unsigned char u_char;
typedef unsigned long u_long;

typedef struct {
	u_long id;
	u_long mask;		/* the channel's bits in the id */
} MpegStrMap;

extern MpegStrMap _strmap[10];

typedef struct sceMpegSysBit sceMpegSysBit;
extern int _sysbitNext(sceMpegSysBit *sb, int n);
extern int _sysbitGet(sceMpegSysBit *sb, int n);

/* a stream type's id with its channel shifted into the masked bits.
 * OPEN (0x68 against 0x88, a first reading): the target's case tree compares the 64-bit mask with
 * 0xFFFF<<24 (beq), then `sltu` for the range, then 0xFF00<<24, under a 16-byte frame it never
 * uses; this switch on a u_long compiles to a 32-bit compare with 0xFFFF and a movn, frameless.
 * The case labels or the mask's type are not yet the vendor's. */
u_long _type2id(int type, int ch)
{
	u_long id = 0;
	int shift = 0;

	if ((u_int)type < 10) {
		switch (_strmap[type].mask) {
		case 0xFFFF000000:
			shift = 24;
			break;
		case 0xFF00000000:
			shift = 32;
			break;
		default:
			shift = 0;
			break;
		}
		id = _strmap[type].id | ((u_long)ch << shift);
	}
	return id;
}

/* the system header skipped: its fixed fields, then a stream entry while the next bit is set */
int _system_header(sceMpegSysBit *sb)
{
	_sysbitGet(sb, 56);
	_sysbitGet(sb, 40);
	while (_sysbitNext(sb, 1) == 1)
		_sysbitGet(sb, 24);
	return 1;
}
