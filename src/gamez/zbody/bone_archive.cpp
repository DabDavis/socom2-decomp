/* bone_archive.cpp -- an archive of motion scripts (CScriptArchive; SOCOM 1's unit, the press beta's
 * asserts name it). OVL1 0x2875D0-0x28891F: both archives' callbacks reset, motion.rdr's reader,
 * the SEAL tables' registration, FindCommon, motion.rdr's three holders, LoadAll, the destructor
 * and Init. SOCOM 1's Fetch is gone; the commons moved in from CZSIScriptCommon's static list.
 * Defined in reverse of their image order (-inline deferred). */

#include <string.h>

#include "zbody.h"
#include "../zrdr/zrdr.h"

/* the ZAR's walk over its keys with a callback (zar's, not named yet) */
extern "C" unsigned short func_0025A390(zar::CZAR *zar, int (*f)(zar::CZAR &, const char *, void *, int, void *),
                                        void *user);
/* the SEAL's anim tables: an archive's anim filed under its index, and every one of an index
 * taken out (OVL2, not named yet) */
extern "C" {
void func_005E5B10(int index, int id);
void func_005E5BF0(int index);
}


void CScriptArchive::Init()
{
    m_name = 0;
    m_buffer = 0;
    m_bufsize = 0;
    SetPresent(false);
    m_nScripts = 0;
    m_rdr = 0;
    SetIndex(0);
}

CScriptArchive::~CScriptArchive()
{
    SetName(0);
    m_scripts.clear();
    zBoneUnloadAll(this);
    m_callbacks.Clear();
}

bool CScriptArchive::LoadAll()
{
    m_nScripts = func_0025A390(&m_zar, CZSIScript::CreateAndReadIt, this);
    return m_nScripts > 0;
}

CRdrFile *CScriptArchive::ReadRdr(const char *name, const char *path)
{
    if (!m_rdr)
        m_rdr = (CRdrFile *)zrdr_read(name, path, 0);
    return m_rdr;
}

CRdrFile *CScriptArchive::SetRdr(CRdrFile *rdr)
{
    if (!m_rdr)
        m_rdr = rdr;
    return m_rdr;
}

void CScriptArchive::FreeRdr(CRdrFile *keep)
{
    if (m_rdr != keep)
        zrdr_free(m_rdr);
    m_rdr = 0;
}

CZSIScriptCommon *CScriptArchive::FindCommon(const char *name)
{
    for (std::vector<CZSIScriptCommon *>::iterator i = m_commons.begin(); i != m_commons.end(); ++i)
        if (!strcmp(name, (*i)->m_pName))
            return *i;
    return 0;
}

void CScriptArchive::Register()
{
    int index = m_index;

    for (std::vector<CZSIScriptCommon *>::iterator i = m_commons.begin(); i != m_commons.end(); ++i)
        func_005E5B10(index, (*i)->m_id);
}

void CScriptArchive::Unregister()
{
    func_005E5BF0(m_index);
    SetIndex(0);
}

/* each of motion.rdr's "animations" applied to the common of its anim_name, which is filed in
 * motion.rdr's order (none for a missing one): the archive, the loop flag and the id, the rates,
 * the zanim callbacks, the flags, the four times, the transition speeds, the reference point, the
 * root object, then the distance, the rate and the script index (SOCOM 1's CZSealBody::
 * ReadMotionRdr read the same tags into the body's anims) */
