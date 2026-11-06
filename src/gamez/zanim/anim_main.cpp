/* anim_main.cpp -- CZAnimMain, the one ZAnim interpreter: its sequences, command stream and
 * loaded ZARs (OVL1 0x2678F0-...). The unit name is SOCOM 1's (the press beta's asserts name it
 * for CmdValid and its neighbours). Defined in reverse of their image order (-inline deferred). */

#include <stdio.h>
#include <string.h>

#include "zanim.h"
#include "../zsys/zsys.h"

/* the interpreter (SOCOM 1's ZAnim, the unit's first static: its initialiser 0x3FFBE0 runs the inline
 * constructor on it) */
CZAnimMain ZAnim;

/* the wildcard search (SOCOM 1's statics): a pattern's stars, up to five, stand for digits that
 * count up together, the last star the lowest */
extern char *_input_str_ptr;
extern char *_digit_ptr[5];
extern int _digit_val[5];
extern int _digit_count;
extern char *_local_wildcard_str;
zdb::CNode *GetMenuNode();
extern "C" float expf(float x);

int CZAnimMain::Unload()
{
    m_name_table.Unload();
    if (m_animset_list) {
        for (int i = 0; i < m_animset_count; i++)
            if (i >= 0 && i < m_animset_count)
                m_animset_list[i].Unload();
        _free(m_animset_list, 0, 0);
        m_animset_list = 0;
    }
    m_animset_count = 0;
    m_CurAnimSet = 0;
    if (m_cmdset_list) {
        for (int i = 0; i < m_cmdset_count; i++)
            UnloadCmdSet(i);
        _free(m_cmdset_list, 0, 0);
        m_cmdset_list = 0;
    }
    m_cmdset_count = 0;
    m_CurAnim = 0;
    m_CurSeq = 0;
    return 1;
}

int CZAnimMain::UnloadCmdSet(int index)
{
    if (index < 0 || index >= m_cmdset_count)
        return 0;
    _zanim_cmd_set *set = &m_cmdset_list[index];

    if (set->cmd_list) {
        for (int i = 0; i < set->count; i++) {
            zanim_cmd *cmd = &set->cmd_list[i];

            if (cmd->name)
                _free(cmd->name, 0, 0);
            cmd->name = 0;
        }
        _free(set->cmd_list, 0, 0);
        set->cmd_list = 0;
        set->count = 0;
    }
    if (set->name)
        _free(set->name, 0, 0);
    set->name = 0;
    return 1;
}


/* a command by name in a set, made when asked; not made, it is looked for in the other sets.
 * The other sets' walk is the target's strength-reduced offset spelled out: a byte offset named
 * after the loop's guard and stepped by the set's size, the guard written as an if over a do-while
 * (the rotated loop MW makes of a for) -- so the offset is numbered after j and set in the
 * preheader, after the guard's delay slot; the command defined after the inner index (round 17's
 * replay: igreplay --want c=s0,@3268=s3 --movers 40,43, the offset right after j and the command
 * after the inner index). Written as a for, the named offset's zero lands before the guard (3 off);
 * the offset left to the optimiser, it is numbered after every named local and colours first (6 off:
 * the second search's command and the offset trade s3/s0) */
int CZAnimMain::GetCmd(int set, char *name, bool create)
{
    _zanim_cmd_set *s = &m_cmdset_list[set];
    zanim_cmd *cmd = s->cmd_list;

    for (int i = 0; i < s->count; i++, cmd++)
        if (cmd->name && !strcmp(name, cmd->name))
            return i;
    if (create) {
        s->cmd_list = (zanim_cmd *)_realloc(s->cmd_list, (s->count + 1) * 0x14, 0, 0);
        cmd = &s->cmd_list[s->count];
        memset(cmd, 0, 0x14);
        cmd->name = _strdup(name, 0, 0);
        cmd->id.b.set = set;
        cmd->id.b.index = s->count;
        return s->count++;
    }
    int j = 0;

    if (j < m_cmdset_count) {
        int off = 0;

        do {
            if (j != set) {
                _zanim_cmd_set *o = (_zanim_cmd_set *)((char *)m_cmdset_list + off);
                int i = 0;
                zanim_cmd *c = o->cmd_list;

                for (; i < o->count; i++, c++)
                    if (!strcmp(name, c->name))
                        return i;
            }
            j++;
            off += sizeof(_zanim_cmd_set);
        } while (j < m_cmdset_count);
    }
    return 0;
}

