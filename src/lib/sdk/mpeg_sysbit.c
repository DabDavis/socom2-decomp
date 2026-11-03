/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libmpeg's system-stream bit reader (MAIN 0x1BA670-0x1BA87C): Sony's SDK, the demo's object
 * 0x1583A0 (C), its seven functions in the same order. A 64-bit window over a ring buffer: the
 * window holds `count` valid bits from its top, refilled a byte at a time to at least 57; `pos`
 * counts the bits consumed since _sysbitInit. Names are SOCOM 1's (s1:exact). */

typedef unsigned int u_int;
typedef unsigned char u_char;
typedef unsigned long u_long;

typedef struct {
	u_long bits;		/* 0x00: the window, its next bit at the top */
	u_char *start;		/* 0x08: where the stream starts */
	u_char *ptr;		/* 0x0C: the next byte to load */
	u_int count;		/* 0x10: valid bits in the window */
	long pos;		/* 0x18: bits consumed */
	u_char *bufStart;	/* 0x20: the ring */
	u_char *bufEnd;		/* 0x24 */
	int bufSize;		/* 0x28 */
} sceMpegSysBit;

void _sysbitFlush(sceMpegSysBit *sb, int n);

void _sysbitInit(sceMpegSysBit *sb, u_char *start, u_char *buf, int size)
{
	sb->start = start;
	sb->ptr = start;
	sb->bits = 0;
	sb->count = 0;
	sb->pos = 0;
	sb->bufStart = buf;
	sb->bufEnd = buf + size;
	sb->bufSize = size;
	_sysbitFlush(sb, 0);
}

int _sysbitNext(sceMpegSysBit *sb, int n)
{
	return sb->bits >> (64 - n);
}

void _sysbitFlush(sceMpegSysBit *sb, int n)
{
	sb->bits <<= n;
	sb->count -= n;
	while (sb->count <= 56) {
		sb->bits |= (u_long)*sb->ptr++ << (56 - sb->count);
		if (sb->ptr >= sb->bufEnd)
			sb->ptr = sb->bufStart;
		sb->count += 8;
	}
	sb->pos += n;
}

int _sysbitGet(sceMpegSysBit *sb, int n)
{
	int v = _sysbitNext(sb, n);

	_sysbitFlush(sb, n);
	return v;
}

int _sysbitMarker(sceMpegSysBit *sb)
{
	int v = _sysbitNext(sb, 1);

	_sysbitFlush(sb, 1);
	return v;
}

void _sysbitJump(sceMpegSysBit *sb, int n)
{
	sb->bits = 0;
	sb->count = 0;
	sb->pos += n * 8;
	sb->ptr = sb->start + (int)(sb->pos >> 3);
	if (sb->ptr >= sb->bufEnd)
		sb->ptr -= sb->bufSize;
	_sysbitFlush(sb, 0);
}

u_char *_sysbitPtr(sceMpegSysBit *sb, int bits)
{
	u_char *p = sb->start + (bits >> 3);

	if (p >= sb->bufEnd)
		p -= sb->bufSize;
	return p;
}
