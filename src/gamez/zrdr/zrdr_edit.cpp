/* zrdr_edit.cpp -- CRdrEditor, an .rdr built in memory a list at a time (OVL1 0x3333F0-0x334D1F):
 * values added to the open list, a list opened under a tag and closed into an array. The unit
 * name is SOCOM 1's (its demo holds CRdrEditor's members in this order, addSTRING to the
 * constructor, with the deque instances after them); SOCOM II adds addbool and addrealnode.
 * Defined in reverse of their image order (-inline deferred). Not written yet: the deque's copy
 * constructor and the rest of its instances in the template tail (0x333B30-0x334BE0), which
 * include/deque does not declare. */

#include "zrdr.h"

void *_calloc(unsigned int n, unsigned int size, const char *file, int line);

struct PNT3D {
    float x, y, z;
};

/* the values allocator (zrdr_pack.cpp's unit, 0x335BC0): `n` zeroed values, the first typed */
extern "C" _zrdr *func_00335BC0(unsigned char type, int n);

/* the top of the stack read in place: MSL's deque back(), the element before start_ + size_
 * (0x3335C0's body) -- pop's first top() inlines it whole, every other caller calls it */
static inline CRdrEditor::CRDRNodeVec *&top_of(std::deque<CRdrEditor::CRDRNodeVec *> &d)
{
    unsigned int n = d.start_ + d.size_ - 1;

    return (CRdrEditor::CRDRNodeVec *&)d.buf_[n / 128][n % 128];
}

/* a value made an array of `n` zeroed values (SOCOM II's; the name ours): defined first, so its
 * code follows the deque copy constructor's out-of-line pieces; pop inlines it and _zrdr::Clone
 * (zrdr_main.cpp) calls it */
void zrdr_newarray(_zrdr *rdr, int n)
{
    rdr->array = func_00335BC0(ZRDR_INTEGER, n);
    rdr->length = n;
    rdr->type = ZRDR_ARRAY;
}

/* SOCOM 1's: the file's value cleared, an empty stack.
 * OPEN, 0x48 bytes: the target builds the stack as MSL's `stack(const Container &x = Container())
 * : c(x)` -- a temporary deque (0x3340F0), the member copy-constructed from it (0x333B30), the
 * temporary's buffer destroyed (0x2CEFB0 with 0) -- and include/deque declares no copy
 * constructor, so the stack here is default-constructed */
CRdrEditor::CRdrEditor()
{
}

/* SOCOM 1's: every open list closed */
CRdrEditor::~CRdrEditor()
{
    while (m_stack.size())
        pop();
}

/* the members the wrappers below call, never inlined in the target (nor in SOCOM 1's) */
#pragma auto_inline off

/* SOCOM 1's: a list opened, its tag (when there is one) its first value. The new vector is a
 * statement of its own: as push()'s argument its constructor would sit a level deeper and be
 * called, not inlined (measured at -inline level=4: an inline height, to recheck at level 5) */
bool CRdrEditor::push(const char *tag)
{
    if (tag && *tag)
        addstringnode(tag);
    CRDRNodeVec *v = new CRDRNodeVec;
    m_stack.push(v);
    return false;
}

/* SOCOM 1's: the open list closed, its values copied into an array and freed; the array added to
 * the enclosing list, or at the bottom made the file's own value */
bool CRdrEditor::pop()
{
    if (m_stack.empty())
        return true;
    CRDRNodeVec *v = top_of(m_stack.c);
    _zrdr *array = func_00335BC0(ZRDR_ARRAY, 1);

    zrdr_newarray(array, v->size());
    int i = 0;
    for (CRDRNodeVec::iterator it = v->begin(); it != v->end(); ++it, i++) {
        _zrdr *r = *it;

        *array->Item(i) = *r;
        _free(r, 0, 0);
    }
    delete v;
    m_stack.pop();
    if (m_stack.size())
        m_stack.top()->push_back(array);
    else {
        type = array->type;
        length = array->length;
        integer = array->integer;
        _free(array, 0, 0);
    }
    return true;
}

/* SOCOM 1's: an integer added to the open list */
bool CRdrEditor::addintnode(int i)
{
    _zrdr *r = func_00335BC0(ZRDR_INTEGER, 1);

    r->integer = i;
    m_stack.top()->push_back(r);
    return true;
}

/* a real added to the open list (SOCOM II's) */
bool CRdrEditor::addrealnode(float f)
{
    _zrdr *r = func_00335BC0(ZRDR_REAL, 1);

    r->real = f;
    m_stack.top()->push_back(r);
    return true;
}

/* SOCOM 1's: a string added to the open list, kept in the file's table */
bool CRdrEditor::addstringnode(const char *s)
{
    _zrdr *r = func_00335BC0(ZRDR_STRING, 1);

    r->string = m_strings.CreateString(s);
    m_stack.top()->push_back(r);
    return true;
}

#pragma auto_inline reset

/* SOCOM 1's: a copy of a value added to the open list, its strings into the file's table */
bool CRdrEditor::addrdrnode(_zrdr *rdr)
{
    _zrdr *r = (_zrdr *)_calloc(8, 1, 0, 0);

    r->Clone(rdr, &m_strings);
    m_stack.top()->push_back(r);
    return true;
}

/* SOCOM 1's: (tag integer) */
bool CRdrEditor::addint(const char *tag, int i)
{
    push(tag);
    addintnode(i);
    pop();
    return true;
}

/* SOCOM 1's: (tag x y z) */
bool CRdrEditor::addPNT3D(const char *tag, const PNT3D &p)
{
    push(tag);
    addrealnode(p.x);
    addrealnode(p.y);
    addrealnode(p.z);
    pop();
    return true;
}

/* (tag 0|1) (SOCOM II's) */
bool CRdrEditor::addbool(const char *tag, bool b)
{
    push(tag);
    addintnode(b);
    pop();
    return true;
}

/* SOCOM 1's: (tag string) */
bool CRdrEditor::addSTRING(const char *tag, const char *s)
{
    push(tag);
    addstringnode(s);
    pop();
    return true;
}
