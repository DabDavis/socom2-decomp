/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* isowdp: the recogniser's iso-word token DP block (OVL2 0x4E8AE8-0x4EB5E8, SOCOM 1's object at
 * 0x356970: InitIsoWordTokenDpContext, FreeIsoWordTokenDp, InitNewUtterance, ExtendNodeWheel,
 * RePruneToPho, FrameDPacoust, ExpandGarbageArrays, IsoWordTokenDpProcess, GetNBest,
 * ControlIsoWordTokenDp, ConstructIsoWordTokenDp, InitIsoWordTokenDpProcess, CalcNrNodes, ...,
 * ScoreCompare -- the object's last), ScanSoft's speech recogniser, prebuilt with ee-gcc 2.96 at
 * -O2 -G0 -fno-strict-aliasing -funroll-loops (README "The GCC front"). Names are SOCOM 1's; a
 * static another object also defines is NAME@isowdp in the map. Only the functions written so
 * far are here, in image order: ScoreCompare. */

/* a token as the N-best sort sees it */
typedef struct TOKEN {
    void *f0;
    int score;                          /* +4 */
} TOKEN;

/* the qsort comparator of the tokens' scores */
static int ScoreCompare(const TOKEN *a, const TOKEN *b)
{
    if (a->score < b->score)
        return -1;
    if (a->score > b->score)
        return 1;
    return 0;
}
