/* zmalloc.cpp -- MAIN's small-block pools (MAIN 0x1801E0-0x18194F, initialiser 0x1D4B00): six
 * free-list pools of 4..24-byte blocks behind operator new/delete, _strdup and _free.
 *
 * SOCOM 1 keeps the pool template in main.cpp (`_zmalloc<four>`, `_zmalloc<twelve>`: __ct, init_page,
 * acquire, release -- SOCOM 1's DWARF lines 63-119 -- with operator new/delete over them); SOCOM II
 * moved it into a MAIN unit of its own, with six pools, two more counters per pool and a watermark
 * of the pools' highest address. The file name is ours. The block classes 'four' and 'twelve' are
 * SOCOM 1's; eight, sixteen, twenty and twentyfour are ours, named alike.
 *
 * The game's compiler and flags (scdiff's default; README "MAIN's Metrowerks runtime": __nwa__ is
 * a tail call, which needs exceptions off). Deferred, so the image holds the functions in reverse:
 * defined here __nw__ first, _strdup last; the template instances follow (ctor and init_page per
 * pool, by size). */

#include <string.h>

void *_calloc(unsigned int n, unsigned int size, const char *file, int line);
void *_malloc(unsigned int size, const char *file, int line);
void *_new(unsigned int size, const char *file, int line);
void _delete(void *p, const char *file, int line);
void *_new_vec(unsigned int size, const char *file, int line);
void _delete_vec(void *p, const char *file, int line);
extern "C" char *func_00181950(const char *s, const char *file, int line);     /* zsys_memory's _strdup */
extern "C" void func_00181BE0(void *p, const char *file, int line);            /* zsys_memory's _free */

/* the blocks: a link, then the payload */
struct four { four *next; };
struct eight { eight *next; unsigned int dummy[1]; };
struct twelve { twelve *next; unsigned int dummy[2]; };
struct sixteen { sixteen *next; unsigned int dummy[3]; };
struct twenty { twenty *next; unsigned int dummy[4]; };
struct twentyfour { twentyfour *next; unsigned int dummy[5]; };

static void *s_top;     /* 0x1D5680: past the highest pool, the pools' watermark */

template <class T>
class _zmalloc {
public:
    _zmalloc(unsigned int size);
    void init_page();
    /* a block of the pool's size; when the pool is spent, one from the caller's allocator */
    void *acquire(unsigned int size, void *(*alloc)(unsigned int, const char *, int))
    {
        void *result;

        if (size != m_bytes)
            return 0;
        if (m_head == 0) {
            m_overflow++;
            return alloc(sizeof(T), "unknown", 0);
        }
        m_count++;
        m_total++;
        if (m_peak < m_count)
            m_peak = m_count;
        result = m_head;
        m_head = m_head->next;
        return result;
    }
    bool release(void *_ptr)
    {
        T *ptr = (T *)_ptr;

        if (ptr >= m_page && ptr < m_tail) {
            m_count--;
            m_freed++;
            ptr->next = m_head;
            m_head = ptr;
            return true;
        }
        return false;
    }

    T *m_page;                  /* 0x00 */
    T *m_head;                  /* 0x04 */
    T *m_tail;                  /* 0x08 */
    unsigned int m_bytes;       /* 0x0C */
    unsigned int m_size;        /* 0x10 */
    unsigned int m_count;       /* 0x14 */
    unsigned int m_peak;        /* 0x18 */
    unsigned int m_overflow;    /* 0x1C */
    unsigned int m_total;       /* 0x20 */
    unsigned int m_freed;       /* 0x24 */
};

template <class T>
_zmalloc<T>::_zmalloc(unsigned int size)
{
    m_bytes = sizeof(T);
    m_size = size;
    m_count = 0;
    m_peak = 0;
    m_overflow = 0;
    m_total = 0;
    m_freed = 0;
    init_page();
}

template <class T>
void _zmalloc<T>::init_page()
{
    T *ptr = m_head = m_page = (T *)_calloc(sizeof(T), m_size, 0, 0);

    if ((T *)s_top < ptr + m_size * sizeof(T))
        s_top = ptr + m_size * sizeof(T) + 4;
    for (int ii = 0; ii < m_size - 1; ii++, ptr++)
        ptr->next = ptr + 1;
    ptr->next = 0;
    m_tail = ptr + 1;
}

