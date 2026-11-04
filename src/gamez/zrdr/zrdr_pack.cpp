/* zrdr_pack.cpp -- the tree packed into one buffer and relocated, and freed (OVL1 0x334D20-
 * 0x335FCF): zrdr_freearray here, the rest not written yet (the packers and relocators recurse
 * and inline themselves twice: 0x334EA0, 0x335010, 0x3352F0, 0x3357C0, 0x3359C0, 0x335C00,
 * 0x335E20; the accessors' out-of-line copies they call, 0x335700-0x3357B0). SOCOM 1 kept
 * zrdr_freearray in zrdr_main.cpp and zrdr_pack/zrdr_relocate after CRdrEditor; SOCOM II links
 * these after the editor's unit, ahead of zrndr_init.cpp. The unit's name is ours. */

#include <string.h>

#include "zrdr.h"

void _free(void *p, const char *file, int line);

/* the values a tree holds, its arrays' items counted down (0x335C00, not written yet: it
 * recurses and inlines itself) */
extern "C" int func_00335C00(_zrdr *rdr);

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

/* SOCOM 1's zrdr_pack(_zrdr *, void *, unsigned): the tree copied into a buffer of `size` bytes, the
 * root first (its copy marked packed), then its arrays -- the bytes used, or 0 when the buffer is too
 * small (SOCOM II counts the values through 0x335C00 and copies the arrays through its own helper) */
unsigned int zrdr_pack(_zrdr *rdr, void *buf, unsigned int size)
{
    unsigned int need = (func_00335C00(rdr) + 1) * sizeof(_zrdr);

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
