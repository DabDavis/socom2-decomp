/* zanim_names.cpp -- the ZAnim name tables (OVL1 0x268960-0x269170). Not anim_main.cpp's:
 * CZAnimMain::Unload calls CZAnimNameTable::Unload, which -inline auto would inline from the
 * same unit (0xA0 bytes: it does, and Unload comes out 0x180 long), and matches only with the
 * call. The unit's name is ours. Defined in reverse of their image order. */

#include <string.h>

#include "zanim.h"

struct ZarNode;

/* a vector's iterator, as MSL's: != is !(==), which keeps MW from folding the first test */
struct ZarIter {
    ZarNode **p;

    ZarIter(ZarNode **q) : p(q) {}
    bool operator==(const ZarIter &o) const { return p == o.p; }
    bool operator!=(const ZarIter &o) const { return !(*this == o); }
    ZarIter &operator++() { ++p; return *this; }
    ZarNode *operator*() const { return *p; }
};

/* a ZAR's directory node, as far as the tables read it: its children a vector */
struct ZarNode {
    unsigned char _0[4];
    int count;                      /* 0x4: children */
    ZarNode **child;                /* 0x8 */
    char *data;                     /* 0xC */

    ZarIter begin() { return ZarIter(child); }
    ZarIter end() { return ZarIter(child + count); }
};

extern "C" ZarNode *func_00258570(zar::CZAR *zar);
extern "C" ZarNode *func_0025AE00(ZarNode *node, const char *name);
extern "C" int func_002585A0(zar::CZAR *zar, ZarNode *node);
extern "C" void func_00258720(zar::CZAR *zar, ZarNode *node);
extern "C" const char *func_0026C8E0(CZAnimSet *set, int index);
extern "C" int func_0026C8D0(CZAnimSet *set, const char *name);   /* the set's name table's AddName */

inline void CZAnimNameTable::Append(const char *name)
{
    m_name = (char **)_realloc(m_name, (m_count + 1) * 4, 0, 0);
    m_name[m_count] = (char *)_malloc(strlen(name) + 1, 0, 0);
    strcpy(m_name[m_count], name);
    m_count++;
}

/* entry 0 of every table: a name that is no name */
int CZAnimNameTable::AddName(const char *name)
{
    if (!name)
        return 0;
    if (m_count == 0)
        Append("NA");
    for (int i = 0; i < m_count; i++)
        if (!strcmp(m_name[i], name))
            return i;
    Append(name);
    return m_count - 1;
}

int CZAnimNameIndexTable::AddName(CZAnimSet *set, const char *name)
{
    if (!name)
        return 0;
    int index = func_0026C8D0(set, name);

    if (m_count == 0) {
        int none = func_0026C8D0(set, "NA");

        m_index = (unsigned short *)_realloc(m_index, (m_count + 1) * 2, 0, 0);
        m_index[m_count] = none;
        m_count++;
    }
    for (int i = 0; i < m_count; i++)
        if (index == m_index[i])
            return i;
    m_index = (unsigned short *)_realloc(m_index, (m_count + 1) * 2, 0, 0);
    m_index[m_count] = index;
    m_count++;
    return m_count - 1;
}

const char *CZAnimNameIndexTable::GetName(CZAnimSet *set, int index)
{
    if (index >= 0 && index < m_count)
        return func_0026C8E0(set, m_index[index]);
    return 0;
}

void CZAnimNameTable::Unload()
{
    if (m_name) {
        for (int i = 0; i < m_count; i++)
            if (m_name[i])
                func_00180280(m_name[i], 0, 0);
        func_00180280(m_name, 0, 0);
    }
    m_name = 0;
    m_count = 0;
}

int CZAnimNameIndexTable::Load(zar::CZAR &zar)
{
    int count;

    func_00258570(&zar);
    zar.Fetch("Name_Index_Table_Count", &count, 4);
    if (count > 0) {
        Unload();
        m_index = (unsigned short *)_realloc(m_index, count * 2, 0, 0);
        zar.Fetch("Name_Index_Table", m_index, count * 2);
        m_count = count;
    }
    return 1;
}

const char *CZAnimNameTable::GetName(int index)
{
    if (index >= 0 && index < m_count)
        return m_name[index];
    return 0;
}

void CZAnimNameIndexTable::Unload()
{
    if (m_index)
        func_00180280(m_index, 0, 0);
    m_index = 0;
    m_count = 0;
}

/* OPEN, 16 instructions: MW folds the loop's first test (begin against begin + count) and
 * keeps AddName's own null test inside the caller's; the target does neither. */
int CZAnimNameTable::Load(zar::CZAR &zar)
{
    int count;
    ZarNode *root = func_00258570(&zar);

    zar.Fetch("Name_Table_Count", &count, 4);
    if (count > 0) {
        Unload();
        ZarNode *table = func_0025AE00(root, "Name_Table");

        if (table) {
            for (ZarIter it = table->begin(); it != table->end(); ++it) {
                if (func_002585A0(&zar, *it)) {
                    const char *name = (*it)->data;

                    if (name)
                        AddName(name);
                    func_00258720(&zar, *it);
                }
            }
        }
    }
    return 1;
}
