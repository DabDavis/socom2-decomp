/* hud_objectives.cpp -- the objectives list (OVL1 0x205C40-0x208254): SOCOM 1's unit (its DWARF names
 * it, Z:\dev\Apps\FTS\hud_objectives.cpp). IsPlayingAnimation, Draw, Tick, Init and the implicit
 * destructor so far, all byte-exact. Defined in reverse of their image order (-inline deferred). */

#include <list>

#include "../../gamez/ztwod/ztwod.h"
#include "../../gamez/zvid/zvid.h"
#include "hud_missioncams.h"

/* the HUD: its menu state and mission cameras placed (SOCOM II's offsets) */
class CHUD {
public:
    unsigned char _0[0x498];
    C2DFont *m_myfont;   // 0x498
    unsigned char _49c[0x1a220 - 0x49c];
    MENU_STATE m_menustate;   // 0x1a220
    unsigned char _1a221[0x1a2dc - 0x1a221];
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
/* an unselected name's alpha (data 0x3DC450) */
static float s_unselectedtrans = 80.0f;
/* an unselected name's colour (OVL1 bss 0x409EA0, three floats; no row yet) */
extern PNT4D data_00409EA0;
/* the backdrop over the whole screen, the map's frame, its box and four edges, the title bars and
 * the boxes RefreshStatics lays out (OVL1 bss from 0x4090F0, statics of this unit's, no rows yet) */
extern C2DBitmapPoly data_004090F0;
extern C2DBitmap data_004092F0;
extern C2DBitmap data_00409380;
extern C2DBitmap data_00409410[4];
extern C2DBitmap data_00409620;
extern C2DBitmap data_00409760;
extern C2DBitmap data_004097F0;
extern C2DBitmap data_00409930;
extern C2DBitmap data_00409A70;
/* the map frame's alpha (data 0x3DC440: a variable, read with lwc1) */
static float s_frametrans = 128.0f;
/* a string placed by its position and centring (no row named yet) */
extern "C" void func_00362B70(C2DString *s);
/* the strings RefreshStatics sets up (OVL1 bss 0x4096B0-0x409E8F, statics of this unit's, no rows
 * yet: called directly, as globals are) */
extern C2DString data_004096B0;
extern C2DString data_00409880;
extern C2DString data_004099C0;
extern C2DString data_00409B00[6];

/* the mission: only its objectives (theMission, fts_state.h's CMission at 0x1854) */
class CMissionCam {
};

class CMissionObjectiveCam : public CMissionCam {
};

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
    /* an element as SOCOM 1's DWARF has it: the vector and the index, read through test_bit */
    class reference {
    public:
        reference(vector &v, unsigned int n) : vec_(&v), n_(n) {}
        operator bool() const { return vec_->test_bit(n_); }

        vector *vec_;
        unsigned int n_;
    };

