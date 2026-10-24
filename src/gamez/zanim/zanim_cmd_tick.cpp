/* zanim_cmd_tick.cpp -- the ZAnim commands' per-frame handlers (OVL1 0x25D230-...). A handler
 * gets its command record -- a _zanim_cmd_hdr and the command's own fields after it -- and the
 * frame time, and returns whether the interpreter may go on to the next command. The unit name
 * is SOCOM 1's (__sinit_zanim_cmd_tick.cpp; the press beta's asserts); the command records'
 * layouts are read off the handlers. Defined in reverse of their image order (-inline deferred). */

#include "zanim.h"

struct zanim_cmd_sequence {
    _zanim_cmd_hdr hdr;
    unsigned char name;             /* a sequence or call-anim index; 0 is the current anim */
};

/* end of a while loop: back to its test */
struct zanim_cmd_endwhile {
    _zanim_cmd_hdr hdr;
    unsigned char pad[2];
    unsigned short target;          /* the while command's offset */
};

/* a while loop: its expression follows the record */
struct zanim_cmd_while {
    _zanim_cmd_hdr hdr;
    unsigned char done;             /* bit 0: the loop has been left */
    unsigned char pad;
    unsigned short target;          /* past the matching endwhile */
};

struct zanim_cmd_weight {
    _zanim_cmd_hdr hdr;
    float weight;                   /* the chance of going on, 0 to 1 */
};


/* fade a node's opacity from one value to another over a time on the command's clock */
struct zanim_cmd_opacity {
    _zanim_cmd_hdr hdr;
    unsigned short node;
    unsigned short pad;
    float from;
    float to;
    float rate;                     /* per second */
    float time;
};

void zAnimObjectOpacityFromToBegin(_zanim_cmd_hdr *hdr)
{
    zanim_cmd_opacity *cmd = (zanim_cmd_opacity *)hdr;
    CNode *node = ZAnim.m_CurAnim->GetNode(cmd->node, false);

    if (node)
        node->m_Opacity = cmd->from;
}

bool zAnimObjectOpacityFromToTick(_zanim_cmd_hdr *hdr, float *dT)
{
    zanim_cmd_opacity *cmd = (zanim_cmd_opacity *)hdr;
    CNode *node = ZAnim.m_CurAnim->GetNode(cmd->node, false);
    bool done;

    if (!node)
        return true;
    float step = *dT;
    float t = ZAnim.m_CurSeq->cmd_timer;
    if (!(step + t <= cmd->time)) {
        done = true;
        step = cmd->time - t;
        *dT -= step;
    } else {
        *dT = 0.0f;
        done = false;
    }
    if (done)
        node->m_Opacity = cmd->to;
    else
        node->m_Opacity += cmd->rate * step;
    return done;
}

/* parent one node of the current anim to another (flag 1: keep its place in the world) */
struct zanim_cmd_child {
    _zanim_cmd_hdr hdr;
    unsigned short flags;
    unsigned char parent;
    unsigned char child;
};

bool zAnimObjectAddChildTick(_zanim_cmd_hdr *hdr, float *dT)
{
    zanim_cmd_child *cmd = (zanim_cmd_child *)hdr;
    CNode *parent = ZAnim.m_CurAnim->GetNode(cmd->parent, false);
    CNode *child = ZAnim.m_CurAnim->GetNode(cmd->child, false);

    if (parent && child) {
        CPnt3D pos(0.0f, 0.0f, 0.0f);

        if (cmd->flags & 1)
            child->TransformToWorld(pos);
        parent->AddChild(child);
        if (cmd->flags & 1) {
            CMatrix m;

            parent->BuildMTW(m);
            Vu0MatrixInverseWithUniformScale(&m, &m);
            func_00308640(&m, &pos, 1);
            child->SetPosition(&pos);
        }
    }
    return true;
}

bool zAnimObjectDeleteChildTick(_zanim_cmd_hdr *hdr, float *dT)
{
    zanim_cmd_child *cmd = (zanim_cmd_child *)hdr;
    CNode *parent = ZAnim.m_CurAnim->GetNode(cmd->parent, false);
    CNode *child = ZAnim.m_CurAnim->GetNode(cmd->child, false);

    if (parent && child)
        parent->DeleteChild(child);
    return true;
}

