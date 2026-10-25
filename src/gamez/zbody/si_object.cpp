/* si_object.cpp -- a motion script's object tracks (CZSIObject; the press beta's asserts name the
 * file, SOCOM 1's initialiser __sinit_si_object.cpp). OVL1 0x289380-0x28A0FC: Read, the rotation
 * and position readers (SOCOM 1's Fast and Raw pairs plus a one-frame pair, VU0 converting the
 * shorts -- vitof15 for the 1.15 rotations, vitof0 and 1/256 for the positions -- through three
 * 16-byte statics, GCC-style asm as libvu0 writes it), GetVelRaw, the destructor, Read from an
 * archive, Initalize, the constructor. Defined in reverse of their image order (-inline deferred). */

#include "zbody.h"
#include <libvu0.h>

int numSiObject;

/* a key's string read from the archive into a fresh copy (zar's, not named yet) */
extern "C" void func_00257EE0(zar::CZAR *zar, char **str);

/* a buffer freed and forgotten (SOCOM II passes no file or line) */
#define SAFE_FREE(p)            \
    if (p) {                    \
        _free((p), 0, 0);       \
        (p) = 0;                \
    }

/* the bit fields in the initialiser list: each a word-wide read-mask-write (lw/sw at 8, the part id
 * under a 64-bit mask 0xFFFFFFFF00FFFFFF, one (0 & 1) shared by the three bits); assigned in the
 * body, MW narrows them to lbu/sb and a byte store, 0x28 bytes short */
CZSIObject::CZSIObject()
    : m_positionData(0), m_rotations(0), m_inUse(false), m_constPos(false), m_constRot(false),
      m_body_part_id(0), m_pName(0)
{
    numSiObject++;
}

/* the track emptied for a script (SOCOM 1's, which left the use bit and the part id) */
bool CZSIObject::Initalize(CZSIScriptCommon *common)
{
    if (!common)
        return false;
    m_constPos = false;
    m_constRot = false;
    m_positionData = 0;
    m_rotations = 0;
    m_inUse = false;
    m_body_part_id = 0;
    return true;
}

/* the track read from an archive's key: its name, its flags word, then its positions and rotations
 * into fresh buffers, read only where the flags say the key holds them (SOCOM 1's) */
bool CZSIObject::Read(CZSIScript *script, zar::CZAR &zar)
{
    unsigned int flags;

    func_00257EE0(&zar, &m_pName);
    zar.m_pFile->fread(&flags, 4);
    m_constPos = (flags & 0x20) != 0;
    m_constRot = (flags & 0x10) != 0;
    int posSize = (m_constPos ? 1 : script->m_pCommon->m_nFrames + 1) * 3 * sizeof(short);
    int rotSize = (m_constRot ? 1 : script->m_pCommon->m_nFrames + 1) * 4 * sizeof(short);

    if (m_rotations)
        SAFE_FREE(m_rotations);
    if (m_positionData)
        SAFE_FREE(m_positionData);
    m_positionData = (short *)_malloc(posSize, 0, 0);
    m_rotations = (short *)_malloc(rotSize, 0, 0);
    if (flags & 8)
        zar.m_pFile->fread(m_positionData, posSize);
    if (flags & 4)
        zar.m_pFile->fread(m_rotations, rotSize);
    return true;
}

CZSIObject::~CZSIObject()
{
    numSiObject--;
}

/* the VU0 conversions' 16-byte staging (OVL1 bss 0x415F80, 0x415F90, 0x415FA0; the names ours) */
static sceVu0FVECTOR siFloatBuf;
static int siIntBuf[4] __attribute__((aligned(16)));
static int siIntBuf2[4] __attribute__((aligned(16)));

/* a rotation's four 1.15 shorts to floats (vitof15), straight into the quaternion */
static inline void RotToFloat(const short *r, CQuat *rot)
{
    int *ib = siIntBuf;

    ib[0] = r[0];
    ib[1] = r[1];
    ib[2] = r[2];
    ib[3] = r[3];
    asm __volatile__ (
        "lqc2 $vf4, 0(%0)\n"
        "vitof15.xyzw $vf5, $vf4\n"
        "sqc2 $vf5, 0(%1)\n"
        : : "r"(ib), "r"(rot) : "memory");
}

/* a position's three shorts in 1/256ths to floats (vitof0, scaled), copied out */
static inline void PosToFloat(const short *p, CPnt3D *pos)
{
    int *ib = siIntBuf;
    float *fb = siFloatBuf;

    ib[0] = p[0];
    ib[1] = p[1];
    ib[2] = p[2];
    asm __volatile__ (
        "lqc2 $vf4, 0(%0)\n"
        "mfc1 $8, %2\n"
        "qmtc2 $8, $vf3\n"
        "vitof0.xyz $vf5, $vf4\n"
        "vmulx.xyz $vf5, $vf5, $vf3x\n"
        "sqc2 $vf5, 0(%1)\n"
        : : "r"(ib), "r"(fb), "f"(0.00390625f) : "$8", "memory");
    pos->x = fb[0];
    pos->y = fb[1];
    pos->z = fb[2];
}

/* the track's velocity between two frames, in units a second: the positions' difference over the
 * script's frame time (SOCOM 1's took the CZSIScript); zero when the position is constant, the
 * pair moved back one when it runs off the last frame */
