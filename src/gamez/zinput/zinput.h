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

/* the IOP reboot notifier: callbacks run before (phase 0) and after a reboot (SOCOM 1's; the
 * registration and its removal declared -- util_main.cpp defines them on its own copy of the class) */
class CIOPManager {
public:
    enum RebootPhase { REBOOT_BEGIN, REBOOT_END };
    enum RebootMode { REBOOT_MODE };
    typedef bool (*RebootCallback)(RebootPhase phase, RebootMode mode, void *data);
    int RegisterRebootCallback(RebootCallback cb, void *data);
    void UnRegisterRebootCallback(int id);
    /* the IOP rebooted for what comes next (util_main.cpp's, 0x34F9B0) */
    void RebootIOP(RebootMode mode);
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
    unsigned char m_socom2_24[2];   // 0x24: an axis's power curve and slew limit on (per axis; the name ours)
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
    /* a stick's travel stretched from square to circle (SOCOM 1's) */
    void Dilate(float &x, float &y, int stick);
    /* a resting stick's axes zeroed (SOCOM 1's) */
    void CheckStickVelocity(float &x, float &y, int stick);
    void SetPreset(int stick, int preset);
    float GetDeadZone(int stick);
    void SetDeadZone(int stick, float zone);
    float GetAcceleration(int stick);
    void SetAcceleration(int stick, float accel);
    void Tick(float dt);
    /* an axis of the sticks as the game reads them (the free camera's; the name ours) */
    float GetStick(int i) const { return m_220[i]; }
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
    int m_dc;   // 0xdc (Flush clears these and the block after them; the names ours)
    int m_e0;   // 0xe0
    unsigned char m_e4[256];   // 0xe4
    unsigned char m_1e4[0x14];   // 0x1e4: the actuators' profile (the name ours)
    unsigned char m_1f8[0x14];   // 0x1f8: the buttons' profile (the name ours)
    unsigned char m_swapSticks;   // 0x20c (SOCOM 1's; the constructor clears it)
    unsigned char _20d[3];
    union {
        int m_210[4];   // 0x210: Flush's view, a word each
        float m_stick[4];   // 0x210: left x, y, right x, y (SOCOM 1's m_LeftStick, m_RightStick)
    };
    float m_220[4];   // 0x220: the sticks as the game reads them (Flush copies the last into the other three)
    union {
        int m_230[4];   // 0x230: SOCOM II's, the constructor's view
        float m_slew[4];   // 0x230: each axis's value held for the slew limit (the name ours)
    };
    unsigned char m_lastStickRaw[4];   // 0x240 (SOCOM 1's DWARF, 0x130 on from there)
    float m_lastStickVal[4];   // 0x244
    float m_maxStick[4];   // 0x254
    float m_minStick[4];   // 0x264
    float m_deadTime[4];   // 0x274
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
