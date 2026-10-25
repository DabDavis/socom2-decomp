/* fts_objectivecam.cpp -- the mission's objectives: CMissionObjectiveCam and its list (SOCOM 1's
 * fts_objectivecam.cpp: an objective is a mission camera with map text, its completion, invalid
 * and display valves and a map position). OVL1 from the list's code to 0x2C763F, before
 * fts_missioncam.cpp's CMissionCam. Only some of it so far. Defined in reverse of their image
 * order (-inline deferred). */

#include <list>

/* a valve (only its value) */
class CValve {
public:
    char *m_name;   // 0x0
    unsigned int m_value : 16;   // 0x4
    unsigned int m_type : 8;   // 0x4
    unsigned int m_name_pooled : 1;   // 0x4
    unsigned int m_pooled : 1;   // 0x4
    unsigned int m_unused : 6;   // 0x4
};

class CZAnim;
class CSnd;

struct PNT2D {
    float x, y;
};

/* a mission camera (SOCOM 1's DWARF) */
class CMissionCam {
public:
    char *m_identifier;   // 0x0
    char *m_name;   // 0x4
    char *m_description[3];   // 0x8
    CZAnim *m_anim;   // 0x14
    CSnd *m_music;   // 0x18
};

/* an objective (SOCOM 1's DWARF; the line vector at 0x44 left as words) */
class CMissionObjectiveCam : public CMissionCam {
public:
    bool IsInvalid() const;
    bool IsDisplayOn() const;
    bool IsComplete() const;
    bool IsAvailable() const;

    char *m_maptext;   // 0x1c
    char *m_valvename;   // 0x20
    char *m_invalidvalvename;   // 0x24
    char *m_szDisplayValveName;   // 0x28
    CValve *m_valve;   // 0x2c
    CValve *m_invalidvalve;   // 0x30
    CValve *m_pDisplayValve;   // 0x34
    PNT2D m_pos;   // 0x38
    unsigned int m_isSecondary : 1;   // 0x40
    unsigned int m_isSubObjective : 1;   // 0x40
    unsigned int m_isCompass : 1;   // 0x40
    unsigned int m_bExtraction : 1;   // 0x40
    unsigned int m_unused : 28;   // 0x40
    unsigned int m_vecLines[3];   // 0x44
};

/* the objectives (SOCOM 1's: a pointer list) */
class CMissionObjectiveCamList : public std::list<CMissionObjectiveCam *> {
public:
    void ClearValves();
};

/* an objective shown on the compass: neither complete nor invalid, its display valve on (or
 * none), and a compass one (OVL1 0x2C7370).
 * OPEN, 0xC0 against 0xA0: the target carries each step's bool in v0, re-extended (andi 0xff)
 * between the tests, as an && chain materialised into a value; here the chain branches. Tried:
 * a bool local, the tests compared == false, the bit-field compared != 0. */
bool CMissionObjectiveCam::IsAvailable() const
{
    return !IsComplete() && !IsInvalid() && IsDisplayOn() && m_isCompass;
}

/* the completion valve set (OVL1 0x2C70D0) */
bool CMissionObjectiveCam::IsComplete() const
{
    return m_valve ? m_valve->m_value != 0 : false;
}

/* the display valve set, or none (OVL1 0x2C70A0) */
bool CMissionObjectiveCam::IsDisplayOn() const
{
    return m_pDisplayValve ? m_pDisplayValve->m_value != 0 : true;
}

/* the invalid valve set (OVL1 0x2C7070) */
bool CMissionObjectiveCam::IsInvalid() const
{
    return m_invalidvalve ? m_invalidvalve->m_value != 0 : false;
}

/* every objective's valves forgotten (OVL1 0x2C7000; the iterator assigned, so it lives in
 * memory, the end kept in a local, the objective read once) */
void CMissionObjectiveCamList::ClearValves()
{
    iterator i, e = end();

    for (i = begin(); i != e; ++i) {
        CMissionObjectiveCam *cam = *i;

        cam->m_valve = 0;
        cam->m_invalidvalve = 0;
        cam->m_pDisplayValve = 0;
    }
}