/* a command set by name (0: the unnamed one), made when asked -- its command 0 "Reserved" */
int CZAnimMain::GetCmdSet(char *name, bool create)
{
    for (int i = 0; i < m_cmdset_count; i++) {
        if (!name) {
            if (!m_cmdset_list[i].name)
                return i;
        } else if (m_cmdset_list[i].name && !strcmp(name, m_cmdset_list[i].name))
            return i;
    }
    int index = 0;

    if (create) {
        m_cmdset_list = (_zanim_cmd_set *)_realloc(m_cmdset_list, (m_cmdset_count + 1) * 0xC, 0, 0);
        memset(&m_cmdset_list[m_cmdset_count], 0, 0xC);
        if (name)
            m_cmdset_list[m_cmdset_count].name = _strdup(name, 0, 0);
        index = m_cmdset_count++;
        GetCmd(index, "Reserved", true);
    }
    return index;
}

/* register a command, `set::name` or just `name`; the id it gets is what records carry */
unsigned short CZAnimMain::AddCmd(const char *name, _zanim_cmd_hdr *(*parse)(_zrdr *),
                                  void (*begin)(_zanim_cmd_hdr *), bool (*tick)(_zanim_cmd_hdr *, float *),
                                  void (*end)(_zanim_cmd_hdr *))
{
    _zanim_cmd_id u;                /* (Cmd's lookup written out: its local takes the higher slot) */
    const char *cmd_name;
    _zanim_cmd_id id;
    int set;
    char *set_name;
    unsigned char index;

    cmd_name = name;
    set_name = 0;
    if (!name)
        cmd_name = 0;
    else if (strstr(name, "::")) {
        set_name = _strdup(name, 0, 0);
        char *sep = strstr(set_name, "::");

        *sep = 0;
        cmd_name = sep + 2;
    } else
        cmd_name = name;
    set = GetCmdSet(set_name, true);
    index = GetCmd(set, (char *)cmd_name, true);

    id.b.set = set;
    id.b.index = index;
    if (set_name)
        _free(set_name, 0, 0);
    u.id = id.id;
    zanim_cmd *cmd = &m_cmdset_list[u.b.set].cmd_list[u.b.index];

    cmd->begin = begin;
    cmd->tick = tick;
    cmd->end = end;
    return id.id;
}

/* run the current command: begin it on its first frame, tick it, end it when it says it is done */
bool CZAnimMain::CmdTick(float *dT)
{
    bool done = true;
    _zanim_cmd_hdr *hdr = CmdHdr();
    zanim_cmd *cmd = GetCmdEntry(hdr->data_type);

    if (m_CurSeq->cmd_state == 2) {
        m_CurSeq->cmd_timer = 0.0f;
        if (cmd->begin)
            cmd->begin(hdr);
        m_CurSeq->cmd_state = 4;
    }
    if (m_CurSeq->cmd_state == 4 && cmd->tick) {
        float t = *dT;

        done = cmd->tick(hdr, dT);
        if (m_CurSeq->state == 5)
            return false;
        m_CurSeq->cmd_timer += t - *dT;
    }
    if (done && cmd->end)
        cmd->end(hdr);
    return done;
}

/* the interpreter set up afresh (closed first when open): SOCOM II's version and gravity,
 * the exp tables (e^x - 1 over e^10 - 1, x from 0 to 10 by 0.25, and one less it), the core
 * commands and eight free anim sets (SOCOM 1's three).
 * IT RETURNS true (round 20; open since round 1 at 18 instructions, registers only): MW's exit
 * reads v0 even in a void function, so v0 stays live from the last call (the inlined NewAnimSet's
 * Name) to the end and nothing after it may take v0 -- the set count went to v1 and the eight
 * flags' 1 to a0. Returning the 1 the flags take ends v0's live range at the return, and the count,
 * its increment and the 1 take v0 as the target's do (the replay could not find it by renumbering:
 * the interference was with v0 itself). The only caller, fts_mission.cpp's, ignores it. */
