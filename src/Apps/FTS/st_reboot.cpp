/* st_reboot.cpp -- SOCOM II's state for rebooting the game: the system shut down and the game's
 * own ELF run again, its menu state opening on dlgAfterReboot.rdr -- and the same reboot after a
 * failed assertion (zVid_Assert's, both overlays' copies, and 0x365A00 call it), opening on
 * dlgAfterErrorReboot.rdr. OVL1 0x22EBD0-0x22EDAF, after st_shutdown.cpp, inside the HUD's range:
 * CRebootState's UnInit, Tick, Init and constructor (its vtable 0x405B10, RTTI 0x3E5C58), then the
 * error reboot. Its own unit (st_shutdown.cpp's header says why); the two argument lists (.data
 * 0x3DC9D0, 0x3DC9E0) in definition order. SOCOM 1 has no such state: the file name and every
 * name below but the class's are ours. Defined in reverse of their image order
 * (-inline deferred). */

#include "fts_state.h"

extern "C" {
char *strcpy(char *dst, const char *src);
char *strcat(char *dst, const char *src);
int sceCdInit(int mode);
int sceSifExitCmd(void);
void LoadExecPS2(const char *filename, int argc, char *argv[]);
}

void zSysShutdown();

extern unsigned char data_0049E2CD;   /* booted from the disc (0: from host0:, the PC) */
extern char *data_004A15E0;   /* the game's directory on host0: */

/* the game's arguments after a reboot: the menu state's dialog */
static char *s_errorRebootArgs[4] = { "--menu_state", "dlgAfterErrorReboot.rdr", "", 0 };
static char *s_rebootArgs[4] = { "--menu_state", "dlgAfterReboot.rdr", "", 0 };

/* the game rebooted after a failed assertion (ours) */
void RebootAfterError()
{
    zSysShutdown();
    sceCdInit(5);   /* SCECdEXIT */
    sceSifExitCmd();
    if (data_0049E2CD) {
        LoadExecPS2("cdrom0:\\SCUS_972.75;1", 3, s_errorRebootArgs);
    } else {
        char path[256];
        strcpy(path, "host0:");
        strcat(path, data_004A15E0);
        LoadExecPS2(path, 3, s_errorRebootArgs);
    }
}

CRebootState::CRebootState() : CGameState("RebootState")
{
}

bool CRebootState::Init()
{
    zSysShutdown();
    sceCdInit(5);   /* SCECdEXIT */
    sceSifExitCmd();
    if (data_0049E2CD) {
        LoadExecPS2("cdrom0:\\SCUS_972.75;1", 3, s_rebootArgs);
    } else {
        char path[256];
        strcpy(path, "host0:");
        strcat(path, data_004A15E0);
        LoadExecPS2(path, 3, s_rebootArgs);
    }
    return true;
}

bool CRebootState::Tick(float dT)
{
    return false;
}

void CRebootState::UnInit()
{
}
