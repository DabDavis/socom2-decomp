/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* rt_table.c: a table of word slots, rows by columns, with a fill count per row (OVL2
 * 0x63C700-0x63CD18), in SCEA's RT library, prebuilt with ee-gcc 2.96 at -O2 -G0 (README "The GCC
 * front"). It alone reads .data 0x656000-0x656010 and .bss 0x676408-0x676410, between rt_p2p.c's and
 * rt_time.c's; no $Header and no SOCOM 1 counterpart, so the file name (between those two in the
 * alphabetical link order) and every name are ours. Error codes are the values returned: 1 already
 * started, 2 a bad argument, 3 not started, 5 the row is full, 6 out of memory.
 *
 * Byte-exact: func_0063C810 (the teardown), func_0063CB48 and func_0063CC60 (round 14b). The
 * accessors are guard clauses, one return per test: the target's single `li v0,3` at the entry,
 * kept through the chain, is each `return 3` hoisted above its branch by 2.96's if-conversion and
 * the copies after the first deleted by the post-reload CSE (round 11b read it as a constant held
 * from the entry and wrote result variables). OPEN: func_0063CBD0 (1 word: memset's row address
 * is `addu a0,a3,a0` in the target, base first -- six spellings of the sum give the offset
 * first), func_0063C8A0 (32: registers -- the target keeps nRow in a1 and moves pnCol to t3, here
 * both move; five spellings of the count update), func_0063C700 (the init, 67s). 0x63C950 (0x1F4)
 * and the thread-status functions after it (0x63CD18-0x63CF20) are not written. */

extern void *memset(void *p, int c, unsigned int n);
extern int rt_memory_allocate_init_zero(void **ppMemory, unsigned long nSize);
extern int rt_memory_free(void **ppMemory);

static unsigned char gbInitialized = 0;    /* 0x656000 */
static char *gpTableBuf = 0;                /* 0x656004: nBufSize bytes */
static int *gpTable = 0;                    /* 0x656008: rows x columns */
static unsigned int *gpTableCounts = 0;     /* 0x65600C: a fill count per row */
static int *gpTable2 = 0;                   /* 0x656010: rows x columns */
unsigned int gnTableCols;                   /* 0x676408 */
unsigned int gnTableRows;                   /* 0x67640C */
int gnTableBufSize;                         /* 0x676410 */

int func_0063C700(int nBufSize, unsigned int nCols, unsigned int nRows)
{
    int result = 1;
    unsigned int nSize;

    if (gbInitialized == 0) {
        result = 2;
        if (nCols <= 256 && nRows <= 256) {
            gnTableCols = nCols;
            gnTableRows = nRows;
            gnTableBufSize = nBufSize;
            result = 6;
            if (rt_memory_allocate_init_zero((void **)&gpTableCounts, nRows * 4) == 0) {
                nSize = nRows * nCols * 4;
                if (rt_memory_allocate_init_zero((void **)&gpTable, nSize) != 0)
                    return 6;
                if (rt_memory_allocate_init_zero((void **)&gpTableBuf, nBufSize) != 0)
                    return 6;
                result = rt_memory_allocate_init_zero((void **)&gpTable2, nSize);
                if (result != 0)
                    return 6;
                gbInitialized = 1;
            }
        }
    }
    return result;
}

int func_0063C810(void)
{
    if (gpTableCounts != 0)
        rt_memory_free((void **)&gpTableCounts);
    if (gpTable != 0)
        rt_memory_free((void **)&gpTable);
    if (gpTableBuf != 0)
        rt_memory_free((void **)&gpTableBuf);
    if (gpTable2 != 0)
        rt_memory_free((void **)&gpTable2);
    gbInitialized = 0;
    return 0;
}

int func_0063C8A0(unsigned int *pnCol, unsigned int nRow, int nValue)
{
    unsigned int *pCount;
    unsigned int n;

    if (gbInitialized != 1)
        return 3;
    if (gpTable == 0)
        return 3;
    if (gpTableCounts == 0)
        return 3;
    if (pnCol == 0)
        return 2;
    if (nRow >= gnTableRows)
        return 2;
    pCount = gpTableCounts + nRow;
    n = *pCount;
    if (n > gnTableCols)
        return 5;
    *pCount = n + 1;
    *pnCol = n;
    gpTable[nRow * gnTableCols + n] = nValue;
    return 0;
}

int func_0063CB48(unsigned int nRow, unsigned int nCol, int *pnValue)
{
    if (pnValue == 0)
        return 2;
    *pnValue = 0;
    if (gbInitialized != 1)
        return 3;
    if (gpTable == 0)
        return 3;
    if (gpTableCounts == 0)
        return 3;
    if (nRow >= gnTableRows)
        return 2;
    if (nCol >= gnTableCols)
        return 2;
    *pnValue = gpTable[nRow * gnTableCols + nCol];
    if (*pnValue == 0)
        return 2;
    return 0;
}

int func_0063CBD0(unsigned int nRow)
{
    if (gbInitialized != 1)
        return 3;
    if (gpTable == 0)
        return 3;
    if (gpTableCounts == 0)
        return 3;
    if (gpTable2 == 0)
        return 3;
    if (nRow >= gnTableRows)
        return 2;
    memset(gpTable2 + nRow * gnTableCols, 0, gnTableCols * 4);
    return 0;
}

int func_0063CC60(unsigned int nRow, int *pOut, unsigned int nOut)
{
    unsigned int i;

    if (gbInitialized != 1)
        return 3;
    if (gpTable == 0)
        return 3;
    if (gpTableCounts == 0)
        return 3;
    if (gpTable2 == 0)
        return 3;
    if (nRow >= gnTableRows)
        return 2;
    if (pOut == 0)
        return 2;
    if (nOut < gnTableCols)
        return 2;
    for (i = 0; i < gnTableCols; i++)
        *pOut++ = gpTable2[nRow * gnTableCols + i];
    return 0;
}
