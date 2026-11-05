/* sc: compiler=ee-gcc2.96; flags=-O2 -G8 */
/* Nellymoser's encoder, DecVcFry.c (OVL1 0x248AD0-0x248F60): the voiced/vocal-fry decision -- the
 * decision itself (0x248AD0, its asserts this file's), the pitch track it keeps (0x248C80), the
 * track's block and its 13-float history allocated and cleared through the host's callbacks
 * (0x248D40) and its spectral distance (0x248E30). Written in the GCC lane's round 26a: 0x248C80, 0x248D40;
 * round 33a: 0x248E30 (the decision 0x248AD0 not yet). The constants
 * are the vendor's truncated conversions, as hex floats (round 12a). Placeholders: the codec's
 * names are not in SOCOM 1. Built as nelly.c is (ee-gcc 2.96, -O2 -G8). */

#define NellyNull 0

extern void *(*data_001D55A8)(int n, int size);		/* the host's allocator (calloc's arguments) */
extern void (*data_001D55B0)(void *p, int c, int n);	/* the host's memset */

extern void __assert(const char *file, int line, const char *expr);
#define assert(e) ((e) ? (void)0 : __assert(__FILE__, __LINE__, #e))

typedef struct {
	float fPitch;		/* the tracked pitch; 1000 when lost */
	float fConf;		/* frames it has held */
	float *pfHist;		/* 13 floats */
} DVF_TRACK;

/* the encoder's frame (sase_encsc.c's EncFrame): its pitch and voicing at 40, its gain at 80 */
typedef struct {
	char pad0[40];
	float afPitch[2];
	char pad48[32];
	float fGain;
} DVF_FRAME;

void func_00248E30(const float *pfLPC, float *pfHist, float *pfGainDist, float *pfDist, int iOrder,
		   float fGain);

/* the cepstral lifter 1 + 6 sin(pi n / 12), n = 1..12, as the vendor's decimals converted (0x3E7B88:
 * the object's .rodata starts with it: defined first) */
static const float afLifter[12] = {
	0x1.46c5e2p+1F, 4.0F, 0x1.4f8768p+2F, 0x1.8c8db8p+2F,
	0x1.b2ea5ep+2F, 7.0F, 0x1.b2ea5ep+2F, 0x1.8c8dc2p+2F,
	0x1.4f8768p+2F, 4.0F, 0x1.46c5e2p+1F, 1.0F,
};


/* the vocal-fry decision (GCC round 33a): a frame whose pitch is about half the track's (0.87 to
 * 1.15 of it, the frame voiced 0.2 or more) is vocal fry when its spectrum is close to the track's
 * history (the gain term within 2, the cepstral distance 0.5/0.4/0.1) and the track has held long
 * enough (3.5/2.5/1.5 frames). The track is read at the declarations (the target's loads before the
 * assert); the pitch pair through a pointer taken after it (the target's s0, reused from the track);
 * 0.1 is a double, the vendor's truncated conversion (0x3E7BF8). The three tests are one ||: their
 * shared then-label has three uses, so CSE has no `bVcFry == 1` there and keeps each `bVcFry = 1`
 * (reorg steals it into each bc1t's slot); as an else-if chain CSE deletes them (13 words), and a
 * reset at the block's head inverts the last test (7). */
int func_00248AD0(DVF_FRAME *hParamMblk, DVF_TRACK *pTrack, const float *pfLPC, int iOrder)
{
	float fGainDist, fDist;
	float fPitch = pTrack->fPitch;
	float fConf = pTrack->fConf;
	int bVcFry = 0;
	const float *pfFrame;
	float fHalf;

#line 72 "../../SaseEncVad/source/DecVcFry.c"
	assert(hParamMblk != NellyNull);
	pfFrame = hParamMblk->afPitch;
	func_00248E30(pfLPC, pTrack->pfHist, &fGainDist, &fDist, iOrder, hParamMblk->fGain);
	if (fPitch < pfFrame[0] && pfFrame[1] >= 0x1.999998p-3F) {
		fHalf = pfFrame[0] * 0.5F;
		if (fHalf * 0x1.bd70a2p-1F < fPitch && fPitch < fHalf * 0x1.266666p+0F)
			bVcFry = 1;
	}
	if (bVcFry == 1 && fGainDist <= 2.0F) {
		if ((fDist <= 0.5F && fConf >= 3.5F) || (fDist <= 0x1.999998p-2F && fConf >= 2.5F) || (fDist <= 0x1.9999999999999p-4 && fConf >= 1.5F))
			bVcFry = 1;
		else
			bVcFry = 0;
	} else
		bVcFry = 0;
	return bVcFry;
}

