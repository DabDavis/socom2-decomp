/* zar_key.cpp -- zar::CKey, a key of the archive's directory: its children by name, read and
 * written with the directory (OVL1 0x25AE00-...). The unit name is ours: zar_search.cpp's fetches
 * and inserts call FindKey and InsertKey out of line, where Read here inlines InsertKey -- the same
 * order as SOCOM 1's (FindKey, Read, Write, InsertKey after its zar_search.cpp fetches). Defined in
 * reverse of their image order (-inline deferred). */

/* a modelling device (the MSL headers round 39): include/vector's end() over __iter(__raw_end()), CountKeys' below */
#define __MSL_VECTOR_END_ARG
/* (the same, the engine core's round 28: include/vector's pointer pod over SOCOM 1's compressed pair, for
 * DeleteChildren's outer FreeVector; its note has the measure) */
#define __MSL_VECTOR_POD_PAIR

#include <algorithm>
#include <string.h>
#include "zar.h"
#include "../zsys/zsys.h"

namespace zar {

/* a key named so, empty (SOCOM 1's; CZAR's constructor makes the root with "") */
CKey::CKey(const char *name) : m_name((char *)name), m_size(0), m_offset(0)
{
}

CKey::CKey() : m_name(0), m_size(0), m_offset(0)
{
}

/* SOCOM 1's destroyed only its list; SOCOM II's first forgets its children (DeleteChildren,
 * 0x25B580) */
CKey::~CKey()
{
    DeleteChildren(false);
}

/* a child deleted after its own children (0x25BAE0), a child's children forgotten (0x25BB20):
 * DeleteChildren's two for_each operations (SOCOM II's; the names ours) */
static void DeleteKey(CKey *key)
{
    if (key) {
        key->DeleteChildren(true);
        delete key;
    }
}

static void ForgetKey(CKey *key)
{
    if (key)
        key->DeleteChildren(false);
}

/* the children forgotten -- each deleted too when asked -- and the ring's storage given up
 * (SOCOM II's, 0x25B580: FreeDirectory calls it on the root). Recursive through its operations:
 * the outer copy inlines both for_each walks with their operations, DeleteKey's DeleteChildren(true)
 * and its delete's ~CKey -> DeleteChildren(false) once each; a call four levels down stays inlined,
 * one five down is called -- so the deep copy calls for_each's pieces, DeleteKey, the base clear
 * 0x25BAD0 and FreeVector's four, the copy under ~CKey for_each 0x25BA50, clear and FreeVector
 * 0x25B9D0 whole (all five rows byte-exact from here, as are DeleteKey 0x25BAE0 and ForgetKey
 * 0x25BB20). OPEN, 0x33C of 0x398, the tail only (every instruction before it the target's, the
 * frame 16 bytes smaller): the outer FreeVector reaches the pointer pod's pieces at depth five, and
 * the image's pod is not include/vector's -- its constructor calls a piece storing the capacity
 * passed by value (0x25B9C0, `sw a1,0(a0)`), its member swap calls three distinct std::swap copies
 * (0x25B9A0 the capacity at +0, 0x25B980 the data at +8, 0x25B960 the count at +4) where ours calls
 * the free swap, and its destructor is `if (*data()) alloc().deallocate(*data(), *cap())` with each
 * accessor a call (0x25B950 data() by reference, 0x25B940 alloc(), 0x25B930 cap(), 0x25B920 the
 * deallocate's operator delete) -- SOCOM 1's compressed pair, include/vector's "LEVEL 8, MEASURED
 * AND NOT ADOPTED" shape: an MSL header change, not this unit's */
void CKey::DeleteChildren(bool del)
{
    if (del)
        std::for_each(begin(), end(), DeleteKey);
    else
        std::for_each(begin(), end(), ForgetKey);
    clear();
    FreeVector<std::vector<CKey *> >(*this);
}

/* a key named so, skipping the deleted (a size of -1): SOCOM 1's predicate, the struct's name ours */
struct CKeyNamed {
    CKeyNamed(const char *name) : m_name(name) {}
    bool operator()(const CKey *key) const
    {
        return key->m_size != -1 && !strcasecmp(m_name, key->m_name);
    }

