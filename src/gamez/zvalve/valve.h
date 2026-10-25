/* The valves' support classes (valve_list.cpp, valve_main.cpp): CValve itself is zanim.h's, where
 * ZAnim and the states use it. SOCOM 1 has no debug information for these; the shapes are read off
 * the code and the header's name is ours. */
#ifndef VALVE_H
#define VALVE_H

#include <vector>

#include "../zanim/zanim.h"

/* the valves' pool: free valves to reuse and a buffer their names are assigned from (SOCOM 1's
 * class, its static `valvePool` in valve_list.cpp: the same 0x1C bytes in both builds, read off
 * Open) */
class CValvePool {
public:
    CValve *Acquire(const char *name, VALVE_TYPE type);
    void Release(CValve *valve);
    bool FlushNames(bool force);
    void Free();

    std::vector<CValve *> m_free;   /* 0x00 */
    bool m_initialized;             /* 0x0C */
    int m_namesSize;                /* 0x10 */
    char *m_names;                  /* 0x14 */
    char *m_namesNext;              /* 0x18 */
};
extern CValvePool data_0049E820;    /* valvePool (SOCOM 1's static; valve_list.cpp's initialiser
                                       builds it, 0x4034F0) */

/* Close's, Reset's and Uninit's remove_if predicate: a valve of the type (any, for 0) deleted, or
 * given back to the pool, its name freed (SOCOM 1's functor; the type a byte, read unsigned).
 * Inline: its out-of-line copy follows remove_if's instance, with the iterator's pieces */
struct CValveOpDelete {
    CValveOpDelete(VALVE_TYPE type) : m_type(type) {}
    bool operator()(CValve *valve)
    {
        bool del = valve->m_type == m_type || m_type == 0;

        if (del) {
            valve->FreeName();
            if (valve->m_pooled)
                data_0049E820.m_free.push_back(valve);
            else
                delete valve;
            return true;
        }
        return false;
    }

    unsigned char m_type;
};

/* a valve command's data (SOCOM 1's struct name, from its mangled members) */
struct VALVE_CMD_DATA {
    static CValve *ParseValveName(const char *name, int &index);
};

#endif
