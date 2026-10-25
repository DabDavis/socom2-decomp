/* fts_objectivecam.cpp -- the mission's objectives: CMissionObjectiveCam and its list (SOCOM 1's
 * fts_objectivecam.cpp: an objective is a mission camera with map text, its completion, invalid
 * and display valves and a map position). OVL1 from the list's code to 0x2C763F, before
 * fts_missioncam.cpp's CMissionCam. Only some of it so far. Defined in reverse of their image
 * order (-inline deferred). */

#include <list>
#include <vector>

#include "../zsys/zsys.h"

void _free(void *p, const char *file, int line);
char *_strdup(const char *s, const char *file, int line);

/* a valve's type (zanim.h's; only the one used here) */
enum VALVE_TYPE {
    VALVE_TYPE_0 = 0,
    VALVE_TYPE_2 = 2
};

/* a valve (only its value) */
class CValve {
public:
    static CValve *Create(const char *name, VALVE_TYPE type);

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
    CMissionCam();
    ~CMissionCam();
    void Set(const char *identifier, const char *name, const char *desc0, const char *desc1,
             const char *desc2, const char *anim, const char *music);

    char *m_identifier;   // 0x0
    char *m_name;   // 0x4
    char *m_description[3];   // 0x8
    CZAnim *m_anim;   // 0x14
    CSnd *m_music;   // 0x18
};

/* an objective (SOCOM 1's DWARF; the line vector at 0x44 left as words) */
class CMissionObjectiveCam : public CMissionCam {
public:
    CMissionObjectiveCam();
    bool IsInvalid() const;
    bool IsDisplayOn() const;
    bool IsComplete() const;
    bool IsAvailable() const;
    void Set(const char *identifier, const char *name, const char *desc0, const char *desc1,
             const char *desc2, const char *anim, const char *music, const char *valvename,
             const char *invalidvalvename, const char *displayvalvename, const PNT2D *pos);
    void FreeText();

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
    unsigned int m_bit4 : 1;   // 0x40: SOCOM II's
    unsigned int m_unused : 27;   // 0x40
    std::vector<int> m_vecLines;   // 0x44
    int m_50;   // 0x50: SOCOM II's
};

/* the objectives (SOCOM 1's: a pointer list) */
class CMissionObjectiveCamList : public std::list<CMissionObjectiveCam *> {
public:
    void Reset();
    void ClearValves();
};

/* (OVL1 0x2C7530, SOCOM 1's: the base's constructor called again in the body makes a temporary,
 * built and destroyed, as both builds do) */
CMissionObjectiveCam::CMissionObjectiveCam()
    : m_szDisplayValveName(0), m_pDisplayValve(0), m_bExtraction(false), m_bit4(false), m_50(0)
{
    CMissionCam();
    m_valvename = 0;
    m_valve = 0;
    m_invalidvalvename = 0;
    m_invalidvalve = 0;
    m_maptext = 0;
    m_isSecondary = false;
    m_isSubObjective = false;
    m_isCompass = false;
    m_unused = 0;
}

/* an objective shown on the compass: neither complete nor invalid, its display valve on (or
 * none), and a compass one (OVL1 0x2C7370).
 * OPEN, 0xC0 against 0xA0: the target carries each step's bool in v0, re-extended (andi 0xff)
 * between the tests, as an && chain materialised into a value; here the chain branches. Tried:
 * a bool local, the tests compared == false, the bit-field compared != 0; round 8: the chain
 * accumulated in a bool step by step (0xB4), and nested inline bools (the first two tests, then
 * the display valve) -- all 0xC0. SOCOM 1's (0xAC) has the same shape with one more andi */
bool CMissionObjectiveCam::IsAvailable() const
{
    return !IsComplete() && !IsInvalid() && IsDisplayOn() && m_isCompass;
}

/* the objective's own strings freed and its lines given up (SOCOM II's, the name ours: SOCOM 1's
 * Set and destructor did this in place; here Set calls it, out of line -- pinned) */
#pragma auto_inline off
void CMissionObjectiveCam::FreeText()
{
    if (m_maptext)
        _free(m_maptext, 0, 0);
    m_maptext = 0;
    if (m_valvename)
        _free(m_valvename, 0, 0);
    m_valvename = 0;
    if (m_invalidvalvename)
        _free(m_invalidvalvename, 0, 0);
    m_invalidvalvename = 0;
    if (m_szDisplayValveName)
        _free(m_szDisplayValveName, 0, 0);
    m_szDisplayValveName = 0;
    FreeVector(m_vecLines);
}
#pragma auto_inline reset

/* the objective set up: the mission camera's part, then its three valves by name (OVL1 0x2C7100,
 * SOCOM 1's) */
void CMissionObjectiveCam::Set(const char *identifier, const char *name, const char *desc0,
                               const char *desc1, const char *desc2, const char *anim,
                               const char *music, const char *valvename,
                               const char *invalidvalvename, const char *displayvalvename,
                               const PNT2D *pos)
{
    CMissionCam::Set(identifier, name, desc0, desc1, desc2, anim, music);
    FreeText();
    if (valvename)
        m_valvename = _strdup(valvename, 0, 0);
    if (invalidvalvename)
        m_invalidvalvename = _strdup(invalidvalvename, 0, 0);
    if (displayvalvename)
        m_szDisplayValveName = _strdup(displayvalvename, 0, 0);
    if (m_valvename)
        m_valve = CValve::Create(m_valvename, VALVE_TYPE_2);
    if (m_invalidvalvename)
        m_invalidvalve = CValve::Create(m_invalidvalvename, VALVE_TYPE_2);
    if (m_szDisplayValveName)
        m_pDisplayValve = CValve::Create(m_szDisplayValveName, VALVE_TYPE_2);
    m_pos = *pos;
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

/* every objective's valves made from their names (OVL1 0x2C6F40, SOCOM 1's Reset: the same walk as
 * ClearValves) */
void CMissionObjectiveCamList::Reset()
{
    iterator i, e = end();

    for (i = begin(); i != e; ++i) {
        CMissionObjectiveCam *cam = *i;

        if (cam->m_valvename)
            cam->m_valve = CValve::Create(cam->m_valvename, VALVE_TYPE_2);
        if (cam->m_invalidvalvename)
            cam->m_invalidvalve = CValve::Create(cam->m_invalidvalvename, VALVE_TYPE_2);
        if (cam->m_szDisplayValveName)
            cam->m_pDisplayValve = CValve::Create(cam->m_szDisplayValveName, VALVE_TYPE_2);
    }
}
