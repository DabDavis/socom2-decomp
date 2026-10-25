/* util_memcard.cpp -- CMemCardIO and CMemCard (OVL1 0x39FEB0-0x3A2370): a file on the memory card
 * through Sony's libmc, and the card itself -- its directories, its icon files, the config and
 * game saves (two CSaveManagers over the card), the space checks. SOCOM 1's demo holds the two
 * classes as one run after CSTable, in this order (GetMode ... Open, chdir ... IsInserted, the
 * saves, Uninit, Init, ~CMemCard, CMemCard), with no assert naming the file: util_memcard.cpp is
 * ours. SOCOM II reports every transfer through MEMCARD_STATUS_VALVE and the menu's
 * MEMCARD_STATUS / MEMCARD_ERROR events, waits for the last asynchronous call before each
 * command, and added the asynchronous slots, the insertion poll and the reserve files. */

#include "zutil.h"

extern "C" {
int sceMcSync(int mode, int *cmd, int *result);
int sceMcOpen(int port, int slot, const char *name, int mode);
int sceMcClose(int fd);
int sceMcSeek(int fd, int offset, int origin);
int sceMcRead(int fd, void *buf, int size);
int sceMcWrite(int fd, const void *buf, int size);
int sceMcMkdir(int port, int slot, const char *name);
int sceMcGetInfo(int port, int slot, int *type, int *free, int *format);
int sceMcGetDir(int port, int slot, const char *name, unsigned int mode, int maxent, void *table);
/* a directory entry (Sony's sceMcTblGetDir, 64-aligned) */
struct sceMcTblGetDir {
    unsigned char _Create[8];
    unsigned char _Modify[8];
    unsigned int FileSizeByte;   // 0x10
    unsigned short AttrFile;
    unsigned short Reserve1;
    unsigned int Reserve2;
    unsigned int PdaAplNo;
    unsigned char EntryName[32];
} __attribute__((aligned(64)));
/* libmc's sceMcInit, sceMcEnd, sceMcChdir, sceMcFormat, sceMcDelete and sceMcFlush (SOCOM 1's
 * names, the same order in MAIN; not named in the map yet) */
int func_001B3970(void);
int func_001B3B40(void);
int func_001B4888(int port, int slot, const char *path, char *pwd);
int func_001B49C8(int port, int slot);
int func_001B4A98(int port, int slot, const char *name);
int func_001B4BB0(int fd);
}

/* the system's public state: whether the memory card's modules are loaded (zsys_public.h) */
struct _zsys_public {
    unsigned char _0[0x20];
    unsigned char mcIrxLoaded;   // 0x20
};
extern _zsys_public zSys;
void zSysLoadMcIRX();
void zSysUnloadMcIRX();

/* the valves, as zanim.h declares them (only what the card uses) */
enum VALVE_TYPE {};
enum VALVE_STATE {
    VALVE_SAME,
    VALVE_ON,
    VALVE_OFF
};

class CValve {
public:
    static CValve *Create(const char *name, VALVE_TYPE type);
    void MakeCallbacks(VALVE_STATE state);

    char *m_name;   // 0x0
    unsigned int m_value : 16;   // 0x4
};

/* CValve::SetValue, inlined in both builds (as zanim_menu.h's SetValveValue) */
inline void SetValveValue(CValve *valve, int value)
{
    VALVE_STATE state = VALVE_SAME;

    if (valve->m_value != value)
        state = value == 0 ? VALVE_OFF : VALVE_ON;
    valve->m_value = value;
    if (state != VALVE_SAME)
        valve->MakeCallbacks(state);
}

/* the game's menu state, as zanim_menu.h declares it */
class CMenuState {
public:
    bool RunEventAnim(const char *eventname, const char *argument);
};

class COurGame {
public:
    unsigned char _0[0x78];
    CMenuState m_menuState;   // 0x78
};
extern COurGame theGame;

/* the save files' manager (zsav_manager.cpp) */
class CSaveManager {
public:
    CSaveManager(CIO *file);
    ~CSaveManager();

    unsigned char _0[0x24];
};

/* CSaveManager::Load and Save (zsav_manager.cpp's; not named in the map yet) */
extern "C" bool func_0033D6D0(CSaveManager *save, const char *name, int flags);
extern "C" bool func_0033DFF0(CSaveManager *save, const char *name, int flags, int a, const char *desc,
                              char *b);

/* ~CSaveManager, deleting (0x33E430, not named in the map yet: `delete p` calls it with 1) */
extern "C" void func_0033E430(CSaveManager *save, int flags);

/* the last asynchronous command and its result, waited for before the next (ours) */
extern int data_004A37C0;
extern int data_004A37C8;
/* whether libmc is up (ours) */
extern unsigned char data_004A37B8;

