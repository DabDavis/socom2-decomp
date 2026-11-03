/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops -ffast-math */
/* train: the recogniser's user-word model training (OVL2 0x4DF470-0x4E1B28, SOCOM 1's object at
 * 0x34E8E0: TrainSmoothCounts .. CalcEntropyMerged4bit), ScanSoft's speech recogniser, prebuilt
 * with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops (README "The GCC front"). Names
 * are SOCOM 1's.
 *
 * THE OBJECT IS SOCOM 1'S, IN ITS ORDER BUT ONE (round 15c; round 11c took 0x4E0D00 for
 * TrainSmoothCounts and the four rows before it for SOCOM II's additions): DoUserWordTraining calls
 * 0x4DFD20 then 0x4E0D00 where SOCOM 1's calls TrainInit then TrainModels, and 0x4E0D00 calls
 * 0x4DF470, 0x4DF898 and 0x4E0050 where SOCOM 1's TrainModels calls TrainSmoothCounts,
 * TrainProbsFromCounts and TrainSelectPhenom (each body SOCOM 1's grown: logf for log, the loops
 * unrolled -- SOCOM 1's copy of this object was built without -funroll-loops); TrainSelectPhenom ends
 * in `j heap_Free` as SOCOM 1's does. The image's order is TrainSmoothCounts, TrainProbsFromCounts,
 * TrainInit, TrainSelectPhenom, TrainModels, TrainCountsFromAcq, TrainFree, CalcEntropy,
 * CalcEntropy4bit, CalcEntropyMerged, CalcEntropyMerged4bit -- SOCOM 1's with TrainFree moved
 * from after TrainInit to after TrainCountsFromAcq: TrainModels inlines it here (its four
 * heap_Free calls and the sibling call to the fifth) where SOCOM 1's tail-calls it.
 * TrainCountsFromAcq and the four entropy functions have no caller in either build.
 *
 * The tables are SOCOM 1's .rodata, byte for byte (data.txt, s1:data): log_p_average_ (511
 * shorts), global_log_p_average (its middle, &log_p_average_[255], a const pointer 2.96 folds into
 * every use: the code addresses 0x658AF6 itself) and p_Log_p (256 words). */

#include "casr.h"

/* a training record: two count tables and the next record */
typedef struct TRAINREC {
    unsigned short *counts;             /* +0: per codeword and state */
    unsigned int size;                  /* +4: the counts' length (codewords times states) */
    unsigned short *totals;             /* +8: per state */
    unsigned int nstates;               /* +12 */
    struct TRAINREC *next;              /* +16 */
} TRAINREC;

extern void heap_Free(void *heap, void *p);
extern void *heap_Alloc(void *heap, unsigned int size);
extern void *heap_Calloc(void *heap, unsigned int n, unsigned int size);
extern float logf(float x);

extern const short log_p_average_[511];
static const short *const global_log_p_average = &log_p_average_[255];
extern const unsigned int p_Log_p[256];

/* each record's counts smoothed through its matrix, state by state: the row sums kept at +1048,
 * then rounded back into the counts, the state's total recounted */
void TrainSmoothCounts(CASR *c)
{
    TRAINREC *r;
    unsigned short nstates = c->nstates;
    unsigned int k = 0;

    for (r = c->trainrecs; r != 0; r = r->next) {
        unsigned int ncw = r->size / nstates;
        unsigned int s;

        for (s = 0; s < nstates; s++) {
            float *acc = c->train1048;
            float *mat = c->smooth[k];
            unsigned short *cnt;
            unsigned int i, j, total = 0;

            for (i = 0; i < ncw; i++) {
                cnt = r->counts + s;
                *acc = 0;
                for (j = 0; j < ncw; j++) {
                    *acc += *mat * *cnt;
                    cnt += nstates;
                    mat++;
                }
                acc++;
            }
            acc = c->train1048;
            cnt = r->counts + s;
            for (i = 0; i < ncw; i++) {
                *cnt = *acc + 0.5;
                total += *cnt;
                cnt += nstates;
                acc++;
            }
            r->totals[s] = total;
        }
        k++;
    }
}

