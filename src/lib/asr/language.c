/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* language: a language file's data, read through a table of accessors (OVL2 0x508508-0x509730,
 * SOCOM 1's object at 0x373338: FillLanguageVirtualTable and its static accessors): ScanSoft's
 * speech recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops
 * (README "The GCC front"). The table is SOCOM 1's LOCAL _LangVirtualTable (data.txt), slot for
 * slot -- 79 words, the same four empty slots -- so each accessor is named by its slot (s1:slot).
 * A language's data starts with its type (0x1003), size and counts; its sections follow in
 * order: the warp factors, the gastone (a short per state per codebook), the ergodic states,
 * phenomes and penalties, the codebooks (each four-aligned), the speech units, the transcribed
 * words, the user-word training phenomes and the tone conversion data. Each section's address is
 * the sum of the counts before it; a codebook's size depends on the file's bit field (bit 0: the
 * probabilities are 4-bit; bit 1: a smoothing matrix follows; bit 2: an extra codebook). */

typedef struct LANGDATA {
    int type;                           /* +0: 0x1003 */
    int size;                           /* +4 */
    char version[128];                  /* +8 */
    int nbr_codebook;                   /* +136 */
    int nbr_state;                      /* +140 */
    int nbr_non_ergodic_phenomes;       /* +144 */
    int nbr_ergodic_states;             /* +148 */
    int nbr_state_ergodic_states;       /* +152 */
    int nbr_ergodic_phenomes;           /* +156 */
    int nbr_state_ergodic_phenomes;     /* +160 */
    int silence_phenome;                /* +164 */
    int uw_silence_phenome;             /* +168 */
    int adapt_sil_state;                /* +172 */
    int single_word_garbage_phenome;    /* +176 */
    int sentence_garbage_phenome;       /* +180 */
    int any_speech_garbage_phenome;     /* +184 */
    int nbr_speech_unit;                /* +188 */
    int nbr_speech_unit_class;          /* +192 */
    int nbr_tones;                      /* +196 */
    int uw_speech_unit_class;           /* +200 */
    int nbr_warp_factors;               /* +204 */
    int nbr_trans_word;                 /* +208 */
    int nbr_phen_uw_training;           /* +212 */
    int bits;                           /* +216 */
    short recog_wtp;                    /* +220 */
    short spelling_wtp;                 /* +222 */
    int warp[2];                        /* +224: two words per warp factor */
} LANGDATA;

typedef struct CDB {                    /* a codebook */
    int dim;                            /* +0 */
    int first_size;                     /* +4 */
    int nbr_in_sec_search;              /* +8 */
    int second_size;                    /* +12 */
    int comp_start;                     /* +16 */
    int nbr_gastone;                    /* +20 */
} CDB;                                  /* the tables follow (cdb + 1) */

typedef struct TRANSW {                 /* the transcribed words */
    int n0;
    int n1;                             /* +4 */
    unsigned short lex[2];              /* +8 */
} TRANSW;

typedef struct EXEV {                   /* the extra-event context */
    int leading_nbs_penalty;            /* +0 */
    int trailing_nbs_penalty;           /* +4 */
    int rejection_nbs_penalty;          /* +8 */
    int rejection_nbs_path_penalty;     /* +12 */
    int silence_phenome;                /* +16 */
    int nbr_items;                      /* +20 */
    int nbr_prons;                      /* +24 */
    int leading_nbs_word;               /* +28 */
    int trailing_nbs_word;              /* +32 */
    int rejection_nbs_word;             /* +36 */
    unsigned short items[2];            /* +40 */
} EXEV;

typedef struct LANG LANG;
typedef int (*LANGFN)();

struct LANG {
    LANGDATA *data;                     /* +0 */
    LANGFN slot[51];                    /* +4 .. +204 */
    int (*CheckBitField)(LANG *l, int mask);    /* +208 */
    LANGFN slot2[26];                   /* +212 .. +312 */
};

#define ALIGN4(n) (((n) + 3) & ~3)

