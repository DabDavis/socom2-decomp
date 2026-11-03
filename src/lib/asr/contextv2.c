/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* contextv2: a compiled context's data, read through a table of accessors (OVL2 0x508258-0x508508,
 * SOCOM 1's object at 0x373080: FillContextV2VirtualTable and its 20 static accessors, in the same
 * order): ScanSoft's speech recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing
 * -funroll-loops (README "The GCC front"). The names are SOCOM 1's symbols, the accessors its
 * LOCAL ones (s1:order: the same position, the same field read); the table is its LOCAL
 * _ContextV2VirtualTable (data.txt), copied whole into the caller's. A context's data starts
 * with its type (6) and size; its lexicon of halfwords follows the counts, then the words, the
 * word properties and the syntax, each found by summing the counts before it. */

typedef struct CTXDATA {
    int type;                           /* +0: 6 */
    int size;                           /* +4 */
    char version[128];                  /* +8: the language's version info */
    int ts;                             /* +136 */
    int accuracy;                       /* +140 */
    int rejection;                      /* +144 */
    int garbage;                        /* +148 */
    int nbr_item;                       /* +152 */
    int nbr_word;                       /* +156 */
    int nbr_pron;                       /* +160 */
    int nbr_follower;                   /* +164 */
    int nbr_start_word;                 /* +168 */
    int nbr_stop_word;                  /* +172 */
    int garbage_word_id;                /* +176 */
    int kind;                           /* +180: 1 spelling, 2 isolated words */
    unsigned short lex[2];              /* +184 */
} CTXDATA;

typedef struct CONTEXTV2 CONTEXTV2;

struct CONTEXTV2 {
    CTXDATA *data;
    int (*CheckDataType)(CONTEXTV2 *c);
    int (*GetSize)(CONTEXTV2 *c);
    char *(*GetLanguageVersionInfo)(CONTEXTV2 *c);
    int (*GetTS)(CONTEXTV2 *c);
    int (*GetAccuracy)(CONTEXTV2 *c);
    int (*GetRejection)(CONTEXTV2 *c);
    int (*GetGarbage)(CONTEXTV2 *c);
    int (*GetNbrItem)(CONTEXTV2 *c);
    int (*GetNbrWord)(CONTEXTV2 *c);
    int (*GetNbrPron)(CONTEXTV2 *c);
    int (*GetNbrFollower)(CONTEXTV2 *c);
    int (*GetNbrStartWord)(CONTEXTV2 *c);
    int (*GetNbrStopWord)(CONTEXTV2 *c);
    int (*GetGarbageWordID)(CONTEXTV2 *c);
    int (*IsSpellingContext)(CONTEXTV2 *c);
    unsigned short *(*GetpLex)(CONTEXTV2 *c);
    unsigned short *(*GetpBeginOfWords)(CONTEXTV2 *c);
    int *(*GetpWordProp)(CONTEXTV2 *c);
    unsigned short *(*GetpSyntax)(CONTEXTV2 *c);
    int (*IsIsoWordContext)(CONTEXTV2 *c);
};

#define EVEN(n) ((n) + ((n) & 1))

static int _contextCheckDataType(CONTEXTV2 *c);
static int _contextGetSize(CONTEXTV2 *c);
static char *_contextGetLanguageVersionInfo(CONTEXTV2 *c);
static int _contextGetTS(CONTEXTV2 *c);
static int _contextGetAccuracy(CONTEXTV2 *c);
static int _contextGetRejection(CONTEXTV2 *c);
static int _contextGetGarbage(CONTEXTV2 *c);
static int _contextGetNbrItem(CONTEXTV2 *c);
static int _contextGetNbrWord(CONTEXTV2 *c);
static int _contextGetNbrPron(CONTEXTV2 *c);
static int _contextGetNbrFollower(CONTEXTV2 *c);
static int _contextGetNbrStartWord(CONTEXTV2 *c);
static int _contextGetNbrStopWord(CONTEXTV2 *c);
static int _contextGetGarbageWordID(CONTEXTV2 *c);
static int _contextIsSpellingContext(CONTEXTV2 *c);
static int _contextIsIsoWordContext(CONTEXTV2 *c);
static unsigned short *_contextGetpLex(CONTEXTV2 *c);
static unsigned short *_contextGetpBeginOfWords(CONTEXTV2 *c);
static int *_contextGetpWordProp(CONTEXTV2 *c);
static unsigned short *_contextGetpSyntax(CONTEXTV2 *c);

