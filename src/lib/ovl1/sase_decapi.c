/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* Nellymoser's decoder API (OVL1 0x255A68-0x255E48): the codec's description (0x255A68, GCC round
 * 28a: its 0x70-byte block as zNetVoice.cpp's CodecInfo reads it), the decoder's handle (magic 0xface, kind 11)
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
	long f32;
	struct SaseDecFrame *frame;     /* 40: the frame being decoded */
} SaseHandle;

typedef struct SaseDecFrame {
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

/* the decoded frame's parameters as the caller sees them: the gain and the two 12-term arrays */
typedef struct {
	float fGain;
	float afA[12];
	float afB[12];
} SaseDecParams;

extern void func_00253DF8(void *blk, void *in, int *pf4, void *out, unsigned short n, unsigned short *pf14,
                          unsigned char *pf16, float *pfB, float *pfA, float *pfGain);

/* a frame decoded behind the API's checks (the handle's, then 0x255D10's), its parameters copied
 * out when asked (GCC round 28a) */
int func_00254EC8(SaseHandle *h, SaseDecFrame *f, SaseDecParams *pOut)
{
	float afB[12];
	float afA[16];
	float fGain;
	int i;

	if (h == 0 || h->magic != 0xface)
		return 6100;
	if (h->type != 11)
		return 6115;
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
	h->frame = f;
	func_00253DF8(h->blk, f->in, &f->f4, f->out, f->f12, &f->f14, &f->f16, afB, afA, &fGain);
	if (pOut != 0) {
		pOut->fGain = fGain;
		for (i = 0; i < 12; i++)
			pOut->afA[i] = afA[i];
		for (i = 0; i < 12; i++)
			pOut->afB[i] = afB[i];
	}
	return 0;
}

/* what the codec says of itself (zNetVoice.cpp's CodecInfo reads the frame's samples at 60 and
 * the packed frame's bits at 96): 0x70 bytes */
typedef struct {
	short sVersion;                 /* 4 */
	char acVersion[12];             /* "SC.01.01.00" */
	char pad14[26];
	long l40;                       /* 100 */
	long l48;                       /* 3000 */
	short sRate;                    /* 8000 */
	short sBits;                    /* 16 */
	short sSamples;                 /* 160 */
	short s62;
	short s64;
	long l72;                       /* 3200 */
	long l80;
	long l88;
	long lFrameBits;                /* 96: 64 */
	unsigned short us104;
	short s106;
} SaseCodecInfo;

extern float ceilf(float x);
extern char *strcpy(char *d, const char *s);

/* the codec described (GCC round 28a) */
int func_00255A68(SaseCodecInfo *p)
{
	unsigned int u;

	if (p == 0)
		return 6170;
	p->sVersion = 4;
	strcpy(p->acVersion, "SC.01.01.00");
	p->l80 = 64;
	p->l88 = 64;
	p->lFrameBits = 64;
	p->l40 = 100;
	p->l48 = 3000;
	p->sRate = 8000;
	p->sBits = 16;
	p->sSamples = 160;
	p->l72 = 3200;
	u = ceilf(0.90625F);
	p->us104 = u;
	p->s62 = 1;
	p->s64 = 1;
	p->s106 = p->us104 * 160 - 145;
	return 0;
}

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
