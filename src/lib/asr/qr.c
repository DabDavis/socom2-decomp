/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* qr: the recogniser's matrix fills and the QR's pre-multiply (OVL2 0x4F77F8-0x4F7A48: SOCOM 1's
 * object at 0x364318 -- mtxFillCopy, mtxFillX, QrPreMult -- here mtxFillX, mtxFillCopy, QrPreMult),
 * ScanSoft's speech recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing
 * -funroll-loops (README "The GCC front"). The file's name is ours (the demo keeps none); the
 * functions' are SOCOM 1's. The matrices are the mtx object's (mtx.c): column-major. Round 28c. */

typedef struct MTX {
    unsigned int rows;                  /* +0 */
    unsigned int cols;                  /* +4 */
    unsigned int n;                     /* +8: rows * cols */
    float *data;                        /* +12 */
    float *end;                         /* +16 */
} MTX;

extern void *memcpy(void *to, const void *from, unsigned int n);

/* every element set to x */
void mtxFillX(MTX *m, float x)
{
    unsigned int n = m->n;
    float *p = m->data;
    unsigned int i;

    for (i = 0; i < n; i++)
        *p++ = x;
}

/* to's elements copied from from's (to's count) */
void mtxFillCopy(MTX *to, MTX *from)
{
    memcpy(to->data, from->data, to->n * sizeof(float));
}

/* x pre-multiplied by Q' of the QR in r: each column's Householder vector is its sub-diagonal
 * (its diagonal 1), c's entry its scale -- x -= (v'x * c[k]) v, column by column. SOCOM 1's body
 * (0x104, not unrolled there): the column counter is `j` from 0 tested as `j++ < r->cols` (the
 * target compares a copy of the old j, then steps it: a `k` counter with `j = k + 1` keeps both,
 * CSE turning `k++` into `k = j`), the address of x's diagonal entry one register (`*(xp - 1)`),
 * each loop's walkers set in the init in the reverse of their use (`b = vp, a = xp, i = j`). */
int QrPreMult(MTX *r, MTX *c, MTX *x)
{
    float *xp = x->data;
    float *cp = c->data;
    float *vp = r->data;
    unsigned int j, i;
    float s, *a, *b;

    for (j = 0; j++ < r->cols; ) {
        vp += j;
        s = *xp++;
        for (b = vp, a = xp, i = j; i < r->rows; i++)
            s += *a++ * *b++;
        s *= *cp++;
        *(xp - 1) -= s;
        for (a = xp, i = j; i < r->rows; i++)
            *a++ -= s * *vp++;
    }
    return 0;
}
