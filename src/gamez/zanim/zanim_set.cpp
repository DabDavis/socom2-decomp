/* zanim_set.cpp -- CZAnimSet, a named set of anims, and CZAnimSetSave, the node states a set
 * restores (OVL1 0x26B430-...). The unit name is SOCOM 1's (the press beta's asserts name it for
 * GetAnimIndex). Defined in reverse of their image order (-inline deferred). */

#include <string.h>

#include "zanim.h"

/* the set's SoftImage scripts, each grown onto the list as its key opens */
int CZAnimSet::LoadSIScript(zar::CZAR &archive)
{
    int count;
    zar::CKey *key = archive.GetOpenKey();

    archive.Fetch("SoftImage_Script_Count", &count, 4);
    zar::CKey *list = key->FindKey("SoftImage_Script_List");

    if (list) {
        int i = 0;

        for (zar::CKeyRing::const_iterator it = list->begin(); it != list->end(); ++it) {
            if (archive.OpenKey(*it)) {
                m_si_script = (_zanim_si_script *)_realloc(m_si_script, (m_si_script_count + 1) * sizeof(_zanim_si_script), 0, 0);
                m_si_script_count++;
                _zanim_si_script *script = &m_si_script[i];

                archive.Fetch("Script_Params", script, sizeof(_zanim_si_script));
                if (script->script_data_size > 0) {
                    script->script_data = (char *)_malloc(script->script_data_size, 0, 0);
                    archive.Fetch("Script_Data", script->script_data, script->script_data_size);
                } else
                    script->script_data = 0;
                archive.CloseKey(*it);
                i++;
            }
        }
    }
    return 1;
}

/* room for count anims; SOCOM 1's NewAnim grew the list the same way, inline in Load too */
inline bool CZAnimSet::Reserve(int count)
{
    if (m_anim_max < count) {
        m_anim_max = count;
        m_anim_list = (CZAnim *)_realloc(m_anim_list, m_anim_max * sizeof(CZAnim), 0, 0);
        return m_anim_list != 0;
    }
    return true;
}

/* the next slot's anim, the list grown 25 at a time (SOCOM 1's, out of line there) */
inline CZAnim *CZAnimSet::NewAnim(const char *name)
{
    if (m_anim_count >= m_anim_max && !Reserve(m_anim_count + 25))
        return 0;
    CZAnim *anim = func_0026CAA0(this, &m_anim_list[m_anim_count], name);

    m_anim_count++;
    return anim;
}

/* the set's name table, its scripts and every anim in its ZAR key */
int CZAnimSet::Load(zar::CZAR &archive)
{
    int count;

    m_gravity = ZAnim.m_gravity;
    zar::CKey *key = archive.GetOpenKey();
    m_name_table.Load(archive);
    LoadSIScript(archive);
    archive.Fetch("Animation_List_Count", &count, 4);
    if (!Reserve(count))
        return 0;
    zar::CKey *list = key->FindKey("Animation_List");

    if (list) {
        for (zar::CKeyRing::const_iterator it = list->begin(); it != list->end(); ++it) {
            if (archive.OpenKey(*it)) {
                NewAnim(0)->Load(archive);
                archive.CloseKey(*it);
            }
        }
    }
    return 1;
}

int CZAnimSet::GarbageCollect()
{
    func_002684D0();
    if (m_anim_list)
        for (int i = 0; i < m_anim_count; i++)
            m_anim_list[i].GarbageCollect();
    return 1;
}

/* every anim stopped, and every anim it chains to */
int CZAnimSet::Stop()
{
    for (int i = 0; i < m_anim_count; i++) {
        CZAnim *anim = Anim(i);

        func_002707C0(anim, false);
        anim->StopLights(true);
        anim->DeleteParticleSources();
        while (anim->m_sibling_anim) {
            anim = anim->m_sibling_anim;
            func_002707C0(anim, false);
            anim->StopLights(true);
            anim->DeleteParticleSources();
        }
    }
    m_IsStarted = 0;
    return 1;
}

/* the set's anims started, unless it is a free slot or running: an anim is invalid when its IF
 * nesting went wrong (SOCOM II's), and a cloning anim makes its clones once, restarts them after */
