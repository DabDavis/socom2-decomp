/* The engine's input: CInput (the pads and the keyboard, all static), CPad (one DualShock 2,
 * read through libpad2), its sticks' response (CStickType) and the USB keyboard (CKeyboard).
 * SOCOM 1's DWARF gives CPad (0x194 bytes), CStickType (0x20), PAD_BUTTON and KEY_STATE; SOCOM
 * II's CPad is 0x2D4 bytes (CreatePad news 724) and its CStickType 0x28 (the constructor reads
 * each at 0x284 + 0x28 * i), laid out here from the code that reads them -- the names past SOCOM
 * 1's are ours. The directory is SOCOM 1's (zin_cheats.cpp); the header's name is ours. */
#ifndef ZINPUT_H
#define ZINPUT_H

/* the buttons, a byte each (SOCOM 1's DWARF) */
enum PAD_BUTTON {
    PAD_START, PAD_SELECT, PAD_RIGHT, PAD_LEFT, PAD_UP, PAD_DOWN, PAD_TRIANGLE, PAD_CIRCLE,
    PAD_SQUARE, PAD_CROSS, PAD_TRIGGER_R1, PAD_TRIGGER_R2, PAD_TRIGGER_L1, PAD_TRIGGER_L2,
    PAD_RIGHT_STICK, PAD_LEFT_STICK, PAD_MAX_BUTTONS
};

/* a button's state (SOCOM 1's DWARF) */
enum KEY_STATE { KEY_UP, KEY_FALLING, KEY_DOWN, KEY_RISING };

/* the IOP reboot notifier: callbacks run before (phase 0) and after a reboot (SOCOM 1's; only the
 * registration declared) */
class CIOPManager {
public:
    enum RebootPhase { REBOOT_BEGIN, REBOOT_END };
    enum RebootMode { REBOOT_MODE };
    typedef bool (*RebootCallback)(RebootPhase phase, RebootMode mode, void *data);
    int RegisterRebootCallback(RebootCallback cb, void *data);
};

extern CIOPManager theIOPManager;

/* a stick's response (SOCOM 1's DWARF, eight bytes longer in SOCOM II) */
class CStickType {
public:
    void ReadControlFile(int stick);

    float m_minStickVelocity;   // 0x0
    float m_absoluteDeadZone;   // 0x4
    float m_velocityDeadZone;   // 0x8
    float m_valueForConstant;   // 0xc
    float m_valueForN;   // 0x10
    unsigned char m_velocitySeparate;   // 0x14
    unsigned char m_deadZoneSeparate;   // 0x15
    float m_minZeroVelocityTime;   // 0x18
    unsigned char m_normalize;   // 0x1c
    unsigned char m_dilate;   // 0x1d
    unsigned char m_socom2_1e;   // 0x1e: SOCOM II's (the names ours)
    float m_socom2_20;   // 0x20
    unsigned char m_socom2_24;   // 0x24
    unsigned char m_socom2_25;   // 0x25
};   // 0x28 bytes

/* the pad's dead zones are kept scaled by this (SOCOM 1's global) */
extern float deadZoneScaleFactor;

class CPad {
public:
    CPad(int port, int slot);
    ~CPad();
    void UpdateButton(PAD_BUTTON button, unsigned int down, float dt);
    void SetButtonState(PAD_BUTTON button, KEY_STATE state);
    bool GetTwoButtons(PAD_BUTTON held, PAD_BUTTON pressed);
    bool GetDoubleClick(PAD_BUTTON button);
    void SetPreset(int stick, int preset);
    float GetDeadZone(int stick);
    void SetDeadZone(int stick, float zone);
    float GetAcceleration(int stick);
    void SetAcceleration(int stick, float accel);
    void Tick(float dt);
    /* SOCOM II's takes whether to read the pad a few frames to settle it (the name SOCOM 1's) */
    void Flush(bool settle);
    bool IsOpen() const;

    unsigned char m_0;   // 0x0
    unsigned char m_state[PAD_MAX_BUTTONS];   // 0x1: KEY_STATE each
    unsigned char _11[0x14 - 0x11];
    float m_held[PAD_MAX_BUTTONS];   // 0x14: each button's pressure scaled to [0, 1] (the name ours)
    unsigned char _54[0x74 - 0x54];
    /* a button's click timer: -2 a double click to be taken, -3 none (SOCOM 1's m_click) */
    float m_click[PAD_MAX_BUTTONS];   // 0x74
    unsigned char m_ActData[2];   // 0xb4: the motors (Flush clears both; SOCOM 1 kept six)
    unsigned char _b6[0xb8 - 0xb6];
    /* the port's open parameters, handed to libpad2 by the constructor (the names ours) */
    int m_socketType;   // 0xb8: 2
    int m_socketPort;   // 0xbc
    int m_socketSlot;   // 0xc0
    int m_socketC4;   // 0xc4
    unsigned char m_socketC8;   // 0xc8
    unsigned char _c9[0xd8 - 0xc9];
    int m_port;   // 0xd8: the libpad2 port, -1 when closed (the name ours)
    unsigned char _dc[0x284 - 0xdc];
    CStickType m_stickType[2];   // 0x284
};   // 0x2d4 bytes

class CKeyboard {
public:
    CKeyboard();
    ~CKeyboard();
    void Tick(float dt);

    unsigned char m_0;   // 0x0: cleared by the constructor, in both builds (the name ours)
};   // 1 byte

/* the pads, the keyboard and their IOP modules (SOCOM 1's statics; SOCOM II reads one pad) */
class CInput {
public:
    enum { NUM_PADS = 1 };

    static void DeletePad(unsigned int n);
    static CPad *CreatePad(unsigned int n);
    static void Flush();
    static void Tick(float dt);
    static void Uninit();
    static int Init();
    static bool IOPRebootHandler(CIOPManager::RebootPhase phase, CIOPManager::RebootMode mode,
                                 void *data);
    static void ClosePadIO();
    static int OpenPadIO();

    static CPad *m_pads[2];
    static bool m_init;
    static CKeyboard *m_keyboard;
};

/* the keyboard reader's set-up and its new SOCOM II teardown (SOCOM 1's InitKeyreadStuff; the
 * other name ours) */
void InitKeyreadStuff();
void UninitKeyreadStuff();

#endif
