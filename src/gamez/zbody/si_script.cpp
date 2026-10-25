/* si_script.cpp -- a motion script and its shared data (CZSIScript, CZSIScriptCommon; SOCOM 1's unit,
 * __sinit_si_script.cpp, CZSIScript's RTTI). OVL1 0x28A100-0x28B26C: ReadLIP, GetObject, the
 * common's destructor (FreeVector's four pieces after it), CZSIScript's destructor, Read,
 * CreateAndReadIt, its constructor, the common's rate, distance, callback-index and loop-time
 * accessors, its defaults and its constructor; the pod vector's resize and erase close it.
 * Defined in reverse of their image order (-inline deferred). */

#include <string.h>

#include "zbody.h"
#include "../zsys/zsys.h"

/* newlib's (include/string.h lacks it) */
extern "C" int strncasecmp(const char *a, const char *b, size_t n);

/* the pod vector's resize, OVL1's out-of-line copy (include/vector has no resize; MAIN's
 * 0x18B050 carries the name) */
extern "C" void func_0028B0E0(void *vec, unsigned int n, const unsigned int &x);
/* an object's position at a frame (si_object.cpp's, not named yet) */
extern "C" void func_00289A30(CZSIObject *obj, int frame, CPnt3D *pos);

int CZSIScript::m_version = 5;
int siNumBodyObjects = 32;
int numSiScript;
int numSiCommon;
/* the name Read shares a common by: a global another unit owns, never set in r0001 (bss) */
extern const char *data_00414B40;

CZSIScriptCommon::CZSIScriptCommon()
{
    m_pName = 0;
    m_nObjects = 0;
    SetNumFrames(0);
    m_4_hi = 63;
    m_nBodyObjects = siNumBodyObjects;
    m_pObjects = 0;
    SetRead(false);
    m_refCount = 0;
    m_e = -1;
    SetId(0);
    SetObjectsReady(false);
    m_10 = 0.0f;
    SetDefaults();
    unsigned int zero = 0;

    func_0028B0E0(&m_objects_for_body, siNumBodyObjects, zero);
    m_root = 0;
    numSiCommon++;
}

void CZSIScriptCommon::SetDefaults()
{
    m_14 = 1.0f;
    m_18 = -1.0f;
    m_1c = 1.0f;
    m_20 = -1.0f;
    m_24 = -1.0f;
    m_28 = -1.0f;
    m_2c = -1.0f;
    m_30 = -1.0f;
    m_34 = -1.0f;
    SetLooped(false);
    SetFlag7(false);
    m_4a_0 = false;
    m_vertical = false;
    m_4a_2 = false;
    m_4a_3 = false;
    m_4a_4 = false;
    m_4a_5 = false;
    m_4a_6 = false;
    m_4a_7 = false;
    m_4b_0 = false;
    m_4c = CPnt3D::zero;
    SetArchive(0);
    m_callback0_index = 0;
    m_callback1_index = 0;
    m_callback2_index = 0;
    m_callback3_index = 0;
}

float CZSIScriptCommon::TrueLoopTime()
{
    float t;

    if (m_looped) {
        t = 1.0f;
    } else if (m_nFrames > 1) {
        float n = (unsigned int)m_nFrames;

        t = (n - 1.0f) / n;
    } else {
        t = 1.0f;
    }
    return t;
}

void CZSIScriptCommon::SetCallbackIndex(int i, unsigned int index)
{
    if (index < 256) {
        unsigned char c = index;

        switch (i) {
        case 0:
            m_callback0_index = c;
            break;
        case 1:
            m_callback1_index = c;
            break;
        case 2:
            m_callback2_index = c;
            break;
        case 3:
            m_callback3_index = c;
            break;
        }
    }
}

int CZSIScriptCommon::GetCallbackIndex(int i)
{
    switch (i) {
    case 0:
        return m_callback0_index;
    case 1:
        return m_callback1_index;
    case 2:
        return m_callback2_index;
    case 3:
        return m_callback3_index;
    }
    return 0;
}

void CZSIScriptCommon::CalcDistance()
{
    CZSIObject *root = m_root;

    if (root) {
        CPnt3D p0;
        CPnt3D p1;
        CPnt3D d;

        func_00289A30(root, 0, &p0);
        func_00289A30(root, m_nFrames - 1, &p1);
        p1.Sub(&p0, &d);
        if (m_looped && m_nFrames > 1) {
            float n = (unsigned int)m_nFrames;

            d.Scale(n / (n - 1.0f), &d);
        }
        float len = sqrtf(d.x * d.x + d.z * d.z);

        if (m_vertical)
            m_18 = d.y;
        else
            m_18 = len;
    } else {
        m_18 = CPnt3D::zero.x || CPnt3D::zero.y || CPnt3D::zero.z;
    }
}

float CZSIScriptCommon::DistanceScale()
{
    if (m_looped)
        return m_49_7 ? 1.0f : 100.0f;
    return 1.0f;
}

