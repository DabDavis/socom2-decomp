/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* gender: the recogniser's gender classifier block (OVL2 0x4F4950-0x4F5F20, SOCOM 1's object at
 * 0x361878: ControlGender, GenderDataCompute, GenderSelection, InitGender, ProcessGender,
 * ConstructGender, ResetGender, all but the constructor LOCAL there), ScanSoft's speech
 * recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops (README
 * "The GCC front"). Names are SOCOM 1's. The block takes four inputs and gives one output; its
 * data (0x94 bytes with the scheduler's record) is named by use. Written so far: the constructor
 * and ResetGender (byte-identical to SOCOM 1's); the process, init and control functions it
 * names are not yet written, so they are declared here rather than defined static. */
#include "tosblk.h"

typedef struct GENDER {
    TOSBLOCK b;
    int selected;                       /* +40: 1 when the gender is fixed by the profile */
    int f44;
    short frames;                       /* +48 */
    unsigned char fixed;                /* +50 */
    char pad51;
    int start;                          /* +52 */
    int f56;
    int pos;                            /* +60 */
    int last;                           /* +64 */
    int *scores;                        /* +68: one per model */
    int f72;
    int f76;
    int f80;
    int f84;
    char pad88[32];
    unsigned int nmodels;               /* +120 */
    char pad124[24];
} GENDER;                               /* 0x94 */

extern void ProcessGender(GENDER *g, unsigned char **in, int n);
extern int InitGender(GENDER *g);
extern int ControlGender(GENDER *g, unsigned char cmd);

TOSBLOCK *ConstructGender(TOS *tos, unsigned char id)
{
    return tosBaseBlockConstruct(tos, id, 4, 1, (TOSFUNC)ProcessGender, (TOSFUNC)InitGender,
                                 (TOSFUNC)ControlGender, sizeof(GENDER));
}

static void ResetGender(GENDER *g)
{
    unsigned int i;

    g->selected = 0;
    if (g->fixed != 0)
        g->selected = 1;
    else
        g->frames = 0;
    g->last = g->start;
    g->pos = g->start;
    for (i = 0; i < g->nmodels; i++)
        g->scores[i] = 0;
    g->f72 = 0;
    g->f76 = 0;
    g->f80 = 0;
    g->f84 = 0;
}
