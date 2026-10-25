/* zsnd_csndgroup.cpp -- CSndGroup, a weighted choice of sounds (OVL1 0x347BA0-0x34885F). The unit
 * name is SOCOM 1's (its asserts' file name). Defined in reverse of their image order (-inline
 * deferred). */

#include <stdlib.h>

#include "zsnd.h"
#include "../zanim/zanim.h"

/* SOCOM 1's: every group of the sounds file's GROUPS record made and added -- SOCOM II names the
 * list to read */
void CSndGroup::LoadSoundGroups(const char *name)
{
    int i;      /* declared first: the list takes the lower saved register */

    if (m_isDisabled)
        return;
    _zrdr *rdr = GetSoundRdrFile();

    if (!rdr)
        return;
    _zrdr *groups = func_0032F0D0(rdr, "GROUPS");

    if (!groups)
        return;
    _zrdr *list = func_0032F0D0(groups, name);

    if (!list)
        return;
    for (i = 0; i < list->Count(); i++) {
        CSndGroup *group = new CSndGroup;

        group->LoadCSndGroup(list->Item(i));
        AddNewCSnd(group);
    }
}

CSndInstance *CSndGroup::PlaySound(CSndInstance *inst, unsigned int flags)
{
    if (m_isDisabled)
        return 0;
    CSnd *sound = PickSound();

    if (sound)
        return sound->PlaySound(inst, flags);
    return 0;
}

CSndInstance *CSndGroup::PlaySoundVol(float vol, CSndInstance *inst, unsigned int flags)
{
    if (m_isDisabled)
        return 0;
    CSnd *sound = PickSound();

    if (sound)
        return sound->PlaySoundVol(vol, inst, flags);
    return 0;
}

CSndInstance *CSndGroup::PlaySound3D(const CPnt3D *pos, float vol, int pitch, CSndInstance *inst,
                                     unsigned int flags)
{
    if (m_isDisabled)
        return 0;
    CSnd *sound = PickSound();

    if (sound)
        return sound->PlaySound3D(pos, vol, pitch, inst, flags);
    return 0;
}

CSndInstance *CSndGroup::PlaySoundVolPanPitch(float vol, int pan, int pitch, CSndInstance *inst,
                                              unsigned int flags)
{
    if (m_isDisabled)
        return 0;
    CSnd *sound = PickSound();

    if (sound)
        return sound->PlaySoundVolPanPitch(vol, pan, pitch, inst, flags);
    return 0;
}

void CSndGroup::NormalizeWeights()
{
    if (m_isDisabled)
        return;
    float total = 0.0f;

    for (int i = 0; i < m_weights.size(); i++)
        total += m_weights[i];
    if (total == 0.0f) {
        float w = 1.0f / m_weights.size();

        for (int i = 0; i < m_weights.size(); i++)
            m_weights[i] = w;
    } else {
        float scale = 1.0f / total;

        for (int i = 0; i < m_weights.size(); i++)
            m_weights[i] *= scale;
    }
}

CSndGroup::~CSndGroup()
{
}