bool CZAnimMain::Open()
{
    if (m_IsOpen)
        Close();
    m_version = 72;
    m_gravity = -98.0f;
    m_Lod = HIGH;
    m_search_external_nodes = 0;
    m_animset_count = 0;
    m_animset_list = 0;
    m_cmdset_count = 0;
    m_cmdset_list = 0;
    m_UserActionAnimIndex = 0;
    m_block_activation_callback = 0;
    float x = 0.0f;

    for (int i = 0; i < 41; i++) {
        m_inv_exp[i] = 1.0f - (expf(x) - 1.0f) / (expf(10.0f) - 1.0f);
        m_exp[i] = (expf(x) - 1.0f) / (expf(10.0f) - 1.0f);
        x += 0.25f;
    }
    InitCommands();
    NewAnimSet(7);
    for (int i = 0; i < 8; i++)
        m_animset_list[i].m_IsAvailable = 1;
    m_CurrentFrame = 0;
    m_RunningAnimCount = 0;
    m_IsOpen = 1;
    return true;
}

void CZAnimMain::AddPath(const char *path)
{
}

void CZAnimMain::Close()
{
    Unload();
    func_002684D0();
    FreeNetworkPacketBuffer();
    m_IsOpen = 0;
    m_IsStarted = 0;
}

/* the inverse exponent table, by a 0..1 value in 40 steps */
float CZAnimMain::InvExp(float x)
{
    int i = (int)(40.0f * x);

    if (i > 40)
        i = 40;
    else if (i < 0)
        i = 0;
    return m_inv_exp[i];
}

void CZAnimMain::ActivateSequence(const char *name)
{
    if (name) {
        _zsequence *save = m_CurSeq;

        for (SeqFirst(); SeqValid(); SeqNext()) {
            if (!strcmp(m_CurAnim->GetName(m_CurSeq->name_index), name) && !StateRunning(m_CurSeq->state)) {
                SeqSetState(2);
                m_anim_complete = 0;
                break;
            }
        }
        m_CurSeq = save;
    }
}

void CZAnimMain::StopSequence(const char *name)
{
    if (name) {
        _zsequence *save = m_CurSeq;

        for (SeqFirst(); SeqValid(); SeqNext()) {
            if (!strcmp(m_CurAnim->GetName(m_CurSeq->name_index), name)) {
                m_CurSeq->paused = false;
                if (StateRunning(m_CurSeq->state)) {
                    SeqSetState(5);
                    break;
                }
            }
        }
        m_CurSeq = save;
    }
}

void CZAnimMain::PauseSequence(const char *name)
{
    if (name) {
        _zsequence *save = m_CurSeq;

        for (SeqFirst(); SeqValid(); SeqNext()) {
            if (!strcmp(m_CurAnim->GetName(m_CurSeq->name_index), name) && StateRunning(m_CurSeq->state)) {
                m_CurSeq->paused = true;
                break;
            }
        }
        m_CurSeq = save;
    }
}

void CZAnimMain::ResumeSequence(const char *name)
{
    if (name) {
        _zsequence *save = m_CurSeq;

        for (SeqFirst(); SeqValid(); SeqNext()) {
            if (!strcmp(m_CurAnim->GetName(m_CurSeq->name_index), name)) {
                m_CurSeq->paused = false;
                break;
            }
        }
        m_CurSeq = save;
    }
}

/* whether a sequence of the name is running (SOCOM 1's). The loop tests `!found` and breaks too:
 * with the flag in its condition MW enters the loop at its test (no copy of it on top, as
 * SequenceExists' plain break gets), and the break threads the found path straight out while the
 * back edge, the flag known false there, skips the flag's test -- the target's shape (either alone
 * is 8 or 0x20 bytes long) */
bool CZAnimMain::SequenceRunning(const char *name)
{
    bool found = false;
    _zsequence *save = m_CurSeq;

    if (!name)
        return false;
    for (SeqFirst(); !found && SeqValid(); SeqNext()) {
        if (!strcmp(m_CurAnim->GetName(m_CurSeq->name_index), name) && StateRunning(m_CurSeq->state)) {
            found = true;
            break;
        }
    }
    m_CurSeq = save;
    return found;
}