CPnt3D *CZSIObject::GetVelRaw(CZSIScriptCommon *script, int f0, int f1, CPnt3D *vel)
{
    if (m_constPos) {
        *vel = CPnt3D::zero;
        return vel;
    }
    if (f1 == script->m_nFrames) {
        if (f0 > 0)
            f0--;
        f1--;
    }
    PosToFloat(&m_positionData[f1 * 3], vel);
    CPnt3D p0;

    PosToFloat(&m_positionData[f0 * 3], &p0);
    vel->Sub(&p0, vel);
    float time = script->m_10;

    vel->Scale((float)(unsigned int)script->m_nFrames / time, vel);
    return vel;
}

/* a frame's rotation */
void CZSIObject::GetRot(int frame, CQuat *rot)
{
    if (m_constRot)
        frame = 0;
    RotToFloat(&m_rotations[frame * 4], rot);
}

/* a frame's position */
void CZSIObject::GetPos(int frame, CPnt3D *pos)
{
    if (m_constPos)
        PosToFloat(m_positionData, pos);
    else
        PosToFloat(&m_positionData[frame * 3], pos);
}

/* the position between two frames: both converted and interpolated by t in VU0 */
CPnt3D *CZSIObject::GetPosRaw(int f0, int f1, float t, CPnt3D *pos)
{
    if (m_constPos) {
        PosToFloat(m_positionData, pos);
    } else {
        short *p0 = &m_positionData[f0 * 3];
        short *p1 = &m_positionData[f1 * 3];
        int *ib0 = siIntBuf;
        int *ib1 = siIntBuf2;
        float *fb = siFloatBuf;

        ib0[0] = p0[0];
        ib0[1] = p0[1];
        ib0[2] = p0[2];
        ib1[0] = p1[0];
        ib1[1] = p1[1];
        ib1[2] = p1[2];
        asm __volatile__ (
            "lqc2 $vf4, 0(%0)\n"
            "lqc2 $vf5, 0(%1)\n"
            "mfc1 $8, %3\n"
            "qmtc2 $8, $vf3\n"
            "mfc1 $8, %4\n"
            "qmtc2 $8, $vf6\n"
            "vitof0.xyz $vf4, $vf4\n"
            "vitof0.xyz $vf5, $vf5\n"
            "vmulx.xyz $vf4, $vf4, $vf3x\n"
            "vmulx.xyz $vf5, $vf5, $vf3x\n"
            "vsub.xyz $vf8, $vf5, $vf4\n"
            "vmulx.xyz $vf8, $vf8, $vf6x\n"
            "vadd.xyz $vf8, $vf8, $vf4\n"
            "sqc2 $vf8, 0(%2)\n"
            : : "r"(ib0), "r"(ib1), "r"(fb), "f"(0.00390625f), "f"(t) : "$8", "memory");
        pos->x = fb[0];
        pos->y = fb[1];
        pos->z = fb[2];
    }
    return pos;
}

/* the nearer frame's position */
CPnt3D *CZSIObject::GetPosFast(int f0, int f1, float t, CPnt3D *pos)
{
    int i;

    if (m_constPos)
        i = 0;
    else if (t < 0.5f)
        i = f0 * 3;
    else
        i = f1 * 3;
    PosToFloat(&m_positionData[i], pos);
    return pos;
}

/* the rotation between two frames: the nearer one close to either, else the two slerped */
CQuat *CZSIObject::GetRotRaw(int f0, int f1, float t, CQuat *rot)
{
    if (t < 0.01f || t > 0.99f)
        return GetRotFast(f0, f1, t, rot);
    CQuat q0;
    CQuat q1;

    GetRot(f0, &q0);
    GetRot(f1, &q1);
    q0.Slerp(&q1, t, rot);
    return rot;
}

/* the nearer frame's rotation */
CQuat *CZSIObject::GetRotFast(int f0, int f1, float t, CQuat *rot)
{
    if (t < 0.5f)
        GetRot(f0, rot);
    else
        GetRot(f1, rot);
    return rot;
}

/* OPEN, 13 instructions (right size): the target finishes the 0x20 bit's store before it tests
 * 0x10, where MW hoists both tests ahead of the first byte store (a register shuffle follows: the
 * target's flags register is allocated later, t5 against t2); SOCOM 1's Read schedules exactly as
 * the target does. Tried: inline bool setters, the flags word read twice (reloaded after the store,
 * longer), `? true : false`, the locals' declaration order; unchanged at level 5. (Read from an
 * archive, the same two tests on a stack word, matches.) Round 4: it is the flags' register alone
 * -- the target colours the flags word last (t5, after the five masks, the old byte and the 0x20
 * test), which then pins the schedule; read before the name's store it does take t5, but the
 * store and both right-hand sides move ahead of the stores (25 off). Also tried: an inline taking
 * the flags by value, a const local, bool locals for both tests, a nested block, `== bit`, `!!`,
 * `> 0`, shifts -- all 13 */
int CZSIObject::Read(unsigned char *data, unsigned int nFrames, const char *name)
{
    int size = 0;
    unsigned int flags;
    int n;

    m_pName = (char *)name;
    flags = *(unsigned int *)data;
    m_constPos = (flags & 0x20) != 0;
    m_constRot = (flags & 0x10) != 0;
    m_inUse = false;
    m_body_part_id = 255;
    data += 4;
    size += 4;
    n = m_constPos ? 1 : nFrames + 1;
    m_positionData = (short *)data;
    data += n * 3 * sizeof(short);
    size += n * 3 * sizeof(short);
    if ((unsigned int)data & 3) {
        int pad = 4 - ((unsigned int)data & 3);

        data += pad;
        size += pad;
    }
    n = m_constRot ? 1 : nFrames + 1;
    m_rotations = (short *)data;
    size += n * 4 * sizeof(short);
    return size;
}
