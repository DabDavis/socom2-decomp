/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* rt_circ_buf: the RT library's ring buffer (OVL2 0x62C1B8-0x62C958, SOCOM 1's object at 0x409C68;
 * the press beta's asserts name the file rt_circ_buf.c), SCEA's Medius/DME network stack, prebuilt
 * with ee-gcc 2.96 at -O2 -G0 (README "The GCC front"); built with sibling calls
 * (rt_circ_buf_normalize ends in `j`). Bytes are stored at the end and freed from the start; a
 * buffer made with a scratch buffer can be normalised -- its used bytes moved to the front -- through
 * it. Names are SOCOM 1's by position (s1:order: SOCOM II's object is SOCOM 1's in the same order,
 * rt_circ_buf_is_valid gone), create and destroy s1:exact.
 *
 * Byte-exact: all but is_wrapped. The stores' orders are ccperm's; a test whose else-block the
 * target carries in a branch-likely's delay slot is written inverted, as the vendor did
 * (get_used_nowrap: `if (nUsed != 0) {...} else *p = 0;`).
 * Round 14b: SCEA's room and count tests are POSITIVE tests around the body with the error in an
 * else -- `if (nUsed >= nBytes) {...} else return RT_CIRC_BUF_ERR_SPACE;`, `if (cb->nStart != 0)
 * {...} return OK;` -- which closed the nine round 4b left open on one diagnosis (after `result =
 * getter(cb, &n); if (result) return result;` the target tests n in v0, 2.96 here in v1). Written
 * as a guard (`if (n < nBytes) return 1;`) the return is a one-insn THEN block that 2.96's first
 * if-conversion pass (ifcvt's find_if_case_1, before register allocation; no flag turns it off)
 * hoists above the branch as `li v0,1`, so v0 is live across n's load and n takes v1; as an else
 * the block is not hoisted and n gets v0 (rt_list.c's own guard-clause function 0x631FD8 keeps the
 * v1: the vendor wrote both). free and use normalise after the if/else; store's wrap counts the
 * used bytes before setting the end; copy's two cases are an if/else joining at one return.
 * OPEN: is_wrapped (16: the target stores a 1 materialised as `sltu a2,zero,a0` -- cb != 0 --
 * hoisted into the first guard's delay slot, and tests nStart != 0 with a bnezl; no source yet). */

typedef struct rt_circ_buf {
    unsigned int nStart;                /* 0x00: the first used byte */
    unsigned int nEnd;                  /* 0x04: the first free byte */
    unsigned int nSize;                 /* 0x08 */
    unsigned int nUsed;                 /* 0x0C */
    char *pBuf;                         /* 0x10 */
    void *pNormalizeBuf;                /* 0x14: scratch for rt_circ_buf_normalize, or none */
} rt_circ_buf;                          /* 0x18 */

#define RT_CIRC_BUF_OK              0
#define RT_CIRC_BUF_ERR_SPACE       1   /* not that many bytes used / free */
#define RT_CIRC_BUF_ERR_INVALID     2
#define RT_CIRC_BUF_ERR_MEMORY      3
#define RT_CIRC_BUF_ERR_NO_BUFFER   4   /* normalising needs the scratch buffer it was not given */

extern void *memset(void *p, int c, unsigned int n);
extern void *memcpy(void *dst, const void *src, unsigned int n);
extern int rt_memory_allocate_init_zero(void **ppMemory, unsigned long nSize);
extern int rt_memory_free(void **ppMemory);

int rt_circ_buf_clear(rt_circ_buf *cb);
int rt_circ_buf_store(rt_circ_buf *cb, const void *pData, unsigned int nBytes);
int rt_circ_buf_copy(void *pDest, rt_circ_buf *cb, unsigned int nBytes);
int rt_circ_buf_normalize(rt_circ_buf *cb, unsigned char bUseBuffer);
int rt_circ_buf_normalize_with_buf(rt_circ_buf *cb, unsigned char bUseBuffer, void *pBuffer);
int rt_circ_buf_get_used(rt_circ_buf *cb, unsigned int *pnUsed);
int rt_circ_buf_get_used_nowrap(rt_circ_buf *cb, unsigned int *pnUsed);
int rt_circ_buf_get_avail(rt_circ_buf *cb, unsigned int *pnAvail);

int rt_circ_buf_create(rt_circ_buf *cb, unsigned int nSize, void *pNormalizeBuf)
{
    if (cb == 0)
        return RT_CIRC_BUF_ERR_INVALID;
    if (nSize == 0)
        return RT_CIRC_BUF_ERR_INVALID;
    memset(cb, 0, sizeof(*cb));
    if (rt_memory_allocate_init_zero((void **)&cb->pBuf, nSize) != 0)
        return RT_CIRC_BUF_ERR_MEMORY;
    if (cb->pBuf == 0)
        return RT_CIRC_BUF_ERR_MEMORY;
    cb->pNormalizeBuf = pNormalizeBuf;
    cb->nSize = nSize;
    cb->nEnd = 0;
    cb->nUsed = 0;
    cb->nStart = 0;
    return RT_CIRC_BUF_OK;
}

int rt_circ_buf_destroy(rt_circ_buf *cb)
{
    if (cb == 0)
        return RT_CIRC_BUF_ERR_INVALID;
    if (cb->pBuf != 0)
        rt_memory_free((void **)&cb->pBuf);
    cb->nStart = 0;
    cb->nEnd = 0;
    cb->nSize = 0;
    cb->nUsed = 0;
    return RT_CIRC_BUF_OK;
}

int rt_circ_buf_clear(rt_circ_buf *cb)
{
    if (cb == 0)
        return RT_CIRC_BUF_ERR_INVALID;
    cb->nStart = 0;
    cb->nEnd = 0;
    cb->nUsed = 0;
    return RT_CIRC_BUF_OK;
}

int rt_circ_buf_free(rt_circ_buf *cb, unsigned int nBytes)
{
    unsigned int nUsed = 0;
    unsigned int nToEnd;
    int result;

    if (cb == 0)
        return RT_CIRC_BUF_ERR_INVALID;
    if (nBytes != 0) {
        result = rt_circ_buf_get_used(cb, &nUsed);
        if (result != RT_CIRC_BUF_OK)
            return result;
        if (nUsed >= nBytes) {
            nToEnd = cb->nSize - cb->nStart;
            if (nBytes < nToEnd)
                cb->nStart += nBytes;
            else
                cb->nStart = nBytes - nToEnd;
            cb->nUsed -= nBytes;
        } else
            return RT_CIRC_BUF_ERR_SPACE;
        result = rt_circ_buf_normalize(cb, 0);
        if (result != RT_CIRC_BUF_OK)
            return result;
    }
    return RT_CIRC_BUF_OK;
}

int rt_circ_buf_use(rt_circ_buf *cb, unsigned int nBytes)
{
    unsigned int nAvail = 0;
    unsigned int nToEnd;
    int result;

    if (cb == 0)
        return RT_CIRC_BUF_ERR_INVALID;
    if (nBytes != 0) {
        result = rt_circ_buf_get_avail(cb, &nAvail);
        if (result != RT_CIRC_BUF_OK)
            return result;
        if (nAvail >= nBytes) {
            nToEnd = cb->nSize - cb->nEnd;
            if (nBytes < nToEnd)
                cb->nEnd += nBytes;
            else
                cb->nEnd = nBytes - nToEnd;
            cb->nUsed += nBytes;
        } else
            return RT_CIRC_BUF_ERR_SPACE;
        result = rt_circ_buf_normalize(cb, 0);
        if (result != RT_CIRC_BUF_OK)
            return result;
    }
    return RT_CIRC_BUF_OK;
}

int rt_circ_buf_store(rt_circ_buf *cb, const void *pData, unsigned int nBytes)
{
    unsigned int nAvail = 0;
    unsigned int nToEnd;
    int result;

    if (cb == 0)
        return RT_CIRC_BUF_ERR_INVALID;
    if (pData == 0)
        return RT_CIRC_BUF_ERR_INVALID;
    if (nBytes == 0)
        return RT_CIRC_BUF_ERR_INVALID;
    result = rt_circ_buf_get_avail(cb, &nAvail);
    if (result != RT_CIRC_BUF_OK)
        return result;
    if (nAvail >= nBytes) {
        nToEnd = cb->nSize - cb->nEnd;
        if (nToEnd >= nBytes) {
            memcpy(cb->pBuf + cb->nEnd, pData, nBytes);
            cb->nUsed += nBytes;
            cb->nEnd += nBytes;
            if (cb->nEnd == cb->nSize)
                cb->nEnd = 0;
        } else {
            memcpy(cb->pBuf + cb->nEnd, pData, nToEnd);
            cb->nUsed += nToEnd;
            nBytes -= nToEnd;
            memcpy(cb->pBuf, (const char *)pData + nToEnd, nBytes);
            cb->nUsed += nBytes;
            cb->nEnd = nBytes;
        }
    } else
        return RT_CIRC_BUF_ERR_SPACE;
    return RT_CIRC_BUF_OK;
}

int rt_circ_buf_copy(void *pDest, rt_circ_buf *cb, unsigned int nBytes)
{
    unsigned int nUsed = 0;
    unsigned int nUsedNoWrap = 0;
    int result;

    if (pDest == 0)
        return RT_CIRC_BUF_ERR_INVALID;
    if (cb == 0)
        return RT_CIRC_BUF_ERR_INVALID;
    if (nBytes == 0)
        return RT_CIRC_BUF_ERR_INVALID;
    result = rt_circ_buf_get_used(cb, &nUsed);
    if (result != RT_CIRC_BUF_OK)
        return result;
    if (nUsed >= nBytes) {
        result = rt_circ_buf_get_used_nowrap(cb, &nUsedNoWrap);
        if (result != RT_CIRC_BUF_OK)
            return result;
        if (nUsedNoWrap >= nBytes)
            memcpy(pDest, cb->pBuf + cb->nStart, nBytes);
        else {
            memcpy(pDest, cb->pBuf + cb->nStart, nUsedNoWrap);
            memcpy((char *)pDest + nUsedNoWrap, cb->pBuf, nBytes - nUsedNoWrap);
        }
    } else
        return RT_CIRC_BUF_ERR_SPACE;
    return RT_CIRC_BUF_OK;
}

int rt_circ_buf_normalize(rt_circ_buf *cb, unsigned char bUseBuffer)
{
    if (cb == 0)
        return RT_CIRC_BUF_ERR_INVALID;
    if (bUseBuffer && cb->pNormalizeBuf == 0)
        return RT_CIRC_BUF_ERR_NO_BUFFER;
    return rt_circ_buf_normalize_with_buf(cb, bUseBuffer, cb->pNormalizeBuf);
}

int rt_circ_buf_normalize_with_buf(rt_circ_buf *cb, unsigned char bUseBuffer, void *pBuffer)
{
    unsigned int nUsed = 0;
    int result;

    if (cb == 0)
        return RT_CIRC_BUF_ERR_INVALID;
    if (bUseBuffer && pBuffer == 0)
        return RT_CIRC_BUF_ERR_INVALID;
    if (cb->nStart != 0) {
        result = rt_circ_buf_get_used(cb, &nUsed);
        if (result != RT_CIRC_BUF_OK)
            return result;
        if (nUsed == 0) {
            cb->nStart = 0;
            cb->nEnd = 0;
        } else if (bUseBuffer == 1) {
            result = rt_circ_buf_copy(pBuffer, cb, nUsed);
            if (result != RT_CIRC_BUF_OK)
                return result;
            result = rt_circ_buf_clear(cb);
            if (result != RT_CIRC_BUF_OK)
                return result;
            result = rt_circ_buf_store(cb, pBuffer, nUsed);
            if (result != RT_CIRC_BUF_OK)
                return result;
        }
    }
    return RT_CIRC_BUF_OK;
}

int rt_circ_buf_get_used(rt_circ_buf *cb, unsigned int *pnUsed)
{
    if (cb == 0)
        return RT_CIRC_BUF_ERR_INVALID;
    if (pnUsed == 0)
        return RT_CIRC_BUF_ERR_INVALID;
    *pnUsed = cb->nUsed;
    return RT_CIRC_BUF_OK;
}

int rt_circ_buf_get_used_nowrap(rt_circ_buf *cb, unsigned int *pnUsed)
{
    if (cb == 0)
        return RT_CIRC_BUF_ERR_INVALID;
    if (pnUsed == 0)
        return RT_CIRC_BUF_ERR_INVALID;
    if (cb->nUsed != 0) {
        if (cb->nStart < cb->nEnd)
            *pnUsed = cb->nEnd - cb->nStart;
        else
            *pnUsed = cb->nSize - cb->nStart;
    } else
        *pnUsed = 0;
    return RT_CIRC_BUF_OK;
}

int rt_circ_buf_get_avail(rt_circ_buf *cb, unsigned int *pnAvail)
{
    if (cb == 0)
        return RT_CIRC_BUF_ERR_INVALID;
    if (pnAvail == 0)
        return RT_CIRC_BUF_ERR_INVALID;
    *pnAvail = cb->nSize - cb->nUsed;
    return RT_CIRC_BUF_OK;
}

int rt_circ_buf_get_avail_nowrap(rt_circ_buf *cb, unsigned int *pnAvail)
{
    if (cb == 0)
        return RT_CIRC_BUF_ERR_INVALID;
    if (pnAvail == 0)
        return RT_CIRC_BUF_ERR_INVALID;
    if (cb->nUsed == cb->nSize)
        *pnAvail = 0;
    else if (cb->nEnd >= cb->nStart)
        *pnAvail = cb->nSize - cb->nEnd;
    else
        *pnAvail = cb->nStart - cb->nEnd;
    return RT_CIRC_BUF_OK;
}

int rt_circ_buf_get_start_used_ptr(rt_circ_buf *cb, char **ppStart)
{
    unsigned int nUsed = 0;
    int result;

    if (cb == 0)
        return RT_CIRC_BUF_ERR_INVALID;
    if (ppStart == 0)
        return RT_CIRC_BUF_ERR_INVALID;
    result = rt_circ_buf_get_used(cb, &nUsed);
    if (result != RT_CIRC_BUF_OK)
        return result;
    if (nUsed != 0)
        *ppStart = cb->pBuf + cb->nStart;
    else
        return RT_CIRC_BUF_ERR_SPACE;
    return RT_CIRC_BUF_OK;
}

int rt_circ_buf_get_start_avail_ptr(rt_circ_buf *cb, char **ppStart)
{
    unsigned int nAvail = 0;
    int result;

    if (cb == 0)
        return RT_CIRC_BUF_ERR_INVALID;
    if (ppStart == 0)
        return RT_CIRC_BUF_ERR_INVALID;
    result = rt_circ_buf_get_avail(cb, &nAvail);
    if (result != RT_CIRC_BUF_OK)
        return result;
    if (nAvail != 0)
        *ppStart = cb->pBuf + cb->nEnd;
    else
        return RT_CIRC_BUF_ERR_SPACE;
    return RT_CIRC_BUF_OK;
}

int rt_circ_buf_is_wrapped(rt_circ_buf *cb, unsigned char *pbWrapped)
{
    if (cb == 0)
        return RT_CIRC_BUF_ERR_INVALID;
    if (pbWrapped == 0)
        return RT_CIRC_BUF_ERR_INVALID;
    if (cb->nStart < cb->nEnd)
        *pbWrapped = 0;
    else if (cb->nUsed == 0)
        *pbWrapped = 0;
    else if (cb->nStart != 0)
        *pbWrapped = 1;
    else
        *pbWrapped = 0;
    return RT_CIRC_BUF_OK;
}
