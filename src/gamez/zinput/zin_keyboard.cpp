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
extern "C" int func_001BEA08(int no, void *data);

/* a key read this frame: its code, and the shift, control and alt keys and the caps lock light
 * (SOCOM 1's KeyInput, not in its DWARF; 32 bytes, as the reader clears it -- the names ours) */
struct KeyInput {
    unsigned char code;   // 0x0
    int shift;   // 0x4
    int ctrl;   // 0x8
    int alt;   // 0xc
    int _10;   // 0x10
    int caps;   // 0x14
    int _18[2];
};   // 0x20 bytes

/* the keyboard library's last read: its lights, its modifier keys and its key codes, and how many of
 * them are taken (SOCOM II's statics, the names ours: 136 bytes, the size of the information block
 * KeyboardAttached reads) */
static struct {
    unsigned int led;   // 0x0
    unsigned int mkey;   // 0x4
    int len;   // 0x8
    unsigned short keycode[62];   // 0xc
} kbdData;
static int kbdIndex;

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

/* the next key the keyboard library read, reading again once the last read is used up: none when
 * no keyboard is attached or nothing was read (SOCOM 1's read through its own RPC; SOCOM II's
 * returns the key's code) */
int GetKeyReadThisFrame(KeyInput *key)
{
    memset(key, 0, sizeof *key);
    if (kbdIndex >= kbdData.len) {
        int done;
        int result;
        unsigned char info[136];

        memset(info, 0, sizeof info);
        func_001BE960(info);
        result = 0;
        func_001BEE88(0, &result);
        if (!info[8])
            return 0;
        func_001BEA08(0, &kbdData);
        done = 0;
        func_001BEE88(0, &done);
        if (done)
            kbdData.len = 0;
        if (kbdData.len <= 0)
            return 0;
        kbdIndex = 0;
    }
    key->code = kbdData.keycode[kbdIndex];
    kbdIndex++;
    key->shift = (kbdData.mkey & 0x22) != 0;
    key->ctrl = (kbdData.mkey & 0x11) != 0;
    key->alt = (kbdData.mkey & 0x44) != 0;
    key->caps = (kbdData.led & 2) != 0;
    return key->code;
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