static unsigned short *_langGetpDimensionsOfToneConversionMatrix(LANG *l);
static int _cdbGetCodeBookSize(LANG *l, CDB *cdb);
static int _transwGetSize(LANG *l, TRANSW *tw);
static int _langCheckBitField(LANG *l, int mask);
static int _langCheckDataType(LANG *l);
static int _langGetSize(LANG *l);
static char *_langGetVersionInfo(LANG *l);
static int _langGetNbrCodeBook(LANG *l);
static int _langGetNbrState(LANG *l);
static int _langGetNbrPhenome(LANG *l);
static int _langGetAdaptSilState(LANG *l);
static int _langGetNbrErgodicStates(LANG *l);
static int _langGetNbrStateErgodicStates(LANG *l);
static int _langGetNbrErgodicPhenomes(LANG *l);
static int _langGetNbrStateErgodicPhenomes(LANG *l);
static int _langGetNbrNonErgodicPhenomes(LANG *l);
static int _langGetSilencePhenome(LANG *l);
static int _langGetUWSilencePhenome(LANG *l);
static int _langGetSingleWordGarbagePhenome(LANG *l);
static int _langGetSentenceGarbagePhenome(LANG *l);
static int _langGetAnySpeechGarbagePhenome(LANG *l);
static int _langGetNbrSpeechUnit(LANG *l);
static int _langGetNbrWarpFactors(LANG *l);
static int _langGetNbrSpeechUnitClass(LANG *l);
static int _langGetNbrTones(LANG *l);
static int _langGetUserWordSpeechUnitClass(LANG *l);
static int _langGetNbrTransWord(LANG *l);
static int _langGetNbrPhenUserWordTraining(LANG *l);
static short _langGetRecogWTP(LANG *l);
static short _langGetSpellingWTP(LANG *l);
static int _cdbGetCodeBookDim(CDB *cdb);
static int _cdbGetFirstCdbSize(CDB *cdb);
static int _cdbGetNbrInSecSearch(CDB *cdb);
static int _cdbGetSecondCdbSize(CDB *cdb);
static int _cdbGetCompStart(CDB *cdb);
static int _cdbGetNbrGastone(CDB *cdb);
static unsigned short _langGetSilPhenome(LANG *l);
static int *_langGetpWarpFactors(LANG *l);
static short *_langGetpGastone(LANG *l);
static unsigned short *_langGetpErgodicStates(LANG *l);
static unsigned short *_langGetpErgodicPhenomes(LANG *l);
static short *_langGetpErgodicPenalty(LANG *l);
static CDB *_langGetpCodeBook(LANG *l, unsigned int n);
static unsigned short *_langGetpSpeechUnit(LANG *l);
static int _langGetSizeSpeechUnits(LANG *l);
static TRANSW *_langGetpTransWord(LANG *l);
static unsigned short *_langGetpPhenUserWordTraining(LANG *l);
static unsigned short *_langGetpOffsetForFinals(LANG *l);
static unsigned short *_langGetpToneConversionMatrix(LANG *l);
static EXEV *_langGetpExtraEventContext(LANG *l);
static int _langIsNormalPhenome(LANG *l, unsigned short ph);
static unsigned short *_transwGetpTransWordLex(TRANSW *tw);
static unsigned short *_transwGetpBeginOfTransWords(TRANSW *tw);
static int *_cdbGetpFirstCdb(CDB *cdb);
static int *_cdbGetpIndexInSecCdb(CDB *cdb);
static int *_cdbGetpSecondCdb(CDB *cdb);
static int *_cdbGetpProbMatrix(CDB *cdb);
static int *_cdbGetpSmoothMatrix(LANG *l, CDB *cdb);
static int _cdbGetProbMatrixSize(LANG *l, CDB *cdb);
static int _cdbGetSmoothMatrixSize(CDB *cdb);
static int _exevGetLeading_NBS_penalty(EXEV *e);
static int _exevGetTrailing_NBS_penalty(EXEV *e);
static int _exevGetRejection_NBS_penalty(EXEV *e);
static int _exevGetRejection_NBS_path_penalty(EXEV *e);
static int _exevGetExtraEventSilencePhenome(EXEV *e);
static int _exevGetNbrItemsInLexicon(EXEV *e);
static int _exevGetNbrOfPronunciations(EXEV *e);
static int _exevGetLeading_NBS_WordIndex(EXEV *e);
static int _exevGetTrailing_NBS_WordIndex(EXEV *e);
static int _exevGetRejection_NBS_WordIndex(EXEV *e);
static unsigned short *_exevGetpBeginOfItems(EXEV *e);
static unsigned short *_exevGetpBeginOfWords(EXEV *e);
static unsigned short *_exevGetpBeginOfProns(EXEV *e);
static unsigned short _GetNbrStatesInPhenome(unsigned short *tab, unsigned short ph);
static unsigned short _ConvPhenomesToStates(unsigned short *tab, unsigned short ph, unsigned short **states);