int CZAnimSet::Start()
{
    int i;

    if (m_IsAvailable)
        return 0;
    if (m_IsStarted)
        return 0;
    m_IsStarted = 1;
    ZAnim.m_CurAnimSet = this;
    ZAnim.m_CurAnim = 0;
    ZAnim.m_CurSeq = 0;
    for (i = 0; i < m_anim_count; i++)
        for (CZAnim *anim = Anim(i); anim; anim = anim->m_sibling_anim)
            anim->m_invalid = anim->m_nest_error;
    for (i = 0; i < m_anim_count; i++) {
        CZAnim *anim = Anim(i);

        if (anim->m_clone_anim) {
            if (m_WasStarted)
                anim->RestartClones();
            else
                anim->MakeClones();
        } else {
            while (anim) {
                anim->Start();
                anim = anim->m_sibling_anim;
            }
        }
    }
    m_WasStarted = 1;
    return 1;
}

/* the saved node states put back, the CZAnimEx list and every anim reset */
void CZAnimSet::Reset()
{
    CZAnimSetSave *save = m_node_states;

    if (save)
        for (CZAnimSetSave::const_iterator it = save->begin(); it != save->end(); ++it)
            save->Reset(*it);
    if (m_AnimExList)
        m_AnimExList->Reset();
    for (int i = 0; i < m_anim_count; i++)
        Anim(i)->Reset();
}

extern "C" int func_0026C240(CZAnimSet *set, int x)
{
    for (int i = 0; i < set->m_anim_count; i++)
        func_00270570(set->Anim(i), x);
    return 1;
}

/* SOCOM II's: SOCOM 1's NewAnim without the slot's allocation -- an anim given its slot,
 * cleared, named and set going; the set itself is not read */
extern "C" CZAnim *func_0026CAA0(CZAnimSet *set, CZAnim *anim, const char *name)
{
    if (!anim)
        return 0;
    ZAnim.m_CurAnim = anim;
    ZAnim.m_CurSeq = 0;
    memset(anim, 0, sizeof(CZAnim));
    anim->m_root_node_index = 0;
    if (!ZAnim.m_search_external_nodes)
        anim->node_search_scope = 1;
    else
        anim->node_search_scope = 0;
    anim->m_animset_index = ZAnim.m_CurAnimSet ? ZAnim.m_CurAnimSet - ZAnim.m_animset_list : 0;
    anim->Name(name);
    anim->State(1);
    anim->Priority(3.0f);
    anim->m_damage_seq_offset = 0;
    anim->m_activation_seq_offset = 0;
    anim->m_execution_seq_offset = 0;
    anim->m_cleanup_seq_offset = 0;
    return anim;
}

int CZAnimSet::GetAnimIndex(CZAnim *anim)
{
    int i = anim - m_anim_list;

    if (i >= m_anim_count)
        i = -1;
    return i;
}

void CZAnimSet::Name(const char *name)
{
    if (m_name)
        func_00180280(m_name, 0, 0);
    m_name = func_001801E0(name, 0, 0);
}

CZAnim *CZAnimSet::Anim(int index)
{
    if (index >= 0 && index < m_anim_count)
        return &m_anim_list[index];
    return 0;
}

CZAnim *CZAnimSet::Anim(const char *name)
{
    if (!name)
        return 0;
    CZAnim *anim = m_anim_list;

    for (int i = 0; i < m_anim_count; i++, anim++)
        if (!strcasecmp(name, anim->GetName()))
            return anim;
    return 0;
}

const char *CZAnimSet::GetName(int index)
{
    return m_name_table.GetName(index);
}

int CZAnimSet::AddName(const char *name)
{
    return m_name_table.AddName(name);
}

void CZAnimSet::AddAnimEx(CZAnimEx *ex)
{
    if (!m_AnimExList)
        m_AnimExList = new CZAnimExVector;
    m_AnimExList->Add(ex);
}

_zanim_si_script *CZAnimSet::SIScript(int index)
{
    if (index >= 0 && index < m_si_script_count)
        return &m_si_script[index];
    return 0;
}

void CZAnimSet::Init()
{
    m_WasStarted = 0;
    m_IsStarted = 0;
    m_IsAvailable = 1;
    m_name = 0;
    m_si_script_count = 0;
    m_si_script = 0;
    m_AnimExList = 0;
    m_anim_count = 0;
    m_anim_max = 0;
    m_anim_list = 0;
    m_node_states = 0;
    m_task_scheduler = 0;
    m_gravity = ZAnim.m_gravity;
}
