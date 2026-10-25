/* fts_mission.cpp -- the mission: CMission, the online game's state (CZNetGame), the ladders, the
 * HUD's ZAnim commands, the archive and the custom character types (SOCOM 1's fts_mission.cpp;
 * its static initialiser is SOCOM II's 0x400160). OVL1 0x2A3B90-0x2B218F, the template instances
 * closing it (the objective cameras' vector, the UI variables' vector) before fts_player.cpp's
 * CSpawners code. Only some of it so far. Defined in reverse of their image order
 * (-inline deferred). */

#include "../zanim/zanim.h"
#include "../zsys/zsys.h"
#include "../../Apps/FTS/hud_letterbox.h"
#include "../../Apps/FTS/hud_missioncams.h"

/* the HUD: only what the commands touch (SOCOM 1's CHUD at SOCOM II's offsets; its On and Off
 * inline here, out of line in SOCOM 1) */
class CHUD {
public:
    void On() { m_ison = true; }
    void Off() { m_ison = false; }

    unsigned char _0[0x1a100];
    CLetterBox m_letterbox;   // 0x1a100 (SOCOM 1's at 0x14280)
    unsigned char _1a230[0x1a2dc - 0x1a230];
    CZHudMissionCams m_missioncams;   // 0x1a2dc (SOCOM 1's at 0x14590)
    unsigned char _1a698[0x1a6f5 - 0x1a698];
    unsigned char m_ison : 1;   // 0x1a6f5 (SOCOM 1's at 0x14955)
};

extern CHUD *theHUD;

/* the file's tables (SOCOM 1's names; SOCOM II has 32 visibility lists of 31 maps where SOCOM 1
 * had 10 of 9) */
namespace {
char g_arrDefCharTypeNames[32][64];
unsigned char g_arrMapVis[32][31];
int g_arrMapVisCounts[32];
}

extern "C" const char *data_003F02B0[32];   /* GetCustomCharTypeName's c_arrCustomCharTypeNames */

/* the number of maps a list shows (OVL1 0x2AF890) */
int ftsGetNumVisibleMaps(unsigned int list)
{
    if (list < 32)
        return g_arrMapVisCounts[list];
    return 0;
}

/* a list's map (OVL1 0x2AF840) */
unsigned char ftsGetVisibleMap(unsigned int list, int i)
{
    return g_arrMapVis[list][i];
}

/* whether a list shows a map (OVL1 0x2AF7B0) */
bool ftsIsMapInVisibleList(unsigned int map, unsigned int list)
{
    if (list >= 32)
        return false;
    for (int i = 0; i < g_arrMapVisCounts[list]; i++)
        if (map == g_arrMapVis[list][i])
            return true;
    return false;
}

/* a custom character type's name (OVL1 0x2AF780) */
const char *GetCustomCharTypeName(int i)
{
    if (i < 32)
        return data_003F02B0[i];
    return 0;
}

/* a default character type's name (OVL1 0x2AF760) */
const char *GetDefCharTypeName(int i)
{
    return g_arrDefCharTypeNames[i];
}

/* a switch command: ON or OFF (the letterbox's, the HUD's) */
struct _hud_switch_cmd {
    _zanim_cmd_hdr hdr;
    signed char on : 1;   // 0x4
    signed char off : 1;
};

/* STOP_IMAGE_RECORDER: a bare timeless command (OVL1 0x2AECC0) */
_zanim_cmd_hdr *StopImageRecorderParse(_zrdr *rdr)
{
    _zanim_cmd_hdr *cmd;

    if (!rdr)
        return 0;
    cmd = (_zanim_cmd_hdr *)_calloc(1, sizeof(_zanim_cmd_hdr), 0, 0);
    cmd->data_size = sizeof(_zanim_cmd_hdr);
    cmd->timeless = 1;
    return cmd;
}

