/* zrdr_main.cpp -- the zrdr finders (OVL1 0x32DC00-0x32F5FB): a tag looked up in a value's array,
 * the value after it read as numbers, strings, a bool, a rect or a range; a value's destructor,
 * its children freed, its clone; the file name an .rdr is read from. The unit name is SOCOM 1's
 * (the press beta's asserts name a zrdr_main.cpp; SOCOM 1's demo holds these functions in this
 * order between zrdr_free and zrdr_read). Defined in reverse of their image order (-inline
 * deferred). */

#include "zrdr.h"
#include <string.h>
#include <stdio.h>

struct PNT2D {
    float x, y;
};

struct PNT3D {
    float x, y, z;
};

/* SOCOM 1's DWARF */
struct IPNT2D {
    int x, y;
};

class Rfloat {
public:
    float m_min;   // 0x0
    float m_range;   // 0x4
};

class tag_RECT {
public:
    int left;   // 0x0
    int top;   // 0x4
    int right;   // 0x8
    int bottom;   // 0xc
};

/* CSTable::CreateString, not written yet: the string table's copy of a string */
extern "C" char *func_0039F1C0(CSTable *table, const char *s);
/* the array allocator (zrdr's, 0x334170): `count` zeroed values for `rdr` */
extern "C" void func_00334170(_zrdr *rdr, int count);
/* zrdr_freearray (0x335AA0): an array's values freed, recursively */
extern "C" void func_00335AA0(_zrdr *rdr);

/* SOCOM 1's: an .rdr's file name, under a directory when one is given */
const char *zrdr_findfile(const char *name, const char *path)
{
    static char _buffer[256];

    if (path) {
        sprintf(_buffer, "%s/%s", path, name);
        return _buffer;
    }
    return name;
}

/* SOCOM 1's: a deep copy of `src`, its strings into `strings` when one is given; every array
 * the copy's own */
void _zrdr::Clone(_zrdr *src, CSTable *strings)
{
    static bool s_warn = true;

    if (!src)
        return;
    type = src->type;
    length = src->length;
    switch (type) {
    case ZRDR_INTEGER:
        integer = src->integer;
        break;
    case ZRDR_REAL:
        real = src->real;
        break;
    case ZRDR_STRING:
        if (!strings && s_warn)
            s_warn = false;
        string = strings ? func_0039F1C0(strings, src->string) : src->string;
        break;
    case ZRDR_ARRAY: {
        int n = src->Count();
        func_00334170(this, n);
        for (int i = 0; i < n; i++) {
            Item(i)->Clone(src->Item(i), strings);
            Item(i)->packed = false;
        }
        packed = false;
        break;
    }
    }
    isclone = true;
}

/* SOCOM 1's: an array the value owns, freed */
void _zrdr::DeleteChildren()
{
    if (isclone && type == ZRDR_ARRAY)
        func_00335AA0(this);
}

_zrdr::~_zrdr()
{
}

/* SOCOM 1's: the value after the tag, in this array only (its arrays not searched) */
_zrdr *zrdr_flat_findtag_startidx(const _zrdr *rdr, const char *tag, int start)
{
    _zrdr *found = 0;

    if (rdr && rdr->type == ZRDR_ARRAY) {
        unsigned int n = rdr->Count();
        for (int i = start; i < n; i++) {
            _zrdr *item = rdr->Item(i);
            bool match = item->type == ZRDR_STRING ? strcmp(tag, item->string) == 0 : false;
            if (match) {
                found = &rdr->array[i + 1];
                break;
            }
        }
    }
    return found;
}

/* SOCOM 1's: the value after the tag, searched depth first */
_zrdr *zrdr_findtag_startidx(const _zrdr *rdr, const char *tag, int start)
{
    _zrdr *found = 0;

    if (rdr && rdr->type == ZRDR_ARRAY) {
        unsigned int n = rdr->Count();
        for (int i = start; !found && i < n; i++) {
            _zrdr *item = rdr->Item(i);
            if (item->type == ZRDR_ARRAY)
                found = zrdr_findtag_startidx(item, tag, 0);
            else {
                bool match = item->type == ZRDR_STRING ? strcmp(tag, item->string) == 0 : false;
                if (match)
                    found = &rdr->array[i + 1];
            }
        }
    }
    return found;
}

