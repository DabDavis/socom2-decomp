/* zrdr_pack.cpp -- the tree packed into one buffer, its equal arrays shared first, its values
 * compared, counted and freed (OVL1 0x334D20-0x335FCF). SOCOM 1 kept zrdr_freearray in
 * zrdr_main.cpp, zrdr_countarray with CRdrFile::Pack and zrdr_pack/zrdr_relocate after CRdrEditor;
 * SOCOM II links these after the editor's unit, ahead of zrndr_init.cpp. The unit's name is ours.
 * Defined in reverse of their image order (-inline deferred); the three recursive helpers are
 * `inline` (MW's one-level fallback: the target inlines each one level into its callers and into
 * itself, where -inline auto would refuse a recursive function outright) and their out-of-line
 * copies sit between the real functions: _zrdr_compress 0x335010, _zrdr_equal 0x3352F0,
 * zrdr_findarray 0x3357C0, with the list's and the value's accessor copies (0x334E70,
 * 0x3352A0-0x3352DF, 0x335700-0x3357BF). 0x335BC0 (the array allocator) is a draft. */

#include <string.h>
#include <list>

#include "zrdr.h"

void _free(void *p, const char *file, int line);

/* the arrays' list's end() through the sentinel's reference accessor (a stand-in, local to this unit:
 * SOCOM 1's MSL reaches the sentinel through its end pair). zrdr_findarray's whole copy then keeps the
 * end's address once, in a saved register, as the target does: MW keeps `&end_` computed as a member
 * offset of the list (a list member at a nonzero offset of its owner, or reached through an inline
 * reference accessor), and recomputes it at each loop test for a list at offset 0 of a pointer
 * (build/scratch/wld/p2-p4.cpp). The out-of-line copy (0x3352C0) is the same this + 4 */
namespace std {
template <>
inline list<_zrdr *>::iterator list<_zrdr *>::end()
{
    return iterator(base::iterator(&this->end_node()));
}
}

/* the compressor's totals (ours): the values the trees it was given held, and the values it saved by
 * sharing equal arrays */
static int g_compressValues;
static int g_compressSaved;

/* the recursive test (below) */
inline bool _zrdr_equal(const _zrdr *a, const _zrdr *b);

/* the public test (ours: CUIVariable::Set calls it with the variable and its new value; the
 * recursive test inlined one level) */
bool zrdr_equal(const _zrdr *a, const _zrdr *b)
{
    return _zrdr_equal(a, b);
}

/* SOCOM 1's zrdr_countarray: the values below an array, each array's items and theirs in turn --
 * SOCOM II's skips an array that shares another's items (nofree), counted where it lives. It
 * inlines itself twice, the accessors cut a level more in each copy (zrdr.h's heights); the counter
 * an int against the unsigned count (the item's address recomputed each pass, as the target's) */
int zrdr_countarray(_zrdr *rdr)
{
    int n = 0;

    if (rdr->IsArray() && !rdr->nofree) {
        unsigned int count = rdr->Count();

        n += count;
        for (int i = 0; i < count; i++)
            n += zrdr_countarray(rdr->Item(i));
    }
    return n;
}

/* SOCOM 1's: an array's values freed depth first, then the array, unless it lives in a packed
 * buffer; a string forgotten */
void zrdr_freearray(_zrdr *rdr)
{
    if (rdr->type == ZRDR_STRING)
        rdr->string = 0;
    else if (rdr->type == ZRDR_ARRAY && (rdr->type == ZRDR_ARRAY ? rdr->array : 0)) {
        unsigned char packed = rdr->nofree;
        if (!packed) {
            if (!packed) {
                /* the count written out: Count() on the non-const value reloads its type */
                int i;
                unsigned int n = rdr->type == ZRDR_ARRAY ? rdr->length : 0;
                for (i = 0; i < n; i++)
                    zrdr_freearray(rdr->Item(i));
                _free(rdr->array, 0, 0);
            }
            rdr->type = ZRDR_INTEGER;
            rdr->integer = 0;
        }
    }
}

/* SOCOM II's (the name ours): an array's items copied into the buffer and the array pointed at the
 * copy, each item that is an array marked packed and copied after them in turn; the buffer's next
 * free byte returned (zrdr_pack calls it on the copied root, and it calls itself). Pinned: neither
 * zrdr_pack nor its own recursion inlines it in the target. The type tested in a one-case switch
 * (the target's beq/b pair), the advance `n * 8` apart from memcpy's `n * sizeof(_zrdr)` (MW
 * computes the product twice, as the target does, only for two different expressions) */
