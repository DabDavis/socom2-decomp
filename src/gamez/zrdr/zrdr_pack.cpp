/* zrdr_pack.cpp -- the tree packed into one buffer and relocated, and freed (OVL1 0x334D20-
 * 0x335FCF): zrdr_freearray here, the rest not written yet (the packers and relocators recurse
 * and inline themselves twice: 0x334EA0, 0x335010, 0x3352F0, 0x3357C0, 0x3359C0, 0x335C00,
 * 0x335E20; the accessors' out-of-line copies they call, 0x335700-0x3357B0). SOCOM 1 kept
 * zrdr_freearray in zrdr_main.cpp and zrdr_pack/zrdr_relocate after CRdrEditor; SOCOM II links
 * these after the editor's unit, ahead of zrndr_init.cpp. The unit's name is ours. */

#include "zrdr.h"

void _free(void *p, const char *file, int line);

/* SOCOM 1's: whether an array holds an array (SOCOM 1 kept it with the writer, _OutputASCII; here
 * it sits just ahead of the packers, which unit's it is unsure) */
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
