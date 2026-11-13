/* zin_pad.cpp -- CPad, one DualShock 2 read through libpad2: its buttons' states and clicks, its
 * sticks' dead zones and response presets, its port opened and closed; and CInput's pad IO (OVL1
 * 0x2D9FF0-0x2DBD3F). SOCOM 1's order: UpdateButton first, then the button queries, the stick code,
 * CStickType::ReadControlFile and the presets, Tick, the destructor, Flush, IsOpen and the
 * constructor, CInput::ClosePadIO and OpenPadIO last. SOCOM II drops GetSingleClick and the
 * sensitivity pair, adds a pressure to UpdateButton and a settle flag to Flush, and keeps one
 * libpad2 socket per pad. The file name is ours. Defined in reverse of their image order
 * (-inline deferred). */

#include <string.h>

#include "zinput.h"

/* libpad2 and the pad modules (MAIN, not named yet: the socket's creation and deletion, the
 * library's init and end, the modules' load and unload, by their use here); the vertical sync wait */
extern "C" int scePad2CreateSocket(void *param, void *dmabuf);
extern "C" int scePad2DeleteSocket(int port);
extern "C" int scePad2Init(int mode);
extern "C" int scePad2End();
extern "C" int sceDbcInit();
extern "C" int sceDbcEnd();
extern "C" int sceGsSyncV(int mode);
/* libpad2's (MAIN, not named yet): the socket's state, its read, one button's value out of the
 * read, the two profiles, the actuators' send */
extern "C" int scePad2GetState(int port);
extern "C" int scePad2Read(int port, void *data);
extern "C" int scePad2GetButtonInfo(int port, void *data, int button);
extern "C" int scePad2GetButtonProfile(int port, void *profile);
extern "C" int sceVibGetProfile(int port, void *profile);
extern "C" int sceVibSetActParam(int port, int a, void *profile, int b, void *act);
/* the math library's (MAIN) */
extern "C" float sinf(float);
extern "C" float atan2f(float, float);
extern "C" float fabsf(float);
extern "C" float sqrtf(float);
/* a value held within two bounds, whether it moved (zmath.h's template, SOCOM 1's _saturate<float>):
 * declared only, so the call is to the instance an earlier unit emits (zanim_cmd_tick.cpp's, OVL1
 * 0x267A60) */
template <class T> bool _saturate(T *v, const T &lo, const T &hi);

/* the pads' DMA buffers, 256 bytes each, and the time step the pad code runs on (SOCOM 1's
 * globals, by name) */
extern unsigned char m_DmaBufArray[2][256];
extern float pad_dt;
/* the pressure's scale and the acceleration's range (constants the code loads from data, as an
 * inline's reference parameters bound to literals are; not named) */
extern float data_003DF268;
extern float data_003DF208;
extern float data_003DF210;

/* a value inside (-range, range) (inline: CheckStickVelocity materialises each test; the name
 * ours) */
inline bool InsideRange(float v, float range)
{
    if (v < range && -v < range)
        return true;
    return false;
}

/* the IOP modules started, libpad2 set up, the pad made (SOCOM 1's) */
int CInput::OpenPadIO()
{
    sceDbcInit();
    scePad2Init(0);
    CreatePad(0);
    return 0;
}

/* the pad deleted, libpad2 and its modules shut down (SOCOM 1's) */
void CInput::ClosePadIO()
{
    DeletePad(0);
    scePad2End();
    sceDbcEnd();
}

/* the pad on a port (0 or 1) opened through libpad2, its sticks' state cleared and their types
 * read, the pad flushed and settled (SOCOM 1's) */
CPad::CPad(int port, int slot)
{
    int p;

    m_port = -1;
    m_swapSticks = 0;
    if (port == 0)
        p = 0;
    else if (port == 1)
        p = 1;
    else
        return;
    m_socketType = 2;
    m_socketPort = p;
    m_socketSlot = slot;
    m_socketC4 = 0;
    m_socketC8 = 0;
    m_port = scePad2CreateSocket(&m_socketType, m_DmaBufArray[port]);
    m_0 = 0;
    for (int i = 0; i < 4; i++) {
        m_lastStickVal[i] = 0.0f;
        m_lastStickRaw[i] = 0;
        m_deadTime[i] = 0.0f;
        m_maxStick[i] = 1.5626f;
        m_minStick[i] = -1.5626f;
        m_230[i] = 0;
    }
    for (int i = 0; i < 2; i++)
        m_stickType[i].ReadControlFile(i);
    Flush(true);
}