bool CZAnimMain::SequenceExists(const char *name)
{
    bool found = false;
    _zsequence *save = m_CurSeq;

    if (!name)
        return false;
    for (SeqFirst(); SeqValid(); SeqNext()) {
        if (!strcmp(m_CurAnim->GetName(m_CurSeq->name_index), name)) {
            found = true;
            break;
        }
    }
    m_CurSeq = save;
    return found;
}

/* an anim's activation: each of its sequences that has a state starts (activation 1, on
 * activate) or waits (any other), between the anim's two state changes */
void CZAnimMain::ActivateBegin(CZAnim *anim)
{
    m_CurAnim = anim;
    m_CurSeq = 0;
    anim->State(2);
    m_CurAnim->m_timer = 0.0f;
    for (SeqFirst(); SeqValid(); SeqNext()) {
        if (m_CurSeq->state) {
            if (m_CurSeq->activation == 1)
                SeqSetState(2);
            else
                SeqSetState(1);
        }
    }
    anim->State(4);
}

/* one frame of the current sequence: started (2) or restarted (3) it runs from its first
 * command; running (4) it ticks commands while time is left (or a command is timeless) and
 * the anim runs -- stopped, SOCOM II goes on while an anim it called still runs */
int CZAnimMain::SeqTick(float dT)
{
    switch (m_CurSeq->state) {
    case 1:
        break;
    case 2:
        m_CurSeq->loop_timer = 0.0f;
        m_CurSeq->loop_counter = 0;
        m_CurSeq->m_IF_NestLevel = 0;
    case 3:
        m_CurSeq->seq_timer = 0.0f;
        SeqSetState(4);
        m_CurSeq->cmd_pc = (char *)(m_CurSeq + 1);
        m_CurSeq->cmd_state = 2;
        if (!CmdValid()) {
            m_CurSeq->state = 5;
            break;
        }
    case 4:
        if (!m_CurSeq->paused) {
            bool done = true;

            while (done && (dT > 0.0f || CmdHdr()->timeless)
                   && (m_CurAnim->m_state != 5 || m_CurAnim->Running()) && (bool)m_CurAnim->m_paused == false) {
                float t = dT;

                done = CmdTick(&dT);
                m_CurSeq->seq_timer += t - dT;
                if (!done)
                    break;
                CmdNext();
                if (!CmdValid()) {
                    m_CurSeq->state = 5;
                    break;
                }
            }
        }
        break;
    case 5:
        break;
    }
    return m_CurSeq->state;
}

/* the current anim's sequences, walked with m_CurSeq itself */
void CZAnimMain::SeqFirst()
{
    m_CurSeq = (_zsequence *)m_CurAnim->m_seq_data;
}

void CZAnimMain::SeqNext()
{
    m_CurSeq = (_zsequence *)((char *)m_CurSeq + m_CurSeq->seq_data_size);
}

bool CZAnimMain::SeqValid()
{
    return (char *)m_CurSeq < m_CurAnim->m_seq_data + m_CurAnim->m_seq_data_size;
}

void CZAnimMain::SeqSetState(int state)
{
    if (m_CurSeq) {
        m_CurSeq->state = state;
        switch (m_CurSeq->state) {
        case 1:
            m_CurSeq->paused = false;
            break;
        case 2:
            m_CurSeq->paused = false;
            break;
        case 3:
        case 4:
            break;
        case 5:
            m_CurSeq->paused = false;
            break;
        }
    }
}

CZAnimSet *CZAnimMain::GetAnimSet(int index)
{
    if (index < 0 || index >= m_animset_count)
        return 0;
    return &m_animset_list[index];
}

bool CZAnimMain::GetAnimIndex(CZAnim *anim, int *set, int *index)
{
    *set = anim->m_animset_index;
    *index = GetAnimSet(*set)->GetAnimIndex(anim);
    if (*index < 0)
        return false;
    return true;
}

