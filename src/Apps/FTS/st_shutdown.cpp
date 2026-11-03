/* st_shutdown.cpp -- SOCOM II's state for leaving the game for the network configuration GUI
 * (SCUSNGUI.ELF): the system shut down and the GUI's ELF run, handed the way back -- the game's
 * own ELF and the dialog it opens on its return, the LAN one when the LAN lobby was up. OVL1
 * 0x22EA50-0x22EBCF, inside the HUD's range (after hud_scorepopup.cpp's tail, before
 * st_reboot.cpp): CShutdownState's UnInit, Tick, Init and constructor (its vtable 0x405AE0, RTTI
 * 0x3E5B58). Its own unit, apart from st_reboot.cpp: each holds its own copy of the literals both
 * use ("--menu_state", "cdrom0:\SCUS_972.75;1", "host0:"), which one unit would pool; its strings
 * are laid out in emission order (Init's, then the constructor's "ShutdownState"). SOCOM 1 has no
 * such state: the file name and the statics' names are ours. Defined in reverse of their image
 * order (-inline deferred). */

#include "fts_state.h"

#include <iostream>

extern "C" {
char *strcpy(char *dst, const char *src);
char *strcat(char *dst, const char *src);
int sceCdInit(int mode);
int sceSifExitCmd(void);
void LoadExecPS2(const char *filename, int argc, char *argv[]);
}

void zSysShutdown();

/* the LAN lobby (zNetLANLobby.cpp's): only whether it is up */
class CZLANLobby {
public:
    bool IsActive();
};
extern CZLANLobby data_0044FE10;   /* the LAN lobby */

extern unsigned char data_0049E2CD;   /* booted from the disc (0: from host0:, the PC) */
extern char *data_004A15E0;   /* the game's directory on host0: */

/* the GUI's arguments: the language, the ELF to come back to, and the menu state's dialog on the
 * way back (SOCOM II's: 0x3DC9C0) */
static char *s_shutdownArgs[4] = { "ENGLISH", "cdrom0:\\SCUS_972.75;1", "--menu_state", 0 };
/* the game's ELF on host0: (0x412EC0) */
static char s_hostElf[256];

CShutdownState::CShutdownState() : CGameState("ShutdownState")
{
}

bool CShutdownState::Init()
{
    if (data_0044FE10.IsActive())
        s_shutdownArgs[3] = "dlgReturnFromNTGUI2LAN.rdr";
    else
        s_shutdownArgs[3] = "dlgReturnFromNTGUI2.rdr";
    zSysShutdown();
    sceCdInit(5);   /* SCECdEXIT */
    sceSifExitCmd();
    if (data_0049E2CD) {
        LoadExecPS2("cdrom0:\\SCUSNGUI.ELF;1", 4, s_shutdownArgs);
    } else {
        s_shutdownArgs[1] = s_hostElf;
        strcpy(s_hostElf, "host0:");
        strcat(s_hostElf, data_004A15E0);
        LoadExecPS2("host0:.\\SCUSHGUI.ELF", 4, s_shutdownArgs);
    }
    return true;
}

bool CShutdownState::Tick(float dT)
{
    return false;
}

void CShutdownState::UnInit()
{
}
