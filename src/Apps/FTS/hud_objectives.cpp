/* hud_objectives.cpp -- the objectives list (OVL1 0x205C40-0x208254): SOCOM 1's unit (its DWARF names
 * it, Z:\dev\Apps\FTS\hud_objectives.cpp). IsPlayingAnimation, Draw, Tick, Init and the implicit
 * destructor so far, all byte-exact. Defined in reverse of their image order (-inline deferred). */

#include <list>

#include "../../gamez/ztwod/ztwod.h"
#include "hud_missioncams.h"

/* the HUD: only its mission cameras placed (SOCOM II's offset) */
class CHUD {
public:
    unsigned char _0[0x1a2dc];
    CZHudMissionCams m_missioncams;   // 0x1a2dc
};

extern CHUD *theHUD;

namespace zdb {
class CWorld {
public:
    static CTexHandle *GetTexHandle(const char *name);
};
}

/* the sound system's tick (its static PostTick, SOCOM 1's CSnd::PostTick(float, void *); no row
 * named yet) */
extern "C" void func_00341A60(float dT, void *data);

/* how fast the selected name pulses (data 0x3DC460: a variable, read with lwc1) */
static float s_pulserate = 1000.0f;

/* the mission: only its objectives (theMission, fts_state.h's CMission at 0x1854) */
class CMissionObjectiveCam;

class CMission {
public:
    unsigned char _0[0x1854];
    std::list<CMissionObjectiveCam *> m_objectives;   // 0x1854
};

extern CMission theMission;

/* the mission's objectives ticked (SOCOM 1's CMission::TickObjectives(float); no row named yet) */
extern "C" void func_002A9F30(CMission *mission, float dT);

/* MSL's vector<bool> as the list's code shows it: the implicit destructor deletes its words (one
 * level), and an element read calls test_bit, out of line in the unit's tail (0x207F60, as SOCOM
 * 1's reference does); its swap and insert there are not modelled here */
namespace std {
template <>
class vector<bool, allocator<bool> > {
public:
    ~vector() { if (data_) operator delete(data_); }
    bool operator[](unsigned int n) { return test_bit(n); }
    bool test_bit(unsigned int n);

    unsigned int capacity_;
    unsigned int size_;
    unsigned int *data_;
};

#pragma auto_inline off
bool vector<bool, allocator<bool> >::test_bit(unsigned int n)
{
    return (data_[n >> 5] & (1 << (n & 31))) != 0;
}
#pragma auto_inline reset
}

/* the list: SOCOM 1's members (SOCOM 1's DWARF names them) at SOCOM II's offsets, C2D 4 bytes
 * shorter and C2DString 0x98 bytes; of SOCOM 1's seven anchor strings one is left (the implicit
 * destructor destroys an array of one). The destructor is implicit: this unit, defining Draw and
 * Tick, emits it (first in the image, 0x205C40) */
class CZObjectiveList : public C2D {
public:
    virtual void Draw(zdb::CCamera *cam);
    virtual void Tick(float dT);
    void Init();
    void SetCurColor(int index);
    static bool IsPlayingAnimation();
    /* an objective's index, if it is one (inline in SOCOM II: no copy of its own). SOCOM 1's
     * line table puts the test and `return false` first: the true block follows the false one */
    bool ResolveIndex(int index, int *newindex) const
    {
        int size = m_objectivenames.size();

        if (index < 0 || index >= size)
            return false;
        *newindex = index;
        return true;
    }
    /* whether an index is an objective's (SOCOM 1's GetObjective tests it so; the name ours) */
    bool IsValidIndex(int index) const
    {
        int size = m_objectivenames.size();

        if (index < 0 || index >= size)
            return false;
        return true;
    }
    /* an objective's camera: the mission's list walked to it (inline in SOCOM II) */
    CMissionObjectiveCam *GetObjective(int index) const
    {
        if (!IsValidIndex(index))
            return 0;
        std::list<CMissionObjectiveCam *>::iterator it = theMission.m_objectives.begin();

        for (int i = 0; i < index; i++)
            ++it;
        return *it;
    }
    CMissionObjectiveCam *GetCurrentObjective() const;
    CMissionObjectiveCam *GetCurrentObjectiveRetainer() const;
    /* the selection's camera held and the objectives ticked (inline; the name ours) */
    void UpdateSelection()
    {
        m_cur_selected_objective = GetCurrentObjective();
        func_002A9F30(&theMission, 1000.0f);
    }
    void SelectPreviousObjective();
    void SelectNextObjective();