/* (each anim set's function at 0x26C240, not named yet) */
extern "C" int func_0026ADA0(CZAnimMain *main, CNode *node)
{
    for (int i = 0; i < main->m_animset_count; i++)
        func_0026C240(&main->m_animset_list[i], node);
    return 1;
}

ZAnimNetworkPacket *CZAnimMain::GetNetworkPacket(int size)
{
    if (m_NetworkPacketMaxSize < size) {
        m_NetworkPacket = (ZAnimNetworkPacket *)_realloc(m_NetworkPacket, size, 0, 0);
        m_NetworkPacketMaxSize = size;
    }
    return m_NetworkPacket;
}

void CZAnimMain::FreeNetworkPacketBuffer()
{
    if (m_NetworkPacket) {
        _free(m_NetworkPacket, 0, 0);
        m_NetworkPacket = 0;
    }
    m_NetworkPacketMaxSize = 0;
}

/* (out of line in SOCOM 1; inline here: there is none in the image) */
inline int CZAnimMain::NewAnimSet(int index)
{
    while (m_animset_count <= index) {
        m_animset_list = (CZAnimSet *)_realloc(m_animset_list, (m_animset_count + 1) * 0x34, 0, 0);
        CZAnimSet *s = &m_animset_list[m_animset_count];

        memset(s, 0, 0x34);
        s->Init();
        s->Name("None");
        m_animset_count++;
    }
    return 1;
}

/* a set by name (0: an unnamed or "None" one), made when asked -- in a set left available, or
 * a new one at the end (SOCOM 1's has the same shape, NewAnimSet called).
 * OPEN, 6 instructions (0x244, the target's size since round 19; 0x22C before): the inlined
 * NewAnimSet's index is the count in its own register, copied to the result only after its loop --
 * the count taken into a local, handed over, then assigned to the index (`index = count` first
 * coalesces the two); the found set re-indexed through a pointer local. Left: after NewAnimSet the
 * list and the set's offset take each other's v0 / v1 (the found path's `&list[index]` through a
 * pointer local fixed the same swap there; a pointer local after the call, 0x23C; `(list +
 * index)->`, the same 7) -- build/scratch/core/v_gasi19*.py; round 21: `(index + list)->`, `index[list]`, the same with n -- 6
 * (MW canonicalises the add: v_gasi21.py); round 22, the replay: the list's load (v97) must be
 * numbered after the offset's five temporaries (v98-v102: `--move 97=102.5` gives the target) --
 * a byte-offset local, the index assigned in the subscript, the list or the store through inline
 * wrappers: 6 or 0x23C (build/scratch/core/v_gasi22.py). Round 27: the target's add is list + offset
 * with the offset generated first and the list's load scheduled first; a held offset (enum, const,
 * unsigned) or element pointer generates them so but hoists the first shift ahead of the load (0x23C,
 * the chain's first temporary in v0); an inline over the list, or the list held, 6
 * (build/scratch/core/v_gasi1-3.py) */
int CZAnimMain::GetAnimSetIndex(const char *name, bool create)
{
    for (int i = 0; i < m_animset_count; i++) {
        if (!name) {
            if (!m_animset_list[i].m_name || !strcasecmp(m_animset_list[i].m_name, "None"))
                return i;
        } else if (m_animset_list[i].m_name && !strcasecmp(name, m_animset_list[i].m_name))
            return i;
    }
    int index;

    if (create) {
        for (index = 0; index < m_animset_count; index++) {
            if (m_animset_list[index].m_IsAvailable) {
                CZAnimSet *s = &m_animset_list[index];

                s->m_IsAvailable = 0;
                goto named;
            }
        }
        int n = m_animset_count;

        NewAnimSet(n);
        index = n;
        m_animset_list[index].m_IsAvailable = 0;
    named:
        if (name)
            m_animset_list[index].Name(name);
    } else
        index = -1;
    return index;
}

/* an anim by name -- `set::name`, in the given set, or in the current one -- and failing that
 * in any other set */
