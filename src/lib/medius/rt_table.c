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
 * from the entry and wrote result variables). Round 21b: func_0063CBD0's row address is a
 * pointer local (`p = gpTable2 + nRow * gnTableCols; memset(p, ...)`) -- the sum passed straight to
 * memset puts the offset first in every one of thirteen spellings. OPEN: func_0063C8A0 (32: registers -- the target keeps nRow in a1 and moves pnCol to t3, here
 * both move: n (pseudo 88, 5 refs over 11 insns) takes a1 before nRow (4 over 37) is allocated, where
 * the target's n is a copy in a3 of the count loaded into a0; thirteen spellings of the count update
 * -- an assignment in the test, the index form, post-increments -- compile alike). The init
 * func_0063C700 (open since round 14b) is byte-exact since round 23b as guard clauses (round 14b's
 * rule: each `return 1`/`return 2` hoisted into v0, the bounds tests two guards -- an `||` gives the
 * second its own block) with the buffer size UNSIGNED: the allocator's `unsigned long` size is a zero
 * extension of it (`dsll32; dsrl32`), as of the two products; the success path returns the last
 * allocation's known-zero v0. Round 22b:
 * func_0063C950 (0x1F4), the record dispatcher: the three words it hands each handler are copied into
 * locals only after the guards (their addresses go to pfnNext: as parameters they are stored at the
 * entry), and the handler's count is added to the offset only after its sign test (added first, the
 * offset test's sltu is scheduled into the sign test's delay slot). The thread-status functions after it
 * (0x63CD18-0x63CF20) are matched drafts. */

extern void *memset(void *p, int c, unsigned int n);
extern void *memcpy(void *d, const void *s, unsigned int n);
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

int func_0063C700(unsigned int nBufSize, unsigned int nCols, unsigned int nRows)
{
    unsigned int nSize;

    if (gbInitialized != 0)
        return 1;
    if (nCols > 256)
        return 2;
    if (nRows > 256)
        return 2;
    gnTableCols = nCols;
    gnTableRows = nRows;
    gnTableBufSize = nBufSize;
    if (rt_memory_allocate_init_zero((void **)&gpTableCounts, nRows * 4) != 0)
        return 6;
    nSize = nRows * nCols * 4;
    if (rt_memory_allocate_init_zero((void **)&gpTable, nSize) != 0)
        return 6;
    if (rt_memory_allocate_init_zero((void **)&gpTableBuf, nBufSize) != 0)
        return 6;
    if (rt_memory_allocate_init_zero((void **)&gpTable2, nSize) != 0)
        return 6;
    gbInitialized = 1;
    return 0;
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

typedef int (*RT_TABLE_HANDLER)(int a, int b, int c, char *pData);

/* a buffer of records dispatched through the table: each record is a row and a column byte, then the
 * handler's data, copied whole into gpTableBuf; the handler returns the bytes it used, and pfnNext
 * (when set) may change the three words handed on between records. 7 a bad record. */
int func_0063C950(int nA, int nB, int nC, unsigned char *pBuf, unsigned int nLength,
                  void (*pfnNext)(int *pa, int *pb, int *pc))
{
    RT_TABLE_HANDLER pfn;
    int a, b, c;
    unsigned int nOffset;
    unsigned char *p;
    unsigned int nRow, nCol;
    int n;

    if (gbInitialized != 1)
        return 3;
    if (gpTable == 0)
        return 3;
    if (gpTableCounts == 0)
        return 3;
    if (gpTableBuf == 0)
        return 3;
    if (gpTable2 == 0)
        return 3;
    if (pBuf == 0)
        return 2;
    if (nLength < 2)
        return 2;
    if (nLength > gnTableBufSize)
        return 2;
    a = nA;
    b = nB;
    c = nC;
    for (nOffset = 0; nOffset < nLength; ) {
        p = pBuf + nOffset;
        nOffset += 2;
        nRow = p[0];
        nCol = p[1];
        if (nRow >= gnTableRows || nCol >= gnTableCols)
            return 7;
        if (func_0063CB48(nRow, nCol, (int *)&pfn) != 0)
            return 7;
        if (pfn == 0)
            return 7;
        gpTable2[nRow * gnTableCols + nCol]++;
        memcpy(gpTableBuf, pBuf + nOffset, nLength - nOffset);
        n = pfn(a, b, c, gpTableBuf);
        if (n < 0)
            return 7;
        nOffset += n;
        if (nOffset > nLength)
            return 7;
        if (pfnNext != 0)
            pfnNext(&a, &b, &c);
    }
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
    int *p;
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
    p = gpTable2 + nRow * gnTableCols;
    memset(p, 0, gnTableCols * 4);
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