    std::vector<C2DString *> m_objectivenames;   // 0x18
    std::vector<C2DBitmap *> m_objectivecompletedbitmap;   // 0x24
    std::vector<bool> m_vecSelectable;   // 0x30
    C2DString m_heading;   // 0x3c
    C2DString m_strMapHeading;   // 0xd4
    C2DBitmap m_textbackground;   // 0x170
    C2DBitmap m_mapbackground;   // 0x1f0
    C2DBitmap m_backgrounddivider;   // 0x270
    C2DString m_strAnalogL;   // 0x2f0
    C2DString m_strAnalogR;   // 0x388
    C2DString m_strCircleLabel;   // 0x420
    C2DString m_strSquareLabel;   // 0x4b8
    C2DString m_strTriLabel;   // 0x550
    C2DString m_strXLabel;   // 0x5e8
    C2DString m_arrAnchorStrings[1];   // 0x680
    int m_selected_objective;   // 0x718
    CMissionObjectiveCam *m_cur_selected_objective;   // 0x71c
    zdb::CTexHandle *m_backgroundtex;   // 0x720
    zdb::CTexHandle *m_checkboxtex;   // 0x724
    zdb::CTexHandle *m_checktex;   // 0x728
    unsigned char m_increment;   // 0x72c
};

/* the list's three textures looked up by name */
void CZObjectiveList::Init()
{
    m_checkboxtex = zdb::CWorld::GetTexHandle("hud_checkbox.tif");
    m_checktex = zdb::CWorld::GetTexHandle("hud_check.tif");
    m_backgroundtex = zdb::CWorld::GetTexHandle("team_background.tif");
}

/* unless a mission camera plays: the sound ticked, and the selected objective's name pulsed
 * between black and yellow (SOCOM 1's red and its rate of 600), the X label with it */
void CZObjectiveList::Tick(float dT)
{

    if (theHUD->m_missioncams.IsPlaying())
        return;
    func_00341A60(0.0f, 0);
    int newindex;

    if (ResolveIndex(m_selected_objective, &newindex)) {
        C2DString *curstring = m_objectivenames[newindex];
        float fCurLevel = curstring->m_color[1];

        if (m_increment) {
            fCurLevel += s_pulserate * dT;
            if (fCurLevel > 255.0f) {
                fCurLevel = 255.0f;
                m_increment = false;
            }
        } else {
            fCurLevel -= s_pulserate * dT;
            if (fCurLevel < 0.0f) {
                fCurLevel = 0.0f;
                m_increment = true;
            }
        }
        curstring->m_color[0] = fCurLevel;
        curstring->m_color[1] = fCurLevel;
        curstring->m_color[2] = 0.0f;
        m_strXLabel.SetTrans(0.5f * fCurLevel);
    }
}

/* the next selectable objective, round to the first (SOCOM 1's) */
void CZObjectiveList::SelectNextObjective()
{
    m_selected_objective++;
    UpdateSelection();
    if (m_selected_objective >= m_objectivenames.size()) {
        m_selected_objective = 0;
        UpdateSelection();
    }
    if (!m_vecSelectable[m_selected_objective])
        SelectNextObjective();
}

/* the previous selectable objective, round to the last (SOCOM 1's) */
void CZObjectiveList::SelectPreviousObjective()
{
    m_selected_objective--;
    UpdateSelection();
    if (m_selected_objective < 0) {
        m_selected_objective = m_objectivenames.size() - 1;
        UpdateSelection();
    }
    if (m_selected_objective < 0) {
        m_selected_objective = 0;
        UpdateSelection();
    }
    if (!m_vecSelectable[m_selected_objective])
        SelectPreviousObjective();
}

/* the selected objective's name given the colour of another's (SOCOM 1's) */
void CZObjectiveList::SetCurColor(int index)
{
    int newindex;

    if (!ResolveIndex(index, &newindex))
        return;
    C2DString *curstring = m_objectivenames[newindex];
    float r = curstring->m_color[0];
    float g = curstring->m_color[1];
    float b = curstring->m_color[2];

    if (ResolveIndex(m_selected_objective, &newindex)) {
        curstring = m_objectivenames[newindex];
        curstring->m_color[0] = r;
        curstring->m_color[1] = g;
        curstring->m_color[2] = b;
    }
}

/* the children drawn while the list is on and no mission camera plays. SOCOM 1 cast each child
 * to C2D first and dropped the result; SOCOM II has no cast. The iterator is assigned from
 * begin() (it lives in memory), and the end taken after it into a local: begun first, `this`
 * dies there and its saved register takes the end (taking the end first keeps both) */
void CZObjectiveList::Draw(zdb::CCamera *cam)
{
    if (IsOn() && !theHUD->m_missioncams.IsPlaying()) {
        std::list<C2D *>::iterator i;

        i = begin();
        std::list<C2D *>::iterator e = end();

        for (; i != e; ++i)
            (*i)->Draw(cam);
    }
}

/* the selected objective's camera (SOCOM 1's, GetObjective inlined: SOCOM II has no copy of it) */
CMissionObjectiveCam *CZObjectiveList::GetCurrentObjective() const
{
    return GetObjective(m_selected_objective);
}

/* the objective whose camera is held (SOCOM 1's) */
CMissionObjectiveCam *CZObjectiveList::GetCurrentObjectiveRetainer() const
{
    return m_cur_selected_objective;
}

bool CZObjectiveList::IsPlayingAnimation()
{
    return theHUD->m_missioncams.IsPlaying();
}