/* a failure and a transfer, told to the menu (ours) */
inline void MemCardError()
{
    theGame.m_menuState.RunEventAnim("MEMCARD_ERROR", 0);
}

inline void MemCardStatus(int status)
{
    CValve *valve = CValve::Create("MEMCARD_STATUS_VALVE", (VALVE_TYPE)2);

    if (valve)
        SetValveValue(valve, status);
    theGame.m_menuState.RunEventAnim("MEMCARD_STATUS", 0);
}

CMemCard::CMemCard()
{
    m_configSave = new CSaveManager(&m_io);
    m_gameSave = new CSaveManager(&m_io);
}

CMemCard::~CMemCard()
{
    func_0033E430(m_configSave, 1);
    func_0033E430(m_gameSave, 1);
}

void CMemCard::Init()
{
    int type0, cmd0, result0, type1, cmd1, result1;

    if (!data_004A37B8) {
        if (!zSys.mcIrxLoaded)
            zSysLoadMcIRX();
        if (func_001B3970())
            MemCardError();
        data_004A37B8 = 1;
    }
    sceMcSync(0, &data_004A37C0, &data_004A37C8);
    sceMcGetInfo(0, 0, &type0, 0, 0);
    sceMcSync(0, &cmd0, &result0);
    sceMcGetInfo(1, 0, &type1, 0, 0);
    sceMcSync(0, &cmd1, &result1);
    MemCardStatus(0);
}

void CMemCard::Reset()
{
    int type0, cmd0, result0, type1, cmd1, result1;

    if (data_004A37B8) {
        data_004A37B8 = 0;
        if (!zSys.mcIrxLoaded)
            zSysLoadMcIRX();
        if (func_001B3970())
            MemCardError();
        data_004A37B8 = 1;
        sceMcSync(0, &data_004A37C0, &data_004A37C8);
        sceMcGetInfo(0, 0, &type0, 0, 0);
        sceMcSync(0, &cmd0, &result0);
        sceMcGetInfo(1, 0, &type1, 0, 0);
        sceMcSync(0, &cmd1, &result1);
        MemCardStatus(0);
    }
}

void CMemCard::Uninit()
{
    if (data_004A37B8) {
        if (zSys.mcIrxLoaded) {
            func_001B3B40();
            zSysUnloadMcIRX();
        }
        data_004A37B8 = 0;
    }
    MemCardStatus(0);
}

void CMemCard::SaveConfig(const char *name, const char *desc)
{
    sceMcSync(0, &data_004A37C0, &data_004A37C8);
    SelectPortSlot();
    if (!func_0033DFF0(m_configSave, name, 0xC, 0, desc, 0))
        MemCardError();
}

void CMemCard::LoadConfig(const char *name)
{
    sceMcSync(0, &data_004A37C0, &data_004A37C8);
    SelectPortSlot();
    if (!func_0033D6D0(m_configSave, name, 0xC))
        MemCardError();
}

/* the card's state: 0 ready, 2 none, 1 unformatted, 6 full (ours) */
int CMemCard::GetCardStatus()
{
    int status = 0;
    int tries = 2;
    int type, cmd, result, format;

    sceMcSync(0, &data_004A37C0, &data_004A37C8);
    do {
        if ((bool)sceMcGetInfo(m_selPort, m_selSlot, &type, 0, &format) < 0) {
            MemCardError();
            return 0;
        }
        sceMcSync(0, &cmd, &result);
        if (result < -9)
            status = 2;
        else if (type != 2)
            status = 2;
        else if (format == 0)
            status = 1;
        else if (result == -5)
            status = 6;
        tries = status == 0 ? 0 : tries - 1;
    } while (tries > 0);
    return status;
}

void CMemCard::DeleteScratchpad()
{
    int cmd, result;

    if (!m_io.IsOpen()) {
        sceMcSync(0, &data_004A37C0, &data_004A37C8);
        if (func_001B4A98(m_selPort, m_selSlot, "SCRATCHPAD.DAT") < 0) {
            MemCardError();
            return;
        }
        sceMcSync(0, &cmd, &result);
    }
}

void CMemCard::DeletePatchFile()
{
    int cmd, result;

    if (!m_io.IsOpen()) {
        sceMcSync(0, &data_004A37C0, &data_004A37C8);
        if (func_001B4A98(m_selPort, m_selSlot, "PATCHFILE.DAT") < 0) {
            MemCardError();
            return;
        }
        sceMcSync(0, &cmd, &result);
    }
}

void CMemCard::SaveGame(const char *name, char *desc)
{
    sceMcSync(0, &data_004A37C0, &data_004A37C8);
    SelectPortSlot();
    if (!func_0033DFF0(m_gameSave, name, 0xE, 0, "", desc))
        MemCardError();
}

