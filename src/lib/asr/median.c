/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* The recogniser's running median (OVL2 0x501690-0x501A8C, SOCOM 1's object at 0x36CAB8:
 * ProcessMedian, ConstructMedian, ControlMedian, Median_Reset, InitMedian, the same order; the
 * process and control functions static there), ScanSoft's speech recogniser, prebuilt with
 * ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops (README "The GCC front"). Names are
 * SOCOM 1's (s1:align, s1:exact; ProcessMedian by the constructor's slot and position). The last
 * `len` values (profile 1, odd) sit in a ring of nodes that are also a list sorted from `first`
 * (the smallest) to `last`; each frame's value replaces the oldest, is linked in from the old
 * median outwards, and the median is walked to from `first`. */
#include "tosblk.h"

typedef struct MEDNODE {
    struct MEDNODE *next;               /* +0: towards the largest */
    struct MEDNODE *prev;               /* +4 */
    float value;                        /* +8 */
} MEDNODE;

typedef struct MEDIAN {
    TOSBLOCK b;
    unsigned int len;                   /* +40 */
    unsigned int count;                 /* +44 */
    int index;                          /* +48: the oldest node */
    MEDNODE *nodes;                     /* +52 */
    MEDNODE *first;                     /* +56 */
    MEDNODE *last;                      /* +60 */
    MEDNODE *median;                    /* +64 */
} MEDIAN;

static void ProcessMedian(MEDIAN *m, float **in);
static int ControlMedian(MEDIAN *m, unsigned char cmd);
int InitMedian(MEDIAN *m);

static void ProcessMedian(MEDIAN *m, float **in)
{
    float *frame = in[0];
    MEDNODE *node, *p;
    float x;
    unsigned int i, half;
    float *out;

    if (frame != 0) {
        x = *frame;
        node = &m->nodes[m->index];
        if (m->count == m->len) {
            if (node->next != 0)
                node->next->prev = node->prev;
            else
                m->last = node->prev;
            if (node->prev != 0)
                node->prev->next = node->next;
            else
                m->first = node->next;
            m->count--;
            if (node == m->median)
                m->median = node->next != 0 ? node->next : node->prev;
        }
        node->value = x;
        if (m->count == 0) {
            m->median = node;
            m->last = node;
            m->first = node;
            node->prev = 0;
            node->next = 0;
        } else if (x < m->median->value) {
            for (p = m->median->prev; p != 0 && x < p->value; p = p->prev)
                ;
            if (p != 0) {
                node->next = p->next;
                p->next = node;
                node->prev = p;
                node->next->prev = node;
            } else {
                node->next = m->first;
                node->prev = 0;
                m->first->prev = node;
                m->first = node;
            }
        } else {
            for (p = m->median->next; p != 0 && p->value <= x; p = p->next)
                ;
            if (p != 0) {
                node->next = p;
                node->prev = p->prev;
                p->prev = node;
                node->prev->next = node;
            } else {
                node->next = 0;
                node->prev = m->last;
                m->last->next = node;
                m->last = node;
            }
        }
        if (++m->index == m->len)
            m->index = 0;
        m->count++;
        half = m->count >> 1;
        m->median = m->first;
        for (i = 0; i < half; i++)
            m->median = m->median->next;
        out = qEnQueueOne(m->b.outputs[0].q);
        if (out != 0)
            *out = m->median->value;
    }
}

TOSBLOCK *ConstructMedian(TOS *tos, unsigned char id)
{
    return tosBaseBlockConstruct(tos, id, 1, 1, (TOSFUNC)ProcessMedian, (TOSFUNC)InitMedian,
                                 (TOSFUNC)ControlMedian, sizeof(MEDIAN));
}

static int ControlMedian(MEDIAN *m, unsigned char cmd)
{
    if (cmd != TOS_DESTROY)
        return 0;
    if (m->nodes != 0)
        heap_Free(m->b.tos->heap, m->nodes);
    tosBaseBlockDestruct(&m->b);
    return 1;
}

void Median_Reset(MEDIAN *m)
{
    m->median = 0;
    m->last = 0;
    m->first = 0;
    m->index = 0;
    m->count = 0;
}

int InitMedian(MEDIAN *m)
{
    int err = 0;

    m->len = 0;
    m->count = 0;
    m->index = 0;
    m->nodes = 0;
    m->first = 0;
    m->last = 0;
    m->median = 0;
    m->b.inputs[0].size = 24;
    m->b.outputs[0].size = sizeof(float);
    m->len = _tosGetProfileU32(m, 1, 1);
    if (!(m->len & 1)) {
        err = 1;
        _tosErrorLog(m, 200);
    } else {
        m->nodes = heap_Calloc(m->b.tos->heap, m->len, sizeof(MEDNODE));
        m->median = 0;
        m->last = 0;
        m->first = 0;
        m->index = 0;
        m->count = 0;
    }
    return err;
}