static void *_LangVirtualTable[79] = {
    0,
    _langCheckDataType,
    _langGetSize,
    _langGetVersionInfo,
    _langGetNbrCodeBook,
    _langGetNbrState,
    _langGetNbrPhenome,
    0,
    _langGetAdaptSilState,
    _langGetNbrErgodicStates,
    _langGetNbrStateErgodicStates,
    _langGetNbrErgodicPhenomes,
    _langGetNbrStateErgodicPhenomes,
    _langGetNbrNonErgodicPhenomes,
    _langGetSilencePhenome,
    _langGetUWSilencePhenome,
    _langGetSingleWordGarbagePhenome,
    _langGetSentenceGarbagePhenome,
    _langGetAnySpeechGarbagePhenome,
    _langGetNbrWarpFactors,
    _langGetNbrSpeechUnit,
    _langGetNbrSpeechUnitClass,
    _langGetNbrTones,
    _langGetUserWordSpeechUnitClass,
    _langGetNbrTransWord,
    _langGetNbrPhenUserWordTraining,
    _langGetRecogWTP,
    _langGetSpellingWTP,
    _cdbGetCodeBookDim,
    _cdbGetFirstCdbSize,
    _cdbGetNbrInSecSearch,
    _cdbGetSecondCdbSize,
    _cdbGetCompStart,
    _cdbGetNbrGastone,
    _langGetpWarpFactors,
    _langGetpGastone,
    0,
    _langGetSilPhenome,
    _langGetpErgodicStates,
    _langGetpErgodicPhenomes,
    _langGetpErgodicPenalty,
    _langGetpCodeBook,
    _langGetpSpeechUnit,
    _langGetpTransWord,
    _langGetSizeSpeechUnits,
    0,
    _langGetpPhenUserWordTraining,
    _langGetpDimensionsOfToneConversionMatrix,
    _langGetpOffsetForFinals,
    _langGetpToneConversionMatrix,
    _langGetpExtraEventContext,
    _langIsNormalPhenome,
    _langCheckBitField,
    _GetNbrStatesInPhenome,
    _ConvPhenomesToStates,
    _cdbGetCodeBookSize,
    _cdbGetProbMatrixSize,
    _cdbGetSmoothMatrixSize,
    _cdbGetpFirstCdb,
    _cdbGetpIndexInSecCdb,
    _cdbGetpSecondCdb,
    _cdbGetpProbMatrix,
    _cdbGetpSmoothMatrix,
    _transwGetSize,
    _transwGetpTransWordLex,
    _transwGetpBeginOfTransWords,
    _exevGetLeading_NBS_penalty,
    _exevGetTrailing_NBS_penalty,
    _exevGetRejection_NBS_penalty,
    _exevGetRejection_NBS_path_penalty,
    _exevGetExtraEventSilencePhenome,
    _exevGetNbrItemsInLexicon,
    _exevGetNbrOfPronunciations,
    _exevGetLeading_NBS_WordIndex,
    _exevGetTrailing_NBS_WordIndex,
    _exevGetRejection_NBS_WordIndex,
    _exevGetpBeginOfWords,
    _exevGetpBeginOfProns,
    _exevGetpBeginOfItems,
};