void CZSIScriptCommon::CalcRate()
{
    float r = 1.0f;

    if (!m_looped) {
        m_14 = 1.0f;
        return;
    }
    if (!(m_18 <= 0.2f)) {
        if (m_49_7)
            r = m_10 / m_18;
        else
            r = 100.0f * m_10 / m_18;
    }
    m_14 = r;
}

CZSIScript::CZSIScript()
{
    m_pCommon = 0;
    numSiScript++;
}

/* the common data of a name: the archive's, or made and filed there; one more script shares it */
inline void CZSIScript::Share(const char *const &name, CScriptArchive *archive)
{
    m_pCommon = archive->FindCommon(name);
    if (!m_pCommon) {
        m_pCommon = new CZSIScriptCommon;
        m_pCommon->m_pName = _strdup(name, 0, 0);
        CZSIScriptCommon *common = m_pCommon;

        archive->m_commons.push_back(common);
    }
    m_pCommon->m_refCount++;
}

static inline CScriptArchive *CurArchive() { return curArchive; }

int CZSIScript::CreateAndReadIt(zar::CZAR &zar, const char *name, void *data, int size, void *user)
{
    int n = 0;

    if (name) {
        CScriptArchive *archive = CurArchive();
        CZSIScript *script = new CZSIScript(name, archive);
        CZSIScript *kept = script;

        n = script->ReadLIP(zar, name, data, size, user);
        archive->m_scripts.push_back(kept);
    }
    return n;
}

bool CZSIScript::Read(zar::CZAR &zar, int size)
{
    float f;
    int version;
    int n;

    if (!m_pCommon)
        Share(data_00414B40, curArchive);
    zar.m_pFile->fread(&version, 4);
    if (version != m_version)
        return false;
    zar.m_pFile->fread(&m_pCommon->m_10, 4);
    zar.m_pFile->fread(&n, 4);
    m_pCommon->SetNumFrames(n);
    zar.m_pFile->fread(&n, 4);
    m_pCommon->m_nObjects = n;
    zar.m_pFile->fread(&f, 4);
    m_pCommon->m_18 = f;
    zar.m_pFile->fread(&f, 4);
    m_pCommon->m_14 = f;
    if (!m_pCommon->m_pObjects) {
        m_pCommon->m_pObjects = new CZSIObject[m_pCommon->m_nObjects];
        for (unsigned int i = 0; i < m_pCommon->m_nObjects; i++) {
            m_pCommon->m_pObjects[i].Initalize(m_pCommon);
            m_pCommon->m_pObjects[i].Read(this, zar);
        }
    }
    if (m_pCommon)
        m_pCommon->SetRead(true);
    return true;
}

CZSIScript::~CZSIScript()
{
    if (m_pCommon)
        m_pCommon = 0;
    numSiScript--;
}

CZSIScriptCommon::~CZSIScriptCommon()
{
    if (m_pName)
        _free(m_pName, 0, 0);
    m_pName = 0;
    m_pObjects = 0;
    FreeVector(m_objects_for_body);
    SetObjectsReady(false);
    SetDefaults();
    numSiCommon--;
}

CZSIObject *CZSIScript::GetObject(const char *name, bool exact)
{
    int n = m_pCommon->m_nObjects;

    for (int i = 0; i < n; i++) {
        CZSIObject *o = &m_pCommon->m_pObjects[i];

        if (o) {
            bool match = exact ? !strcasecmp(o->m_pName, name) : !strncasecmp(o->m_pName, name, strlen(name));

            if (match) {
                o->m_inUse = true;
                return o;
            }
        }
    }
    return 0;
}

/* a LIP block's header (SOCOM 1's; the names ours) */
struct LIP_HEADER {
    int version;   // 0x0
    float time;   // 0x4
    int nFrames;   // 0x8
    int nObjects;   // 0xc
    int _10[2];
    int names;   // 0x18: the objects' names, from the block's start
    int tracks;   // 0x1c: their tracks
};

int CZSIScript::ReadLIP(zar::CZAR &zar, const char *name, void *data, int size, void *user)
{
    LIP_HEADER *h = (LIP_HEADER *)data;
    unsigned int n;
    unsigned int i = 0;

    m_version = h->version;
    m_pCommon->m_10 = h->time;
    m_pCommon->SetNumFrames(h->nFrames);
    m_pCommon->m_nObjects = h->nObjects;
    n = m_pCommon->m_nObjects;

    m_pCommon->m_pObjects = (CZSIObject *)(h + 1);
    const char *names = (const char *)data + h->names;
    unsigned char *track = (unsigned char *)data + h->tracks;
    unsigned int nFrames = m_pCommon->m_nFrames;

    for (; i < n; i++) {
        m_pCommon->m_pObjects[i].Initalize(m_pCommon);
        int used = m_pCommon->m_pObjects[i].Read(track, nFrames, names);
        int pad;
        int len = strlen(names) + 1;
        pad = (len + 4) & 3;

        if (pad)
            pad = 4 - pad;
        names += len + pad;
        track += used;
    }
    return n;
}