static CONTEXTV2 _ContextV2VirtualTable = {
    0,
    _contextCheckDataType,
    _contextGetSize,
    _contextGetLanguageVersionInfo,
    _contextGetTS,
    _contextGetAccuracy,
    _contextGetRejection,
    _contextGetGarbage,
    _contextGetNbrItem,
    _contextGetNbrWord,
    _contextGetNbrPron,
    _contextGetNbrFollower,
    _contextGetNbrStartWord,
    _contextGetNbrStopWord,
    _contextGetGarbageWordID,
    _contextIsSpellingContext,
    _contextGetpLex,
    _contextGetpBeginOfWords,
    _contextGetpWordProp,
    _contextGetpSyntax,
    _contextIsIsoWordContext,
};

void FillContextV2VirtualTable(CONTEXTV2 *vt)
{
    *vt = _ContextV2VirtualTable;
}

static int _contextCheckDataType(CONTEXTV2 *c)
{
    return c->data->type == 6;
}

static int _contextGetSize(CONTEXTV2 *c)
{
    return c->data->size;
}

static char *_contextGetLanguageVersionInfo(CONTEXTV2 *c)
{
    return c->data->version;
}

static int _contextGetTS(CONTEXTV2 *c)
{
    return c->data->ts;
}

static int _contextGetAccuracy(CONTEXTV2 *c)
{
    return c->data->accuracy;
}

static int _contextGetRejection(CONTEXTV2 *c)
{
    return c->data->rejection;
}

static int _contextGetGarbage(CONTEXTV2 *c)
{
    return c->data->garbage;
}

static int _contextGetNbrItem(CONTEXTV2 *c)
{
    return c->data->nbr_item;
}

static int _contextGetNbrWord(CONTEXTV2 *c)
{
    return c->data->nbr_word;
}

static int _contextGetNbrPron(CONTEXTV2 *c)
{
    return c->data->nbr_pron;
}

static int _contextGetNbrFollower(CONTEXTV2 *c)
{
    return c->data->nbr_follower;
}

static int _contextGetNbrStartWord(CONTEXTV2 *c)
{
    return c->data->nbr_start_word;
}

static int _contextGetNbrStopWord(CONTEXTV2 *c)
{
    return c->data->nbr_stop_word;
}

static int _contextGetGarbageWordID(CONTEXTV2 *c)
{
    return c->data->garbage_word_id;
}

static int _contextIsSpellingContext(CONTEXTV2 *c)
{
    return c->data->kind == 1;
}

static int _contextIsIsoWordContext(CONTEXTV2 *c)
{
    return c->data->kind == 2;
}

static unsigned short *_contextGetpLex(CONTEXTV2 *c)
{
    return c->data->lex;
}

static unsigned short *_contextGetpBeginOfWords(CONTEXTV2 *c)
{
    CTXDATA *d = c->data;

    return d->lex + EVEN(d->nbr_item + d->nbr_pron);
}

/* the words' table starts two words (four halfwords) past the lexicon: 2.96 folds that 8 into
 * the 184 only when it is added to the halfword pointer (as an int count it keeps (n + 2) * 4) */
static int *_contextGetpWordProp(CONTEXTV2 *c)
{
    CTXDATA *d = c->data;

    return (int *)(d->lex + EVEN(d->nbr_item + d->nbr_pron) + 4) + (d->nbr_pron + d->nbr_word);
}

/* OPEN (1 word, the size 4 short): the target copies the word count before rounding it (move
 * a0,a3) and keeps its unrounded copy for pron + word; every shape tried (a local count, the
 * rounding first or last, the sum through WordProp's) rounds the loaded register in place. */
static unsigned short *_contextGetpSyntax(CONTEXTV2 *c)
{
    CTXDATA *d = c->data;
    int n = EVEN(d->nbr_word);

    return (unsigned short *)((int *)(d->lex + EVEN(d->nbr_item + d->nbr_pron) + 4)
                              + (d->nbr_pron + d->nbr_word)) + n;
}
