/* sc: compiler=ee-gcc2.96; flags=-O2 -G8 */
/* Nellymoser's encoder API (OVL1 0x24F518-0x24FAC0): the handle the host holds -- a 64-bit magic
 * 0xface, the codec's kind (10) and the encoder's memory block -- checked by every entry point
 * (6100 a bad handle, 6110 the wrong kind; the frame's own checks 6125-6170), then the work done in
 * EncSC.c (sase_encsc.c) and the coder (12 of 13). The handle's constructor, func_0024F990, stands
 * as a draft. The release call returns a value the destructor discards: declared int, the free
 * callback's pointer is loaded into v1 after it, as the target's is. Placeholders: the codec's names are not in SOCOM 1. Built as nelly.c is (ee-gcc 2.96,
 * -O2 -G8: the instance count and the host's free callback are $gp small data). */

typedef struct {
	long magic;             /* 0xface while the handle is live */
	long f8;
	long f16;
	unsigned char type;     /* 10 */
	void *blk;              /* 28: the encoder's memory block */
	int f32;
	int f36;
} SaseHandle;

typedef struct {
	void *in;               /* the PCM samples */
	int f4;
	void *out;              /* the packed frame */
	unsigned short f12;
	unsigned short f14;
	unsigned char f16;
	unsigned short f18;
	unsigned char f20;
} SaseEncFrame;

extern unsigned short data_001D55C0;            /* live handles */
extern void (*data_001D55AC)(void *);           /* the host's free callback */

extern void func_00249608(void *blk, float x);
extern void func_00249660(void *blk, float x);
extern void func_002496D8(void *blk, float x);
extern void func_00249750(void *blk, float x);
extern void func_002497A8(void *blk, float x, float y);
extern void func_00249810(void *blk);
extern void func_00249858(void *blk);
extern void func_00249898(void *blk);
extern void func_002498E0(void *blk);
extern void func_00249920(void *blk);
extern void func_00249968(void *blk);
extern void func_00248FB0(void *in, void *blk, void *work);
extern void func_002488A0(void *coded, void *blk, void *work);
extern void func_0024FAE8(void *coded, void *out, unsigned short *a, unsigned char *b, int n, int f);
extern int func_00249588(void *enc);

int func_0024F518(SaseHandle *h, float x)
{
	if (h == 0 || h->magic != 0xface)
		return 6100;
	if (h->type != 10)
		return 6110;
	func_00249608(h->blk, x);
	return 0;
}

int func_0024F568(SaseHandle *h, float x)
{
	if (h == 0 || h->magic != 0xface)
		return 6100;
	if (h->type != 10)
		return 6110;
	func_00249660(h->blk, x);
	return 0;
}

int func_0024F5B8(SaseHandle *h, float x)
{
	if (h == 0 || h->magic != 0xface)
		return 6100;
	if (h->type != 10)
		return 6110;
	func_002496D8(h->blk, x);
	return 0;
}

int func_0024F608(SaseHandle *h, float x)
{
	if (h == 0 || h->magic != 0xface)
		return 6100;
	if (h->type != 10)
		return 6110;
	func_00249750(h->blk, x);
	return 0;
}

int func_0024F658(SaseHandle *h, float x, float y)
{
	if (h == 0 || h->magic != 0xface)
		return 6100;
	if (h->type != 10)
		return 6110;
	func_002497A8(h->blk, x, y);
	return 0;
}

int func_0024F6A8(SaseHandle *h)
{
	if (h == 0 || h->magic != 0xface)
		return 6100;
	if (h->type != 10)
		return 6110;
	func_00249810(h->blk);
	return 0;
}

int func_0024F6F8(SaseHandle *h)
{
	if (h == 0 || h->magic != 0xface)
		return 6100;
	if (h->type != 10)
		return 6110;
	func_00249858(h->blk);
	return 0;
}

int func_0024F748(SaseHandle *h)
{
	if (h == 0 || h->magic != 0xface)
		return 6100;
	if (h->type != 10)
		return 6110;
	func_00249898(h->blk);
	return 0;
}

int func_0024F798(SaseHandle *h)
{
	if (h == 0 || h->magic != 0xface)
		return 6100;
	if (h->type != 10)
		return 6110;
	func_002498E0(h->blk);
	return 0;
}

int func_0024F7E8(SaseHandle *h)
{
	if (h == 0 || h->magic != 0xface)
		return 6100;
	if (h->type != 10)
		return 6110;
	func_00249920(h->blk);
	return 0;
}

int func_0024F838(SaseHandle *h)
{
	if (h == 0 || h->magic != 0xface)
		return 6100;
	if (h->type != 10)
		return 6110;
	func_00249968(h->blk);
	return 0;
}

/* Encode one frame: the frame's arguments checked first, then the handle.
 * OPEN (2 words short): the target keeps the coded buffer's address (sp+192) in s0, the handle in
 * s1 and the frame in s2 -- the buffer's address born while the handle is still live -- where this
 * build reuses the handle's register for it after the handle's last load (a pointer to the buffer
 * declared first: 50 words). */
int func_0024F888(SaseHandle *h, SaseEncFrame *f)
{
	char work[192];
	char coded[80];

	if (f == 0)
		return 6170;
	if (f->out == 0)
		return 6130;
	if (f->in == 0)
		return 6125;
	if (f->f14 >= f->f12)
		return 6145;
	if (f->f18 >= f->f12)
		return 6150;
	if (f->f16 >= 8)
		return 6135;
	if (f->f20 >= 8)
		return 6140;
	if (h == 0 || h->magic != 0xface)
		return 6100;
	if (h->type != 10)
		return 6110;
	func_00248FB0(f->in, h->blk, work);
	func_002488A0(coded, h->blk, work);
	func_0024FAE8(coded, f->out, &f->f18, &f->f20, f->f12, 0);
	return 0;
}

/* Destroy: the magic cleared, the encoder's block released, the handle freed and zeroed. */
int func_0024FA30(SaseHandle **ph)
{
	SaseHandle *h;

	if (ph == 0)
		return 6170;
	if (*ph == 0 || (*ph)->magic != 0xface)
		return 6100;
	if ((*ph)->type != 10)
		return 6115;
	data_001D55C0--;
	h = *ph;
	h->magic = 0;
	func_00249588(&h->blk);
	data_001D55AC(h);
	*ph = 0;
	return 0;
}