/* whether a node of the current anim is active (mode 1) or inactive (mode 2) */
struct zanim_cmd_nodetest {
    _zanim_cmd_hdr hdr;
    unsigned short mode;
    unsigned short node;
};

/* OPEN, 5 instructions: mode 2 turns the flag into a bool (sltu) before testing it, as SOCOM 1
 * did too; neither a cast, a bool local nor an inline accessor gives it. */
bool zAnimNodeActiveTick(_zanim_cmd_hdr *hdr, float *dT)
{
    zanim_cmd_nodetest *cmd = (zanim_cmd_nodetest *)hdr;
    CNode *node = ZAnim.m_CurAnim->GetNode(cmd->node, false);

    if (node) {
        if (cmd->mode == 1) {
            if (node->m_active)
                return true;
        } else if (cmd->mode == 2) {
            if (!node->m_active)
                return true;
        }
    }
    return false;
}

bool zAnimNodeRenderedTick(_zanim_cmd_hdr *hdr, float *dT)
{
    CNode *node = ZAnim.m_CurAnim->GetNode(((zanim_cmd_nodetest *)hdr)->mode, false);

    if (node && func_00315580(node, 2))
        return true;
    return false;
}

/* the current anim's health against a bound (flag 1: a range) */
struct zanim_cmd_health {
    _zanim_cmd_hdr hdr;
    unsigned int flags;
    float lo;
    float hi;
};

bool zAnimHealthTestTick(_zanim_cmd_hdr *hdr, float *dT)
{
    zanim_cmd_health *cmd = (zanim_cmd_health *)hdr;
    float health = ZAnim.m_CurAnim->GetHealth();

    if (cmd->flags & 1) {
        if (health >= cmd->lo && health <= cmd->hi)
            return true;
    } else if (health <= cmd->lo) {
        return true;
    }
    return false;
}

/* the level of detail against a level: ==, <, <=, >, >= by flag */
struct zanim_cmd_lod {
    _zanim_cmd_hdr hdr;
    unsigned char flags;
    unsigned char level;
};

/* OPEN, 8 bytes long: the target loads the level before narrowing the LOD (SOCOM 1 the same),
 * which neither order of the operands nor reading m_Lod per test reproduces. */
bool zAnimLODTestTick(_zanim_cmd_hdr *hdr, float *dT)
{
    zanim_cmd_lod *cmd = (zanim_cmd_lod *)hdr;
    LOD lod = ZAnim.m_Lod;

    if (cmd->flags & 1)
        return (unsigned char)lod == cmd->level;
    if (cmd->flags & 2)
        return (unsigned char)lod < cmd->level;
    if (cmd->flags & 4)
        return (unsigned char)lod <= cmd->level;
    if (cmd->flags & 8)
        return (unsigned char)lod > cmd->level;
    if (cmd->flags & 0x10)
        return (unsigned char)lod >= cmd->level;
    return false;
}

/* call an anim by its call-anim reference, unless it is still running from the last call */
struct zanim_cmd_callanim {
    _zanim_cmd_hdr hdr;
    unsigned short flags;           /* 0x200: wait for the call to finish */
    unsigned char local;
    unsigned char index;
};

bool zAnimCallAnimTick(_zanim_cmd_hdr *hdr, float *dT)
{
    zanim_cmd_callanim *cmd = (zanim_cmd_callanim *)hdr;
    CZAnim *anim = ZAnim.m_CurAnim;

    if (cmd->flags & 0x200) {
        _zanim_call_anim_ref *ref = anim->GetCallAnimRef(cmd->index, cmd->local);
        if (ref && ref->executing_anim && ref->executing_anim->Running())
            return false;
    }
    return true;
}

/* pause the current anim (index 0) or a called one */
bool zAnimPauseAnimTick(_zanim_cmd_hdr *hdr, float *dT)
{
    int index = ((zanim_cmd_sequence *)hdr)->name;
    CZAnim *anim = ZAnim.m_CurAnim;

    if (!index) {
        func_00270BE0(anim, 0);
    } else {
        _zanim_call_anim_ref *ref = anim->GetCallAnimRef(index);
        if (ref) {
            if (ref->executing_anim) {
                if (ref->executing_anim->Running())
                    func_00270BE0(ref->executing_anim, 0);
            } else if (ref->base_anim && ref->base_anim->Running()) {
                func_00270BE0(ref->base_anim, 0);
            }
        }
    }
    return true;
}