void CMemCard::LoadGame(const char *name)
{
    sceMcSync(0, &data_004A37C0, &data_004A37C8);
    SelectPortSlot();
    if (!func_0033D6D0(m_gameSave, name, 0xE)) {
        MemCardError();
        return;
    }
    theGame.m_menuState.RunEventAnim("LOADED_FROM_MEMCARD", 0);
}

bool CMemCard::IsInserted()
{
    bool inserted = false;
    int tries = 2;
    int type, cmd, result;

    sceMcSync(0, &data_004A37C0, &data_004A37C8);
    do {
        if (sceMcGetInfo(m_selPort, m_selSlot, &type, 0, 0) < 0) {
            MemCardError();
            return inserted;
        }
        sceMcSync(0, &cmd, &result);
        inserted = result >= -9 && type == 2;
        tries = inserted ? 0 : tries - 1;
    } while (tries > 0);
    return inserted;
}

bool CMemCard::IsFormatted()
{
    bool formatted = false;
    int tries = 2;
    int format, cmd, result;

    sceMcSync(0, &data_004A37C0, &data_004A37C8);
    do {
        if (sceMcGetInfo(m_selPort, m_selSlot, 0, 0, &format) < 0) {
            MemCardError();
            return formatted;
        }
        sceMcSync(0, &cmd, &result);
        formatted = format != 0;
        tries = formatted ? 0 : tries - 1;
    } while (tries > 0);
    return formatted;
}

bool CMemCard::Format()
{
    int cmd, result;

    sceMcSync(0, &data_004A37C0, &data_004A37C8);
    if (func_001B49C8(m_selPort, m_selSlot) < 0) {
        MemCardError();
        return false;
    }
    sceMcSync(0, &cmd, &result);
    return result == 0;
}

int CMemCard::GetFreeSpace()
{
    int cmd, result;
    int free = 0;
    int tries = 2;

    sceMcSync(0, &data_004A37C0, &data_004A37C8);
    do {
        if (sceMcGetInfo(m_selPort, m_selSlot, 0, &free, 0) < 0) {
            MemCardError();
            return 0;
        }
        sceMcSync(0, &cmd, &result);
        tries = result > -10 ? 0 : tries - 1;
    } while (tries > 0);
    return free;
}

/* Sony's ASCII-to-Shift-JIS sample, for the icon's title (its tables at 0x3E0660 and 0x3E06E8) */
extern unsigned short data_003E0660[33][2];
extern unsigned short data_003E06E8[3][2];
extern "C" int printf(const char *fmt, ...);

unsigned short Ascii2Sjis(unsigned char ascii_code)
{
    unsigned char stmp;
    unsigned char stmp2 = 0;
    unsigned short sjis_code = 0;

    if (ascii_code >= 0x20 && ascii_code <= 0x2f)
        stmp2 = 1;
    else if (ascii_code >= 0x30 && ascii_code <= 0x39)
        stmp = 0;
    else if (ascii_code >= 0x3a && ascii_code <= 0x40)
        stmp2 = 11;
    else if (ascii_code >= 0x41 && ascii_code <= 0x5a)
        stmp = 1;
    else if (ascii_code >= 0x5b && ascii_code <= 0x60)
        stmp2 = 37;
    else if (ascii_code >= 0x61 && ascii_code <= 0x7a)
        stmp = 2;
    else if (ascii_code >= 0x7b && ascii_code <= 0x7e)
        stmp2 = 63;
    else {
        printf("bad ASCII code 0x%x\n", ascii_code);
        return 0;
    }
    if (stmp2)
        sjis_code = data_003E0660[ascii_code - 0x20 - (stmp2 - 1)][0];
    else
        sjis_code = data_003E06E8[stmp][0] + ascii_code - data_003E06E8[stmp][1];
    return sjis_code;
}

bool CMemCard::Exists(const char *name)
{
    bool found = false;
    sceMcTblGetDir table;
    int cmd, result;

    sceMcSync(0, &data_004A37C0, &data_004A37C8);
    if (sceMcGetDir(m_selPort, m_selSlot, name, 0, 1, &table) < 0) {
        MemCardError();
        return false;
    }
    sceMcSync(0, &cmd, &result);
    if (result > 0)
        found = true;
    return found;
}

bool CMemCard::mkdir(const char *dir)
{
    int cmd, result;

    sceMcSync(0, &data_004A37C0, &data_004A37C8);
    if (sceMcMkdir(m_selPort, m_selSlot, dir) < 0) {
        MemCardError();
        return false;
    }
    sceMcSync(0, &cmd, &result);
    if (result != 0 && result != -4)
        MemCardError();
    return true;
}