/* the pitch track: a candidate within 20 % of the track pulls it (0.3/0.7) and adds a frame,
 * else the candidate restarts it if voiced enough; a track within 10 % of the frame's own pitch
 * adds a frame too. Both stores come last, the count's first: the pitch's store then follows the
 * load of pfIn[0] it may alias and reorg puts it in the second test's slot (stored before the
 * test, it is scheduled above that load: 7 words, build/scratch/gcc/v_track.py). */
void func_00248C80(const float *pfIn, DVF_TRACK *p)
{
	const float *pfCand = pfIn + 10;
	float fPitch, fConf;

	fPitch = p->fPitch;
	fConf = p->fConf;
	if (pfCand[0] * 0x1.999998p-1F < fPitch && fPitch < pfCand[0] * 0x1.333332p+0F) {
		fPitch = fPitch * 0x1.333332p-2F + pfCand[0] * 0x1.666666p-1F;
		fConf += 1.0F;
	} else if (pfCand[1] >= 0x1.333332p-2F) {
		fPitch = pfCand[0];
		fConf = pfCand[1];
	} else {
		fPitch = 1000.0F;
		fConf = 0.0F;
	}
	if (pfIn[0] * 0x1.cccccc0p-1F < fPitch && fPitch < pfIn[0] * 0x1.199998p+0F)
		fConf += 1.0F;
	p->fConf = fConf;
	p->fPitch = fPitch;
}

/* the track's block and its history, cleared */
int func_00248D40(DVF_TRACK **ppTrack)
{
	DVF_TRACK *p;

	p = (DVF_TRACK *)data_001D55A8(1, sizeof(DVF_TRACK));
	*ppTrack = p;
	if (p == NellyNull)
		return 1;
	p->pfHist = (float *)data_001D55A8(13, sizeof(float));
	if (p->pfHist == NellyNull)
		return 1;
	p->fPitch = 1000.0F;
	p->fConf = 0.0F;
	data_001D55B0(p->pfHist, 0, 13 * sizeof(float));
	return 0;
}

extern void func_0024D8B8(const float *pfLPC, int iOrder, float *pfCs, int nCs, float fGain);
extern float fabsf(float);

/* the frame's spectral distance from the track's history (GCC round 33a): the frame's 13-point LPC
 * cepstrum (libtran.c's 0x24D8B8), its gain term's difference apart, the rest liftered and their
 * squared differences averaged over 12; a close frame is blended into the history (0.9 / 0.1),
 * another replaces it. The sum accumulates in *pfDist itself (a float pointer may alias the
 * history's: loaded and stored every pass, as the target); the table indexed, or its pointer
 * taken after the call (initialised at its declaration it is formed at the entry and held in a
 * fourth saved register: 0x134). */
void func_00248E30(const float *pfLPC, float *pfHist, float *pfGainDist, float *pfDist, int iOrder,
		   float fGain)
{
	float afCs[13];
	float fD;
	int i;

	func_0024D8B8(pfLPC, iOrder, afCs, 13, fGain);
	*pfGainDist = fabsf(pfHist[0] - afCs[0]);
	*pfDist = 0.0F;
	for (i = 1; i < 13; i++) {
		afCs[i] *= afLifter[i - 1];
		fD = afCs[i] - pfHist[i];
		*pfDist += fD * fD;
	}
	*pfDist *= 0x1.555554p-4F;
	if (*pfDist < 0x1.999998p-5F) {
		for (i = 0; i < 13; i++)
			pfHist[i] = afCs[i] * 0x1.ccccccp-1F + pfHist[i] * 0x1.999998p-4F;
	} else {
		for (i = 0; i < 13; i++)
			pfHist[i] = afCs[i];
	}
}