/* the port open (SOCOM 1's) */
bool CPad::IsOpen() const
{
    return m_port >= 0;
}

/* the state cleared, the pad read a few frames to settle when asked, then once, and every click
 * timer reset (SOCOM 1's) */
void CPad::Flush(bool settle)
{
    m_dc = m_e0 = 0;
    m_ActData[0] = m_ActData[1] = 0;
    m_210[0] = m_210[1] = m_210[2] = m_210[3] = 0;
    m_220[0] = m_220[1] = m_220[2] = m_220[3];
    memset(m_e4, 0, sizeof m_e4);
    memset(m_state, 0, sizeof m_state);
    memset(m_held, 0, sizeof m_held);
    if (settle)
        for (unsigned int i = 0; i < 3; i++) {
            Tick(1.0f / 30.0f);
            sceGsSyncV(0);
        }
    Tick(1.0f / 30.0f);
    for (int i = 0; i < PAD_MAX_BUTTONS; i++)
        m_click[i] = -3.0f;
}

/* the socket deleted (SOCOM 1's closed the port) */
CPad::~CPad()
{
    if (IsOpen()) {
        scePad2DeleteSocket(m_port);
        m_port = -1;
    }
}

/* one stick axis from its raw byte (SOCOM 1's expression) */
inline float StickValue(unsigned char raw)
{
    return 0.007843138f * (127.5f - raw);
}

/* an axis through the stick's power curve (SOCOM 1's, for every stick; SOCOM II's asks per axis) */
#define RESPONSE(v_, stick_) do { \
        float v = m_stick[v_]; \
        float sign = v < 0.0f ? -1.0f : 1.0f; \
        int n = (int)m_stickType[stick_].m_valueForN; \
        float r = v * m_stickType[stick_].m_valueForConstant; \
        for (int j = 1; j < n; j++) \
            r *= v; \
        m_stick[v_] = sign * fabsf(r); \
    } while (0)

/* the frame's read: the socket's state and phase (the profiles once, then the read and the
 * actuators), the raw sticks through their velocity test, dead zone, stretch, dilation, power curve
 * and SOCOM II's slew limit, the sticks' copy, the buttons (SOCOM 1's, read through libpad2) */