CZAnim *CZAnimMain::GetAnim(const char *name, const char *set_name)
{
    int set;
    const char *anim_name = name;

    if (set_name)
        set = GetAnimSetIndex(set_name, true);
    else {
        char *copy = 0;

        if (!name)
            anim_name = 0;
        else if (strstr(name, "::")) {
            copy = _strdup(name, 0, 0);
            char *sep = strstr(copy, "::");

            *sep = 0;
            anim_name = sep + 2;
        } else
            anim_name = name;
        if (copy) {
            set = GetAnimSetIndex(copy, true);
            _free(copy, 0, 0);
        } else {
            set = m_CurAnimSet ? m_CurAnimSet - m_animset_list : 0;
        }
    }
    CZAnim *anim = AnimInSet(set, anim_name);

    if (!anim) {
        for (int i = 0; i < m_animset_count; i++) {
            if (i != set) {
                anim = AnimInSet(i, anim_name);
                if (anim)
                    return anim;
            }
        }
    }
    return anim;
}

CZAnim *CZAnimMain::GetAnim(int set, int anim)
{
    CZAnimSet *s = &m_animset_list[set];

    return s ? s->Anim(anim) : 0;
}

/* (a bool: out of line its `return true` is li, inlined -- SequenceRunning, PauseSequence --
 * it is assigned, daddiu) */
bool CZAnimMain::StateRunning(int state)
{
    switch (state) {
    case 2:
    case 4:
    case 3:
        return true;
    }
    return false;
}

bool CZAnimMain::SeqRunning()
{
    bool r;

    switch (m_CurSeq->state) {      /* (a bool assigned: `return true` would be li, not daddiu) */
    case 2:
    case 4:
    case 3:
        r = true;
        break;
    default:
        r = false;
        break;
    }
    return r;
}

/* a node by name in one search scope (GetNodeSearchScope's GLOBAL 0, LOCAL 1, MODEL 2, WORLD 3,
 * MENU 4): the world and its landmarks, the models, the menu, then the node universe */
zdb::CNode *CZAnimMain::GetDatabaseNode(const char *name, int scope)
{
    zdb::CWorld *world = zdb::CWorld::m_world;
    zdb::CNode *node = 0;
    bool world_scope = scope == 3;
    bool global = scope == 0;

    if (world && (world_scope || global)) {
        if (!strcmp(world->m_name, name))
            node = world;
        if (!node)
            node = world->FindChild(name, true);
        if (!node) {
            zdb::CNodeVector &landmarks = world->m_landmarks;

            for (zdb::CNodeVector::iterator it = landmarks.begin(); it != landmarks.end() && !node; it++) {
                if (!strcmp((*it)->m_name, name))
                    node = *it;
                else
                    node = (*it)->FindChild(name, true);
            }
        }
    }
    if (!node && world && (scope == 2 || global))
        node = zdb::CWorld::GetModel(name);
    if (!node && scope == 4) {
        if (GetMenuNode())
            node = GetMenuNode()->FindChild(name, true);
    }
    if (!node && scope == 0) {
        if (zdb::NodeUniverse)
            node = zdb::NodeUniverse->GetNode(name);
    }
    return node;
}

/* a direction in a node's frame turned into the world's, normalised */
extern "C" void func_00269170(CPnt3D *dir, zdb::CNode *node)
{
    CPnt3D origin(0.0f, 0.0f, 0.0f);
    CMatrix m;

    node->BuildMTW(m);
    m.Transform(dir, 1);
    m.Transform(&origin, 1);
    *dir = *dir - origin;
    dir->Normalize();
}

/* a pattern's stars found from its end (five at most), each standing for 0: the pattern with
 * the digits written in, or 0 without a star */
const char *ZAnimWildcardInit(char *str)
{
    int i;

    _digit_count = 0;
    if (!str)
        return 0;
    _input_str_ptr = str;
    for (i = 0; i < 5; i++) {
        _digit_ptr[i] = 0;
        _digit_val[i] = 0;
    }
    for (i = strlen(str) - 1; i >= 0; i--) {
        if (_input_str_ptr[i] == '*') {
            _digit_ptr[_digit_count] = &_input_str_ptr[i];
            _digit_count++;
            if (_digit_count == 5)
                i = -1;
        }
    }
    if (!_digit_count)
        return 0;
    for (i = _digit_count - 1; i >= 0; i--) {
        char digit[4];

        sprintf(digit, "%d", _digit_val[i]);
        *_digit_ptr[i] = digit[0];
    }
    return _input_str_ptr;
}