    const char *m_name;
};

/* the key's record -- its name's offset, its data's offset and size -- then the child count and
 * each child's own, through the archive's file (SOCOM 1's, 0x25B030: SOCOM 1's secured each word
 * around its write; it returns 1). Recursive: it self-inlines twice -- the second copy inlines its
 * callees one level (the count's size() 0x25B2E0, the raw begin and __iter, end(), ==, the postfix
 * ++'s pieces called), the third calls them all (WriteFile 0x25B2C0, Count 0x25B2B0, begin, end,
 * !=, *, postfix ++) and Write itself; every loop is entered at its bottom test by a `b`.
 * OPEN, 0x2CC of 0x280: the outer copy and the second's prologue are the target's -- the loop a
 * `while` (a `for` gets a guard: 0x2C4), the count through a Count() over size() (the second copy
 * calling size() as the target's) -- but the inlined copies' while loops are rotated behind a
 * duplicated test where the target's enter by a jump; a named key local in the body, a `for (;;)`
 * with a break, a goto into a do-while, prefix ++: 0x264-0x2D4 (build/scratch/core/v_write19*.py) */
bool CKey::Write(CZAR *zar)
{
    zar->WriteFile(&m_name, 4);
    zar->WriteFile(&m_offset, 4);
    zar->WriteFile(&m_size, 4);

    int count = Count();

    zar->WriteFile(&count, 4);
    iterator i = begin();

    while (i != end()) {
        (*i)->Write(zar);
        i++;
    }
    return true;
}

/* a key's directory record (SOCOM II's, called out of line by WriteDirectory) */
int CKey::RecordSize()
{
    return 16;
}

/* the keys counted, the key itself included, its children's own when deep (SOCOM II's, 0x25B300:
 * WriteDirectory sizes the directory with it on the root, true). Recursive: it self-inlines four
 * levels, each cutting vector<CKey *>'s pieces a level more (the MSL headers round 24, on the
 * image's pointer iterator). The end taken once ahead of the loop; the flag tested `!= false`,
 * so MW hoists the narrowed flag and tests it with beqz -- `deep && ...` hoists `!deep` instead
 * (sltu/xori, bnez), a bool copy or the end in the test change the copies' sizes.
 * BYTE-EXACT SINCE THE MSL HEADERS' ROUND 39, through this unit's __MSL_VECTOR_END_ARG (include/vector's note on end()). Until then
 * one call off (0x240 both, every instruction the target's): the second copy's end() inlined
 * the raw end and called the pod's end 0x34D920 where the target calls CKey's raw end 0x257BA0 --
 * one level deeper; `#pragma inline_max_size` 240 to 300 does not move it (120-238: 0x18C), and every
 * spelling with the end in the loop test (as _countKeys' and fixupKey's, byte-exact) is 0x1B8-0x27C;
 * round 15: the end or the iterator assigned rather than initialised 0x19C-0x1A4, the end first 20
 * off (build/scratch/core/v_ck*.py) */
int CKey::CountKeys(bool deep)
{
    int count = 1;

    for (iterator i = begin(), e = end(); deep != false && i != e; ++i)
        count += (*i)->CountKeys(true);
    return count;
}

/* a child key added at the end (SOCOM 1's inserted into a list) */
CKey *CKey::InsertKey(CKey *key)
{
    push_back(key);
    return key;
}

/* the directory record: the name's offset, the data's offset, its size, the child count */
struct KEY_RECORD {
    int name;
    int offset;
    int size;
    int count;
};

bool CKey::Read(CZAR *zar, CIO *io, int base)
{
    /* the file through a copy: numbered after the parameters, it takes s1 where io would take s2
     * (README "MW's saved-register order is a NUMBERING") */
    CIO *file = io;
    KEY_RECORD rec;

    file->fread(&rec, sizeof(rec));
    if (zar->m_root != this) {
        m_name = (char *)(rec.name + base);
        m_offset = rec.offset;
        m_size = rec.size;
    }

    int count = rec.count;

    if (count < 0)
        count = 0;
    if (count)
        reserve(count);
    while (count--) {
        CKey *key = zar->NewKey(0);

        key->Read(zar, file, base);
        InsertKey(key);
    }
    return true;
}

/* a child key by name (SOCOM 1's walked a std::list) */
CKey *CKey::FindKey(const char *name)
{
    if (empty())
        return 0;

    iterator it = std::find_if(begin(), end(), CKeyNamed(name));

    return it == end() ? 0 : *it;
}

}

/* The unit inlines to depth 8, not the flags' 5 (seal_ai.cpp's and zanim_cmd_tick.cpp's opt-in; a pragma acts through its state
 * at the unit's end): with the pointer pod over the compressed pair FreeVector's instance 0x25B9D0 and ~CKey's four
 * this-tests inline whole (7 or more), the outer DeleteChildren's FreeVector calling the pieces */
#pragma inline_depth(8)