void CPad::Tick(float dt)
{
    int read = 0;

    pad_dt = dt;
    m_dc = scePad2GetState(m_port);
    if (m_dc == 1) {
        switch (m_e0) {
        case 0: {
            int a = scePad2GetButtonProfile(m_port, m_1f8);
            int b = sceVibGetProfile(m_port, m_1e4);

            if (a >= 0 && b >= 0)
                m_e0++;
            break;
        }
        case 1:
            read = scePad2Read(m_port, m_e4);
            if (m_1e4[0]) {
                short act = (m_ActData[1] << 1) | m_ActData[0];

                sceVibSetActParam(m_port, 1, m_1e4, 2, &act);
            }
            break;
        }
    } else {
        m_ActData[0] = m_ActData[1] = 0;
        m_210[0] = m_210[1] = m_210[2] = m_210[3] = 0;
        m_220[0] = m_220[1] = m_220[2] = m_220[3];
        memset(m_e4, 0, sizeof m_e4);
        memset(m_state, 0, sizeof m_state);
        memset(m_held, 0, sizeof m_held);
        m_e0 = 0;
    }
    if (read) {
        m_lastStickRaw[0] = scePad2GetButtonInfo(m_port, m_e4, 18);
        m_lastStickRaw[1] = scePad2GetButtonInfo(m_port, m_e4, 19);
        m_lastStickRaw[2] = scePad2GetButtonInfo(m_port, m_e4, 16);
        m_lastStickRaw[3] = scePad2GetButtonInfo(m_port, m_e4, 17);
        m_stick[0] = StickValue(scePad2GetButtonInfo(m_port, m_e4, 18));
        m_stick[1] = StickValue(scePad2GetButtonInfo(m_port, m_e4, 19));
        m_stick[2] = StickValue(scePad2GetButtonInfo(m_port, m_e4, 16));
        m_stick[3] = StickValue(scePad2GetButtonInfo(m_port, m_e4, 17));
        CheckStickVelocity(m_stick[0], m_stick[1], 0);
        CheckStickVelocity(m_stick[2], m_stick[3], 1);
        if (InsideRange(m_stick[0], m_stickType[0].m_absoluteDeadZone))
            m_stick[0] = 0.0f;
        if (InsideRange(m_stick[1], m_stickType[0].m_absoluteDeadZone))
            m_stick[1] = 0.0f;
        if (InsideRange(m_stick[2], m_stickType[1].m_absoluteDeadZone))
            m_stick[2] = 0.0f;
        if (InsideRange(m_stick[3], m_stickType[1].m_absoluteDeadZone))
            m_stick[3] = 0.0f;
        if (m_stickType[0].m_normalize) {
            if (m_stick[0] > 0.0f)
                m_stick[0] -= m_stickType[0].m_absoluteDeadZone;
            if (m_stick[1] > 0.0f)
                m_stick[1] -= m_stickType[0].m_absoluteDeadZone;
            if (m_stick[0] < 0.0f)
                m_stick[0] += m_stickType[0].m_absoluteDeadZone;
            if (m_stick[1] < 0.0f)
                m_stick[1] += m_stickType[0].m_absoluteDeadZone;
            m_stick[0] *= 1.0f / (1.0f - m_stickType[0].m_absoluteDeadZone);
            m_stick[1] *= 1.0f / (1.0f - m_stickType[0].m_absoluteDeadZone);
            _saturate(&m_stick[0], -1.0f, 1.0f);
            _saturate(&m_stick[1], -1.0f, 1.0f);
        }
        if (m_stickType[1].m_normalize) {
            if (m_stick[2] > 0.0f)
                m_stick[2] -= m_stickType[1].m_absoluteDeadZone;
            if (m_stick[3] > 0.0f)
                m_stick[3] -= m_stickType[1].m_absoluteDeadZone;
            if (m_stick[2] < 0.0f)
                m_stick[2] += m_stickType[1].m_absoluteDeadZone;
            if (m_stick[3] < 0.0f)
                m_stick[3] += m_stickType[1].m_absoluteDeadZone;
            m_stick[2] *= 1.0f / (1.0f - m_stickType[1].m_absoluteDeadZone);
            m_stick[3] *= 1.0f / (1.0f - m_stickType[1].m_absoluteDeadZone);
            _saturate(&m_stick[2], -1.0f, 1.0f);
            _saturate(&m_stick[3], -1.0f, 1.0f);
        }
        Dilate(m_stick[0], m_stick[1], 0);
        Dilate(m_stick[2], m_stick[3], 1);
        if (m_stickType[0].m_socom2_24[0])
            RESPONSE(0, 0);
        if (m_stickType[0].m_socom2_24[1])
            RESPONSE(1, 0);
        if (m_stickType[1].m_socom2_24[0])
            RESPONSE(2, 1);
        if (m_stickType[1].m_socom2_24[1])
            RESPONSE(3, 1);

        for (int i = 0; i < 4; i++) {
            float v;
            float *p;
            int stick, axis;

            switch (i) {
            case 0:
                stick = 0;
                v = m_stick[0];
                axis = 0;
                p = &m_stick[0];
                break;
            case 1:
                axis = 1;
                v = m_stick[1];
                stick = 0;
                p = &m_stick[1];
                break;
            case 2:
                stick = 1;
                v = m_stick[2];
                axis = 0;
                p = &m_stick[2];
                break;
            case 3:
                stick = 1;
                v = m_stick[3];
                axis = 1;
                p = &m_stick[3];
                break;
            }
            if (m_stickType[stick].m_socom2_24[axis] && m_stickType[stick].m_socom2_1e) {
                float k = m_stickType[stick].m_socom2_20;
                float a = dt + (1.0f - dt) * (1.0f - (0.87f + 0.13f * k));

                if (dt > 1.0f)
                    a = 1.0f;
                if (k > 0.5f)
                    a *= 1.0f / (0.5f + k);

                float sign = v < 0.0f ? -1.0f : 1.0f;
                float *q = &m_slew[stick * 2 + axis];

                if (sign * *q <= 0.0f)
                    *q = 0.0f;

                float fv = fabsf(v);

                if (fv > a + fabsf(*q)) {
                    float m = sign * a;

                    *p = m + *q;
                }
                _saturate(p, -1.0f, 1.0f);
                *q = *p;
            }
        }
        if (m_swapSticks) {
            float x;

            x = m_stick[0];
            m_stick[0] = m_stick[2];
            m_stick[2] = x;
            x = m_stick[1];
            m_stick[1] = m_stick[3];
            m_stick[3] = x;
        }
        m_220[2] = m_stick[2];
        m_220[3] = m_stick[3];
        m_220[0] = m_stick[0];
        m_220[1] = m_stick[1];
        if (m_e4[4] || m_e4[5] || m_e4[6] || m_e4[7])
            m_0 = 1;
        else
            m_0 = 0;
        float press;

        UpdateButton(PAD_START, scePad2GetButtonInfo(m_port, m_e4, 3), 0.0f);
        UpdateButton(PAD_SELECT, scePad2GetButtonInfo(m_port, m_e4, 0), 0.0f);
        press = scePad2GetButtonInfo(m_port, m_e4, 20);
        UpdateButton(PAD_RIGHT, scePad2GetButtonInfo(m_port, m_e4, 5), press);
        press = scePad2GetButtonInfo(m_port, m_e4, 21);
        UpdateButton(PAD_LEFT, scePad2GetButtonInfo(m_port, m_e4, 7), press);
        press = scePad2GetButtonInfo(m_port, m_e4, 22);
        UpdateButton(PAD_UP, scePad2GetButtonInfo(m_port, m_e4, 4), press);
        press = scePad2GetButtonInfo(m_port, m_e4, 23);
        UpdateButton(PAD_DOWN, scePad2GetButtonInfo(m_port, m_e4, 6), press);
        press = scePad2GetButtonInfo(m_port, m_e4, 24);
        UpdateButton(PAD_TRIANGLE, scePad2GetButtonInfo(m_port, m_e4, 12), press);
        press = scePad2GetButtonInfo(m_port, m_e4, 25);
        UpdateButton(PAD_CIRCLE, scePad2GetButtonInfo(m_port, m_e4, 13), press);
        press = scePad2GetButtonInfo(m_port, m_e4, 27);
        UpdateButton(PAD_SQUARE, scePad2GetButtonInfo(m_port, m_e4, 15), press);
        press = scePad2GetButtonInfo(m_port, m_e4, 26);
        UpdateButton(PAD_CROSS, scePad2GetButtonInfo(m_port, m_e4, 14), press);
        press = scePad2GetButtonInfo(m_port, m_e4, 29);
        UpdateButton(PAD_TRIGGER_R1, scePad2GetButtonInfo(m_port, m_e4, 11), press);
        press = scePad2GetButtonInfo(m_port, m_e4, 31);
        UpdateButton(PAD_TRIGGER_R2, scePad2GetButtonInfo(m_port, m_e4, 9), press);
        press = scePad2GetButtonInfo(m_port, m_e4, 28);
        UpdateButton(PAD_TRIGGER_L1, scePad2GetButtonInfo(m_port, m_e4, 10), press);
        press = scePad2GetButtonInfo(m_port, m_e4, 30);
        UpdateButton(PAD_TRIGGER_L2, scePad2GetButtonInfo(m_port, m_e4, 8), press);
        UpdateButton(PAD_LEFT_STICK, scePad2GetButtonInfo(m_port, m_e4, 1), 0.0f);
        UpdateButton(PAD_RIGHT_STICK, scePad2GetButtonInfo(m_port, m_e4, 2), 0.0f);
    }
}

