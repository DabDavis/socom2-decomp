/* seal_aiparams.cpp -- SEAL_AI_PARAMS's constructor (OVL2 0x60BF40): CSealCtrlAi's second base,
 * built out of line before CSealCtrlAi's table is stored (the SEAL lane's round 7). The unit's name
 * and extent are ours: the constructor sits in the stretch after seal_airesponse.cpp that opens
 * with SOCOM 1's ai::CheckLineOfFire and CAiTarget's members (0x609DF0 on, CAiTarget's
 * constructor at 0x60A940), followed by the out-of-line copies it calls first (0x60C170, the
 * event queue's list).
 *
 * This unit declares SEAL_AI_PARAMS for itself, layout-compatible with zseal.h's (whose members
 * are byte stand-ins its inline destructor destroys by hand): here each member is a type whose
 * constructor runs what the target runs, in declaration order -- the out-of-line constructors by
 * their names or placeholders. Offsets below are the base's (the controller's minus 0x250). */

#include <string.h>

#include "../gamez/zmath/zmath.h"
#include "../gamez/zentity/zentity.h"
#include "../gamez/zseal/aitimer.h"

extern "C" {
/* the out-of-line constructors and setters the members call (OVL2): a 0x24-byte block's
 * (0x5585B0), the fire parameters' reset (0x55F6F0, SOCOM 1's AI_FIREPARAMS::reset), the event
 * queue's list (0x60C170, the deleter's constructor), Rfloat's two constructors (0x5F6F30 from a
 * value, 0x5609D0 from a minimum and a maximum) and the trigger's times set (0x5FF200) */
void *func_005585B0(void *p);
void func_0055F6F0(void *p);
void *func_0060C170(void *list);
class Rfloat;
Rfloat *func_005F6F30(Rfloat *r, float f);
Rfloat *func_005609D0(Rfloat *r, float min, float max);
class CAiTrigger;
void func_005FF200(CAiTrigger *t, const Rfloat *a, const Rfloat *b);
}

/* the members built out of line, by their own constructors' names (OVL2 0x5F8520, 0x528340,
 * 0x50E960, 0x60A940) -- only their sizes here */
class CAiStateStack {
public:
    CAiStateStack();
    unsigned int _0[0x20 / 4];
};

/* (its destructor declared, as the real class's is -- OVL2 0x5281B0: only so does MW keep the
 * route's address in a saved register across its path's construction, `addiu s1,s0,708`, rather
 * than folding the route's state store into the base's offsets) */
class CAiPath {
public:
    CAiPath();
    ~CAiPath();
    unsigned int _0[0xa8 / 4];
};

class CAiEvent {
public:
    CAiEvent();
    unsigned int _0[0x28 / 4];
};

class CAiTarget {
public:
    CAiTarget();
    unsigned int _0[0x78 / 4];
};

/* a random float: a minimum and a range (SOCOM 1's DWARF, 8 bytes) */
class Rfloat {
public:
    Rfloat() {}
    Rfloat(float f) { func_005F6F30(this, f); }

    float m_min;   // 0x0
    float m_range;   // 0x4
};

/* SOCOM 1's DWARF: CAiTrigger, 0x18 bytes (its times set out of line, 0x5FF200) */
class CAiTrigger {
public:
    CAiTrigger() : m_up_time(0.0f), m_dn_time(0.0f) {}

    Rfloat m_up_time;   // 0x0
    Rfloat m_dn_time;   // 0x8
    float m_timer;   // 0x10
    float m_marker;   // 0x14
};

/* a value easing toward a goal (SOCOM 1's DWARF: CAiDecay, 0xc bytes) */
class CAiDecay {
public:
    CAiDecay() : m_value(0.0f), m_goal(0.0f), m_8(0) {}

    float m_value;   // 0x0
    float m_goal;   // 0x4
    unsigned char m_8;   // 0x8
};

/* one of two 0x48-byte records (the names ours): two words, then eight cells */
struct CAiParamsRecord {
    CAiParamsRecord() : m_0(0), m_4(15) {}

    int m_0;   // 0x0
    int m_4;   // 0x4 (15)
    unsigned char _8[0x28 - 0x8];
    CAiMapLoc m_locs[8];   // 0x28
};

/* the block at 0x34 (the names ours): a cell, three words, the records -- zeroed after they are
 * built, the cell made the invalid one */