/* resume a called anim */
bool zAnimResumeAnimTick(_zanim_cmd_hdr *hdr, float *dT)
{
    _zanim_call_anim_ref *ref = ZAnim.m_CurAnim->GetCallAnimRef(((zanim_cmd_sequence *)hdr)->name);

    if (ref) {
        if (ref->executing_anim) {
            if (ref->executing_anim->Running())
                func_002709B0(ref->executing_anim, 0);
        } else if (ref->base_anim && ref->base_anim->Running()) {
            func_002709B0(ref->base_anim, 0);
        }
    }
    return true;
}

/* mark the current anim (index 0) or a called one invalid */
bool zAnimInvalidateAnimTick(_zanim_cmd_hdr *hdr, float *dT)
{
    int index = ((zanim_cmd_sequence *)hdr)->name;
    CZAnim *anim = ZAnim.m_CurAnim;

    if (!index) {
        anim->m_invalid = 1;
    } else {
        _zanim_call_anim_ref *ref = anim->GetCallAnimRef(index);
        if (ref && ref->base_anim)
            ref->base_anim->m_invalid = 1;
    }
    return true;
}

/* repeat the sequence: until a loop count (flag 1, -1 for ever) or a time (flag 2) is reached */
struct zanim_cmd_loop {
    _zanim_cmd_hdr hdr;
    unsigned int flags;
    union {
        int count;
        float time;
    };
};

bool zAnimLoopTick(_zanim_cmd_hdr *hdr, float *dT)
{
    zanim_cmd_loop *cmd = (zanim_cmd_loop *)hdr;

    ZAnim.m_CurSeq->loop_counter++;
    ZAnim.m_CurSeq->loop_timer += *dT + ZAnim.m_CurSeq->seq_timer;
    if (cmd->flags & 1) {
        if (cmd->count != -1 && cmd->count == ZAnim.m_CurSeq->loop_counter)
            return true;
    } else if (cmd->flags & 2) {
        if (!(cmd->time < 0.0f) && !(ZAnim.m_CurSeq->loop_timer < cmd->time))
            return true;
    }
    ZAnim.SeqSetState(3);
    return false;
}

/* wait: for a time on one of three clocks (flag 8: 4 the anim's, 2 the sequence's, else the
 * command's), and/or for a number of passes (flag 0x10); flag 0x20 draws the time from a
 * range when the wait begins */
struct zanim_cmd_wait {
    _zanim_cmd_hdr hdr;
    unsigned short flags;
    unsigned short count;
    float value[4];                 /* the time, then the pass limit (an int) or the range */
};

void zAnimWaitBegin(_zanim_cmd_hdr *hdr)
{
    zanim_cmd_wait *cmd = (zanim_cmd_wait *)hdr;

    if (cmd->flags & 0x10)
        cmd->count = 0;
    if (cmd->flags & 0x20) {
        int i = (cmd->flags & 0x10) ? 2 : 1;
        float r = rand() * 4.656613e-10f;
        cmd->value[0] = cmd->value[i] + cmd->value[i + 1] * r;
    }
}

bool zAnimWaitTick(_zanim_cmd_hdr *hdr, float *dT)
{
    zanim_cmd_wait *cmd = (zanim_cmd_wait *)hdr;
    bool done = true;

    if (cmd->flags & 8) {
        float t;
        if (cmd->flags & 4)
            t = ZAnim.m_CurAnim->m_timer;
        else if (cmd->flags & 2)
            t = ZAnim.m_CurSeq->seq_timer;
        else
            t = ZAnim.m_CurSeq->cmd_timer;
        if (!(t + *dT <= cmd->value[0])) {
            float left = cmd->value[0] - t;
            if (left < 0.0f)
                left = 0.0f;
            done = true;
            *dT -= left;
        } else {
            *dT = 0.0f;
            done = false;
        }
    }
    if (cmd->flags & 0x10) {
        cmd->count++;
        if (done) {
            if (cmd->count <= ((int *)cmd->value)[(cmd->flags & 8) != 0])
                done = false;
        }
    }
    return done;
}

/* if, elseif, else and endif carry their nesting level */
struct zanim_cmd_if {
    _zanim_cmd_hdr hdr;
    unsigned char level;
};

static inline int NestLevelOf(_zanim_cmd_hdr *hdr)
{
    return ((zanim_cmd_if *)hdr)->level;
}

