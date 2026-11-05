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

/* first a block within 8 bytes of the rounded size, else the closest larger one split. Every local
 * initialised in its declaration, the free list's head too: the load's place among the
 * initialisations sets MW's pre-colouring order (the world lane's round 21, a sweep of the seven
 * initialisations' 5040 orders, build/scratch/wld/v_alloc_b.py: six match, all with the load before
 * the rounding and found/prev before best; with the load in the for's init, as before, the best of
 * all 1957 declaration/assignment splits was 3 -- `move t1,zero` one place early). Registers settled
 * by the declaration order (the later-declared takes the lower temporary); `found` a `void *` (the
 * return's type) keeps the split's last store through it (v0) -- a copy that changes the pointer's
 * type is not propagated (round 18) */
void *CHeap::Alloc(unsigned int size)
{
    void *found = 0;
    Block *b = m_free;
    Block *prev = 0;
    unsigned int need = ((size + 3) & ~3) + 4;
    unsigned int best = 999999999;
    Block *bestBlock = 0;
    Block *bestPrev;
    bool exact = false;

    for (; b; prev = b, b = b->next) {
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
        ((Block *)found)->size = need;
    }
    m_count++;
    if (found)
        return (char *)found + 4;
    m_fallbacks++;
    return _malloc(size, 0, 0);
}

/* the block given back to the free list in address order, merged with its neighbours when they
 * touch; a pointer outside the heap freed by the allocator. The walk's cursor is
 * defined ahead of `prev`: MW numbers a variable at its first definition, so cur then takes a1 and
 * prev v1, as the target's (declared first but assigned in the for, cur is numbered after prev);
 * and the skip test is a bool local, which keeps the target's materialised sltu/xori -- either
 * alone leaves the function 2 instructions short (the world lane's round 14) */
unsigned int CHeap::Free(void *p)
{
    if ((char *)p < m_base || (char *)p > m_end) {
        m_fallbacks--;
        _free(p, 0, 0);
        return 0;
    }

    unsigned int size = *(unsigned int *)((char *)p - 4);
    Block *b = (Block *)((char *)p - 4);
    Block *cur = m_free;
    Block *prev = 0;

    for (; cur; prev = cur, cur = cur->next) {
        bool after = !(b < cur);

        if (after)
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