struct CAiParamsBlock {
    CAiParamsBlock() : m_4(15), m_8(7), m_c(0)
    {
        memset(m_records, 0, sizeof(m_records));
        *(unsigned int *)&m_loc = *(unsigned int *)&CAiMapLoc::invalid;
    }

    CAiMapLoc m_loc;   // 0x0
    int m_4;   // 0x4
    int m_8;   // 0x8
    int m_c;   // 0xc
    CAiParamsRecord m_records[2];   // 0x10
};

/* the 0x24-byte block at 0xD4, built out of line (0x5585B0) */
struct CAiParamsD4 {
    CAiParamsD4() { func_005585B0(this); }

    unsigned int _0[0x24 / 4];
};

/* the fire parameters (SOCOM 1's AI_FIREPARAMS, 0x60 bytes there; 0x70 here): the word at 0x6C
 * zeroed, then the reset out of line */
struct CAiFireParams {
    CAiFireParams() : m_6c(0) { func_0055F6F0(this); }

    unsigned int _0[0x6c / 4];
    unsigned int m_6c;   // 0x6c
};

/* the block at 0x190 (the names ours): sixteen bytes zeroed, then a signed byte made -1 */
struct CAiParams190 {
    CAiParams190()
    {
        memset(m_4, 0, sizeof(m_4));
        m_0 = -1;
    }

    signed char m_0;   // 0x0
    unsigned char _1[3];
    unsigned char m_4[16];   // 0x4
};

/* a route: a path and its state byte (its destructor -- OVL2 0x573F20, seal_ai.cpp's -- declared,
 * so this unit emits no implicit copy) */
struct CAiRoute {
    CAiRoute() : m_state(0) {}
    ~CAiRoute();

    CAiPath m_path;   // 0x0
    unsigned char m_state;   // 0xa8
};

/* the event queue (SOCOM 1's CAiEventQ : list, 0x10 bytes): the list's deleter built out of line */
struct CAiEventQBytes {
    CAiEventQBytes() { func_0060C170(this); }

    unsigned int _0[0x10 / 4];
};

class SEAL_AI_PARAMS {
public:
    SEAL_AI_PARAMS();

    unsigned char _0[0x18];
    unsigned int m_18;   // 0x18 (the controller's 0x268)
    unsigned char _1c[0x34 - 0x1c];
    CAiParamsBlock m_34;   // 0x34
    CAiParamsD4 m_d4;   // 0xd4
    unsigned char _f8[0x114 - 0xf8];
    CAiTimer m_timer;   // 0x114
    CAiFireParams m_fire_params;   // 0x120
    CAiParams190 m_190;   // 0x190
    unsigned char _1a4[0x1cc - 0x1a4];
    CAiStateStack m_stateStack;   // 0x1cc (the controller's 0x41C)
    unsigned char _1ec[0x21c - 0x1ec];
    CAiPath m_path;   // 0x21c
    CAiRoute m_route;   // 0x2c4
    unsigned char _370[0x3d8 - 0x370];
    CAiEvent m_event;   // 0x3d8
    CAiEventQBytes m_eventQueue;   // 0x400
    unsigned char m_410;   // 0x410
    void *m_414;   // 0x414
    CPnt3D m_418;   // 0x418
    float m_424;   // 0x424
    float m_428;   // 0x428
    CAiTarget m_target;   // 0x42c
    CAiDecay m_courage;   // 0x4a4 (SOCOM 1's four decays, in its order)
    CAiDecay m_audiofactor;   // 0x4b0
    CAiDecay m_dicipline;   // 0x4bc
    CAiDecay m_underfire;   // 0x4c8
    unsigned char _4d4[0x4e4 - 0x4d4];
    CAiTrigger m_trigger;   // 0x4e4
    unsigned char _4fc[0x570 - 0x4fc];
};

/* the AI's data built (OVL2 0x60BF40): every member in order, then the trigger's times set */
SEAL_AI_PARAMS::SEAL_AI_PARAMS()
    : m_18(0), m_timer(0.0f), m_410(0), m_414(0), m_418(CPnt3D::zero), m_424(0.0f), m_428(0.0f)
{
    Rfloat a;
    Rfloat b;

    func_005FF200(&m_trigger, func_005609D0(&a, 0.5f, 1.2f), func_005609D0(&b, 0.1f, 0.5f));
}
