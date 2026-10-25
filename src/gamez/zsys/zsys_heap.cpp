/* zsys_heap.cpp -- SOCOM II's sub-heap (OVL1 0x34ED00-0x34F0D7). Not in SOCOM 1; a unit of its own
 * because it calls _malloc out of line, which zsys_memory.cpp's -inline auto would inline. The
 * file name is ours. Defined in reverse of their image order (-inline deferred). */

#include "zsys_public.h"

/* SOCOM II's: a block of the system heap carved up first-fit, best-fit, with a free list kept in
 * address order and merged on free; a request it cannot meet goes to _malloc (names ours; the
 * single instance is a global at 0x45B328, wrapped at 0x30E300-0x30E3C8) */
class CHeap {
public:
    struct Block {
        unsigned int size;      /* 0x0: the header's 4 bytes included */
        Block *next;            /* 0x4: free blocks only */
    };

    CHeap(unsigned int size);
    ~CHeap();
    void *Alloc(unsigned int size);
    unsigned int Free(void *p);

    char *m_base;               /* 0x00 */
    char *m_end;                /* 0x04: the last byte */
    unsigned int m_size;        /* 0x08 */
    Block *m_free;              /* 0x0C */
    int m_count;                /* 0x10: blocks given out */
    int m_fallbacks;            /* 0x14: requests passed to _malloc */
};

CHeap::CHeap(unsigned int size)
{
    m_base = (char *)_malloc(size, 0, 0);
    m_free = (Block *)m_base;
    m_end = m_base + size - 1;
    m_size = size;
    m_count = 0;
    m_fallbacks = 0;
    m_free->size = size;
    m_free->next = 0;
}

CHeap::~CHeap()
{
    _free(m_base, 0, 0);
}

/* OPEN, 7 instructions: registers settled by this declaration order (the later-declared takes
 * the lower temporary); left: the target computes the rounded size before best's lui, and makes
 * the split's last store through `found` (v0) where MW here propagates bestBlock (t1) */
void *CHeap::Alloc(unsigned int size)
{
    Block *found = 0;
    Block *b;
    Block *prev = 0;
    unsigned int need = ((size + 3) & ~3) + 4;
    unsigned int best = 999999999;
    Block *bestBlock = 0;
    Block *bestPrev;
    bool exact = false;

    for (b = m_free; b; prev = b, b = b->next) {
        if (b->size >= need && b->size - need < 8) {
            b->size = need;
            found = b;
            if (prev)
                prev->next = b->next;
            else
                m_free = b->next;
            exact = true;
            break;
        }
        if (need < b->size && b->size - need < best) {
            best = b->size - need;
            bestPrev = prev;
            bestBlock = b;
        }
    }
    if (!exact && bestBlock) {
        Block *rest = (Block *)((char *)bestBlock + need);

        found = bestBlock;
        rest->size = bestBlock->size - need;
        rest->next = bestBlock->next;
        if (bestPrev)
            bestPrev->next = rest;
        else
            m_free = rest;
        found->size = need;
    }
    m_count++;
    if (found)
        return (char *)found + 4;
    m_fallbacks++;
    return _malloc(size, 0, 0);
}

/* OPEN, 2 instructions short (0x1A4 of 0x1AC), the same blocks: the target materialises the
 * address test (sltu, xori, bnez) where MW here branches on sltu, and gives prev v1 and cur a1
 * (here the other way round). Tried: `!(b < cur)`, this inline bool (== false too), a bool local,
 * cur declared first */
unsigned int CHeap::Free(void *p)
{
    if ((char *)p < m_base || (char *)p > m_end) {
        m_fallbacks--;
        _free(p, 0, 0);
        return 0;
    }

    unsigned int size = *(unsigned int *)((char *)p - 4);
    Block *b = (Block *)((char *)p - 4);
    Block *prev = 0;
    Block *cur;

    for (cur = m_free; cur; prev = cur, cur = cur->next) {
        if (b >= cur)
            continue;
        if (prev) {
            if ((char *)prev + prev->size == (char *)b) {
                prev->size += size;
                if ((char *)prev + prev->size == (char *)cur) {
                    prev->size += cur->size;
                    prev->next = cur->next;
                }
            } else if ((char *)b + size == (char *)cur) {
                prev->next = b;
                b->size += cur->size;
                b->next = cur->next;
            } else {
                b->next = prev->next;
                prev->next = b;
            }
        } else {
            if ((char *)b + size == (char *)m_free) {
                b->size += m_free->size;
                b->next = m_free->next;
            } else
                b->next = m_free;
            m_free = b;
        }
        break;
    }
    if (!cur) {
        if (prev) {
            if ((char *)prev + prev->size == (char *)b)
                prev->size += size;
            else {
                b->next = prev->next;
                prev->next = b;
            }
        } else {
            b->next = 0;
            m_free = b;
        }
    }
    m_count--;
    return size;
}