/* each record's counts turned into scaled log probabilities, state by state: floored and
 * normalised into +1060, then -log(p / sum) * 255 / scale, rounded, back into the counts */
void TrainProbsFromCounts(CASR *c, int pron)
{
    TRAINREC *r;
    unsigned short nstates = c->nstates;
    float scale = c->f1056;
    float floor = c->f1052;

    for (r = c->trainrecs; r != 0; r = r->next) {
        unsigned int ncw = r->size / nstates;
        float norm = floor * ncw + 1.0f;
        unsigned int s;

        for (s = 0; s < nstates; s++) {
            unsigned short *cnt = r->counts + s;
            float *prob = c->train1060;
            float total = r->totals[s];
            float sum = 0;
            unsigned int i;

            for (i = 0; i < ncw; i++) {
                float t = (*cnt / total + floor) / norm;

                sum += t;
                *prob = t;
                cnt += nstates;
                prob++;
            }
            prob = c->train1060;
            cnt = r->counts + s;
            for (i = 0; i < ncw; i++) {
                *cnt = -logf(*prob++ / sum) * 255.0 / scale + 0.5;
                cnt += nstates;
            }
        }
    }
}

/* a record per codebook (its counts and totals, cleared), the probabilities' scale (the largest
 * -log of a floored probability) and the two per-state buffers sized for the largest codebook;
 * 1 when an allocation fails */
int TrainInit(CASR *c)
{
    TRAINREC **link;
    LANGVT *l;
    unsigned int n, i;
    float maxent;

    c->trainrecs = 0;
    link = &c->trainrecs;
    l = c->lng;
    c->maxcw = 0;
    n = l->GetNbrCodeBook(l);
    for (i = 0; i < n; i++) {
        unsigned int ncw = l->GetSecondCdbSize(l->GetpCodeBook(l, i));
        TRAINREC *r;

        if (c->maxcw < ncw)
            c->maxcw = ncw;
        r = heap_Alloc(c->heap, 20);
        if (r == 0)
            return 1;
        r->counts = 0;
        r->totals = 0;
        *link = r;
        ncw *= c->nstates;
        if ((r->counts = heap_Calloc(c->heap, ncw * 2, 1)) == 0)
            return 1;
        r->size = ncw;
        if ((r->totals = heap_Calloc(c->heap, c->nstates * 2, 1)) == 0)
            return 1;
        r->nstates = c->nstates;
        r->next = 0;
        link = &r->next;
    }
    maxent = 0;
    for (i = 0; i < n; i++) {
        unsigned int ncw = l->GetSecondCdbSize(l->GetpCodeBook(l, i));
        float e = -logf(c->f1052 / (ncw * c->f1052 + 1.0f));

        if (maxent < e)
            maxent = e;
    }
    c->f1056 = maxent;
    c->train1060 = heap_Alloc(c->heap, c->maxcw * 4);
    c->train1048 = heap_Calloc(c->heap, c->maxcw, 4);
    if (c->train1060 == 0 || c->train1048 == 0)
        return 1;
    return 0;
}

/* one utterance's labels counted into every record's tables along its segmentation: each state's
 * frames, from the last back, add 50 to the codeword's count and to the state's total */
__inline__ void TrainCountsFromAcq(CASR *c, int utt, int pron)
{
    unsigned short stride = ((SAMPLEFMT *)c->slot[3].cb)->frame;
    unsigned short *seg = (unsigned short *)c->segtab[pron * 3 + utt];
    unsigned short nstates = c->nstates;
    TRAINREC *r;
    unsigned short k = 0;

    for (r = c->trainrecs; r != 0; r = r->next) {
        int frame = seg[0] - 1;
        unsigned char *acq = (unsigned char *)c->lab_buf[utt] + frame * stride + k;
        unsigned short *counts = r->counts;
        unsigned short *totals = r->totals;
        unsigned int s;

        for (s = 0; s < nstates; s++) {
            while (frame >= seg[s + 1]) {
                counts[*acq * nstates + s] += 50;
                totals[s] += 50;
                frame--;
                acq -= stride;
            }
        }
        k++;
    }
}