    ~vector() { if (data_) operator delete(data_); }
    reference operator[](unsigned int n) { return reference(*this, n); }
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
    void RefreshStatics();
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
    /* whether any objective can be selected (SOCOM 1's; inline in SOCOM II) */
    bool IsAnySelectable()
    {
        int size = m_objectivenames.size();

        for (int i = 0; i < size; i++) {
            if (m_vecSelectable[i])
                return true;
        }
        return false;
    }
    /* an objective's name greyed (inline; the name ours) */
    void GreyObjective(int index)
    {
        int newindex;

        if (ResolveIndex(index, &newindex)) {
            C2DString *curstring = m_objectivenames[newindex];

            curstring->SetColor(data_00409EA0.x, data_00409EA0.y, data_00409EA0.z);
            curstring->SetTrans(s_unselectedtrans);
        }
    }
    void SelectObjective();
    void FreeList(bool wipe_vector);
    void MissionEndCleanup();
    void PreviousObjective();
    void NextObjective();

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

/* the selected objective's name given the colour of another's (SOCOM 1's). Never inlined in the
 * target (NextObjective and PreviousObjective call it): pinned */
#pragma auto_inline off
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

#pragma auto_inline reset

/* on to the next selectable objective: its colour the old one's, the old one greyed (SOCOM 1's
 * 80 grey; SOCOM II's colour and alpha its own) */
void CZObjectiveList::NextObjective()
{
    int oldindex = m_selected_objective;

    if (!IsAnySelectable())
        return;
    SelectNextObjective();
    SetCurColor(oldindex);
    GreyObjective(oldindex);
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

/* back to the previous selectable objective, as NextObjective (SOCOM 1's) */
void CZObjectiveList::PreviousObjective()
{
    int oldindex = m_selected_objective;

    if (!IsAnySelectable())
        return;
    SelectPreviousObjective();
    SetCurColor(oldindex);
    GreyObjective(oldindex);
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

/* the selected objective's camera played, unless one plays or the orders menu is up (SOCOM 1's) */
void CZObjectiveList::SelectObjective()
{
    CMissionObjectiveCam *objective = GetObjective(m_selected_objective);

    if (!objective)
        return;
    if (theHUD->m_missioncams.IsPlaying() || theHUD->m_menustate == stateOrders)
        return;
    theHUD->m_missioncams.PlayCamera(objective, false);
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

/* at the mission's end: every string wiped (the list's own through their tables, the unit's
 * static ones directly), then the list freed with its vectors (SOCOM II's; SOCOM 1's freed the
 * vectors here itself) */
void CZObjectiveList::MissionEndCleanup()
{
    m_heading.ClearAndWipe();
    m_strMapHeading.ClearAndWipe();
    m_strAnalogL.ClearAndWipe();
    m_strAnalogR.ClearAndWipe();
    m_strCircleLabel.ClearAndWipe();
    m_strSquareLabel.ClearAndWipe();
    m_strTriLabel.ClearAndWipe();
    m_strXLabel.ClearAndWipe();
    data_004096B0.ClearAndWipe();
    data_00409880.ClearAndWipe();
    data_004099C0.ClearAndWipe();
    for (unsigned int i = 0; i < 6; i++)
        data_00409B00[i].ClearAndWipe();
    m_arrAnchorStrings[0].ClearAndWipe();
    FreeList(true);
}

bool CZObjectiveList::IsPlayingAnimation()
{
    return theHUD->m_missioncams.IsPlaying();
}

namespace zdb {
class CAssetList {
public:
    CTexHandle *GetTexHandle(const char *name);
};

class CAssetMgr {
public:
    static CAssetList m_assets;
};
}

/* the tactical map's statics laid out and listed: the darkened screen, the map's frame and edges,
 * the title bars and their titles, the legend's box and its six lines (SOCOM 1's; moved to the
 * unit's end in SOCOM II, so first in the image after the implicit destructor) */
void CZObjectiveList::RefreshStatics()
{
    unsigned int i;
    zdb::CTexHandle *tex = zdb::CAssetMgr::m_assets.GetTexHandle("newweapnbkrnd.tif");

    push_back(&data_004090F0);
    data_004090F0.SetPos(0.0f, 0.0f, zVid.renderWidth, zVid.renderHeight);
    data_004090F0.SetHasTexture(false);
    data_004090F0.SetColor(0.0f, 0.0f, 0.0f);
    data_004090F0.SetTrans(100.0f);
    data_004090F0.SetTrans(1, 0.0f);
    data_004090F0.SetTrans(2, 0.0f);
    data_004092F0.SetHasTexture(false);
    data_004092F0.SetPos(230, 60, 630, 365);
    data_004092F0.SetIsTrans(true);
    data_004092F0.SetColor(0.0f, 0.0f, 0.0f);
    data_004092F0.SetTrans(s_frametrans);
    push_back(&data_004092F0);
    data_00409380.Load(230.0f, 60.0f, 630.0f, 365.0f, tex);
    push_back(&data_00409380);
    data_00409380.SetUV(0.3f, 0.3f, 0.4f, 0.4f);
    data_00409380.SetTrans(72.0f);
    for (i = 0; i < 4; i++) {
        data_00409410[i].SetHasTexture(false);
        data_00409410[i].SetColor(64.0f, 128.0f, 128.0f);
        data_00409410[i].SetTrans(128.0f);
        push_back(&data_00409410[i]);
    }
    data_00409410[0].SetPos(230, 60, 232, 365);
    data_00409410[1].SetPos(230, 60, 630, 62);
    data_00409410[2].SetPos(630, 60, 628, 365);
    data_00409410[3].SetPos(230, 363, 630, 365);
    data_00409620.Load(230.0f, 15.0f, 630.0f, 55.0f, tex);
    push_back(&data_00409620);
    data_00409620.SetUV(0.3f, 0.3f, 0.4f, 0.4f);
    data_00409620.SetTrans(128.0f);
    data_004096B0.Load(1.5f, "TACTICAL MAP", theHUD->m_myfont, 430.0f, 48.0f, true);
    push_back(&data_004096B0);
    data_004096B0.m_isCentered = 1;
    func_00362B70(&data_004096B0);
    data_00409760.Load(10.0f, 84.0f, 224.0f, 365.0f, tex);
    data_00409760.SetUV(0.3f, 0.3f, 0.4f, 0.4f);
    data_00409760.SetTrans(72.0f);
    push_back(&data_00409760);
    data_004097F0.Load(10.0f, 60.0f, 224.0f, 84.0f, tex);
    data_004097F0.SetUV(0.3f, 0.3f, 0.4f, 0.4f);
    data_004097F0.SetTrans(128.0f);
    push_back(&data_004097F0);
    data_00409880.Load(0.8f, "\xa6  PLAY MOVIE", theHUD->m_myfont, 20.0f, 77.0f, true);
    push_back(&data_00409880);
    data_00409930.Load(10.0f, 15.0f, 224.0f, 55.0f, tex);
    push_back(&data_00409930);
    data_00409930.SetUV(0.3f, 0.3f, 0.4f, 0.4f);
    data_00409930.SetTrans(128.0f);
    data_004099C0.Load(1.5f, "OBJECTIVES", theHUD->m_myfont, 117.0f, 48.0f, true);
    push_back(&data_004099C0);
    data_004099C0.m_isCentered = 1;
    func_00362B70(&data_004099C0);
    data_00409A70.Load(10.0f, 370.0f, 630.0f, 434.0f, tex);
    push_back(&data_00409A70);
    data_00409A70.SetUV(0.3f, 0.3f, 0.4f, 0.4f);
    data_00409A70.SetTrans(128.0f);
    for (unsigned int j = 0; j < 6; j++) {
        data_00409B00[j].Load(0.8f, " ", theHUD->m_myfont, 0.0f, 0.0f, true);
        push_back(&data_00409B00[j]);
    }
    data_00409B00[0].SetString("\xb4  SNAP TO PLAYER", true);
    data_00409B00[0].SetPos(20, 395);
    data_00409B00[1].SetString("\xb7  SNAP TO OBJECTIVE", true);
    data_00409B00[1].SetPos(20, 419);
    data_00409B00[2].SetString("LEFT ANALOG STICK = MOVE MAP", true);
    data_00409B00[2].SetPos(230, 395);
    data_00409B00[3].SetString("RIGHT ANALOG STICK = ZOOM MAP", true);
    data_00409B00[3].SetPos(230, 419);
    data_00409B00[4].SetString("\xac  RESUME GAME", true);
    data_00409B00[4].SetPos(500, 395);
    data_00409B00[5].SetString("\xbe  LEGEND", true);
    data_00409B00[5].SetPos(500, 419);
}