bool CMemCard::chdir(const char *dir)
{
    int cmd, result;

    sceMcSync(0, &data_004A37C0, &data_004A37C8);
    if (func_001B4888(m_selPort, m_selSlot, dir, 0) < 0) {
        MemCardError();
        return false;
    }
    sceMcSync(0, &cmd, &result);
    if (result != 0)
        MemCardError();
    return true;
}

bool CMemCardIO::Open(const char *name, unsigned int mode)
{
    int mcmode = 0;
    sceMcTblGetDir table;
    int cmd, result;
    bool open;

    if (mode & 1)
        mcmode |= 1;
    if (mode & 2)
        mcmode |= 2;
    if (mode & 4)
        mcmode |= 3;
    if (mode & 8)
        mcmode |= 0x200;
    if ((mode & 0x10) && (mode & 8)) {
        func_001B4A98(m_port, m_slot, name);
        sceMcSync(0, &cmd, &result);
        m_size = 0;
    }
    m_mode = mode;
    if (sceMcOpen(m_port, m_slot, name, mcmode) < 0) {
        MemCardError();
        return false;
    }
    sceMcSync(0, &cmd, &m_fd);
    if (IsOpen() && (mode & 5)) {
        if (sceMcGetDir(m_port, m_slot, name, 0, 1, &table) < 0) {
            MemCardError();
            return false;
        }
        sceMcSync(0, &cmd, &result);
        if (result > 0)
            m_size = table.FileSizeByte;
    }
    open = IsOpen();
    if (open)
        MemCardStatus(1);
    return open;
}

void CMemCardIO::Close()
{
    int cmd, result;

    if (IsOpen()) {
        if (sceMcClose(m_fd) < 0) {
            MemCardError();
            return;
        }
        sceMcSync(0, &cmd, &result);
        m_fd = -1;
        MemCardStatus(0);
    }
}

bool CMemCardIO::IsOpen() const
{
    return m_fd >= 0;
}

int CMemCardIO::GetSize() const
{
    return m_size;
}

int CMemCardIO::ftell() const
{
    int pos = 0;
    int cmd, result;

    if (sceMcSeek(m_fd, 0, 1) < 0) {
        MemCardError();
        return 0;
    }
    sceMcSync(0, &cmd, &result);
    if (result >= 0)
        pos = result;
    return pos;
}

int CMemCardIO::fseek(int offset, int whence)
{
    int origin = 0;
    int pos = 0;
    int cmd, result;

    if (whence == 1)
        origin = 1;
    else if (whence == 2)
        origin = 2;
    if (sceMcSeek(m_fd, offset, origin) < 0) {
        MemCardError();
        return 0;
    }
    sceMcSync(0, &cmd, &result);
    if (result >= 0)
        pos = result;
    return pos;
}

int CMemCardIO::fread(int size, void **buf)
{
    *buf = 0;
    return 0;
}

int CMemCardIO::fread(void *buf, int size)
{
    int n = 0;
    int cmd, result;

    MemCardStatus(2);
    if (sceMcRead(m_fd, buf, size) < 0) {
        MemCardError();
        return 0;
    }
    sceMcSync(0, &cmd, &result);
    if (result >= 0)
        n = result;
    return n;
}

int CMemCardIO::fwrite(const void *buf, int size)
{
    int n = 0;
    int cmd, result;

    MemCardStatus(3);
    if (sceMcWrite(m_fd, buf, size) < 0) {
        MemCardError();
        return 0;
    }
    sceMcSync(0, &cmd, &result);
    if (result >= 0)
        n = result;
    return n;
}

void CMemCardIO::fflush()
{
    int cmd, result;

    if (func_001B4BB0(m_fd) < 0) {
        MemCardError();
        return;
    }
    sceMcSync(0, &cmd, &result);
}

int CMemCardIO::freadAsync(void *buf, int size)
{
    MemCardStatus(2);
    if (sceMcRead(m_fd, buf, size) < 0) {
        MemCardError();
        return 0;
    }
    return 5;
}

int CMemCardIO::fwriteAsync(const void *buf, int size)
{
    MemCardStatus(3);
    if (sceMcWrite(m_fd, buf, size) < 0) {
        MemCardError();
        return 0;
    }
    return 6;
}

bool CMemCardIO::fsyncAsync(int mode, int *done)
{
    bool finished = false;
    int cmd, result;

    switch (sceMcSync(1, &cmd, &result)) {
    case 0:
        *done = 0;
        break;
    case -1:
    case 1:
        finished = true;
        *done = result;
        break;
    }
    return finished;
}

unsigned int CMemCardIO::GetMode() const
{
    return m_mode;
}
