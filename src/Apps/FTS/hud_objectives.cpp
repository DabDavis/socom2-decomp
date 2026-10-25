/* hud_objectives.cpp -- the objectives list (OVL1 0x205C40-0x208254): SOCOM 1's unit (its DWARF names
 * it, Z:\dev\Apps\FTS\hud_objectives.cpp). Only IsPlayingAnimation, Draw and Init so far;
 * CZObjectiveList's members past C2D are placed only as far as Init reaches. Defined in reverse of
 * their image order (-inline deferred). */

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

/* the list: its check box, check and background textures placed (SOCOM II's offsets) */
class CZObjectiveList : public C2D {
public:
    virtual ~CZObjectiveList();
    virtual void Draw(zdb::CCamera *cam);
    void Init();
    static bool IsPlayingAnimation();

    unsigned char _18[0x720 - 0x18];
    zdb::CTexHandle *m_backgroundtex;   // 0x720
    zdb::CTexHandle *m_checkboxtex;   // 0x724
    zdb::CTexHandle *m_checktex;   // 0x728
};

/* the list's three textures looked up by name */
void CZObjectiveList::Init()
{
    m_checkboxtex = zdb::CWorld::GetTexHandle("hud_checkbox.tif");
    m_checktex = zdb::CWorld::GetTexHandle("hud_check.tif");
    m_backgroundtex = zdb::CWorld::GetTexHandle("team_background.tif");
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

bool CZObjectiveList::IsPlayingAnimation()
{
    return theHUD->m_missioncams.IsPlaying();
}
