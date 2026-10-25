/* zin_keyboard.cpp -- the USB keyboard: its reader set up and torn down, a keyboard's presence, and
 * CKeyboard (OVL1 0x2DBD40-0x2DBFAB). SOCOM 1's keyboard unit (after zin_cheats.cpp there) carried
 * its own sceUsbKeybd RPC wrappers; SOCOM II calls a keyboard library in MAIN instead (the
 * placeholders below), keeping SOCOM 1's order: the readers, InitKeyreadStuff, then CKeyboard's
 * Tick, destructor and constructor. CInput's unit calls the empty Tick and the constructor out of
 * line, so this is a unit of its own. The file name is ours. Defined in reverse of their image
 * order (-inline deferred). */

#include <string.h>

#include "zinput.h"

/* the keyboard library's calls (MAIN, not named yet: its set-up, read mode, key repeat, teardown,
 * information and synchronisation, by their use here) */
extern "C" int func_001BE6A0(int *maxConnect);
extern "C" int func_001BEDE8(int no, int mode);
extern "C" int func_001BEDA0(int no, int sta, int interval);
extern "C" int func_001BE898();
extern "C" int func_001BE960(void *info);
extern "C" int func_001BEE88(int mode, int *result);

CKeyboard::CKeyboard()
{
    m_0 = 0;
}

CKeyboard::~CKeyboard()
{
}

/* nothing to tick (SOCOM 1's) */
void CKeyboard::Tick(float dt)
{
}

/* the keyboard library set up for one keyboard, its read mode and key repeat (SOCOM 1's started its
 * own RPC client) */
void InitKeyreadStuff()
{
    int maxConnect = 1;

    func_001BE6A0(&maxConnect);
    func_001BEDE8(0, 0);
    func_001BEDA0(0, 20, 6);
}

void UninitKeyreadStuff()
{
    func_001BE898();
}

/* whether a keyboard is attached: the library's information read, its first status byte (new in
 * SOCOM II; the name is ours) */
bool KeyboardAttached()
{
    int result;
    unsigned char info[136];

    memset(info, 0, sizeof info);
    func_001BE960(info);
    result = 0;
    func_001BEE88(0, &result);
    return info[8] != 0;
}