/* the stick's response to an acceleration in [0, 1]: a two-piece curve into its constant (SOCOM
 * 1's) */
void CPad::SetAcceleration(int stick, float accel)
{
    if (accel < 0.5f)
        accel *= 0.6f;
    else
        accel = 0.3f + 1.4f * (accel - 0.5f);
    m_stickType[stick].m_valueForConstant = data_003DF208 + accel * (data_003DF210 - data_003DF208);
}

/* the stick's acceleration back from its constant (SOCOM 1's) */
float CPad::GetAcceleration(int stick)
{
    float x;

    x = (m_stickType[stick].m_valueForConstant - data_003DF208);
    x /= data_003DF210 - data_003DF208;

    if (x < 0.5f)
        x = x / 0.6f;
    else
        x = 0.5f + 0.71428573f * (x - 0.3f);
    return x;
}

/* the stick's dead zone, kept scaled (SOCOM 1's) */
void CPad::SetDeadZone(int stick, float zone)
{
    m_stickType[stick].m_absoluteDeadZone = zone / deadZoneScaleFactor;
}

float CPad::GetDeadZone(int stick)
{
    return m_stickType[stick].m_absoluteDeadZone * deadZoneScaleFactor;
}

/* one of three response presets for a stick (SOCOM 1's) */
void CPad::SetPreset(int stick, int preset)
{
    if (preset == 0) {
        m_stickType[stick].m_valueForConstant = 0.65f;
        m_stickType[stick].m_valueForN = 3.0f;
        m_stickType[stick].m_absoluteDeadZone = 0.3f;
        m_stickType[stick].m_socom2_20 = 0.75f;
    } else if (preset == 1) {
        m_stickType[stick].m_valueForConstant = 0.82f;
        m_stickType[stick].m_valueForN = 3.0f;
        m_stickType[stick].m_absoluteDeadZone = 0.16f;
        m_stickType[stick].m_socom2_20 = 0.5f;
    } else if (preset == 2) {
        m_stickType[stick].m_valueForConstant = 1.0f;
        m_stickType[stick].m_valueForN = 3.0f;
        m_stickType[stick].m_absoluteDeadZone = 0.16f;
        m_stickType[stick].m_socom2_20 = 0.25f;
    }
}