__inline__ void TrainFree(CASR *c)
{
    TRAINREC *r, *next;

    for (r = c->trainrecs; r != 0; r = next) {
        next = r->next;
        heap_Free(c->heap, r->counts);
        heap_Free(c->heap, r->totals);
        heap_Free(c->heap, r);
    }
    heap_Free(c->heap, c->train1060);
    heap_Free(c->heap, c->train1048);
}

/* the entropy of n labels, stride bytes apart */
__inline__ unsigned int CalcEntropy(unsigned char *p, unsigned int n, int stride)
{
    unsigned int i, sum = 0;

    for (i = 0; i < n; i++) {
        sum += p_Log_p[*p];
        p += stride;
    }
    return sum;
}

/* the same over 4-bit labels, two to a byte: the low nibble or the high */
__inline__ unsigned int CalcEntropy4bit(unsigned char *p, int low, unsigned int n, unsigned int dim)
{
    unsigned int i, sum = 0;
    unsigned int stride = (dim + 1) >> 1;

    if (low) {
        for (i = 0; i < n; i++) {
            sum += p_Log_p[(*p << 4) & 0xF0];
            p += stride;
        }
    } else {
        for (i = 0; i < n; i++) {
            sum += p_Log_p[*p & 0xF0];
            p += stride;
        }
    }
    return sum;
}

/* each of n candidates' entropy over m labels merged with its own, summed into out: the average
 * table's pointer moved back by the label and forward again around each row (in place, as the
 * target's subu/addu on one register) */
__inline__ void CalcEntropyMerged(unsigned int *out, unsigned char *p, int stride, unsigned short *idx,
                       unsigned int n, unsigned int m)
{
    unsigned int i, j;
    const short *avg = global_log_p_average;

    for (i = 0; i < m; i++) {
        unsigned char c = *p;
        unsigned int *o = out;

        p += stride;
        avg -= c;
        for (j = 0; j < n; j++) {
            *o += p_Log_p[avg[*idx] + c];
            idx++;
            o++;
        }
        avg += c;
    }
}

/* the same over 4-bit labels */
__inline__ void CalcEntropyMerged4bit(unsigned int *out, unsigned char *p, int low, unsigned int dim,
                           unsigned short *idx, unsigned int n, unsigned int m)
{
    unsigned int i, j;
    unsigned int stride = (dim + 1) >> 1;
    const short *avg = global_log_p_average;

    if (low) {
        for (i = 0; i < m; i++) {
            unsigned int c = (*p << 4) & 0xF0;
            unsigned int *o = out;

            p += stride;
            avg -= c;
            for (j = 0; j < n; j++) {
                *o += p_Log_p[avg[*idx] + c];
                idx++;
                o++;
            }
            avg += c;
        }
    } else {
        for (i = 0; i < m; i++) {
            unsigned int c = *p & 0xF0;
            unsigned int *o = out;

            p += stride;
            avg -= c;
            for (j = 0; j < n; j++) {
                *o += p_Log_p[avg[*idx] + c];
                idx++;
                o++;
            }
            avg += c;
        }
    }
}

extern void TrainSelectPhenom(CASR *c, int pron);

/* the training's models from every utterance's labels: counted, smoothed if asked, turned into
 * probabilities, the phonemes selected; then the records freed */
void TrainModels(CASR *c, int pron)
{
    unsigned int i;

    for (i = 0; i < c->uw_recs_needed; i++)
        TrainCountsFromAcq(c, i, pron);
    if (c->smooth[0])
        TrainSmoothCounts(c);
    TrainProbsFromCounts(c, pron);
    TrainSelectPhenom(c, pron);
    TrainFree(c);
}