#pragma auto_inline off
char *zrdr_packarray(_zrdr *rdr, char *buf)
{
    int n = rdr->Count();

    if (n) {
        _zrdr *items = (_zrdr *)buf;

        memcpy(items, rdr->array, n * sizeof(_zrdr));
        rdr->array = items;
        buf += n * 8;
        for (int i = 0; i < n; i++) {
            switch (items[i].type) {
            case ZRDR_ARRAY:
                items[i].SetPacked(true);
                buf = zrdr_packarray(&items[i], buf);
                break;
            }
        }
    }
    return buf;
}
#pragma auto_inline reset

/* the arrays seen so far (ours): an array alike to one of them, or 0. end() in the loop test: the
 * one-level copy in _zrdr_compress calls it before the loop and at every step (the whole copy here
 * keeps it once, through the end() above) */
inline _zrdr *zrdr_findarray(std::list<_zrdr *> &seen, _zrdr *rdr)
{
    if (rdr->IsArray()) {
        for (std::list<_zrdr *>::iterator i = seen.begin(); i != seen.end(); i++) {
            if (_zrdr_equal(*i, rdr))
                return *i;
        }
    }
    return 0;
}

/* SOCOM II's (the name ours): whether two values are alike -- the same type and the same number, text
 * or, for an array, the same count of items alike in turn (an empty array is never alike). The cases
 * in the target's block order (the compare chain runs them backwards) */
inline bool _zrdr_equal(const _zrdr *a, const _zrdr *b)
{
    bool r = false;

    if (a->Type() == b->Type()) {
        switch (a->Type()) {
        case ZRDR_REAL:
            r = a->Real() == b->Real();
            break;
        case ZRDR_INTEGER:
            r = a->Int() == b->Int();
            break;
        case ZRDR_STRING:
            r = strcmp(a->String(), b->String()) == 0;
            break;
        case ZRDR_ARRAY: {
            unsigned int n = a->Count();

            if (n && n == b->Count()) {
                r = true;
                for (int i = 0; r && i < n; i++)
                    r = _zrdr_equal(a->Item(i), b->Item(i));
            }
            break;
        }
        }
    }
    return r;
}

/* an array's items that are alike to an array seen before made to share its items (their own
 * freed, the shared ones marked not to free), the others walked in turn (ours) */
inline void _zrdr_compress(std::list<_zrdr *> &seen, _zrdr *rdr)
{
    if (rdr->IsArray()) {
        seen.push_back(rdr);
        int n = rdr->Count();
        for (int i = 0; i < n; i++) {
            _zrdr *item = rdr->Item(i);
            _zrdr *found = zrdr_findarray(seen, item);

            if (found) {
                zrdr_freearray(item);
                item->array = found->array;
                item->type = ZRDR_ARRAY;
                item->nofree = true;
            } else
                _zrdr_compress(seen, item);
        }
    }
}

/* a tree's equal arrays shared (SOCOM II's, the name ours: CRdrFile::Pack calls it before packing),
 * the totals counted */
void zrdr_compress(_zrdr *rdr)
{
    std::list<_zrdr *> seen;
    int before = zrdr_countarray(rdr);

    _zrdr_compress(seen, rdr);
    int after = zrdr_countarray(rdr);
    g_compressValues += before;
    g_compressSaved += before - after;
}

/* SOCOM 1's zrdr_pack(_zrdr *, void *, unsigned): the tree copied into a buffer of `size` bytes, the
 * root first (its copy marked packed), then its arrays -- the bytes used, or 0 when the buffer is too
 * small (SOCOM II counts the values through 0x335C00 and copies the arrays through its own helper) */
unsigned int zrdr_pack(_zrdr *rdr, void *buf, unsigned int size)
{
    unsigned int need = (zrdr_countarray(rdr) + 1) * sizeof(_zrdr);

    if (size < need)
        return 0;
    _zrdr *root = (_zrdr *)buf;

    *root = *rdr;
    root->SetPacked(true);
    switch (root->type) {
    case ZRDR_ARRAY:
        zrdr_packarray(root, (char *)(root + 1));
        break;
    }
    return need;
}

/* SOCOM 1's: whether an array holds an array (SOCOM 1 kept it with the writer, _OutputASCII; here
 * it sits just ahead of the packers, the unit's last-defined function) */
bool zrdr_IsComplex(_zrdr *rdr)
{
    bool r = false;

    for (int i = 0; i < rdr->Count(); i++) {
        if (rdr->Item(i)->type == ZRDR_ARRAY) {
            r = true;
            break;
        }
    }
    return r;
}
