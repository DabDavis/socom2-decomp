/* anim_main.cpp -- CZAnimMain, the one ZAnim interpreter: its sequences, command stream and
 * loaded ZARs (OVL1 0x2678F0-...). The unit name is SOCOM 1's (the press beta's asserts name it
 * for CmdValid and its neighbours). Defined in reverse of their image order (-inline deferred). */

#include <string.h>

#include "zanim.h"

extern "C" void func_00258CE0(CZAnimZAR *zar);
extern "C" const char *func_0026C8E0(CZAnimSet *set, int index);

extern char *_local_wildcard_str;
extern int data_00414D78;
extern int data_00414DB8;
extern "C" void func_0026C240(CZAnimSet *set, int x);

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

const char *CZAnimNameTable::GetName(int index)
{
    if (index >= 0 && index < m_count)
        return m_name[index];
    return 0;
}

extern "C" void func_00258570(zar::CZAR *zar);

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

const char *CZAnimNameIndexTable::GetName(CZAnimSet *set, int index)
{
    if (index >= 0 && index < m_count)
        return func_0026C8E0(set, m_index[index]);
    return 0;
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
