/* hud_missioncams.cpp -- the HUD's mission cameras (OVL1 0x228ED0-0x229C9F): SOCOM 1's unit (its
 * DWARF names it, Z:\dev\Apps\FTS\hud_missioncams.cpp), the same functions in the same order,
 * from IsPlaying to the constructor. Not written yet: PlayCamera(CMissionCam *, bool), which
 * SOCOM II grew from 0x3DC to 0x54C bytes, CleanupCamera and HudTick -- all three call into the
 * HUD, the seal's controller, the jukebox and the camera through rows not named yet. Defined in
 * reverse of their image order (-inline deferred). */

#include "hud_missioncams.h"

class CMission {
public:
    CMissionCam *GetMissionCamByIdentifier(const char *id);
};

extern CMission theMission;

CZHudMissionCams::CZHudMissionCams()
{
    m_currentcam = 0;
    m_animhandle = 0;
    m_pause_for_frame = 0;
}

CZHudMissionCams::~CZHudMissionCams()
{
}

/* the strings shown in the letterbox listed as the HUD's children, each off.
 * OPEN (0xE8 of 0x104) until include/list changes: the target hoists the list's end node
 * (this + 4, in s2) out of the loop, which MW does once the pointer list's push_back builds its
 * iterator from a node_base & accessor -- `iterator(&this->end_node())`, end_node() returning
 * end_ by reference (SOCOM 1's end node came through compressed_pair's second()). With that
 * change this matches and every other hand-written unit still does (CNode::CreateInstance by
 * name too: the accessor sits in argument position, so the tree is no taller) */
void CZHudMissionCams::Init()
{
    push_back(&m_letterboxobjectivename);
    m_letterboxobjectivename.Off();
    for (int i = 0; i < 3; i++) {
        push_back(&m_letterboxobjectivedescription[i]);
        m_letterboxobjectivedescription[i].Off();
    }
    push_back(&m_crosstext);
    m_crosstext.Off();
}

void CZHudMissionCams::UnInit()
{
    for (int i = 0; i < 3; i++)
        m_letterboxobjectivedescription[i].ClearAndWipe();
    m_letterboxobjectivename.ClearAndWipe();
    m_crosstext.ClearAndWipe();
    m_lastmenustate = stateNone;
}

/* the letterbox's strings drawn while a camera plays */
void CZHudMissionCams::Draw(zdb::CCamera *theCamera)
{
    if (IsPlaying()) {
        m_letterboxobjectivename.Draw(theCamera);
        for (int i = 0; i < 3; i++)
            m_letterboxobjectivedescription[i].Draw(theCamera);
        m_socom2_280.Draw(theCamera);
        m_crosstext.Draw(theCamera);
    }
}

/* a camera of the mission's, by its identifier, played with the lens swap */
void CZHudMissionCams::PlayCamera(const char *camname)
{
    CMissionCam *cam = theMission.GetMissionCamByIdentifier(camname);

    if (cam)
        PlayCamera(cam, true);
}

bool CZHudMissionCams::IsPlaying()
{
    return m_animhandle != 0;
}