bool CScriptArchive::ReadMotionRdr()
{
    if (!m_rdr)
        return false;
    _zrdr *anims = zrdr_findtag(m_rdr, "animations");

    if (!anims)
        return false;
    int count = anims->Count();

    m_byRdr.reserve(count);
    for (int i = 0; i < count; i++) {
        _zrdr *anim = anims->Item(i);
        char name[64];

        memset(name, 0, 4);
        strcpy(name, zrdr_findstring(anim, "anim_name"));
        CZSIScriptCommon *common = FindCommon(name);

        if (!common) {
            m_byRdr.push_back(0);
            continue;
        }
        m_byRdr.push_back(common);
        common->SetArchive(m_index);
        float max_velocity = 0.0f;
        zrdr_findreal(anim, "max_velocity", &max_velocity, 1);
        bool looped = false;
        zrdr_findbool(anim, "looped", &looped);
        float playback = 0.0f;
        zrdr_findreal(anim, "playback", &playback, 1);
        float speedA = 0.0f;
        zrdr_findreal(anim, "transition_speed_A", &speedA, 1);
        float speedB = 0.0f;
        zrdr_findreal(anim, "transition_speed_B", &speedB, 1);

        if (looped)
            common->SetLooped(true);
        else
            common->SetLooped(false);
        common->SetId(i);
        if (max_velocity < 0.0f) {
            common->SetFlag7(true);
            max_velocity = 0.0f;
        } else {
            common->SetFlag7(false);
        }
        float scale;

        if (looped) {
            scale = 10.0f * max_velocity / 100.0f;
            if (scale == 0.0f)
                scale = 1.0f;
        } else {
            scale = 1.0f;
        }
        common->m_1c = scale;
        if (looped) {
            if (common->m_49_7)
                common->m_10 = playback;
            else
                common->m_10 = common->m_10 / playback;
        } else {
            common->m_10 = playback;
        }

        int ncb = 0;
        for (_zrdr *cb = zrdr_findtag(anim, "zanim_callback"); cb; cb = zrdr_nexttag(anim, "zanim_callback", cb)) {
            const char *cbname = zrdr_findstring(cb, "name");
            float t;

            zrdr_findreal(cb, "time", &t, 1);
            if (t > 1.0f)
                t /= common->LoopTime();
            int index = m_callbacks.AddCallback(cbname, t);

            if (index >= 0) {
                common->SetCallbackIndex(ncb, index);
                ncb++;
            }
        }

        if (zrdr_findtag(anim, "NoPitchtwist"))
            common->SetPitchtwist(false);
        else
            common->SetPitchtwist(true);
        if (zrdr_findtag(anim, "NoHeadlooks"))
            common->SetHeadlooks(false);
        else
            common->SetHeadlooks(true);
        if (zrdr_findtag(anim, "NoTurn"))
            common->SetTurn(false);
        else
            common->SetTurn(true);
        if (zrdr_findtag(anim, "OverridePlayerControl"))
            common->SetOverrideControl(true);
        else
            common->SetOverrideControl(false);
        if (zrdr_findtag(anim, "VerticalMotion"))
            common->SetVertical(true);
        else
            common->SetVertical(false);
        if (zrdr_findtag(anim, "Lateral"))
            common->SetLateral(true);
        else
            common->SetLateral(false);
        if (zrdr_findtag(anim, "BlendOverlay"))
            common->SetBlendOverlay(true);
        else
            common->SetBlendOverlay(false);
        if (zrdr_findtag(anim, "UseVelY"))
            common->SetUseVelY(true);
        else
            common->SetUseVelY(false);

        _zrdr *tag = zrdr_findtag(anim, "NoFire");

        if (tag) {
            if ((int)tag->Count() > 0)
                common->m_2c = tag->Item(0)->Real();
            else
                common->m_2c = 1.0f;
        } else {
            common->m_2c = 0.0f;
        }
        tag = zrdr_findtag(anim, "NoInterrupt");
        if (tag) {
            if ((int)tag->Count() > 0)
                common->m_30 = tag->Item(0)->Real();
            else
                common->m_30 = 1.0f;
        } else {
            common->m_30 = 0.0f;
        }
        common->m_20 = 0.4f;
        tag = zrdr_findtag(anim, "BlendTime");
        if (tag && (int)tag->Count() > 0)
            common->m_20 = tag->Item(0)->Real();
        common->m_34 = 1.0f;
        tag = zrdr_findtag(anim, "DropWeapon");
        if (tag && (int)tag->Count() > 0)
            common->m_34 = tag->Item(0)->Real();
        tag = zrdr_findtag(anim, "Normalize");
        if (tag) {
            int n = tag->Count();

            if (n > 0) {
                float a = tag->Item(0)->Real();
                float b;

                if (n > 1)
                    b = tag->Item(1)->Real();
                else
                    b = 1.0f;
                common->m_24 = a;
                common->m_28 = b;
            } else {
                common->m_24 = 0.0f;
                common->m_28 = 0.0f;
            }
        } else {
            common->m_24 = 1.0f;
            common->m_28 = 1.0f;
        }

        if (speedA || speedB)
            m_transitions.push_back(new MOTION_TRANSITION(i, speedA, speedB));

        common->m_4c = CPnt3D::zero;
        common->SetRefPtValid(false);
        CPnt3D refpt = CPnt3D::zero;

        if (zrdr_findPNT3D(anim, "refPt", &refpt)) {
            if (refpt.x || refpt.y || refpt.z) {
                common->m_4c = refpt;
                common->SetRefPtValid(true);
            }
        }

        common->m_root = 0;
        common->SetRootId(63);
        CZSIObject *obj = common->m_pObjects;

        for (unsigned int j = 0; j < common->m_nObjects; j++, obj++) {
            if (!strcmp(obj->m_pName, "skel_root")) {
                common->m_root = obj;
                common->SetRootId(func_0058CB30("skel_root"));
                break;
            }
        }
        common->CalcDistance();
        common->CalcRate();
        int k = 0;

        for (std::vector<CZSIScript *>::iterator it = m_scripts.begin(), e = m_scripts.end(); it != e; ++it, k++) {
            CZSIScript *script = *it;

            if (script && script->m_pCommon->m_pName && !strcmp(script->m_pCommon->m_pName, common->m_pName)) {
                common->m_e = k;
                break;
            }
        }
    }
    return true;
}

/* both archives' callbacks to be looked up again (CMission::Init, after zBoneLoad) */
void zBoneResetCallbacks()
{
    CScriptArchive *archive = zBoneArchive();

    if (archive)
        archive->m_callbacks.ResetAnims();
    archive = zBoneMapArchive();
    if (archive)
        archive->m_callbacks.ResetAnims();
}