static _zmalloc<twentyfour> s_pool24(2200);     /* 0x1D5690 */
static _zmalloc<twenty> s_pool20(7300);         /* 0x1D56C0 */
static _zmalloc<sixteen> s_pool16(5250);        /* 0x1D56F0 */
static _zmalloc<twelve> s_pool12(11600);        /* 0x1D5720 */
static _zmalloc<eight> s_pool8(5000);           /* 0x1D5750 */
static _zmalloc<four> s_pool4(8800);            /* 0x1D5780 */

/* the pool of the size asked for, else the system's allocator (byte-exact since round 6, as
 * func_001805A0 below): the switch's compare chain runs 24 down to 4, the bodies 4 up to 24 (cases
 * written ascending), each case testing its own result. The pool's overflow allocates sizeof(T) from
 * the allocator its caller names (_new here, _malloc for the C pools): MW keeps that constant in a0
 * from the compare (`li a0,4; beq s0,a0`), every delay slot empty -- a `size` argument had put the
 * constants in v0, the parameter in a0 (rounds 1-5's open). */
void *operator new(unsigned int size)
{
    void *p;

    switch (size) {
    case 4:
        p = s_pool4.acquire(size, _new);
        if (p)
            return p;
        break;
    case 8:
        p = s_pool8.acquire(size, _new);
        if (p)
            return p;
        break;
    case 12:
        p = s_pool12.acquire(size, _new);
        if (p)
            return p;
        break;
    case 16:
        p = s_pool16.acquire(size, _new);
        if (p)
            return p;
        break;
    case 20:
        p = s_pool20.acquire(size, _new);
        if (p)
            return p;
        break;
    case 24:
        p = s_pool24.acquire(size, _new);
        if (p)
            return p;
        break;
    }
    return _new(size, "unknown", 0);
}

void *operator new[](unsigned int size)
{
    return _new_vec(size, "unknown", 0);
}

void operator delete(void *p)
{
    if (p) {
        if (p < s_top) {
            if (!s_pool12.release(p) && !s_pool16.release(p) && !s_pool4.release(p)
                && !s_pool20.release(p) && !s_pool8.release(p) && !s_pool24.release(p))
                _delete(p, "unknown", 0);
        } else
            _delete(p, "unknown", 0);
    }
}

void operator delete[](void *p)
{
    if (p)
        _delete_vec(p, "unknown", 0);
}

/* operator new's pools for the C allocators (_strdup's short strings): the system's _malloc behind them */
extern "C" void *func_001805A0(unsigned int size)
{
    void *p;

    switch (size) {
    case 4:
        p = s_pool4.acquire(size, _malloc);
        if (p)
            return p;
        break;
    case 8:
        p = s_pool8.acquire(size, _malloc);
        if (p)
            return p;
        break;
    case 12:
        p = s_pool12.acquire(size, _malloc);
        if (p)
            return p;
        break;
    case 16:
        p = s_pool16.acquire(size, _malloc);
        if (p)
            return p;
        break;
    case 20:
        p = s_pool20.acquire(size, _malloc);
        if (p)
            return p;
        break;
    case 24:
        p = s_pool24.acquire(size, _malloc);
        if (p)
            return p;
        break;
    }
    return _malloc(size, 0, 0);
}

void _free(void *p, const char *file, int line)
{
    if (p) {
        if (p < s_top) {
            if (!s_pool12.release(p) && !s_pool16.release(p) && !s_pool4.release(p)
                && !s_pool20.release(p) && !s_pool8.release(p) && !s_pool24.release(p))
                func_00181BE0(p, file, line);
        } else
            func_00181BE0(p, file, line);
    }
}

char *_strdup(const char *s, const char *file, int line)
{
    char *p;
    unsigned int n = strlen(s) + 1;

    if (n <= 24) {
        p = (char *)func_001805A0((int)(n + 3) / 4 * 4);
        if (p)
            strcpy(p, s);
    } else
        p = func_00181950(s, file, line);
    return p;
}