/* the sections up to the first codebook (the four-aligned end of the ergodic penalties): the
 * phenomes' state lists and the penalties (2 * sep), then the sections before them in file order
 * -- fold makes the target's `((2 * sep + 4 * w) + (ses + es)) + (sc + ep)` of it (55 of 30,240
 * trees do, treesearch.py); _langGetpCodeBook matches so */
#define CODEBOOKS(d) ((char *)(d) + ALIGN4(0xE4 + ((d)->nbr_state_ergodic_phenomes * 2 \
        + ((d)->nbr_warp_factors * 4 + (d)->nbr_state * (d)->nbr_codebook + (d)->nbr_ergodic_states \
           + (d)->nbr_state_ergodic_states + (d)->nbr_ergodic_phenomes)) * 2))

/* the tone conversion matrix's dimensions: past the codebooks (the walk 2.96 unrolls by four, each
 * codebook four-aligned), the speech units (their size the last word of their offset table, in
 * halfwords), the transcribed words and the user-word training phenomes, and an extra codebook when
 * the file's bit 2 says so -- each walker's head as _langGetpSpeechUnit's (below). The offset past
 * the speech units is an int aligned in place (`off = ALIGN4(off)`: the `+ 3` then takes a register
 * of its own, the target's v1 between two v0s -- one expression ties it to its input). In SOCOM II
 * the file opens with this function (SOCOM 1's with _langGetpPhenUserWordTraining and
 * _langGetpExtraEventContext), ahead of FillLanguageVirtualTable. */
static unsigned short *_langGetpDimensionsOfToneConversionMatrix(LANG *l)
{
    LANGDATA *d = l->data;
    unsigned int n = d->nbr_codebook;
    unsigned int i;
    char *p;
    int off;

    if (d->bits & 4)
        n++;
    d = l->data;
    p = CODEBOOKS(d);
    for (i = 0; i < n; i++)
        p += ALIGN4(_cdbGetCodeBookSize(l, (CDB *)p));
    off = p + ((int *)p)[d->nbr_speech_unit] * 2 - (char *)d;
    off = ALIGN4(off);
    p = (char *)d + off;
    p += _transwGetSize(l, (TRANSW *)p);
    p += l->data->nbr_phen_uw_training * 2;
    if (l->CheckBitField(l, 4))
        return (unsigned short *)(p + _cdbGetCodeBookSize(l, (CDB *)p));
    return (unsigned short *)p;
}

void FillLanguageVirtualTable(void **vt)
{
    struct VT { void *p[79]; };

    *(struct VT *)vt = *(struct VT *)_LangVirtualTable;
}

/* OPEN (32 words, 4 short): the target reloads the second codebook's size inside the smoothing
 * test (a separate expression -- an inlined _cdbGetSmoothMatrixSize?) and adds the gastone term
 * with a madd at the join; here 2.96 hoists the square to the top. Round 18c: combine does form
 * the multiply-add here too (`*mul_acc_si_r5900`); the tables' size pseudo, its addend, costs the
 * same in GR_REGS and LO_REG (36 each, the .lreg dump), so its class is their union and global
 * gives it v0 -- the split alternative, `mult; addu` -- where the target's sits in LO (`mtlo` after
 * the `+ 24`, before the branch), and the madd's result is another register (a0): the addend and
 * the result are two values in the target. The square's reload SOCOM 1's copy has as well (its
 * own compiler), so it is the source's; an inlined helper (a static inline probe), the gastone
 * term added in each arm and seven statement shapes (build/scratch/gcc/lang/v_cbs*.py, v_sm.py,
 * the same on _cdbGetpSmoothMatrix) leave both. */
static int _cdbGetCodeBookSize(LANG *l, CDB *cdb)
{
    int bits = l->data->bits;
    unsigned int ng;
    int size;

    size = (cdb->dim * cdb->first_size + cdb->nbr_in_sec_search * cdb->first_size
            + cdb->dim * cdb->second_size + cdb->second_size) * 4 + 24;
    ng = cdb->nbr_gastone;
    if (bits & 1)
        ng = (ng + 1) >> 1;
    size += cdb->second_size * ng;
    if (bits & 2)
        size = ALIGN4(size) + cdb->second_size * cdb->second_size * 4;
    return size;
}

