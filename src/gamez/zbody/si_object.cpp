/* si_object.cpp -- a motion script's object tracks (CZSIObject; the press beta's asserts name the
 * file, SOCOM 1's initialiser __sinit_si_object.cpp). OVL1 0x289380-0x28A0FC: Read, the rotation
 * and position readers (VU0, the 1.15 shorts converted with vitof15; not written yet), the
 * destructor, Read from an archive, Initalize, the constructor. Defined in reverse of their image
 * order (-inline deferred). */

#include "zbody.h"

int numSiObject;

/* OPEN, 0x28 bytes short: the target sets the four bit fields with word-wide read-modify-writes
 * (lw/sw at 8, the part id under a 64-bit mask 0xFFFFFFFF00FFFFFF) -- SOCOM 1's constructor did
 * its two the same way -- where MW gives these lbu/sb and a byte store of the id, as it does (and
 * the target does) in Initalize and Read. Inline bool setters change nothing. */
CZSIObject::CZSIObject() : m_positionData(0), m_rotations(0)
{
    m_inUse = false;
    m_constPos = false;
    m_constRot = false;
    m_body_part_id = 0;
    m_pName = 0;
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

CZSIObject::~CZSIObject()
{
    numSiObject--;
}

/* OPEN, 13 instructions (right size): the target finishes the 0x20 bit's store before it tests
 * 0x10, where MW hoists both tests ahead of the first byte store (a register shuffle follows);
 * inline bool setters for the bits give the same */
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