/* the stick's response, the left (0) and the right (1) alike but for a few settings (SOCOM 1's
 * read a control file) */
void CStickType::ReadControlFile(int stick)
{
    if (stick == 0) {
        m_minStickVelocity = 0.01640625f;
        m_velocityDeadZone = 0.0099999998f;
        m_absoluteDeadZone = 0.33000001f;
        m_valueForConstant = 1.0f;
        m_valueForN = 1.0f;
        m_velocitySeparate = 1;
        m_deadZoneSeparate = 1;
        m_minZeroVelocityTime = 0.2f;
        m_normalize = 1;
        m_dilate = 1;
        m_socom2_1e = 0;
        m_socom2_20 = 1.0f;
        m_socom2_24[0] = 0;
        m_socom2_24[1] = 0;
    } else {
        m_minStickVelocity = 0.01640625f;
        m_velocityDeadZone = 0.0099999998f;
        m_absoluteDeadZone = 0.33000001f;
        m_valueForConstant = 1.0f;
        m_valueForN = 3.0f;
        m_velocitySeparate = 0;
        m_deadZoneSeparate = 1;
        m_minZeroVelocityTime = 0.40000001f;
        m_normalize = 1;
        m_dilate = 1;
        m_socom2_20 = 1.0f;
        m_socom2_1e = 1;
        m_socom2_24[0] = 1;
        m_socom2_24[1] = 1;
    }
}

/* a stick resting: an axis inside its velocity dead zone that has barely moved for long enough
 * is zeroed -- both axes together unless they are taken separately (SOCOM 1's; its test of the
 * pair reads the x axis twice, as SOCOM 1's does).
 * OPEN, 2 instructions (0x238 of 0x240): the target indexes the y axis by i + 1 computed and scaled
 * on its own (addiu, sll -- for the last value, the dead time and the final store alike), where MW
 * folds the +1 into the offsets here; the flags take t1/t0 there. Not the flags' type or
 * declaration order, nor an index variable of its own; round 18: `int j = i + 1` (first or before the
 * second axis) computes it apart but goes 0x248, an axis helper inlined twice 124 off
 * (build/scratch/core/v_csv.py). Round 19: MW folds the +1 of `[v + 1]` into the displacement only
 * when v is a plain variable at reassociation -- a named, const or block-local `j = i + 1` is
 * propagated and folded alike (0x238), while `[stick * 2 + 1]` with `int i = stick * 2` kept for the
 * x axis is left unfolded, CSE'd onto i (addiu, sll) at the target's size 0x240 -- but computes
 * stick * 40 ahead of i and colours i t2 (109 off; no placement of i and the offset's temporary
 * gives the target's v1/t5 in that graph): build/scratch/core/v_csv19*.py */
