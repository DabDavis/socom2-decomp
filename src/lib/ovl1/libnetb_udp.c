/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* libnetb's UDP receive ring (OVL1 0x245690-0x245AD8): SCEA's network buffers, the demo's object
 * 0x424DF8 (C, five functions, the same order), prebuilt with ee-gcc 2.96 at -O2 -G0 (README "The
 * GCC front"). The ring is a run of datagrams, each behind a 64-byte header, mirrored to the IOP
 * (libnetb_trans_data); a header past the ring's end wraps it to 0. Results are RT_RESULTs (2: an
 * invalid parameter). Names are SOCOM 1's (s1:exact/callee); the fields are named by their use. */

typedef unsigned char u_char;
typedef unsigned int u_int;

typedef struct UDP_HDR {
	int used;                       /* 0x00: 1 once a datagram is written */
	int valid;                      /* 0x04: 1 while it is unread */
	u_int size;                     /* 0x08: the bytes behind the header */
	u_int capacity;                 /* 0x0C */
	int pad10[7];
	struct UDP_HDR *self;           /* 0x2C */
	int pad30[3];
	int end;                        /* 0x3C: 1 once complete */
} UDP_HDR;

typedef struct LIBNETB_RING {
	int pending;                    /* 0x00: the TCP side's bytes waiting */
	int pad04[4];
	int pad14;
	int size;                       /* 0x18: the ring's bytes */
	char *buf;                      /* 0x1C: the EE's copy */
	int pad20[3];
	char *iop_buf;                  /* 0x2C: the IOP's */
	int pad30[3];
	int off;                        /* 0x3C: the read position */
} LIBNETB_RING;

extern int libnetb_trans_data(void *src, void *dst, int size);

int libnetb_get_valid_data_header_ptr(LIBNETB_RING *n, UDP_HDR **phdr, u_char *valid);

int libnetb_is_data_waiting(LIBNETB_RING *n, u_char type, u_char peek, u_char *waiting)
{
	u_char valid;
	UDP_HDR *hdr;
	int ret;

	if (waiting == 0)
		return 2;
	*waiting = 0;
	if (n == 0)
		return 2;
	if (type == 1) {
		hdr = 0;
		if (peek == 0) {
			valid = 0;
			ret = libnetb_get_valid_data_header_ptr(n, &hdr, &valid);
			if (ret != 0)
				return ret;
			*waiting = valid;
			return 0;
		}
		hdr = (UDP_HDR *)(n->buf + n->off);
		if (hdr->used == 1 && hdr->end == 1)
			*waiting = 1;
		else
			*waiting = 0;
	} else {
		if (n->pending > 0)
			*waiting = 1;
		else
			*waiting = 0;
	}
	return 0;
}

/* OPEN (12 words and one more): the target copies hdr to v1 and computes size - 64 into a0 -- the
 * allocator's answer to a pseudo local to the last block; neither a local copy, the size computed
 * first, a combined test nor a result variable reproduces it. */
int libnetb_udp_init_buffer(UDP_HDR *hdr, u_int size)
{
	if (hdr == 0)
		return 2;
	if (size < 128)
		return 2;
	if (size < 64)
		return 2;
	hdr->capacity = size - 64;
	hdr->end = 0;
	hdr->size = size - 64;
	hdr->valid = 0;
	hdr->used = 0;
	return 0;
}

int libnetb_udp_calc_avail(UDP_HDR *hdr, u_int left, u_int *avail)
{
	u_int total = 0;
	u_int step;

	if (avail == 0)
		return 2;
	*avail = 0;
	if (hdr == 0)
		return 2;
	while (left >= 64 && hdr->used == 0 && hdr->end == 0) {
		step = hdr->size + 64;
		total += step;
		left -= step;
		hdr = (UDP_HDR *)((char *)hdr + step);
	}
	*avail = total;
	return 0;
}

/* OPEN (80 words, the size differs): the checks and the wrap are the target's; the split and the
 * final header's stores and trans_data's arguments are not yet in its order. */
int libnetb_udp_create_space(LIBNETB_RING *n, u_int size, u_int *alloc_size, u_char *ok)
{
	u_int avail;
	u_int need;
	u_int left;
	UDP_HDR *hdr;
	UDP_HDR *next;
	int ret;

	avail = 0;
	if (n == 0)
		return 2;
	if (alloc_size == 0)
		return 2;
	if (ok == 0)
		return 2;
	need = size + 64;
	if (need & 63)
		need = need - (need & 63) + 64;
	*alloc_size = need;
	if (n->off >= n->size)
		return 1;
	hdr = (UDP_HDR *)(n->buf + n->off);
	if (hdr->used != 0 || hdr->end != 0) {
		*ok = 0;
		return 0;
	}
	left = n->size - n->off;
	ret = libnetb_udp_calc_avail(hdr, left, &avail);
	if (ret != 0)
		return ret;
	if (avail < need && left - avail < 64 && hdr != (UDP_HDR *)n->buf) {
		hdr->capacity = left - 64;
		hdr->end = 1;
		hdr->used = 1;
		hdr->valid = 1;
		hdr->size = left - 64;
		hdr->self = hdr;
		ret = libnetb_trans_data(hdr, n->iop_buf + n->off, 64);
		if (ret != 0)
			return ret;
		n->off = 0;
		return libnetb_udp_create_space(n, size, alloc_size, ok);
	}
	if (avail < need) {
		*ok = 0;
		return 0;
	}
	if (need < avail) {
		next = (UDP_HDR *)(n->buf + n->off + need);
		next->used = 0;
		next->valid = 0;
		next->end = 0;
		next->capacity = avail - need - 64;
		next->size = avail - need - 64;
		ret = libnetb_trans_data(n->buf + n->off + need, n->iop_buf + n->off + need, 64);
		if (ret != 0)
			return ret;
	}
	hdr = (UDP_HDR *)(n->buf + n->off);
	hdr->capacity = need - 64;
	hdr->end = 0;
	hdr->used = 0;
	hdr->valid = 0;
	hdr->size = need - 64;
	ret = libnetb_trans_data(n->buf + n->off, n->iop_buf + n->off, 64);
	if (ret != 0)
		return ret;
	*ok = 1;
	return 0;
}

/* OPEN (44 words, the size differs): the walk is the target's (an unsigned position wrapping at the
 * ring's size, stopping where it began); its loop's invariants and the exits' stores are not. */
int libnetb_get_valid_data_header_ptr(LIBNETB_RING *n, UDP_HDR **phdr, u_char *valid)
{
	UDP_HDR *hdr;
	u_int off;
	u_int start;

	if (n == 0)
		return 2;
	if (phdr == 0)
		return 2;
	if (valid == 0)
		return 2;
	*phdr = 0;
	*valid = 0;
	start = n->off;
	hdr = (UDP_HDR *)(n->buf + start);
	off = start;
	while (hdr->valid == 1) {
		off = off + hdr->size + 64;
		if (off >= n->size)
			off = 0;
		if (off == start) {
			*phdr = 0;
			*valid = 0;
			return 0;
		}
		hdr = (UDP_HDR *)(n->buf + off);
	}
	if (hdr->used == 1 && hdr->end == 1) {
		*phdr = hdr;
		*valid = 1;
	} else {
		*phdr = 0;
		*valid = 0;
	}
	return 0;
}