static int _transwGetSize(LANG *l, TRANSW *tw)
{
    return (tw->n0 + (l->data->nbr_trans_word + tw->n1)) * 2 + 12;
}

static int _langCheckBitField(LANG *l, int mask)
{
    return l->data->bits & mask;
}

static int _langCheckDataType(LANG *l)
{
    return l->data->type == 0x1003;
}

static int _langGetSize(LANG *l)
{
    return l->data->size;
}

static char *_langGetVersionInfo(LANG *l)
{
    return l->data->version;
}

static int _langGetNbrCodeBook(LANG *l)
{
    return l->data->nbr_codebook;
}

static int _langGetNbrState(LANG *l)
{
    return l->data->nbr_state;
}

static int _langGetNbrPhenome(LANG *l)
{
    LANGDATA *d = l->data;

    return d->nbr_ergodic_phenomes + d->nbr_non_ergodic_phenomes;
}

static int _langGetAdaptSilState(LANG *l)
{
    return l->data->adapt_sil_state;
}

static int _langGetNbrErgodicStates(LANG *l)
{
    return l->data->nbr_ergodic_states;
}

static int _langGetNbrStateErgodicStates(LANG *l)
{
    return l->data->nbr_state_ergodic_states;
}

static int _langGetNbrErgodicPhenomes(LANG *l)
{
    return l->data->nbr_ergodic_phenomes;
}

static int _langGetNbrStateErgodicPhenomes(LANG *l)
{
    return l->data->nbr_state_ergodic_phenomes;
}

static int _langGetNbrNonErgodicPhenomes(LANG *l)
{
    return l->data->nbr_non_ergodic_phenomes;
}

static int _langGetSilencePhenome(LANG *l)
{
    return l->data->silence_phenome;
}

static int _langGetUWSilencePhenome(LANG *l)
{
    return l->data->uw_silence_phenome;
}

static int _langGetSingleWordGarbagePhenome(LANG *l)
{
    return l->data->single_word_garbage_phenome;
}

static int _langGetSentenceGarbagePhenome(LANG *l)
{
    return l->data->sentence_garbage_phenome;
}

static int _langGetAnySpeechGarbagePhenome(LANG *l)
{
    return l->data->any_speech_garbage_phenome;
}

static int _langGetNbrSpeechUnit(LANG *l)
{
    return l->data->nbr_speech_unit;
}

static int _langGetNbrWarpFactors(LANG *l)
{
    return l->data->nbr_warp_factors;
}

static int _langGetNbrSpeechUnitClass(LANG *l)
{
    return l->data->nbr_speech_unit_class;
}

static int _langGetNbrTones(LANG *l)
{
    return l->data->nbr_tones;
}

static int _langGetUserWordSpeechUnitClass(LANG *l)
{
    return l->data->uw_speech_unit_class;
}

static int _langGetNbrTransWord(LANG *l)
{
    return l->data->nbr_trans_word;
}

static int _langGetNbrPhenUserWordTraining(LANG *l)
{
    return l->data->nbr_phen_uw_training;
}

static short _langGetRecogWTP(LANG *l)
{
    return l->data->recog_wtp;
}

static short _langGetSpellingWTP(LANG *l)
{
    return l->data->spelling_wtp;
}

static int _cdbGetCodeBookDim(CDB *cdb)
{
    return cdb->dim;
}

static int _cdbGetFirstCdbSize(CDB *cdb)
{
    return cdb->first_size;
}

static int _cdbGetNbrInSecSearch(CDB *cdb)
{
    return cdb->nbr_in_sec_search;
}

static int _cdbGetSecondCdbSize(CDB *cdb)
{
    return cdb->second_size;
}

static int _cdbGetCompStart(CDB *cdb)
{
    return cdb->comp_start;
}

static int _cdbGetNbrGastone(CDB *cdb)
{
    return cdb->nbr_gastone;
}

static unsigned short _langGetSilPhenome(LANG *l)
{
    return l->data->silence_phenome;
}

static int *_langGetpWarpFactors(LANG *l)
{
    return l->data->warp;
}

