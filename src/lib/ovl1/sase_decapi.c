/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* Nellymoser's decoder API (OVL1 0x255A68-0x255E48): the codec's description (0x255A68, GCC round
 * 28a: its 0x70-byte block as zNetVoice.cpp's CodecInfo reads it), the decoder's handle (magic 0xface, kind 11)
 * made, destroyed (the caller's pointer left as it was, where the encoder's is zeroed), a frame
 * decoded and two of DecSC.c's setters (sase_decsc.c) reached, behind the checks the encoder's API makes (sase_encapi.c: 6100 a bad handle, 6115 not the decoder, 6000 out
 * of memory, the frame's 6125-6170). Built at -G0: the host's allocator, free and clear callbacks
 * and the instance count (MAIN 0x1D55A8-0x1D55C0) are reached by lui and an offset here, by $gp in
 * the encoder's objects. GCC round 45a: the three- and four-handle frames 0x255028, 0x255490 (each decoder's parameters by
 * 0x254020, the last handle's samples summed by DecSC.c's 0x254730 / 0x254A88): byte-exact as generated from 0x254EC8's text
 * once the handle's blk and frame share an alias set, so that the earlier handles' frame stores have the blk load as one more
 * dependent (rank_for_schedule's tie-break after priority and class: the insn with more later insns depending on it) and go
 * ahead of the f12 load; and the float beside each int in the synthesis's prototype, (fPitch1, iPeaks1, ...), which orders the
 * argument moves. Placeholders: the codec's names are not in SOCOM 1. */

typedef struct {
	long magic;             /* 0xface while the handle is live */
	long f8;
	long f16;
	unsigned char type;     /* 11 */
	void *blk;              /* 28: the decoder's memory block */
	long f32;
	void *frame;     /* 40: the frame being decoded */
} SaseHandle;

typedef struct SaseDecFrame {
	unsigned char *in;      /* the packed frame */
	int f4;
	short *out;             /* the PCM samples */
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

extern void func_00254020(void *blk, void *in, int *pf4, void *out, unsigned short n, unsigned short *pf14,
			  unsigned char *pf16, float *pfPitch, int *piPeaks, float *pfB, float *pfA, float *pfGain, int iPcm);
extern void func_00254730(float f1, int n1, float f2, int n2, float f3, int n3, int iLength, void *psOut, void *p1, void *p2, void *p3);
extern void func_00254A88(float f1, int n1, float f2, int n2, float f3, int n3, float f4, int n4, int iLength, void *psOut,
			  void *p1, void *p2, void *p3, void *p4);

int func_00255028(SaseHandle *h1, SaseHandle *h2, SaseHandle *h3,
		   SaseDecFrame *f1, SaseDecFrame *f2, SaseDecFrame *f3,
		   SaseDecParams *p1, SaseDecParams *p2, SaseDecParams *p3)
{
	float afB1[12];
	float afA1[16];
	float afB2[12];
	float afA2[16];
	float afB3[12];
	float afA3[16];
	int iPeaks3;
	float fGain3;
	float fPitch3;
	int iPeaks2;
	float fGain2;
	float fPitch2;
	int iPeaks1;
	float fGain1;
	float fPitch1;
	int i;

	if (h1 == 0 || h1->magic != 0xface)
		return 6100;
	if (h1->type != 11)
		return 6115;
	if (h2 == 0 || h2->magic != 0xface)
		return 6100;
	if (h2->type != 11)
		return 6115;
	if (h3 == 0 || h3->magic != 0xface)
		return 6100;
	if (h3->type != 11)
		return 6115;
	if (f1 == 0)
		return 6170;
	if (f1->out == 0)
		return 6130;
	if (f1->in == 0)
		return 6125;
	if (f1->f14 >= f1->f12)
		return 6145;
	if (f1->f18 >= f1->f12)
		return 6150;
	if (f1->f16 >= 8)
		return 6135;
	if (f1->f20 >= 8)
		return 6140;
	if (f2 == 0)
		return 6170;
	if (f2->out == 0)
		return 6130;
	if (f2->in == 0)
		return 6125;
	if (f2->f14 >= f2->f12)
		return 6145;
	if (f2->f18 >= f2->f12)
		return 6150;
	if (f2->f16 >= 8)
		return 6135;
	if (f2->f20 >= 8)
		return 6140;
	if (f3 == 0)
		return 6170;
	if (f3->out == 0)
		return 6130;
	if (f3->in == 0)
		return 6125;
	if (f3->f14 >= f3->f12)
		return 6145;
	if (f3->f18 >= f3->f12)
		return 6150;
	if (f3->f16 >= 8)
		return 6135;
	if (f3->f20 >= 8)
		return 6140;
	h1->frame = f1;
	h2->frame = f2;
	h3->frame = f3;
	func_00254020(h3->blk, f3->in, &f3->f4, f3->out, f3->f12, &f3->f14, &f3->f16, &fPitch3, &iPeaks3, afB3, afA3, &fGain3, 0);
	func_00254020(h2->blk, f2->in, &f2->f4, f2->out, f2->f12, &f2->f14, &f2->f16, &fPitch2, &iPeaks2, afB2, afA2, &fGain2, 0);
	func_00254020(h1->blk, f1->in, &f1->f4, f1->out, f1->f12, &f1->f14, &f1->f16, &fPitch1, &iPeaks1, afB1, afA1, &fGain1, 1);
	func_00254730(fPitch1, iPeaks1, fPitch2, iPeaks2, fPitch3, iPeaks3, 160, f1->in, h1->blk, h2->blk, h3->blk);
	if (p1 != 0) {
		p1->fGain = fGain1;
		for (i = 0; i < 12; i++)
			p1->afA[i] = afA1[i];
		for (i = 0; i < 12; i++)
			p1->afB[i] = afB1[i];
	}
	if (p2 != 0) {
		p2->fGain = fGain2;
		for (i = 0; i < 12; i++)
			p2->afA[i] = afA2[i];
		for (i = 0; i < 12; i++)
			p2->afB[i] = afB2[i];
	}
	if (p3 != 0) {
		p3->fGain = fGain3;
		for (i = 0; i < 12; i++)
			p3->afA[i] = afA3[i];
		for (i = 0; i < 12; i++)
			p3->afB[i] = afB3[i];
	}
	return 0;
}

int func_00255490(SaseHandle *h1, SaseHandle *h2, SaseHandle *h3, SaseHandle *h4,
		   SaseDecFrame *f1, SaseDecFrame *f2, SaseDecFrame *f3, SaseDecFrame *f4,
		   SaseDecParams *p1, SaseDecParams *p2, SaseDecParams *p3, SaseDecParams *p4)
{
	float afB1[12];
	float afA1[16];
	float afB2[12];
	float afA2[16];
	float afB3[12];
	float afA3[16];
	float afB4[12];
	float afA4[16];
	int iPeaks4;
	float fGain4;
	float fPitch4;
	int iPeaks3;
	float fGain3;
	float fPitch3;
	int iPeaks2;
	float fGain2;
	float fPitch2;
	int iPeaks1;
	float fGain1;
	float fPitch1;
	int i;

	if (h1 == 0 || h1->magic != 0xface)
		return 6100;
	if (h1->type != 11)
		return 6115;
	if (h2 == 0 || h2->magic != 0xface)
		return 6100;
	if (h2->type != 11)
		return 6115;
	if (h3 == 0 || h3->magic != 0xface)
		return 6100;
	if (h3->type != 11)
		return 6115;
	if (h4 == 0 || h4->magic != 0xface)
		return 6100;
	if (h4->type != 11)
		return 6115;
	if (f1 == 0)
		return 6170;
	if (f1->out == 0)
		return 6130;
	if (f1->in == 0)
		return 6125;
	if (f1->f14 >= f1->f12)
		return 6145;
	if (f1->f18 >= f1->f12)
		return 6150;
	if (f1->f16 >= 8)
		return 6135;
	if (f1->f20 >= 8)
		return 6140;
	if (f2 == 0)
		return 6170;
	if (f2->out == 0)
		return 6130;
	if (f2->in == 0)
		return 6125;
	if (f2->f14 >= f2->f12)
		return 6145;
	if (f2->f18 >= f2->f12)
		return 6150;
	if (f2->f16 >= 8)
		return 6135;
	if (f2->f20 >= 8)
		return 6140;
	if (f3 == 0)
		return 6170;
	if (f3->out == 0)
		return 6130;
	if (f3->in == 0)
		return 6125;
	if (f3->f14 >= f3->f12)
		return 6145;
	if (f3->f18 >= f3->f12)
		return 6150;
	if (f3->f16 >= 8)
		return 6135;
	if (f3->f20 >= 8)
		return 6140;
	if (f4 == 0)
		return 6170;
	if (f4->out == 0)
		return 6130;
	if (f4->in == 0)
		return 6125;
	if (f4->f14 >= f4->f12)
		return 6145;
	if (f4->f18 >= f4->f12)
		return 6150;
	if (f4->f16 >= 8)
		return 6135;
	if (f4->f20 >= 8)
		return 6140;
	h1->frame = f1;
	h2->frame = f2;
	h3->frame = f3;
	h4->frame = f4;
	func_00254020(h4->blk, f4->in, &f4->f4, f4->out, f4->f12, &f4->f14, &f4->f16, &fPitch4, &iPeaks4, afB4, afA4, &fGain4, 0);
	func_00254020(h3->blk, f3->in, &f3->f4, f3->out, f3->f12, &f3->f14, &f3->f16, &fPitch3, &iPeaks3, afB3, afA3, &fGain3, 0);
	func_00254020(h2->blk, f2->in, &f2->f4, f2->out, f2->f12, &f2->f14, &f2->f16, &fPitch2, &iPeaks2, afB2, afA2, &fGain2, 0);
	func_00254020(h1->blk, f1->in, &f1->f4, f1->out, f1->f12, &f1->f14, &f1->f16, &fPitch1, &iPeaks1, afB1, afA1, &fGain1, 1);
	func_00254A88(fPitch1, iPeaks1, fPitch2, iPeaks2, fPitch3, iPeaks3, fPitch4, iPeaks4, 160, f1->in, h1->blk, h2->blk, h3->blk, h4->blk);
	if (p1 != 0) {
		p1->fGain = fGain1;
		for (i = 0; i < 12; i++)
			p1->afA[i] = afA1[i];
		for (i = 0; i < 12; i++)
			p1->afB[i] = afB1[i];
	}
	if (p2 != 0) {
		p2->fGain = fGain2;
		for (i = 0; i < 12; i++)
			p2->afA[i] = afA2[i];
		for (i = 0; i < 12; i++)
			p2->afB[i] = afB2[i];
	}
	if (p3 != 0) {
		p3->fGain = fGain3;
		for (i = 0; i < 12; i++)
			p3->afA[i] = afA3[i];
		for (i = 0; i < 12; i++)
			p3->afB[i] = afB3[i];
	}
	if (p4 != 0) {
		p4->fGain = fGain4;
		for (i = 0; i < 12; i++)
			p4->afA[i] = afA4[i];
		for (i = 0; i < 12; i++)
			p4->afB[i] = afB4[i];
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
