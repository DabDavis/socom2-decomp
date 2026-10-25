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

int zrdr_findreal(const _zrdr *rdr, const char *tag, float *out, int n);
_zrdr *zrdr_findtag(const _zrdr *rdr, const char *tag);
extern "C" unsigned int func_003A59C0(const char *name);   /* a name's hash (not named yet) */

/* SOCOM 1's: a group's name (its record's first value, hashed into m_18 as a sound's), its decay
 * and its ENTRIES -- each a sound by name and its weight -- then the weights normalised */
void CSndGroup::LoadCSndGroup(_zrdr *rdr)
{
    if (!(!m_isDisabled && rdr))
        return;
    float decay = 0.0f;
    char *name = rdr->Item(0)->String();

    if (name) {
        m_name = name;
        m_18 = func_003A59C0(name);
    }
    zrdr_findreal(rdr, "DYNAMIC_WEIGHTS", &decay, 1);
    m_decay = decay;
    _zrdr *list = zrdr_findtag(rdr, "ENTRIES");

    for (unsigned int i = 0; (int)i < (int)list->Count(); i++) {
        _zrdr *item = list->Item(i);

        if (!m_isDisabled && item) {
            CSnd *sound = 0;
            float weight = 0.0f;
            char *sname = item->Item(0)->String();

            zrdr_findreal(item, "WEIGHT", &weight, 1);
            if (sname)
                sound = GetSoundByName(sname);
            if (sound) {
                m_sounds.push_back(sound);
                m_weights.push_back(weight);
            }
        }
    }
    NormalizeWeights();
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