static short *_langGetpGastone(LANG *l)
{
    LANGDATA *d = l->data;

    return (short *)(d->warp + d->nbr_warp_factors * 2);
}

/* The section getters' sums, and what 2.96's fold does to them (round 18c's probes,
 * build/scratch/gcc/lang/fold2.c, read off the `-dr` dump): `(A + B) + (C + D)` is regrouped
 * `(A + C) + (B + D)`, `A + (B + C)` becomes `(A + B) + C` when C is a sum, a sum and a product
 * `(M + x) + N` becomes `(M + N) + x`, and two products of one power of two are factored (`w * 8 +
 * x * 2` is `(w * 4 + x) * 2`). So the target's groupings are fold's, not the vendor's spelling:
 * build/scratch/gcc/lang/treesearch.py compiles every tree of a sum's terms and reads each one's
 * expanded dataflow back -- 50 of the penalties' 30,240 give the target's, the sections in file
 * order among them.
 *
 * the gastone term onto the base through a pointer of its own, then the warp factors and the
 * header: a pointer local stops fold merging the terms into one halfword index (round 16c; one
 * expression gives `(s * c + w * 4) * 2` or the warp factors first). */
static unsigned short *_langGetpErgodicStates(LANG *l)
{
    LANGDATA *d = l->data;
    unsigned short *p = (unsigned short *)d + d->nbr_state * d->nbr_codebook;

    return p + d->nbr_warp_factors * 4 + 112;
}

/* the gastone section's address as an integer of its own (`w * 8 + d`, the warp factors first:
 * the target's `addu v1,v1,a1`), then the gastone, the ergodic states and their state lists past
 * it in halfwords: as a pointer, or in one expression, fold factors `w * 8` into the halfword
 * index (the target keeps it apart) -- and `ses + (sc + es)` is the one tree of the three counts
 * that comes out as the target sums them. */
static unsigned short *_langGetpErgodicPhenomes(LANG *l)
{
    LANGDATA *d = l->data;
    int g = d->nbr_warp_factors * 8 + (int)d;

    return (unsigned short *)((d->nbr_state_ergodic_states
                               + (d->nbr_state * d->nbr_codebook + d->nbr_ergodic_states)) * 2 + g + 226);
}

/* the sections up to the penalties in file order -- warp factors, gastone, ergodic states and
 * phenomes (their counts and their state lists) -- as a byte offset of its own, the base added
 * last: fold regroups the sum into the target's `((w * 4 + sep) + (ses + es)) + (sc + ep)` */
static short *_langGetpErgodicPenalty(LANG *l)
{
    LANGDATA *d = l->data;
    int off = 0xE4 + (d->nbr_warp_factors * 4 + d->nbr_state * d->nbr_codebook + d->nbr_ergodic_states
                      + d->nbr_state_ergodic_states + d->nbr_ergodic_phenomes
                      + d->nbr_state_ergodic_phenomes) * 2;

    return (short *)((char *)d + off);
}

/* the n-th codebook: the walk from the first (2.96 unrolls it by four), each four-aligned */
static CDB *_langGetpCodeBook(LANG *l, unsigned int n)
{
    LANGDATA *d = l->data;
    char *p = CODEBOOKS(d);
    unsigned int i;

    for (i = 0; i < n; i++)
        p += ALIGN4(_cdbGetCodeBookSize(l, (CDB *)p));
    return (CDB *)p;
}

/* the speech units: past the codebooks, counted first -- the count read and incremented in place
 * (`if (d->bits & 4) n++`), then the first codebook's address from l->data read again. The target
 * (and SOCOM 1's copy) loads nbr_codebook twice, once for the count and once for the gastone term,
 * and selects the count with a movn; it takes this order: the count's `if` is still a branch when CSE
 * runs, so the sum after its join starts a new extended block, and GCSE's one pass hashes the sum's
 * load against the second l->data load's register, another expression than the count's -- the
 * two loads survive until the if-conversion after combine makes the branch a movn. With the sum
 * first, or through the same d, CSE merges the two reads (round 18c's diagnosis: "the source keeps
 * the two reads apart"); a count copied to another variable leaves a move (round 19c,
 * build/scratch/gcc/r19c/v_su*.py). */
