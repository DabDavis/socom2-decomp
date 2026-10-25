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
    /* its out-of-line copy in valve_list.cpp's tail is 0x352610: OPEN, 6 instructions, registers
     * only -- the target loads m_type into a0 (this dying there) and builds the test's bool in v1,
     * here the other way round. Tried: the test through an inline bool member (as the condition or
     * into the local), m_type read into a local first, the comparison's operands swapped */
    bool operator()(CValve *valve)
    {
        bool del = valve->m_type == m_type || m_type == 0;

        if (del) {
            /* the copy the push takes by reference: only it goes to memory (stored ahead of the
             * call, reloaded after it) -- pushing the parameter homes it at entry */
            CValve *kept = valve;

            valve->FreeName();
            if (kept->m_pooled)
                data_0049E820.m_free.push_back(kept);
            else
                delete kept;
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