_zrdr *zrdr_findtag(const _zrdr *rdr, const char *tag)
{
    return zrdr_findtag_startidx(rdr, tag, 0);
}

/* SOCOM 1's: the value after the next tag starting with `tag` past `after` (0: the first) */
_zrdr *_zrdr_nexttag(const _zrdr *rdr, const char *tag, int len, _zrdr *after)
{
    _zrdr *found = 0;
    unsigned int n = rdr->Count();

    for (int i = 0; !found && i < n; i++) {
        _zrdr *item = rdr->Item(i);
        if (item == after)
            after = 0;
        else if (item->type == ZRDR_ARRAY)
            found = _zrdr_nexttag(item, tag, len, after);
        else if (!after && item->type == ZRDR_STRING && strncmp(item->string, tag, len) == 0)
            found = &rdr->array[i + 1];
    }
    return found;
}

_zrdr *zrdr_nexttag(const _zrdr *rdr, const char *tag, _zrdr *after)
{
    _zrdr *found = 0;

    if (rdr && rdr->type == ZRDR_ARRAY)
        found = _zrdr_nexttag(rdr, tag, strlen(tag), after);
    return found;
}

/* SOCOM 1's: the tag's string, or its array's first item's */
char *zrdr_findstring(const _zrdr *rdr, const char *tag)
{
    char *s = 0;
    const _zrdr *t = zrdr_findtag(rdr, tag);

    if (t) {
        if (t->type == ZRDR_STRING)
            s = t->String();
        else if (t->type == ZRDR_ARRAY && t->Item(0) && t->Item(0)->type == ZRDR_STRING)
            s = t->Item(0)->String();
    }
    return s;
}

/* zrdr_findstring over the flat search from `start` (SOCOM II's; the name ours) */
char *zrdr_flat_findstring(const _zrdr *rdr, const char *tag, int start)
{
    char *s = 0;
    const _zrdr *t = zrdr_flat_findtag_startidx(rdr, tag, start);

    if (t) {
        if (t->type == ZRDR_STRING)
            s = t->String();
        else if (t->type == ZRDR_ARRAY && t->Item(0) && t->Item(0)->type == ZRDR_STRING)
            s = t->Item(0)->String();
    }
    return s;
}

/* a value that is an array (ours: zrdr_isitemtype materialises the test, an inline bool's) */
static inline bool _isarray(const _zrdr *rdr)
{
    return rdr && rdr->type == ZRDR_ARRAY;
}

/* whether an array's item `index` is of `type` (SOCOM II's; the name ours) */
bool zrdr_isitemtype(const _zrdr *rdr, int index, int type)
{
    bool r = false;
    bool ok = false;

    if (_isarray(rdr) && index < rdr->Count())
        ok = true;
    if (ok && rdr->Item(index)->type == type)
        r = true;
    return r;
}

/* SOCOM 1's: n numbers after the tag, read as reals */
bool zrdr_findreal(const _zrdr *rdr, const char *tag, float *out, int n)
{
    return zrdr_toREAL(zrdr_findtag(rdr, tag), out, n);
}

/* SOCOM 1's: n numbers after the tag, read as integers */
bool zrdr_findint(const _zrdr *rdr, const char *tag, int *out, int n)
{
    return zrdr_toINT(zrdr_findtag(rdr, tag), out, n);
}

bool zrdr_findRfloat(const _zrdr *rdr, const char *tag, Rfloat *out)
{
    return zrdr_toRfloat(zrdr_findtag(rdr, tag), out);
}

bool zrdr_finduint(const _zrdr *rdr, const char *tag, unsigned int *out, int n)
{
    return zrdr_toINT(zrdr_findtag(rdr, tag), (int *)out, n);
}

/* a rect, four integers (SOCOM II's; the name ours) */
bool zrdr_toRECT(const _zrdr *rdr, tag_RECT *out)
{
    bool ok = false;

    if (rdr && rdr->type == ZRDR_ARRAY && rdr->Count() >= 4) {
        out->left = rdr->Item(0)->Int();
        out->top = rdr->Item(1)->Int();
        out->right = rdr->Item(2)->Int();
        out->bottom = rdr->Item(3)->Int();
        ok = true;
    }
    return ok;
}

