/* anim_main.cpp -- CZAnimMain, the one ZAnim interpreter: its sequences, command stream and
 * loaded ZARs (OVL1 0x2678F0-...). The unit name is SOCOM 1's (the press beta's asserts name it
 * for CmdValid and its neighbours). Defined in reverse of their image order (-inline deferred). */

#include <stdio.h>
#include <string.h>

#include "zanim.h"

extern "C" void func_00258CE0(CZAnimZAR *zar);
extern "C" char *func_001801E0(const char *s, const char *file, int line);
extern "C" const char *func_0026C8E0(CZAnimSet *set, int index);

extern char *_local_wildcard_str;
extern int data_00414D78;
extern int data_00414DB8;
extern "C" void func_0026C240(CZAnimSet *set, int x);

int CZAnimMain::Unload()
{
    m_name_table.Unload();
    if (m_animset_list) {
        for (int i = 0; i < m_animset_count; i++)
            if (i >= 0 && i < m_animset_count)
                m_animset_list[i].Unload();
        func_00180280(m_animset_list, 0, 0);
        m_animset_list = 0;
    }
    m_animset_count = 0;
    m_CurAnimSet = 0;
    if (m_cmdset_list) {
        for (int i = 0; i < m_cmdset_count; i++)
            UnloadCmdSet(i);
        func_00180280(m_cmdset_list, 0, 0);
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
                func_00180280(cmd->name, 0, 0);
            cmd->name = 0;
        }
        func_00180280(set->cmd_list, 0, 0);
        set->cmd_list = 0;
        set->count = 0;
    }
    if (set->name)
        func_00180280(set->name, 0, 0);
    set->name = 0;
    return 1;
}


/* a command by name in a set, made when asked; not made, it is looked for in the other sets.
 * OPEN, 8 instructions: the second search's set, command and index registers permuted (s2/s3/s0
 * against the target's s3/s2/s0); reusing s and cmd, or hoisting its locals, is further off. */
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
        cmd->name = func_001801E0(name, 0, 0);
        cmd->id.b.set = set;
        cmd->id.b.index = s->count;
        return s->count++;
    }
    for (int j = 0; j < m_cmdset_count; j++) {
        if (j != set) {
            _zanim_cmd_set *o = &m_cmdset_list[j];
            zanim_cmd *c = o->cmd_list;

            for (int i = 0; i < o->count; i++, c++)
                if (!strcmp(name, c->name))
                    return i;
        }
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
            m_cmdset_list[m_cmdset_count].name = func_001801E0(name, 0, 0);
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
        set_name = func_001801E0(name, 0, 0);
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
        func_00180280(set_name, 0, 0);
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

void CZAnimMain::Close()
{
    Unload();
    if (_local_wildcard_str) {
        func_00180280(_local_wildcard_str, 0, 0);
        _local_wildcard_str = 0;
    }
    data_00414D78 = 0;
    data_00414DB8 = 0;
    FreeNetworkPacketBuffer();
    m_socom2_12 = 0;
    m_socom2_13 = 0;
}

/* the inverse exponent table, by a 0..1 value in 40 steps */
extern "C" float func_0026B0D0(CZAnimMain *main, float x)
{
    int i = (int)(40.0f * x);

    if (i > 40)
        i = 40;
    else if (i < 0)
        i = 0;
    return main->m_inv_exp[i];
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

/* OPEN, 8 bytes long: the target enters the loop through a jump to its test and leaves with a
 * break, where `for`/`while` with a break (SequenceExists' shape, which matches) duplicate the
 * test at the top, and `!found` in the condition (this, the closest) tests found each pass. */
bool CZAnimMain::SequenceRunning(const char *name)
{
    bool found = false;
    _zsequence *save = m_CurSeq;

    if (!name)
        return false;
    for (SeqFirst(); !found && SeqValid(); SeqNext()) {
        if (!strcmp(m_CurAnim->GetName(m_CurSeq->name_index), name) && StateRunning(m_CurSeq->state))
            found = true;
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
extern "C" int func_0026ADA0(CZAnimMain *main, int x)
{
    for (int i = 0; i < main->m_animset_count; i++)
        func_0026C240(&main->m_animset_list[i], x);
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
        func_00180280(m_NetworkPacket, 0, 0);
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
 * a new one at the end. OPEN, 6 instructions short: the target walks the available sets with a
 * pointer and indexes the found one afresh (SOCOM 1's has the same shape, NewAnimSet called). */
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
                m_animset_list[index].m_IsAvailable = 0;
                goto named;
            }
        }
        index = m_animset_count;
        NewAnimSet(index);
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
            copy = func_001801E0(name, 0, 0);
            char *sep = strstr(copy, "::");

            *sep = 0;
            anim_name = sep + 2;
        } else
            anim_name = name;
        if (copy) {
            set = GetAnimSetIndex(copy, true);
            func_00180280(copy, 0, 0);
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

/* a direction in a node's frame turned into the world's, normalised */
extern "C" void func_00269170(CPnt3D *dir, zdb::CNode *node)
{
    CPnt3D origin(0.0f, 0.0f, 0.0f);
    CMatrix m;

    node->BuildMTW(m);
    func_00308640(&m, dir, 1);
    func_00308640(&m, &origin, 1);
    *dir = *dir - origin;
    dir->Normalize();
}

int CZAnimMain::UnloadSIScript(_zanim_si_script *script)
{
    if (script->script_data)
        func_00180280(script->script_data, 0, 0);
    script->script_data = 0;
    script->script_data_size = 0;
    script->total_frame_count = 0;
    return 1;
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

/* zar::CZAR::Open with a fifth argument SOCOM 1's did not have: not named */
extern "C" int func_00258920(zar::CZAR *zar, const char *name, int mode, unsigned int flags, unsigned int a, int b);

/* a ZAR by file or directory name (the directory's zanim.zar) */
CZAnimZAR *CZAnimMain::OpenZAR(const char *name)
{
    if (!m_LoadFromZAR)
        return 0;
    CZAnimZAR *zar = new CZAnimZAR;

    if (zar) {
        char path[128];

        if (!strstr(name, "zanim.zar"))
            sprintf(path, "%s/%s", name, "zanim.zar");
        else
            sprintf(path, "%s", name);
        if (func_00258920(zar, path, 0, 1, 16, 0))
            return zar;
        delete zar;
        return 0;
    }
    return 0;
}

int CZAnimMain::CloseZAR(CZAnimZAR *zar)
{
    func_00258CE0(zar);
    delete zar;
    return 1;
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

CZAnimZAR::~CZAnimZAR()
{
}
