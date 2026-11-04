/* The valves' support classes (valve_list.cpp, valve_main.cpp): CValve itself is zanim.h's, where
 * ZAnim and the states use it. SOCOM 1 has no debug information for these; the shapes are read off
 * the code and the header's name is ours. */
#ifndef VALVE_H
#define VALVE_H

#include <vector>

#include "../zanim/zanim.h"

/* the pool's free valves: a class of its own over the pointer vector (~CValvePool 0x352870 runs
 * four this-tested levels before the data's delete, a std::vector three; the name ours) */
class CValveVector : public std::vector<CValve *> {
};

/* the valves' pool: free valves to reuse and a buffer their names are assigned from (SOCOM 1's
 * class, its static `valvePool` in valve_list.cpp: the same 0x1C bytes in both builds, read off
 * Open) */
class CValvePool {
public:
    /* the vector empty, the rest cleared (inline: valve_list.cpp's initialiser stores the six words
     * and the flag byte itself, 0x4034F0) */
    CValvePool() : m_initialized(false), m_namesSize(0), m_names(0), m_namesNext(0) {}
    /* the pool emptied, then the vector freed (its copy 0x352870 in valve_list.cpp's tail, the
     * initialiser's registered destructor) */
    ~CValvePool() { Free(); }
    CValve *Acquire(const char *name, VALVE_TYPE type);
    void Release(CValve *valve);
    bool FlushNames(bool force);
    void Free();

    CValveVector m_free;            /* 0x00 */
    bool m_initialized;             /* 0x0C */
    int m_namesSize;                /* 0x10 */
    char *m_names;                  /* 0x14 */
    char *m_namesNext;              /* 0x18 */
};
extern CValvePool valvePool;        /* 0x49E820 (SOCOM 1's static of valve_list.cpp, whose
                                       initialiser builds it, 0x4034F0) */

/* Close's, Reset's and Uninit's remove_if predicate: a valve of the type (any, for 0) deleted, or
 * given back to the pool, its name freed (SOCOM 1's functor; the type a byte, read unsigned).
 * Inline: its out-of-line copy follows remove_if's instance, with the iterator's pieces */
struct CValveOpDelete {
    CValveOpDelete(VALVE_TYPE type) : m_type(type) {}
    /* its out-of-line copy in valve_list.cpp's tail is 0x352610: OPEN, 6 instructions, registers
     * only -- the target loads m_type into a0 (this dying there) and builds the test's bool in v1,
     * here the other way round. Tried: the test through an inline bool member (as the condition or
     * into the local), m_type read into a local first, the comparison's operands swapped; the world
     * lane's round 7: an if-assigned or ternary bool, the kept copy declared first, the type through
     * a byte accessor, static helpers taking the type before or after the valve (all 6 words, or
     * longer) -- build/scratch/wld/v_vod*.py */
    bool operator()(CValve *valve)
    {
        bool del = valve->m_type == m_type || m_type == 0;

        if (del) {
            /* the copy the push takes by reference: only it goes to memory (stored ahead of the
             * call, reloaded after it) -- pushing the parameter homes it at entry */
            CValve *kept = valve;

            valve->FreeName();
            if (kept->m_pooled)
                valvePool.m_free.push_back(kept);
            else
                delete kept;
            return true;
        }
        return false;
    }

    unsigned char m_type;
};

/* a valve command's data (SOCOM 1's struct name, from its mangled members): the valve, and the
 * value it is compared with or set to -- a constant or another valve, each named as an argument's,
 * the anim's or itself (the layout read off Compare; the names ours). The VALVE and VBIT blocks
 * hold it from +4 */
struct VALVE_CMD_DATA {
    int ref;   // 0x0: the valve: an argument's index, the anim's valve index, or the valve
    int value;   // 0x4: the constant, or the second valve named as ref is
    unsigned char op;   // 0x8: ParseOperator's code
    unsigned char isArg : 1;   // 0x9
    unsigned char isAnim : 1;   // 0x9
    unsigned char valIsArg : 1;   // 0x9
    unsigned char valIsAnim : 1;   // 0x9
    unsigned char valIsValve : 1;   // 0x9: value names a valve

    static CValve *ParseValveName(const char *name, int &index);
    /* the valve's value against the block's, by its operator (0x353FD0, SOCOM 1's) */
    bool Compare();
};

#endif