void CPad::CheckStickVelocity(float &x, float &y, int stick)
{
    int i = stick * 2;
    bool zeroX = false;
    bool zeroY = false;

    if (InsideRange(x, m_stickType[stick].m_velocityDeadZone)) {
        bool still = false;

        if (InsideRange(x - m_lastStickVal[i], m_stickType[stick].m_minStickVelocity))
            still = true;
        if (still) {
            m_deadTime[i] += pad_dt;
            if (m_deadTime[i] > m_stickType[stick].m_minZeroVelocityTime)
                zeroX = true;
        } else
            m_deadTime[i] = 0.0f;
    }
    if (InsideRange(y, m_stickType[stick].m_velocityDeadZone)) {
        bool still = false;

        if (InsideRange(y - m_lastStickVal[i + 1], m_stickType[stick].m_minStickVelocity))
            still = true;
        if (still) {
            m_deadTime[i + 1] += pad_dt;
            if (m_deadTime[i + 1] > m_stickType[stick].m_minZeroVelocityTime)
                zeroY = true;
        } else
            m_deadTime[i + 1] = 0.0f;
    }
    m_lastStickVal[i] = x;
    m_lastStickVal[i + 1] = y;
    if (!m_stickType[stick].m_velocitySeparate && !(zeroX && zeroX)) {
        zeroX = false;
        zeroY = false;
    }
    if (zeroX)
        x = 0.0f;
    if (zeroY)
        y = 0.0f;
}

/* a stick's square travel stretched toward its circle, by the angle's sine (dilation 0) or the
 * axes' ratio (1, the one used), then held to [-1, 1] (SOCOM 1's; its choice a function-local
 * static, initialised statically here where SOCOM 1's had a guard) */
void CPad::Dilate(float &x, float &y, int stick)
{
    static int which_dilation = 1;

    if (m_stickType[stick].m_dilate) {
        float k = 1.0f;

        if (which_dilation == 0) {
            float s = sinf(2.0f * atan2f(y, x));

            k = 1.0f / (1.0f + 0.28f * s * s);
        } else if (which_dilation == 1) {
            int ax = fabsf(x);
            int ay = fabsf(y);

            if (ay < ax)
                k = y / x;
            else if (ax < ay)
                k = x / y;
            k = sqrtf(1.0f + fabsf(k));
        }
        x *= k;
        y *= k;
        _saturate(&x, -1.0f, 1.0f);
        _saturate(&y, -1.0f, 1.0f);
    }
}

/* a double click taken, once (SOCOM 1's) */
bool CPad::GetDoubleClick(PAD_BUTTON button)
{
    bool r = false;

    if (m_click[button] == -2.0f) {
        m_click[button] = -3.0f;
        r = true;
    }
    return r;
}

/* one button held while another goes down (SOCOM 1's) */
bool CPad::GetTwoButtons(PAD_BUTTON held, PAD_BUTTON pressed)
{
    return m_state[held] == KEY_DOWN && m_state[pressed] == KEY_FALLING;
}

/* a button's state forced, its click timer reset (SOCOM 1's) */
void CPad::SetButtonState(PAD_BUTTON button, KEY_STATE state)
{
    unsigned char *s = &m_state[button];

    *s = state;
    m_click[button] = -3.0f;
}

/* a button's state stepped by whether it is down, its pressure kept, its click timer run: a press
 * within a quarter second of the last makes a double click (SOCOM 1's, the pressure SOCOM II's) */
void CPad::UpdateButton(PAD_BUTTON button, unsigned int down, float pressure)
{
    unsigned char *state = &m_state[button];

    if (!down) {
        switch (*state) {
        case KEY_UP:
            break;
        case KEY_DOWN:
            *state = KEY_RISING;
            break;
        case KEY_FALLING:
            *state = KEY_RISING;
            break;
        case KEY_RISING:
            *state = KEY_UP;
            break;
        default:
            *state = KEY_UP;
            break;
        }
    } else {
        switch (*state) {
        case KEY_UP:
            *state = KEY_FALLING;
            break;
        case KEY_DOWN:
            break;
        case KEY_FALLING:
            *state = KEY_DOWN;
            break;
        case KEY_RISING:
            *state = KEY_FALLING;
            break;
        default:
            *state = KEY_DOWN;
            break;
        }
    }
    m_held[button] = pressure * data_003DF268;
    if (*state == KEY_FALLING) {
        if (m_click[button] > 0.0f)
            m_click[button] = -2.0f;
        else
            m_click[button] = 0.25f;
    } else if (m_click[button] > 0.0f) {
        m_click[button] = m_click[button] - pad_dt;
        if (m_click[button] < 0.0f)
            m_click[button] = -1.0f;
    }
}