static unsigned short *_langGetpSpeechUnit(LANG *l)
{
    LANGDATA *d = l->data;
    unsigned int n = d->nbr_codebook;
    unsigned int i;
    char *p;

    if (d->bits & 4)
        n++;
    d = l->data;
    p = CODEBOOKS(d);
    for (i = 0; i < n; i++)
        p += ALIGN4(_cdbGetCodeBookSize(l, (CDB *)p));
    return (unsigned short *)p;
}

/* the speech units' size: the last word of their offset table */
static int _langGetSizeSpeechUnits(LANG *l)
{
    LANGDATA *d = l->data;
    unsigned int n = d->nbr_codebook;
    unsigned int i;
    char *p;

    if (d->bits & 4)
        n++;
    d = l->data;
    p = CODEBOOKS(d);
    for (i = 0; i < n; i++)
        p += ALIGN4(_cdbGetCodeBookSize(l, (CDB *)p));
    return ((int *)p)[d->nbr_speech_unit];
}

/* the transcribed words: past the speech units, four-aligned from the file's start */
static TRANSW *_langGetpTransWord(LANG *l)
{
    LANGDATA *d = l->data;
    unsigned int n = d->nbr_codebook;
    unsigned int i;
    char *p;
    int off;

    if (d->bits & 4)
        n++;
    d = l->data;
    p = CODEBOOKS(d);
    for (i = 0; i < n; i++)
        p += ALIGN4(_cdbGetCodeBookSize(l, (CDB *)p));
    off = p + ((int *)p)[d->nbr_speech_unit] * 2 - (char *)d;
    off = ALIGN4(off);
    return (TRANSW *)((char *)d + off);
}

/* the user-word training phenomes: past the transcribed words */
static unsigned short *_langGetpPhenUserWordTraining(LANG *l)
{
    LANGDATA *d = l->data;
    unsigned int n = d->nbr_codebook;
    unsigned int i;
    char *p;
    int off;

    if (d->bits & 4)
        n++;
    d = l->data;
    p = CODEBOOKS(d);
    for (i = 0; i < n; i++)
        p += ALIGN4(_cdbGetCodeBookSize(l, (CDB *)p));
    off = p + ((int *)p)[d->nbr_speech_unit] * 2 - (char *)d;
    off = ALIGN4(off);
    p = (char *)d + off;
    return (unsigned short *)(p + _transwGetSize(l, (TRANSW *)p));
}

static unsigned short *_langGetpOffsetForFinals(LANG *l)
{
    return (unsigned short *)((char *)_langGetpDimensionsOfToneConversionMatrix(l) + 6);
}

static unsigned short *_langGetpToneConversionMatrix(LANG *l)
{
    return (unsigned short *)((char *)_langGetpDimensionsOfToneConversionMatrix(l) + 8);
}

static EXEV *_langGetpExtraEventContext(LANG *l)
{
    unsigned short *dim;
    unsigned short n;
    unsigned short *m;

    if (l->data->nbr_tones != 0) {
        dim = _langGetpDimensionsOfToneConversionMatrix(l);
        n = dim[0] * dim[1] * dim[2];
        m = (unsigned short *)((char *)_langGetpDimensionsOfToneConversionMatrix(l) + 8);
        return (EXEV *)(m + ((n + 3) & 0xFFFC));
    }
    return (EXEV *)((char *)l + ALIGN4((char *)_langGetpDimensionsOfToneConversionMatrix(l) - (char *)l));
}

static int _langIsNormalPhenome(LANG *l, unsigned short ph)
{
    if (ph < (unsigned int)l->data->nbr_ergodic_phenomes)
        return 0;
    return 1;
}

static unsigned short *_transwGetpTransWordLex(TRANSW *tw)
{
    return tw->lex;
}

static unsigned short *_transwGetpBeginOfTransWords(TRANSW *tw)
{
    unsigned short *lex = tw->lex;

    return lex + tw->n1;
}

static int *_cdbGetpFirstCdb(CDB *cdb)
{
    return (int *)(cdb + 1);
}