/* HUD ON (OVL1 0x2AEC20) */
_zanim_cmd_hdr *HUDCmdParseOn(_zrdr *rdr)
{
    _hud_switch_cmd *cmd;

    if (!rdr)
        return 0;
    cmd = (_hud_switch_cmd *)_calloc(1, sizeof(_hud_switch_cmd), 0, 0);
    cmd->hdr.data_size = sizeof(_hud_switch_cmd);
    cmd->hdr.timeless = 1;
    cmd->on = 1;
    return (_zanim_cmd_hdr *)cmd;
}

/* HUD OFF (OVL1 0x2AEBA0) */
_zanim_cmd_hdr *HUDCmdParseOff(_zrdr *rdr)
{
    _hud_switch_cmd *cmd;

    if (!rdr)
        return 0;
    cmd = (_hud_switch_cmd *)_calloc(1, sizeof(_hud_switch_cmd), 0, 0);
    cmd->hdr.data_size = sizeof(_hud_switch_cmd);
    cmd->hdr.timeless = 1;
    cmd->off = 1;
    return (_zanim_cmd_hdr *)cmd;
}

/* HUD ON / OFF: the HUD shown or hidden (OVL1 0x2AEB00) */
bool HUDCmdTickControlSwitch(_zanim_cmd_hdr *hdr, float *dT)
{
    _hud_switch_cmd *cmd = (_hud_switch_cmd *)hdr;

    if (!theHUD)
        return true;
    if (cmd->on)
        theHUD->On();
    else if (cmd->off)
        theHUD->Off();
    return true;
}

/* LETTERBOX ON (OVL1 0x2AEA70) */
_zanim_cmd_hdr *HUDLetterBoxCmdParseOn(_zrdr *rdr)
{
    _hud_switch_cmd *cmd;

    if (!rdr)
        return 0;
    cmd = (_hud_switch_cmd *)_calloc(1, sizeof(_hud_switch_cmd), 0, 0);
    cmd->hdr.data_size = sizeof(_hud_switch_cmd);
    cmd->hdr.timeless = 1;
    cmd->on = 1;
    return (_zanim_cmd_hdr *)cmd;
}

/* LETTERBOX OFF (OVL1 0x2AE9F0) */
_zanim_cmd_hdr *HUDLetterBoxCmdParseOff(_zrdr *rdr)
{
    _hud_switch_cmd *cmd;

    if (!rdr)
        return 0;
    cmd = (_hud_switch_cmd *)_calloc(1, sizeof(_hud_switch_cmd), 0, 0);
    cmd->hdr.data_size = sizeof(_hud_switch_cmd);
    cmd->hdr.timeless = 1;
    cmd->off = 1;
    return (_zanim_cmd_hdr *)cmd;
}

/* LETTERBOX ON / OFF: the bars brought in or taken away (OVL1 0x2AE970) */
bool HUDLetterBoxCmdTick(_zanim_cmd_hdr *hdr, float *dT)
{
    _hud_switch_cmd *cmd = (_hud_switch_cmd *)hdr;

    if (!theHUD)
        return true;
    if (cmd->on)
        theHUD->m_letterbox.AddLetterbox();
    else if (cmd->off)
        theHUD->m_letterbox.RemoveLetterbox();
    return true;
}

/* TRIGGER_MISSION_CAM: the mission camera the command names played (OVL1 0x2AE7D0) */
struct _trigger_mission_cam_cmd {
    _zanim_cmd_hdr hdr;
    unsigned char name;   // 0x4
};

void HUDTriggerMissionCamBegin(_zanim_cmd_hdr *hdr)
{
    CHUD *hud = theHUD;

    if (hud)
        hud->m_missioncams.PlayCamera(ZAnim.m_CurAnim->GetName(((_trigger_mission_cam_cmd *)hdr)->name));
}

/* TRIGGER_MISSION_CAM: done when no mission camera plays (OVL1 0x2AE760) */
bool HUDTriggerMissionCamTick(_zanim_cmd_hdr *hdr, float *dT)
{
    if (!theHUD)
        return true;
    CZHudMissionCams *cams = &theHUD->m_missioncams;

    if (cams && cams->IsPlaying())
        return false;
    return true;
}