/* an if: its expression follows the record; false skips to the matching else, elseif (whose
 * expression is tried in turn) or endif. OPEN, 8 bytes long: the target loads m_CurSeq once and
 * re-tests it for each of the three type comparisons; every spelling tried reloads it, or (with
 * the type in a local) shares the read and comes out 0x20 short. */
bool zAnimIfTick(_zanim_cmd_hdr *hdr, float *dT)
{
    bool found;

    func_002681C0(&ZAnim, 1);
    found = false;
    do {
        bool done = false;

        ZAnim.CmdNext(8);
        if (!CZAnimExpression::ProcExpr(dT)) {
            do {
                ZAnim.CmdNext();
                if (!ZAnim.CmdValid()) {
                    done = true;
                } else {
                    if (ZAnim.CmdHdr()->data_type == ZAnim.m_cmd_else.data_type ||
                        ZAnim.CmdHdr()->data_type == ZAnim.m_cmd_elseif.data_type ||
                        ZAnim.CmdHdr()->data_type == ZAnim.m_cmd_endif.data_type) {
                        int level = NestLevelOf(ZAnim.CmdHdr());
                        if (level == func_002681C0(&ZAnim, 0)) {
                            done = true;
                            if (ZAnim.CmdHdr()->data_type == ZAnim.m_cmd_endif.data_type)
                                func_002681C0(&ZAnim, -1);
                        }
                    }
                }
            } while (!done);
            if (ZAnim.CmdHdr()->data_type != ZAnim.m_cmd_elseif.data_type)
                found = true;
        } else {
            found = true;
        }
    } while (!found);
    return true;
}

/* past the matching endif (the end of a taken branch) */
bool zAnimGoToEndIf(_zanim_cmd_hdr *hdr, float *dT)
{
    bool done = false;

    do {
        ZAnim.CmdNext();
        if (!ZAnim.CmdValid()) {
            done = true;
        } else {
            int type = ZAnim.CmdHdr()->data_type;
            if (type == ZAnim.m_cmd_endif.data_type) {
                int level = NestLevelOf(ZAnim.CmdHdr());
                if (level == func_002681C0(&ZAnim, 0)) {
                    done = true;
                    func_002681C0(&ZAnim, -1);
                }
            }
        }
    } while (!done);
    return true;
}

/* out of a while loop: past its endwhile, closing the ifs on the way */
bool zAnimBreakTick(_zanim_cmd_hdr *hdr, float *dT)
{
    for (;;) {
        ZAnim.CmdNext();
        if (ZAnim.CmdValid()) {
            int type = ZAnim.CmdHdr()->data_type;
            if (type == ZAnim.m_cmd_endif.data_type) {
                int level = NestLevelOf(ZAnim.CmdHdr());
                if (level == func_002681C0(&ZAnim, 0))
                    func_002681C0(&ZAnim, -1);
            }
        }
        if (!ZAnim.CmdValid())
            break;
        int type = ZAnim.CmdHdr()->data_type;
        if (type == ZAnim.m_cmd_endwhile.data_type)
            break;
    }
    return true;
}

bool zAnimWhileTick(_zanim_cmd_hdr *hdr, float *dT)
{
    zanim_cmd_while *cmd = (zanim_cmd_while *)hdr;

    if (!(cmd->done & 1)) {
        ZAnim.CmdNext(sizeof(zanim_cmd_while));
        if (!CZAnimExpression::ProcExpr(dT))
            ZAnim.CmdSet(cmd->target);
    }
    return true;
}

bool zAnimEndWhileTick(_zanim_cmd_hdr *hdr, float *dT)
{
    ZAnim.CmdSet(((zanim_cmd_endwhile *)hdr)->target);
    return false;
}

bool zAnimRandomWeightTick(_zanim_cmd_hdr *hdr, float *dT)
{
    return rand() * 4.656613e-10f <= ((zanim_cmd_weight *)hdr)->weight;
}

bool zAnimCallSequenceTick(_zanim_cmd_hdr *hdr, float *dT)
{
    ZAnim.ActivateSequence(ZAnim.m_CurAnim->GetName(((zanim_cmd_sequence *)hdr)->name));
    return true;
}

bool zAnimStopSequenceTick(_zanim_cmd_hdr *hdr, float *dT)
{
    ZAnim.StopSequence(ZAnim.m_CurAnim->GetName(((zanim_cmd_sequence *)hdr)->name));
    return true;
}