static int *_cdbGetpIndexInSecCdb(CDB *cdb)
{
    return (int *)(cdb + 1) + cdb->dim * cdb->first_size;
}

static int *_cdbGetpSecondCdb(CDB *cdb)
{
    return (int *)(cdb + 1) + cdb->dim * cdb->first_size + cdb->first_size * cdb->nbr_in_sec_search;
}

/* the tables in file order past the header -- the first codebook, the second's index, the second
 * codebook, the second's sizes -- as the walkers lay them out: fold regroups the sum into the
 * target's `(nsec * first + second) * 4 + cdb`, then the first table, then the second
 * (build/scratch/gcc/lang/tsearch2.py: 128 of 30,240 trees give it, `cdb + 6 ints + each table in
 * order` among them; `cdb->first` through an array member comes out otherwise -- an ADDR_EXPR
 * base keeps the header's 24 apart) */
static int *_cdbGetpProbMatrix(CDB *cdb)
{
    return (int *)(cdb + 1) + cdb->dim * cdb->first_size + cdb->first_size * cdb->nbr_in_sec_search
           + cdb->dim * cdb->second_size + cdb->second_size;
}

static int *_cdbGetpSmoothMatrix(LANG *l, CDB *cdb)
{
    int bits = l->data->bits;
    unsigned int ng;
    int size;

    if ((bits & 2) == 0)
        return 0;
    size = (cdb->dim * cdb->first_size + cdb->nbr_in_sec_search * cdb->first_size
            + cdb->dim * cdb->second_size + cdb->second_size) * 4 + 24;
    ng = cdb->nbr_gastone;
    if (bits & 1)
        ng = (ng + 1) >> 1;
    return (int *)((char *)cdb + ALIGN4(size + cdb->second_size * ng));
}

static int _cdbGetProbMatrixSize(LANG *l, CDB *cdb)
{
    int n = cdb->second_size;

    if (l->data->bits & 1)
        return n * ((unsigned int)(cdb->nbr_gastone + 1) >> 1);
    return n * cdb->nbr_gastone;
}

static int _cdbGetSmoothMatrixSize(CDB *cdb)
{
    return cdb->second_size * cdb->second_size * 4;
}

static int _exevGetLeading_NBS_penalty(EXEV *e)
{
    return e->leading_nbs_penalty;
}

static int _exevGetTrailing_NBS_penalty(EXEV *e)
{
    return e->trailing_nbs_penalty;
}

static int _exevGetRejection_NBS_penalty(EXEV *e)
{
    return e->rejection_nbs_penalty;
}

static int _exevGetRejection_NBS_path_penalty(EXEV *e)
{
    return e->rejection_nbs_path_penalty;
}

static int _exevGetExtraEventSilencePhenome(EXEV *e)
{
    return e->silence_phenome;
}

static int _exevGetNbrItemsInLexicon(EXEV *e)
{
    return e->nbr_items;
}

static int _exevGetNbrOfPronunciations(EXEV *e)
{
    return e->nbr_prons;
}

static int _exevGetLeading_NBS_WordIndex(EXEV *e)
{
    return e->leading_nbs_word;
}

static int _exevGetTrailing_NBS_WordIndex(EXEV *e)
{
    return e->trailing_nbs_word;
}

static int _exevGetRejection_NBS_WordIndex(EXEV *e)
{
    return e->rejection_nbs_word;
}

static unsigned short *_exevGetpBeginOfItems(EXEV *e)
{
    return e->items;
}

static unsigned short *_exevGetpBeginOfWords(EXEV *e)
{
    return e->items + (e->nbr_items + (e->nbr_items & 1));
}

static unsigned short *_exevGetpBeginOfProns(EXEV *e)
{
    return 0;
}

static unsigned short _GetNbrStatesInPhenome(unsigned short *tab, unsigned short ph)
{
    return tab[ph + 1] - tab[ph];
}

static unsigned short _ConvPhenomesToStates(unsigned short *tab, unsigned short ph, unsigned short **states)
{
    *states = tab + tab[ph];
    return tab[ph + 1] - tab[ph];
}
