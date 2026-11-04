/* sc: compiler=ee-gcc2.96; flags=-O2 -G8 */
/* Nellymoser's encoder, DecVcFry.c (OVL1 0x248AD0-0x248F60): the voiced/vocal-fry decision -- the
 * decision itself (0x248AD0, its asserts this file's), the pitch track it keeps (0x248C80), the
 * track's block and its 13-float history allocated and cleared through the host's callbacks
 * (0x248D40) and 0x248E30. Written in the GCC lane's round 26a: 0x248C80, 0x248D40. The constants
 * are the vendor's truncated conversions, as hex floats (round 12a). Placeholders: the codec's
 * names are not in SOCOM 1. Built as nelly.c is (ee-gcc 2.96, -O2 -G8). */

#define NellyNull 0

extern void *(*data_001D55A8)(int n, int size);		/* the host's allocator (calloc's arguments) */
extern void (*data_001D55B0)(void *p, int c, int n);	/* the host's memset */

typedef struct {
	float fPitch;		/* the tracked pitch; 1000 when lost */
	float fConf;		/* frames it has held */
	float *pfHist;		/* 13 floats */
} DVF_TRACK;

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
