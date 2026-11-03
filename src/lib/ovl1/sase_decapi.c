/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* Nellymoser's decoder API (OVL1 0x255BD0-0x255E48): the decoder's handle (magic 0xface, kind 11)
 * made, destroyed (the caller's pointer left as it was, where the encoder's is zeroed), a frame
 * decoded and two of DecSC.c's setters (sase_decsc.c) reached, behind the checks the encoder's API makes (sase_encapi.c: 6100 a bad handle, 6115 not the decoder, 6000 out
 * of memory, the frame's 6125-6170). Built at -G0: the host's allocator, free and clear callbacks
 * and the instance count (MAIN 0x1D55A8-0x1D55C0) are reached by lui and an offset here, by $gp in
 * the encoder's objects. Placeholders: the codec's names are not in SOCOM 1. */

typedef struct {
	long magic;             /* 0xface while the handle is live */
	long f8;
	long f16;
	unsigned char type;     /* 11 */
	void *blk;              /* 28: the decoder's memory block */
} SaseHandle;

typedef struct {
	void *in;               /* the packed frame */
	int f4;
	void *out;              /* the PCM samples */
	unsigned short f12;
	unsigned short f14;
	unsigned char f16;
	unsigned short f18;
	unsigned char f20;
} SaseDecFrame;

extern void *(*data_001D55A8)(int n, int size);  /* the host's allocator (calloc's arguments) */
extern void (*data_001D55AC)(void *);           /* the host's free callback */
extern void (*data_001D55B0)(void *p, int c, int n);    /* the host's memset */
extern unsigned short data_001D55C0;            /* live handles */
extern int func_00253A98(void *blk);
extern int func_00254250(void *blk);
extern void func_00254368(void *blk, float x);
extern void func_002543B8(void *blk, float x);

int func_00255C68(SaseHandle **ph);

int func_00255BD0(SaseHandle **ph)
{
	SaseHandle *h;
	int ret;

	if (ph == 0)
		return 6170;
	h = data_001D55A8(1, 48);
	*ph = h;
	if (h == 0)
		return 6000;
	h->type = 11;
	h->magic = 0xface;
	ret = func_00253A98(&h->blk);
	if (ret != 0) {
		func_00255C68(ph);
		return 6000;
	}
	data_001D55C0++;
	return ret;
}

int func_00255C68(SaseHandle **ph)
{
	SaseHandle *h;

	if (ph == 0)
		return 6170;
	if (*ph == 0 || (*ph)->magic != 0xface)
		return 6100;
	if ((*ph)->type != 11)
		return 6115;
	data_001D55C0--;
	h = *ph;
	h->magic = 0;
	func_00254250(&h->blk);
	data_001D55AC(h);
	return 0;
}

int func_00255D10(SaseDecFrame *f)
{
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
	data_001D55B0(f->out, 0, f->f12);
	return 0;
}

int func_00255DA8(SaseHandle *h, float x)
{
	if (h == 0 || h->magic != 0xface)
		return 6100;
	if (h->type != 11)
		return 6115;
	func_00254368(h->blk, x);
	return 0;
}

int func_00255DF8(SaseHandle *h, float x)
{
	if (h == 0 || h->magic != 0xface)
		return 6100;
	if (h->type != 11)
		return 6115;
	func_002543B8(h->blk, x);
	return 0;
}
