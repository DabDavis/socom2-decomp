/* game_state.cpp -- the game's states: CGame's tick, its state stack and the commands that change
 * it, kept in a pool (the file name SOCOM 1's: its static initialiser builds the pool, m_pool, and
 * registers ~CPool, as SOCOM II's 0x401BD0 does -- after game_main.cpp's, which only sets
 * OurGameName). OVL1 0x2CE930-0x2D032F, NetTickThreadEntry to the pool's destructor (SOCOM 1's
 * order; the engine's start and stop follow, game_engine.cpp's), before graph_tree.cpp. Only some
 * of it so far. Defined in reverse of their image order (-inline deferred). */

#include <vector>

class CGameState;

/* a change of state waiting in the game's queue: what to do, to which state, with what -- kept
 * in a pool (m_pool) and newed only when the pool runs dry */
class CGameStateChangeCmd {
public:
    /* the bools stored through parameters: MW computes `b & 1` for each bit even when the
     * argument is a constant (Release, the constructor, resize's default element) */
    CGameStateChangeCmd(unsigned int cmd = 0, CGameState *state = 0, unsigned int arg = 0,
                        bool pooled = false, bool inuse = false)
    {
        m_pooled = pooled;
        m_inuse = inuse;
        m_cmd = cmd;
        m_state = state;
        m_arg = arg;
    }
    /* user-declared (SOCOM 1 links an out-of-line copy): Release's delete keeps its null test,
     * which a trivial destructor lets MW fold into the test before it */
    ~CGameStateChangeCmd() {}
    void Set(unsigned int cmd, CGameState *state, unsigned int arg, bool inuse)
    {
        m_cmd = cmd;
        m_state = state;
        m_arg = arg;
        m_inuse = inuse;
    }

    static void DestroyPool();
    static void CreatePool(unsigned int n);
    static void Release(CGameStateChangeCmd *cmd);
    static CGameStateChangeCmd *Acquire(unsigned int cmd, CGameState *state, unsigned int arg);

    CGameState *m_state;   // 0x0
    unsigned int m_arg;   // 0x4
    unsigned char m_cmd;   // 0x8
    bool m_pooled : 1;   // 0x9, bit 0: one of the pool's
    bool m_inuse : 1;   // 0x9, bit 1: handed out
};   // 0xc bytes

typedef std::vector<CGameStateChangeCmd> CGameStateChangeCmdVector;

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
