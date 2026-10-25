/* hud_objectives.cpp -- the objectives list (OVL1 0x205C40-0x208254): SOCOM 1's unit (its DWARF names
 * it, Z:\dev\Apps\FTS\hud_objectives.cpp). Only IsPlayingAnimation and Draw so far; CZObjectiveList's
 * members past C2D are not placed. Defined in reverse of their image order (-inline deferred). */

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

class CZObjectiveList : public C2D {
public:
    virtual ~CZObjectiveList();
    virtual void Draw(zdb::CCamera *cam);
    static bool IsPlayingAnimation();
};

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