/* the next match: the digits counted up, the first star the lowest; 0 once all are 9 */
const char *ZAnimWildcardNext()
{
    bool done = false;
    int i;

    for (i = 0; i < _digit_count; i++) {
        done = true;
        if (_digit_val[i] < 9) {
            done = false;
            _digit_val[i]++;
            break;
        }
        _digit_val[i] = 0;
    }
    if (done)
        return 0;
    for (i = _digit_count - 1; i >= 0; i--) {
        char digit[4];

        sprintf(digit, "%d", _digit_val[i]);
        *_digit_ptr[i] = digit[0];
    }
    return _input_str_ptr;
}

/* a name with stars given the search's current digits, in a buffer of its own; the name as it
 * is without a search or a star.
 * Byte-exact (0x268520; open rounds 9-18, 10 off on the saved registers alone): the target's
 * name, with the buffer coalesced into it, numbered after len and the digit index ahead of len
 * (igreplay --want name=s4,n=s0 --movers name,n). The parameter cannot move and every plain copy
 * of it is propagated back into it -- a CAST copy (`char *name = (char *)str`) is a variable of its
 * own, numbered where it is declared, and with the index declared first both land (round 19) */
const char *ZAnimWildcardApply(const char *str)
{
    int n = 0;
    int len;
    char *name = (char *)str;
    bool found = false;
    int i;

    if (_digit_count <= 0)
        return name;
    len = strlen(name);

    for (i = len - 1; i >= 0; i--) {
        if (name[i] == '*') {
            found = true;
            break;
        }
    }
    if (found) {
        _local_wildcard_str = (char *)_realloc(_local_wildcard_str, len + 1, 0, 0);
        strcpy(_local_wildcard_str, name);
        char *s = _local_wildcard_str;

        for (i = len - 1; i >= 0; i--) {
            if (s[i] == '*') {
                char digit[4];

                sprintf(digit, "%d", _digit_val[n]);
                s[i] = digit[0];
                if (n < _digit_count)
                    n++;
            }
        }
        name = s;
    }
    return name;
}

extern "C" void func_002684D0()
{
    if (_local_wildcard_str) {
        _free(_local_wildcard_str, 0, 0);
        _local_wildcard_str = 0;
    }
    _input_str_ptr = 0;
    _digit_count = 0;
}

int CZAnimMain::UnloadSIScript(_zanim_si_script *script)
{
    if (script->script_data)
        _free(script->script_data, 0, 0);
    script->script_data = 0;
    script->script_data_size = 0;
    script->total_frame_count = 0;
    return 1;
}

/* a sequence file read into a set (SOCOM 1's Load(name, set), which read the rdr and filed its
 * sequences): SOCOM II's sequences come from the ZARs, and the entry point is left returning true --
 * CMission::ZAnimLoad still calls it with "data/common/zrdr/czanim.rdr" and "common", then the
 * mission's rdr and "mission", where SOCOM 1's called its Load, and CGameMenu::LoadFromDesign too */
bool CZAnimMain::Load(const char *name, const char *set)
{
    return true;
}

/* the current sequence's IF nesting, moved by delta: out of 0..255 it marks the anim instead */
extern "C" int func_002681C0(CZAnimMain *main, int delta)
{
    int level = main->m_CurSeq->m_IF_NestLevel + delta;

    if (level < 0)
        main->m_CurAnim->m_nest_error = 1;
    else if (level > 255)
        main->m_CurAnim->m_nest_error = 1;
    else
        main->m_CurSeq->m_IF_NestLevel += delta;
    return main->m_CurSeq->m_IF_NestLevel;
}





/* step past the current command (its record's size is in its header) */
void CZAnimMain::CmdNext()
{
    m_CurSeq->cmd_pc += ((_zanim_cmd_hdr *)m_CurSeq->cmd_pc)->data_size;
    m_CurSeq->cmd_state = 2;
}

/* is the current command still inside its sequence's block? */
bool CZAnimMain::CmdValid()
{
    return m_CurSeq->cmd_pc < (char *)m_CurSeq + m_CurSeq->seq_data_size;
}