/* a range: one number (no range) or a low and a high (SOCOM 1's findRfloat did it in place; the
 * name ours) */
bool zrdr_toRfloat(const _zrdr *rdr, Rfloat *out)
{
    bool ok = false;
    unsigned int n = rdr && rdr->type == ZRDR_ARRAY ? rdr->Count() : 0;

    if (n == 1) {
        _zrdr *a = rdr->Item(0);
        if (a->IsNumber()) {
            out->m_min = a->Real();
            ok = true;
            out->m_range = 0.0f;
        }
    } else if (n == 2) {
        _zrdr *a = rdr->Item(0);
        _zrdr *b = rdr->Item(1);
        if (a->IsNumber() && b->IsNumber()) {
            float hi = b->Real();
            float lo = a->Real();
            ok = true;
            out->m_min = lo;
            out->m_range = hi - lo;
        }
    }
    return ok;
}

/* SOCOM 1's: "true"/"on", "false"/"off", or a number; an array's first item */
bool zrdr_tobool(const _zrdr *rdr, bool *out)
{
    bool ok = false;
    bool dummy;

    if (!out)
        out = &dummy;
    if (rdr) {
        if (rdr->type == ZRDR_ARRAY && rdr->Count())
            rdr = rdr->Item(0);
        if (rdr->type == ZRDR_STRING) {
            if (!strcasecmp(rdr->string, "true") || !strcasecmp(rdr->string, "on")) {
                *out = true;
                ok = true;
            } else if (!strcasecmp(rdr->string, "false") || !strcasecmp(rdr->string, "off")) {
                *out = false;
                ok = true;
            }
        } else if (rdr->IsNumber()) {
            ok = true;
            if (rdr->Int())
                *out = true;
            else
                *out = false;
        }
    }
    return ok;
}

/* SOCOM 1's: an array of at least n numbers read as integers */
bool zrdr_toINT(const _zrdr *rdr, int *out, int n)
{
    bool ok = false;

    if (rdr && rdr->type == ZRDR_ARRAY && rdr->Count() >= n) {
        ok = true;
        for (int i = 0; i < n; i++)
            out[i] = rdr->Item(i)->Int();
    }
    return ok;
}

/* SOCOM 1's: an array of at least n numbers read as reals */
bool zrdr_toREAL(const _zrdr *rdr, float *out, int n)
{
    bool ok = false;

    if (rdr && rdr->type == ZRDR_ARRAY && rdr->Count() >= n) {
        ok = true;
        for (int i = 0; i < n; i++)
            out[i] = rdr->Item(i)->Real();
    }
    return ok;
}

/* SOCOM 1's: the tag's string; an integer 0 is no string */
bool zrdr_findSTRING(const _zrdr *rdr, const char *tag, const char **out)
{
    const _zrdr *t = zrdr_findtag(rdr, tag);
    bool ok = false;

    if (t && out) {
        if (t->type == ZRDR_ARRAY && t->Count())
            t = t->Item(0);
        if (t->type == ZRDR_INTEGER && t->Int() == 0) {
            *out = 0;
            ok = true;
        } else if (t->type == ZRDR_STRING) {
            *out = t->String();
            ok = true;
        }
    }
    return ok;
}

bool zrdr_findPNT3D(const _zrdr *rdr, const char *tag, PNT3D *out)
{
    return zrdr_toREAL(zrdr_findtag(rdr, tag), &out->x, 3);
}

bool zrdr_findPNT2D(const _zrdr *rdr, const char *tag, PNT2D *out)
{
    return zrdr_toREAL(zrdr_findtag(rdr, tag), &out->x, 2);
}

bool zrdr_findIPNT2D(const _zrdr *rdr, const char *tag, IPNT2D *out)
{
    return zrdr_toINT(zrdr_findtag(rdr, tag), &out->x, 2);
}

/* SOCOM II's (DEST_RECT and SOURCE_RECT); the name ours */
bool zrdr_findRECT(const _zrdr *rdr, const char *tag, tag_RECT *out)
{
    return zrdr_toRECT(zrdr_findtag(rdr, tag), out);
}

bool zrdr_findbool(const _zrdr *rdr, const char *tag, bool *out)
{
    return zrdr_tobool(zrdr_findtag(rdr, tag), out);
}
