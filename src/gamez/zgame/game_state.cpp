/* game_state.cpp -- the commands that change the game's state stack (CGame's Pop, Push, Switch and
 * PopTo queue them; CGame::Tick in game_main.cpp carries them out), kept in a pool (the file name
 * SOCOM 1's: its static initialiser builds the pool, m_pool, and registers ~CPool, as SOCOM II's
 * 0x401BD0 does -- after game_main.cpp's, which only sets OurGameName). OVL1 0x2CF1D0-0x2D032F,
 * PopTo to the pool's destructor (SOCOM 1's order; the engine's start and stop follow,
 * game_engine.cpp's), before graph_tree.cpp; the deque's push_back (0x2CF970) is this unit's, in its
 * tail among the pool's vector pieces. Only some of it so far. Defined in reverse of their image
 * order (-inline deferred). */

#include "zgame.h"
#include "../zsys/zsys.h"

/* the pool (SOCOM 1's global m_pool, a CPool: its destructor's name) */
class CPool : public CGameStateChangeCmdVector {
};

CPool m_pool;

/* a command from the pool, the first not in use; a new one when none is free (OVL1 0x2CF700) */
CGameStateChangeCmd *CGameStateChangeCmd::Acquire(unsigned int cmd, CGameState *state,
                                                  unsigned int arg)
{
    CGameStateChangeCmd *found = 0;

    for (CGameStateChangeCmdVector::iterator i = m_pool.begin(); i != m_pool.end(); ++i) {
        found = &*i;
        if (!(*i).m_inuse) {
            (*i).m_cmd = cmd;
            (*i).m_state = state;
            (*i).m_arg = arg;
            (*i).m_inuse = true;
            break;
        }
        found = 0;
    }
    if (!found)
        found = new CGameStateChangeCmd(cmd, state, arg);
    return found;
}

/* a command given back: a pooled one cleared for reuse, a newed one deleted (OVL1 0x2CF690, as
 * SOCOM 1's) */
void CGameStateChangeCmd::Release(CGameStateChangeCmd *cmd)
{
    if (cmd) {
        if (cmd->m_pooled)
            cmd->Set(0, 0, 0, false);
        else
            delete cmd;
    }
}

/* the pool emptied and its storage freed (OVL1 0x2CF4C0): FreeVector on the base vector, its four
 * pieces called out of line (the temporary's constructor 0x2CF5C0, clear 0x2CF5B0, swap 0x2CF570,
 * the temporary's destructor 0x2CF510) */
void CGameStateChangeCmd::DestroyPool()
{
    FreeVector<CGameStateChangeCmdVector>(m_pool);
}

/* the top state replaced: the switch queued, the old state told it is going and the new one to get
 * ready; the old one given back (OVL1 0x2CF410, SOCOM 1's) */
CGameState *CGame::Switch(CGameState *state, unsigned int arg)
{
    if (!state)
        return 0;
    CGameState *cur = GetCurrentState();

    m_StateChangeQueue.push_back(CGameStateChangeCmd::Acquire(CGameStateChangeCmd::SWITCH, state, arg));
    if (cur)
        cur->PreUnInit();
    state->PreInit();
    return cur;
}

/* a state pushed on the stack: the push queued, the state told to get ready; the state that was on
 * top given back (OVL1 0x2CF380, SOCOM 1's) */
CGameState *CGame::Push(CGameState *state, unsigned int arg)
{
    if (!state)
        return 0;
    CGameState *cur = GetCurrentState();

    m_StateChangeQueue.push_back(CGameStateChangeCmd::Acquire(CGameStateChangeCmd::PUSH, state, arg));
    state->PreInit();
    return cur;
}

/* the top state popped: the pop queued, the state told it is going (OVL1 0x2CF2F0, SOCOM 1's
 * CGame::Pop. Pinned: PopTo calls it, where SOCOM 1's inlined it) */
#pragma auto_inline off
CGameState *CGame::Pop(unsigned int arg)
{
    CGameState *cur = GetCurrentState();

    m_StateChangeQueue.push_back(CGameStateChangeCmd::Acquire(CGameStateChangeCmd::POP, 0, arg));
    if (cur)
        cur->PreUnInit();
    return cur;
}
#pragma auto_inline reset

/* pops queued down to a state, the state given back -- none when it is already on top; with no
 * state, down to the bottom one (OVL1 0x2CF1D0, SOCOM 1's, its Pop now a call) */
CGameState *CGame::PopTo(CGameState *state, unsigned int arg)
{
    if (state) {
        /* i declared ahead of top: top takes s1, i s2, as the target's */
        unsigned int i;
        unsigned int top = m_CurrentState;

        i = top;
        for (;;) {
            if (m_Stack[i] == state)
                break;
            i--;
        }
        if (i < top) {
            for (; i < top; top--) {
                if (m_Stack[top] == state)
                    break;
                Pop(arg);
            }
            return m_Stack[top];
        }
        return 0;
    }
    for (unsigned int n = m_CurrentState; n; n--)
        Pop(arg);
    return m_Stack[0];
}
