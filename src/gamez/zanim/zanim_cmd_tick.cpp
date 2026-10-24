/* zanim_cmd_tick.cpp -- the ZAnim commands' per-frame handlers (OVL1 0x25D230-...). A handler
 * gets its command record -- a _zanim_cmd_hdr and the command's own fields after it -- and the
 * frame time, and returns whether the interpreter may go on to the next command. The unit name
 * is SOCOM 1's (__sinit_zanim_cmd_tick.cpp; the press beta's asserts); the command records'
 * layouts are read off the handlers. Defined in reverse of their image order (-inline deferred). */

#include <stdio.h>
#include <string.h>

#include "zanim.h"

/* OBJECT_AIM's tick: turn a node from one orientation to another over a time on the
 * command's clock */
struct zanim_cmd_aim {
    _zanim_cmd_hdr hdr;
    unsigned short flags;           /* 1: keep the axes 2/4/8 name (x/y/z) from the forward */
    unsigned char node;
    unsigned char target;
    PNT3D at;                       /* 0x8: in the target node's frame */
    PNT3D forward;                  /* 0x14: in the node's frame */
    CQuat from;                     /* 0x20 */
    CQuat to;                       /* 0x30 */
    float time;                     /* 0x40 */
};

extern "C" void func_003156D0(zdb::CNode *node, CQuat *q);    /* the rotation as a quaternion */

/* OBJECT_AIM's begin: the turn that takes the node's forward onto the line to a point in the
 * target node's frame, as the tick's from and to orientations */
extern "C" void func_002634A0(_zanim_cmd_hdr *hdr)
{
    zanim_cmd_aim *cmd = (zanim_cmd_aim *)hdr;
    CZAnim *anim = ZAnim.m_CurAnim;
    CNode *node = anim->GetNode(cmd->node, false);
    CNode *target = anim->GetNode(cmd->target, false);

    if (node && target) {
        CMatrix m;
        CMatrix *inv = 0;

        if (node->m_parent) {
            node->m_parent->BuildMTW(m);
            Vu0MatrixInverseWithUniformScale(&m, &m);
            inv = &m;
        }
        CPnt3D origin(0.0f, 0.0f, 0.0f);

        func_00308640(&node->m_matrix, &origin, 1);
        CPnt3D fwd(cmd->forward.x, cmd->forward.y, cmd->forward.z);

        func_00308640(&node->m_matrix, &fwd, 1);
        CPnt3D at(cmd->at.x, cmd->at.y, cmd->at.z);

        target->TransformToWorld(at);
        if (inv)
            func_00308640(inv, &at, 1);
        fwd.Sub(&origin, &fwd);
        fwd.Normalize();
        at.Sub(&origin, &at);
        at.Normalize();
        if (cmd->flags & 1) {
            if (cmd->flags & 2)
                at.x = fwd.x;
            if (cmd->flags & 4)
                at.y = fwd.y;
            if (cmd->flags & 8)
                at.z = fwd.z;
        }
        CQuat turn;

        turn.Align(fwd, at);
        turn.Normalize();
        CQuat cur;

        func_003156D0(node, &cur);
        CQuat to;

        to = turn * cur;                    /* (assigned: the temporary's copy is kept) */

        to.Normalize();
        cmd->from = cur;
        cmd->to = to;
    }
}

extern "C" bool func_002633A0(_zanim_cmd_hdr *hdr, float *dT)
{
    zanim_cmd_aim *cmd = (zanim_cmd_aim *)hdr;
    CNode *node = ZAnim.m_CurAnim->GetNode(cmd->node, false);
    bool done = true;

    if (node) {
        float step = *dT;
        float t = ZAnim.m_CurSeq->cmd_timer;

        if (!(step + t <= cmd->time)) {
            step = cmd->time - t;
            *dT -= step;
        } else {
            *dT = 0.0f;
            done = false;
        }
        if (done)
            node->SetRotation(cmd->to);
        else {
            CQuat q;

            cmd->from.Slerp(&cmd->to, (step + ZAnim.m_CurSeq->cmd_timer) / cmd->time, &q);
            node->SetRotation(q);
        }
    }
    return done;
}

/* move a node: to a point (flag 1), by it from where the node is (flag 4), and by the anim's
 * call argument (flag 2), in another node's frame if there is one; flags 8, 0x10 and 0x20 keep
 * the node's own x, y and z */
struct zanim_cmd_transstate {
    _zanim_cmd_hdr hdr;
    unsigned short flags;
    unsigned char node;
    unsigned char ref;
    CPnt3D pos;
};

bool zAnimObjectTranslateStateTick(_zanim_cmd_hdr *hdr, float *dT)
{
    zanim_cmd_transstate *cmd = (zanim_cmd_transstate *)hdr;
    CNode *node;
    CNode *ref;
    bool moved;
    CZAnim *anim = ZAnim.m_CurAnim;

    node = anim->GetNode(cmd->node, false);
    ref = anim->GetNode(cmd->ref, false);
    if (node) {
        CPnt3D pos(0.0f, 0.0f, 0.0f);
        moved = false;

        if (cmd->flags & 4) {
            pos.x = node->m_matrix.m_matrix[3][0];
            pos.y = node->m_matrix.m_matrix[3][1];
            pos.z = node->m_matrix.m_matrix[3][2];
            pos.Add(&cmd->pos, &pos);
            moved = true;
        } else if (cmd->flags & 1) {
            pos.Add(&cmd->pos, &pos);
            moved = true;
        }
        if (cmd->flags & 2) {
            CPnt3D *arg = anim->GetArgPnt3D();

            if (arg) {
                pos = pos + *arg;
                moved = true;
            }
        }
        if (moved || ref) {
            if (ref)
                ref->TransformToWorld(pos);
            CPnt3D cur = node->GetPosition();

            if (!(cmd->flags & 8))
                cur.x = pos.x;
            if (!(cmd->flags & 0x10))
                cur.y = pos.y;
            if (!(cmd->flags & 0x20))
                cur.z = pos.z;
            node->SetPosition(cur.x, cur.y, cur.z);
        }
    }
    return true;
}

/* set (flag 1) or add to (flag 2) a node's rotation, after giving it another node's matrix
 * -- that node's own (flag 4) or its world one -- and keep it where it was */
struct zanim_cmd_rotstate {
    _zanim_cmd_hdr hdr;
    unsigned short flags;
    unsigned char node;
    unsigned char ref;
    CPnt3D rot;
};

bool zAnimObjectRotateStateTick(_zanim_cmd_hdr *hdr, float *dT)
{
    zanim_cmd_rotstate *cmd = (zanim_cmd_rotstate *)hdr;
    CZAnim *anim = ZAnim.m_CurAnim;
    CNode *node = anim->GetNode(cmd->node, false);
    CNode *ref = anim->GetNode(cmd->ref, false);

    if (node) {
        CPnt3D rot(0.0f, 0.0f, 0.0f);
        float x = node->m_matrix.m_matrix[3][0];
        float y = node->m_matrix.m_matrix[3][1];
        float z = node->m_matrix.m_matrix[3][2];

        if (ref) {
            if (cmd->flags & 4)
                node->SetMatrix(&ref->m_matrix);
            else {
                CMatrix m;

                ref->BuildMTW(m);
                node->SetMatrix(&m);
            }
        }
        if (cmd->flags & 1) {
            rot.Add(&cmd->rot, &rot);
            node->SetRotation(&rot);
        }
        if (cmd->flags & 2) {
            node->GetRotation(&rot);
            rot.Add(&cmd->rot, &rot);
            node->SetRotation(&rot);
        }
        node->SetPosition(x, y, z);
    }
    return true;
}

/* a node of the current anim */
struct zanim_cmd_node {
    _zanim_cmd_hdr hdr;
    unsigned char node;
};

/* OBJECT_MAKE_WORLDCHILD (new in SOCOM II): move a node of the current anim to the world's
 * root, keeping where it is in the world */
extern "C" bool func_00263B30(_zanim_cmd_hdr *hdr, float *dT)
{
    zanim_cmd_node *cmd = (zanim_cmd_node *)hdr;
    CNode *node = ZAnim.m_CurAnim->GetNode(cmd->node, false);

    if (node) {
        CNode *parent = node->m_parent;

        if (parent != zdb::CWorld::m_world) {
            CMatrix m;

            node->BuildMTW(m);
            if (parent)
                parent->DeleteChild(node);
            zdb::CWorld::m_world->AddChild(node);
            node->SetMatrix(&m);
        }
    }
    return true;
}

/* TIMER: the anims' shared timer against a value -- ==, !=, >=, <=, >, < -- or set it (6) */
struct zanim_cmd_timer {
    _zanim_cmd_hdr hdr;
    unsigned char op;
    unsigned char pad[3];
    float value;
};

extern float data_004365C8;

bool zAnimTimerTick(_zanim_cmd_hdr *hdr, float *dT)
{
    zanim_cmd_timer *cmd = (zanim_cmd_timer *)hdr;
    float timer = data_004365C8;

    switch (cmd->op) {
    case 6:
        data_004365C8 = cmd->value;
        return true;
    case 0:
        if (timer == cmd->value)
            return true;
        return false;
    case 1:
        if (timer != cmd->value)
            return true;
        return false;
    case 2:
        if (timer >= cmd->value)
            return true;
        return false;
    case 3:
        if (timer <= cmd->value)
            return true;
        return false;
    case 4:
        if (timer > cmd->value)
            return true;
        return false;
    case 5:
        if (timer < cmd->value)
            return true;
        return false;
    }
    return true;
}

/* DESTRUCTOR: break a node of the current anim apart through the particle system's
 * destruction manager, under the name of one of the anim's names */
struct zanim_cmd_destructor {
    _zanim_cmd_hdr hdr;
    unsigned char node;
    unsigned char name;
    unsigned char count;
    unsigned char pad;
    PNT4D color;                    /* (CPnt4D's 16-byte alignment would put it at +0x10) */
    float scale;
    float range[3][2];              /* per axis, low and high */
    CPnt3D spin;
    CPnt3D velocity;
    float life;
};

namespace Particle {
void DestructionManager(zdb::CNode *node, char *name, CPnt4D *color, float scale, CPnt3D *lo,
                        CPnt3D *hi, CPnt3D *spin, CPnt3D *velocity, int count, float life);
}

bool zAnimDestructorTick(_zanim_cmd_hdr *hdr, float *dT)
{
    zanim_cmd_destructor *cmd = (zanim_cmd_destructor *)hdr;
    CZAnim *anim = ZAnim.m_CurAnim;
    CPnt3D lo(cmd->range[0][0], cmd->range[1][0], cmd->range[2][0]);
    CPnt3D hi(cmd->range[0][1], cmd->range[1][1], cmd->range[2][1]);
    CPnt3D spin(cmd->spin.x, cmd->spin.y, cmd->spin.z);
    CPnt3D velocity(cmd->velocity.x, cmd->velocity.y, cmd->velocity.z);
    char name[144];                 /* (129 to 144 all fit the frame) */

    sprintf(name, "%s", anim->GetName(cmd->name));
    Particle::DestructionManager(anim->GetNode(cmd->node, false), name, (CPnt4D *)&cmd->color, cmd->scale,
                                 &lo, &hi, &spin, &velocity, cmd->count, cmd->life);
    return true;
}

/* MESSAGE: put up a message -- string `text` of the game's text table, or else the anim's name
 * number `name` -- for `time` seconds. SOCOM 1 called C2DMessage_Q::Add_Message on its
 * message queue; SOCOM II calls a six-argument function at 0x2B6530 (this, the text, 0, the
 * byte at +11, 0 in t0, the time) whose name is not carried. */
struct zanim_cmd_message {
    _zanim_cmd_hdr hdr;
    unsigned short name;
    unsigned short text;
    unsigned char pad[3];
    unsigned char style;
    float time;
};

extern char data_004366A0[];
extern "C" const char *func_00361B40(int id);
extern "C" void func_002B6530(char *queue, const char *text, int a, int style, int b, float time);

bool zAnimMessageTick(_zanim_cmd_hdr *hdr, float *dT)
{
    zanim_cmd_message *cmd = (zanim_cmd_message *)hdr;
    CZAnim *anim = ZAnim.m_CurAnim;
    const char *text;

    if (cmd->text > 0)
        text = func_00361B40(cmd->text);
    else
        text = anim->GetName(cmd->name);
    func_002B6530(data_004366A0, text, 0, cmd->style, 0, cmd->time);
    return true;
}

/* TRUE_COLOR_SCALE: set the four true-colour scales (one per channel bit 0-3) -- their rgb
 * (flag 0x10) and/or their a (0x20) -- and/or drive them by an expression, seeded from the
 * first chosen channel, over a time (the table and the expression's function not named yet).
 * OPEN, 8 bytes short, in the channel search alone: the target enters it through a jump to
 * the `i < 4` test and reloads `channels` each pass, where this hoists the load, converts the
 * bit to a bool (sltu/xori) and runs the first pass straight. Tried: while, no-break with
 * `first < 0` in the test (right size, 31 off), continue, an inline helper returning i. */
struct zanim_cmd_truecolor {
    _zanim_cmd_hdr hdr;
    unsigned char channels;
    unsigned char pad[3];
    float r, g, b, a;
    short expr_len;
    short expr_off;                 /* from the record */
    float time;
};

extern PNT4D data_004B4D40[4];

static inline void SetRGB(PNT4D *c, float r, float g, float b)
{
    c->x = r;
    c->y = g;
    c->z = b;
}
extern "C" void func_0025C6E0(PNT4D *color, char *expr, int len, int channel, float t);

extern "C" bool func_002642F0(_zanim_cmd_hdr *hdr, float *dT)
{
    bool done = true;
    zanim_cmd_truecolor *cmd = (zanim_cmd_truecolor *)hdr;

    if ((cmd->channels & 0x10) || (cmd->channels & 0x20)) {
        for (int i = 0; i < 4; i++) {
            if (cmd->channels & (1 << i)) {
                if (cmd->channels & 0x10)
                    SetRGB(&data_004B4D40[i], cmd->r, cmd->g, cmd->b);
                if (cmd->channels & 0x20)
                    data_004B4D40[i].w = cmd->a;
            }
        }
    }
    if (cmd->expr_len > 0) {
        char *expr = (char *)cmd + cmd->expr_off;
        float t = *dT + ZAnim.m_CurSeq->cmd_timer;

        if (!(cmd->time <= 0.0f) && !(t <= cmd->time))
            t = cmd->time;
        int first = -1;

        for (int i = 0; i < 4; i++) {
            if (cmd->channels & (1 << i)) {
                first = i;
                break;
            }
        }
        PNT4D c = { 0.0f, 0.0f, 0.0f, 0.0f };

        if (first >= 0) {
            PNT4D from = data_004B4D40[first];

            c = from;
        }
        func_0025C6E0(&c, expr, cmd->expr_len, first, t);
        for (int i = 0; i < 4; i++)
            if (cmd->channels & (1 << i))
                data_004B4D40[i] = c;
    }
    if (!(cmd->time <= 0.0f)) {
        float step = *dT;
        float t = ZAnim.m_CurSeq->cmd_timer;

        if (!(step + t <= cmd->time)) {
            done = true;
            *dT = step - (cmd->time - t);
        } else {
            done = false;
            *dT = 0.0f;
        }
    }
    return done;
}

/* SCALE_COLOR: scale a colour's channels -- all of them (flag 0x10), or each one whose bit
 * (0-3) is set -- by a factor; the colour object's two functions are not named yet */
struct zanim_cmd_scalecolor {
    _zanim_cmd_hdr hdr;
    unsigned char channels;
    unsigned char pad[3];
    unsigned char color[0x10];
    float scale;
};

extern "C" void func_003B78D0(void *color, float scale);
extern "C" void func_003B76B0(void *color, int channel, float scale);

extern "C" bool func_00264580(_zanim_cmd_hdr *hdr, float *dT)
{
    zanim_cmd_scalecolor *cmd = (zanim_cmd_scalecolor *)hdr;

    if (cmd->channels & 0x10)
        func_003B78D0(cmd->color, cmd->scale);
    else {
        for (int i = 0; i < 4; i++)
            if (cmd->channels & (1 << i))
                func_003B76B0(cmd->color, i, cmd->scale);
    }
    return true;
}

/* a node of the current anim and a 16-bit argument: ACTIVE_STATE's state, NODE_ACTIVE's mode */
struct zanim_cmd_nodetest {
    _zanim_cmd_hdr hdr;
    unsigned short mode;
    unsigned short node;
};

extern char data_004B5170[];       /* the ripple manager */

/* the ripple manager's ripple, as far as RIPPLE touches it */
struct CRipple {
    unsigned char _0[8];
    zdb::CNode *node;               /* 0x8 */
    unsigned char _c[0x34];
    unsigned char _b0 : 2;          /* 0x40 */
    unsigned char has_offset : 1;
    unsigned char timed : 1;
    unsigned char _b4 : 1;
    unsigned char b5 : 1;
    unsigned char b6 : 1;
    unsigned char _b7 : 1;
    unsigned char _41[3];
    PNT3D offset;                   /* 0x44 */
    unsigned char _50[0x24];
    float f74;                      /* 0x74 */
    float _78;
    float rate;                     /* 0x7C */
    float _80;
    float period;                   /* 0x84 */

    void SetOffset(const PNT3D *p)
    {
        if (p) {
            has_offset = true;
            offset.x = p->x;
            offset.y = p->y;
            offset.z = p->z;
        } else
            has_offset = false;
    }
    void SetPeriod(float p)
    {
        timed = true;
        period = p;
        if (b5 && !b6)
            rate = f74 / period;
    }
};

/* RIPPLE: start a ripple -- or find the one of that name -- at a point (plus the call
 * argument, flag 0x80) in a node's frame, then set what the flags say; the manager's functions
 * are not named yet */
struct zanim_cmd_ripple {
    _zanim_cmd_hdr hdr;
    unsigned short flags;
    unsigned char name;
    unsigned char group;            /* an anim name (flag 1) or node */
    unsigned char node;
    unsigned char at;
    unsigned char pad[2];
    CPnt3D pos;                     /* 0xC */
    PNT3D offset;                   /* 0x18 */
    float a, b, c;                  /* 0x24 */
    float f30;
    float period;                   /* 0x34 */
    float f38;
    float f3C;
};

extern "C" CRipple *func_003C2950(char *owner, const char *name);
extern "C" CRipple *func_003C2410(char *owner, const char *group, CPnt3D *pos, float *p, float a, float b, float c);
extern "C" CRipple *func_003C26E0(char *owner, zdb::CNode *node, CPnt3D *pos, float *p, float a, float b, float c);
extern "C" CRipple *func_003C2250(char *owner, CPnt3D *pos, float *p, float a, float b, float c);
extern "C" void func_003C1BE0(CRipple *r, const char *name);
extern "C" void func_003C1A40(CRipple *r, float v);
extern "C" void func_003C19D0(CRipple *r, float *p, float v);

extern "C" bool func_00263F90(_zanim_cmd_hdr *hdr, float *dT)
{
    zanim_cmd_ripple *cmd = (zanim_cmd_ripple *)hdr;
    CRipple *r;
    bool found;
    const char *name;
    CNode *at;
    CZAnim *anim = ZAnim.m_CurAnim;
    CPnt3D pos = CPnt3D::zero;

    at = anim->GetNode(cmd->at, false);
    pos = cmd->pos;
    if (cmd->flags & 0x80) {
        CPnt3D *arg = anim->GetArgPnt3D();

        if (arg)
            pos.Add(arg, &pos);
    }
    if (at)
        at->TransformToWorld(pos);
    r = 0;
    found = false;
    name = 0;
    if (cmd->name > 0) {
        name = anim->GetName(cmd->name);
        if (name)
            r = func_003C2950(data_004B5170, name);
    }
    if (r)
        found = true;
    if (!r) {
        if (cmd->flags & 1)
            r = func_003C2410(data_004B5170, anim->GetName(cmd->group), &pos, &cmd->period, cmd->a, cmd->b, cmd->c);
        else {
            CNode *n = anim->GetNode(cmd->group, false);

            if (n)
                r = func_003C26E0(data_004B5170, n, &pos, &cmd->period, cmd->a, cmd->b, cmd->c);
        }
        if (!r)
            r = func_003C2250(data_004B5170, &pos, &cmd->period, cmd->a, cmd->b, cmd->c);
    }
    if (r) {
        if (name && !found)
            func_003C1BE0(r, name);
        CNode *node = anim->GetNode(cmd->node, false);

        if (cmd->flags & 8)
            r->SetOffset(&cmd->offset);
        if ((cmd->flags & 0x40) && !found)
            func_003C1A40(r, cmd->f30);
        if (node)
            r->node = node;
        if (cmd->flags & 0x10) {
            float *p = 0;

            if (cmd->flags & 0x20)
                p = &cmd->f3C;
            func_003C19D0(r, p, cmd->f38);
        }
        if (cmd->flags & 2)
            r->SetPeriod(cmd->period);
    }
    return true;
}

/* RIPPLE_NODE_GROUP: gather world nodes into a ripple group named by one of the anim's names
 * -- each name in the record a node's, or with a `*` a pattern every match of which is taken
 * (the group's and the pattern's functions are not named yet) */
struct zanim_cmd_ripplegroup {
    _zanim_cmd_hdr hdr;
    unsigned char group;
    unsigned char count;
    unsigned char pad[2];
    int name[1];
};

extern "C" void *func_003C2120(char *owner, const char *name);
extern "C" void func_003C20A0(void *group, zdb::CNode *node);
extern "C" void *func_001801E0(const char *pattern, int a, int b);
extern "C" void func_00180280(void *pattern, int a, int b);
extern "C" const char *func_00268790(void *pattern);
extern "C" const char *func_00268680();

extern "C" bool func_00263E20(_zanim_cmd_hdr *hdr, float *dT)
{
    zanim_cmd_ripplegroup *cmd = (zanim_cmd_ripplegroup *)hdr;
    CZAnim *anim = ZAnim.m_CurAnim;
    void *group = func_003C2120(data_004B5170, anim->GetName(cmd->group));

    for (int i = 0; i < cmd->count; i++) {
        const char *name = anim->GetName(cmd->name[i]);

        if (strstr(name, "*")) {
            void *pattern = func_001801E0(name, 0, 0);

            for (const char *n = func_00268790(pattern); n; n = func_00268680()) {
                CNode *node = zdb::CWorld::m_world->FindChild(n, true);

                if (node)
                    func_003C20A0(group, node);
            }
            func_00180280(pattern, 0, 0);
        } else {
            CNode *node = zdb::CWorld::m_world->FindChild(name, true);

            if (node)
                func_003C20A0(group, node);
        }
    }
    return true;
}

/* the GS ALPHA register (sceGsAlpha's layout): (A - B) * C + D, C or D the FIX alpha */
struct GsAlpha {
    unsigned long A : 2;
    unsigned long B : 2;
    unsigned long C : 2;
    unsigned long D : 2;
    unsigned long pad8 : 24;
    unsigned long FIX : 8;
    unsigned long pad40 : 24;
};

/* OBJECT_BLENDMODE: a node's alpha blend -- the four selectors two bits each, the fixed alpha
 * 128 * `alpha` -- optionally driven by an expression (flag 8) over a time; flags 1, 2, 4
 * passed on to the node (the two functions are not named yet) */
struct zanim_cmd_blendmode {
    _zanim_cmd_hdr hdr;
    unsigned long flags : 16;       /* (64-bit bitfields: the selectors are read with dsll32/dsrl32) */
    unsigned long node : 8;
    unsigned long A : 2;
    unsigned long B : 2;
    unsigned long C : 2;
    unsigned long D : 2;
    float alpha;
    short expr_len;
    short expr_off;                 /* from the record */
    float time;
};

extern "C" void func_0025C5F0(GsAlpha *alpha, char *expr, int len, float t);
extern "C" void func_00312860(zdb::CNode *node, GsAlpha *alpha, bool a, bool b, bool c);

bool zAnimObjectBlendModeTick(_zanim_cmd_hdr *hdr, float *dT)
{
    bool done;
    zanim_cmd_blendmode *cmd = (zanim_cmd_blendmode *)hdr;
    GsAlpha alpha;

    *(unsigned long *)&alpha = 0x44;        /* A 0, B 1, C 0, D 1: Cs - Cd, by As, plus Cd */
    done = true;
    CNode *node = ZAnim.m_CurAnim->GetNode(cmd->node, false);
    float step = *dT;
    bool b1 = false;
    bool b2 = false;
    bool b3 = false;

    if (!node)
        return true;
    float time = cmd->time;

    if (!(time <= 0.0f)) {
        float t = ZAnim.m_CurSeq->cmd_timer;

        if (!(step + t <= time)) {
            step = time - t;
            *dT -= step;
        } else {
            done = false;
            *dT = 0.0f;
        }
    }
    if (cmd->flags & 1)
        b1 = true;
    if (cmd->flags & 2)
        b2 = true;
    if (cmd->flags & 4)
        b3 = true;
    int fix = (int)(128.0f * cmd->alpha);
    if (fix < 0)
        fix = 0;
    if (fix > 255)
        fix = 255;
    unsigned char d = cmd->D;               /* (read last to first, into bytes: one load) */
    unsigned char c = cmd->C;
    unsigned char b = cmd->B;
    unsigned char a = cmd->A;

    alpha.FIX = fix;
    alpha.A = a;
    alpha.B = b;
    alpha.C = c;
    alpha.D = d;
    if (cmd->flags & 8)
        func_0025C5F0(&alpha, (char *)cmd + cmd->expr_off, cmd->expr_len, step + ZAnim.m_CurSeq->cmd_timer);
    func_00312860(node, &alpha, b1, b2, b3);
    return done;
}

/* make a node of the current anim active (state 1) or inactive (anything else) */
bool zAnimObjectActiveStateTick(_zanim_cmd_hdr *hdr, float *dT)
{
    zanim_cmd_nodetest *cmd = (zanim_cmd_nodetest *)hdr;
    CNode *node = ZAnim.m_CurAnim->GetNode(cmd->node, false);

    if (node) {
        if (cmd->mode == 1)
            node->SetActive(true);
        else
            node->SetActive(false);
    }
    return true;
}

struct zanim_cmd_sequence {
    _zanim_cmd_hdr hdr;
    unsigned char name;             /* a sequence or call-anim index; 0 is the current anim */
};

/* end of a while loop: back to its test */
struct zanim_cmd_endwhile {
    _zanim_cmd_hdr hdr;
    unsigned char pad[2];
    unsigned short target;          /* the while command's offset */
};

/* a while loop: its expression follows the record */
struct zanim_cmd_while {
    _zanim_cmd_hdr hdr;
    unsigned char done;             /* bit 0: the loop has been left */
    unsigned char pad;
    unsigned short target;          /* past the matching endwhile */
};

struct zanim_cmd_weight {
    _zanim_cmd_hdr hdr;
    float weight;                   /* the chance of going on, 0 to 1 */
};


/* SCROLL_MOTION's begin: the reference node's place in the world kept in the record, and
 * something started on the moving node (two calls not named yet) whose handle is kept too */
struct zanim_cmd_scroll {
    _zanim_cmd_hdr hdr;
    unsigned short flags;
    unsigned char node;
    unsigned char ref;
    CPnt3D pos;
    unsigned char _14[0x2C];
    int handle;                     /* 0x40 */
};

extern char data_004B50D0[];
extern "C" int func_003134C0(zdb::CNode *node, int a, int b, int c, float d, float e);
extern "C" void func_003C0340(char *owner, int handle, float a, float b);

extern "C" void func_00260FB0(_zanim_cmd_hdr *hdr)
{
    zanim_cmd_scroll *cmd = (zanim_cmd_scroll *)hdr;
    CNode *node;
    CZAnim *anim = ZAnim.m_CurAnim;

    node = anim->GetNode(cmd->node, false);
    if (node) {
        CNode *ref = anim->GetNode(cmd->ref, false);

        if (ref) {
            cmd->pos = CPnt3D::zero;
            ref->TransformToWorld(cmd->pos);
            cmd->handle = func_003134C0(node, 0, 1, -1, 0.0f, 0.0f);
            func_003C0340(data_004B50D0, cmd->handle, 0.0f, 0.0f);
        }
    }
}

/* WHEEL_MOTION: turn a wheel by the distance its node has moved since the last frame -- the
 * wheel's place kept in the record -- forwards or back by the side of a reference node it went */
struct zanim_cmd_wheel {
    _zanim_cmd_hdr hdr;
    unsigned char flags;            /* 1: timed */
    unsigned char pad;
    unsigned char node;
    unsigned char ref;
    CPnt3D pos;                     /* 0x8: where the wheel was */
    PNT3D forward;                  /* 0x14: in the reference node's frame */
    CPnt3D axis;                    /* 0x20 */
    float scale;                    /* 0x2C: turn per unit moved */
    float time;                     /* 0x30 */
};

extern "C" void func_003156D0(zdb::CNode *node, CQuat *q);    /* the rotation as a quaternion */

extern "C" bool func_00261080(_zanim_cmd_hdr *hdr, float *dT)
{
    zanim_cmd_wheel *cmd = (zanim_cmd_wheel *)hdr;
    bool done = false;
    CNode *wheel = ZAnim.m_CurAnim->GetNode(cmd->node, false);

    if (!wheel)
        return true;
    CNode *ref = ZAnim.m_CurAnim->GetNode(cmd->ref, false);

    if (!ref)
        return true;
    CPnt3D pos(0.0f, 0.0f, 0.0f);

    wheel->TransformToWorld(pos);
    CPnt3D at(0.0f, 0.0f, 0.0f);

    ref->TransformToWorld(at);
    CPnt3D fwd(cmd->forward.x, cmd->forward.y, cmd->forward.z);

    ref->TransformToWorld(fwd);
    float dz = fwd.z - at.z;
    float dx = fwd.x - at.x;
    CPnt3D moved = pos - cmd->pos;
    float len = sqrtf(moved.x * moved.x + moved.y * moved.y + moved.z * moved.z);
    float angle = -(len * cmd->scale);

    if (dx * moved.x + dz * moved.z < 0.0f)
        angle *= -1.0f;
    CPnt3D rot = cmd->axis * angle;
    CQuat cur;

    func_003156D0(wheel, &cur);
    CQuat spin;

    spin.MakeYXZRot(rot.y, rot.x, rot.z);
    CQuat q;

    cur.Mul(&spin, &q);
    q.Normalize();
    wheel->SetRotation(q);
    cmd->pos = pos;
    if (cmd->flags & 1) {
        float t = ZAnim.m_CurSeq->cmd_timer;
        float time = cmd->time;
        float step = *dT;

        if (!(step + t <= time)) {
            done = true;
            *dT = step - (time - t);
        } else {
            *dT = 0.0f;
            done = false;
        }
    }
    return done;
}

extern "C" void func_00261300(_zanim_cmd_hdr *hdr)
{
    zanim_cmd_wheel *cmd = (zanim_cmd_wheel *)hdr;
    CNode *node = ZAnim.m_CurAnim->GetNode(cmd->node, false);

    if (node) {
        cmd->pos = CPnt3D::zero;
        node->TransformToWorld(cmd->pos);
    }
}

/* fade a node's opacity from one value to another over a time on the command's clock */
struct zanim_cmd_opacity {
    _zanim_cmd_hdr hdr;
    unsigned short node;
    unsigned short pad;
    float from;
    float to;
    float rate;                     /* per second */
    float time;
};

void zAnimObjectOpacityFromToBegin(_zanim_cmd_hdr *hdr)
{
    zanim_cmd_opacity *cmd = (zanim_cmd_opacity *)hdr;
    CNode *node = ZAnim.m_CurAnim->GetNode(cmd->node, false);

    if (node)
        node->m_Opacity = cmd->from;
}

/* OPEN, 5 instructions: the blend's two registers swapped. */
bool zAnimObjectOpacityFromToTick(_zanim_cmd_hdr *hdr, float *dT)
{
    zanim_cmd_opacity *cmd = (zanim_cmd_opacity *)hdr;
    CNode *node = ZAnim.m_CurAnim->GetNode(cmd->node, false);
    bool done;

    if (!node)
        return true;
    float step = *dT;
    float t = ZAnim.m_CurSeq->cmd_timer;
    if (!(step + t <= cmd->time)) {
        done = true;
        step = cmd->time - t;
        *dT -= step;
    } else {
        *dT = 0.0f;
        done = false;
    }
    if (done)
        node->m_Opacity = cmd->to;
    else
        node->m_Opacity += cmd->rate * step;
    return done;
}

/* OBJECT_MOTION_SI_SCRIPT: several nodes each run a motion script; 0x3C bytes a node */
struct zanim_object_motion_script {
    unsigned char _0[0xC];
    int frame;                      /* 0xC */
    int timer;                      /* 0x10 */
    unsigned char _14[0x24];
    int done;                       /* 0x38 */
};

struct zanim_cmd_multiscript {
    _zanim_cmd_hdr hdr;
    unsigned short count;
    unsigned short pad;
    zanim_object_motion_script script[1];
};

void zAnimMultiObjectMotionScriptBegin(_zanim_cmd_hdr *hdr)
{
    zanim_cmd_multiscript *cmd = (zanim_cmd_multiscript *)hdr;

    if (cmd == 0 || cmd->count <= 0)
        return;
    for (int i = 0; i < cmd->count; i++) {
        zanim_object_motion_script *s = &cmd->script[i];

        if (s) {                    /* (tested, as SOCOM 1's is too) */
            s->timer = 0;
            s->frame = 0;
            s->done = 0;
        }
    }
}

bool zAnimObjectMotionScriptTick(_zanim_cmd_hdr *hdr, float *dT);

/* each node's script run on the same frame time; done when all are, with the least time left */
bool zAnimMultiObjectMotionScriptTick(_zanim_cmd_hdr *hdr, float *dT)
{
    zanim_cmd_multiscript *cmd = (zanim_cmd_multiscript *)hdr;

    if (cmd == 0 || cmd->count <= 0)
        return true;
    float t = *dT;
    float left = t;
    int i;
    bool done = true;

    for (i = 0; i < cmd->count; i++) {
        *dT = t;
        if (!zAnimObjectMotionScriptTick((_zanim_cmd_hdr *)&cmd->script[i], dT))
            done = false;
        if (*dT < left)
            left = *dT;
    }
    *dT = left;
    return done;
}

/* parent one node of the current anim to another (flag 1: keep its place in the world) */
struct zanim_cmd_child {
    _zanim_cmd_hdr hdr;
    unsigned short flags;
    unsigned char parent;
    unsigned char child;
};

bool zAnimObjectAddChildTick(_zanim_cmd_hdr *hdr, float *dT)
{
    zanim_cmd_child *cmd = (zanim_cmd_child *)hdr;
    CNode *parent = ZAnim.m_CurAnim->GetNode(cmd->parent, false);
    CNode *child = ZAnim.m_CurAnim->GetNode(cmd->child, false);

    if (parent && child) {
        CPnt3D pos(0.0f, 0.0f, 0.0f);

        if (cmd->flags & 1)
            child->TransformToWorld(pos);
        parent->AddChild(child);
        if (cmd->flags & 1) {
            CMatrix m;

            parent->BuildMTW(m);
            Vu0MatrixInverseWithUniformScale(&m, &m);
            func_00308640(&m, &pos, 1);
            child->SetPosition(&pos);
        }
    }
    return true;
}

bool zAnimObjectDeleteChildTick(_zanim_cmd_hdr *hdr, float *dT)
{
    zanim_cmd_child *cmd = (zanim_cmd_child *)hdr;
    CNode *parent = ZAnim.m_CurAnim->GetNode(cmd->parent, false);
    CNode *child = ZAnim.m_CurAnim->GetNode(cmd->child, false);

    if (parent && child)
        parent->DeleteChild(child);
    return true;
}

/* whether a node of the current anim is active (mode 1) or inactive (mode 2): zanim_cmd_nodetest */

/* Mode 2 turns the flag into a bool (sltu) before testing it, as SOCOM 1 did too: a bool
 * compared with `false` keeps the conversion, where `!` and `== 0` test the bit itself (the
 * original likely compared a bool accessor's result). */
bool zAnimNodeActiveTick(_zanim_cmd_hdr *hdr, float *dT)
{
    zanim_cmd_nodetest *cmd = (zanim_cmd_nodetest *)hdr;
    CNode *node = ZAnim.m_CurAnim->GetNode(cmd->node, false);

    if (node) {
        if (cmd->mode == 1) {
            if (node->m_active)
                return true;
        } else if (cmd->mode == 2) {
            if ((bool)node->m_active == false)
                return true;
        }
    }
    return false;
}

bool zAnimNodeRenderedTick(_zanim_cmd_hdr *hdr, float *dT)
{
    CNode *node = ZAnim.m_CurAnim->GetNode(((zanim_cmd_nodetest *)hdr)->mode, false);

    if (node && func_00315580(node, 2))
        return true;
    return false;
}

/* the squared distance between two points against a value: each point an offset, an anim
 * argument and/or a node's place in the world, by flag; then <, <=, >, >=, ==, != by flag */
struct zanim_cmd_range {
    _zanim_cmd_hdr hdr;
    unsigned int flags;
    unsigned char node_a;
    unsigned char node_b;
    unsigned short pad;
    CPnt3D offset_a;
    CPnt3D offset_b;
    float range;                    /* squared */
};

/* Three things it took, each moving the whole function: the tests as `if (d2 < r) return
 * true;` falling through to one `return false` (`return d2 < r;` left eleven delay slots
 * unfilled and fourteen instructions over, all far from the tail), the argument and node
 * pointers scoped to their blocks, and the squared length as Dot(a, a) (spelt out, the a and b
 * field-address registers swap). */
bool zAnimRangeTestTick(_zanim_cmd_hdr *hdr, float *dT)
{
    zanim_cmd_range *cmd = (zanim_cmd_range *)hdr;
    CZAnim *anim = ZAnim.m_CurAnim;
    CPnt3D a(0.0f, 0.0f, 0.0f);
    CPnt3D b(0.0f, 0.0f, 0.0f);

    if (cmd->flags & 2)
        a.Add(&cmd->offset_a, &a);
    if (cmd->flags & 4) {
        CPnt3D *arg = anim->GetArgPnt3D();

        if (arg)
            a = a + *arg;
    }
    if (cmd->flags & 1) {
        CNode *node = ZAnim.m_CurAnim->GetNode(cmd->node_a, false);

        if (node)
            node->TransformToWorld(a);
    }
    if (cmd->flags & 0x10)
        b.Add(&cmd->offset_b, &b);
    if (cmd->flags & 0x20) {
        CPnt3D *arg = anim->GetArgPnt3D();

        if (arg)
            b = b + *arg;
    }
    if (cmd->flags & 8) {
        CNode *node = ZAnim.m_CurAnim->GetNode(cmd->node_b, false);

        if (node)
            node->TransformToWorld(b);
    }
    a = a - b;
    float d2 = Dot(a, a);
    if (cmd->flags & 0x40) {
        if (d2 < cmd->range)
            return true;
    } else if (cmd->flags & 0x80) {
        if (d2 <= cmd->range)
            return true;
    } else if (cmd->flags & 0x100) {
        if (d2 > cmd->range)
            return true;
    } else if (cmd->flags & 0x200) {
        if (d2 >= cmd->range)
            return true;
    } else if (cmd->flags & 0x400) {
        if (d2 == cmd->range)
            return true;
    } else if (cmd->flags & 0x800) {
        if (d2 != cmd->range)
            return true;
    }
    return false;
}

/* the current anim's health against a bound (flag 1: a range) */
struct zanim_cmd_health {
    _zanim_cmd_hdr hdr;
    unsigned int flags;
    float lo;
    float hi;
};

bool zAnimHealthTestTick(_zanim_cmd_hdr *hdr, float *dT)
{
    zanim_cmd_health *cmd = (zanim_cmd_health *)hdr;
    float health = ZAnim.m_CurAnim->GetHealth();

    if (cmd->flags & 1) {
        if (health >= cmd->lo && health <= cmd->hi)
            return true;
    } else if (health <= cmd->lo) {
        return true;
    }
    return false;
}

/* the level of detail against a level: ==, <, <=, >, >= by flag */
struct zanim_cmd_lod {
    _zanim_cmd_hdr hdr;
    unsigned char flags;
    unsigned char level;
};

/* OPEN, 8 bytes short, in the last two tests only: `>` takes the level into v0 where the target
 * keeps the LOD there (SOCOM 1 the same), and the last test's branch carries `return false` in
 * its delay slot where the target leaves a nop and a block of its own. Neither order of the
 * operands, reading m_Lod per test, else-if, nor one result variable (worse) gives it. */
bool zAnimLODTestTick(_zanim_cmd_hdr *hdr, float *dT)
{
    zanim_cmd_lod *cmd = (zanim_cmd_lod *)hdr;
    LOD lod = ZAnim.m_Lod;

    if (cmd->flags & 1)
        return (unsigned char)lod == cmd->level;
    if (cmd->flags & 2)
        return (unsigned char)lod < cmd->level;
    if (cmd->flags & 4)
        return (unsigned char)lod <= cmd->level;
    if (cmd->flags & 8)
        return (unsigned char)lod > cmd->level;
    if (cmd->flags & 0x10)
        return (unsigned char)lod >= cmd->level;
    return false;
}

/* call an anim by its call-anim reference, unless it is still running from the last call */
struct zanim_cmd_callanim {
    _zanim_cmd_hdr hdr;
    unsigned short flags;           /* 0x200: wait for the call to finish */
    unsigned char local;
    unsigned char index;
};

bool zAnimCallAnimTick(_zanim_cmd_hdr *hdr, float *dT)
{
    zanim_cmd_callanim *cmd = (zanim_cmd_callanim *)hdr;
    CZAnim *anim = ZAnim.m_CurAnim;

    if (cmd->flags & 0x200) {
        _zanim_call_anim_ref *ref = anim->GetCallAnimRef(cmd->index, cmd->local);
        if (ref && ref->executing_anim && ref->executing_anim->Running())
            return false;
    }
    return true;
}

/* pause the current anim (index 0) or a called one */
bool zAnimPauseAnimTick(_zanim_cmd_hdr *hdr, float *dT)
{
    int index = ((zanim_cmd_sequence *)hdr)->name;
    CZAnim *anim = ZAnim.m_CurAnim;

    if (!index) {
        func_00270BE0(anim, 0);
    } else {
        _zanim_call_anim_ref *ref = anim->GetCallAnimRef(index);
        if (ref) {
            if (ref->executing_anim) {
                if (ref->executing_anim->Running())
                    func_00270BE0(ref->executing_anim, 0);
            } else if (ref->base_anim && ref->base_anim->Running()) {
                func_00270BE0(ref->base_anim, 0);
            }
        }
    }
    return true;
}

/* resume a called anim */
bool zAnimResumeAnimTick(_zanim_cmd_hdr *hdr, float *dT)
{
    _zanim_call_anim_ref *ref = ZAnim.m_CurAnim->GetCallAnimRef(((zanim_cmd_sequence *)hdr)->name);

    if (ref) {
        if (ref->executing_anim) {
            if (ref->executing_anim->Running())
                func_002709B0(ref->executing_anim, 0);
        } else if (ref->base_anim && ref->base_anim->Running()) {
            func_002709B0(ref->base_anim, 0);
        }
    }
    return true;
}

/* mark the current anim (index 0) or a called one invalid */
bool zAnimInvalidateAnimTick(_zanim_cmd_hdr *hdr, float *dT)
{
    int index = ((zanim_cmd_sequence *)hdr)->name;
    CZAnim *anim = ZAnim.m_CurAnim;

    if (!index) {
        anim->m_invalid = 1;
    } else {
        _zanim_call_anim_ref *ref = anim->GetCallAnimRef(index);
        if (ref && ref->base_anim)
            ref->base_anim->m_invalid = 1;
    }
    return true;
}

/* repeat the sequence: until a loop count (flag 1, -1 for ever) or a time (flag 2) is reached */
struct zanim_cmd_loop {
    _zanim_cmd_hdr hdr;
    unsigned int flags;
    union {
        int count;
        float time;
    };
};

bool zAnimLoopTick(_zanim_cmd_hdr *hdr, float *dT)
{
    zanim_cmd_loop *cmd = (zanim_cmd_loop *)hdr;

    ZAnim.m_CurSeq->loop_counter++;
    ZAnim.m_CurSeq->loop_timer += *dT + ZAnim.m_CurSeq->seq_timer;
    if (cmd->flags & 1) {
        if (cmd->count != -1 && cmd->count == ZAnim.m_CurSeq->loop_counter)
            return true;
    } else if (cmd->flags & 2) {
        if (!(cmd->time < 0.0f) && !(ZAnim.m_CurSeq->loop_timer < cmd->time))
            return true;
    }
    ZAnim.SeqSetState(3);
    return false;
}

/* wait: for a time on one of three clocks (flag 8: 4 the anim's, 2 the sequence's, else the
 * command's), and/or for a number of passes (flag 0x10); flag 0x20 draws the time from a
 * range when the wait begins */
struct zanim_cmd_wait {
    _zanim_cmd_hdr hdr;
    unsigned short flags;
    unsigned short count;
    float value[4];                 /* the time, then the pass limit (an int) or the range */
};

void zAnimWaitBegin(_zanim_cmd_hdr *hdr)
{
    zanim_cmd_wait *cmd = (zanim_cmd_wait *)hdr;

    if (cmd->flags & 0x10)
        cmd->count = 0;
    if (cmd->flags & 0x20) {
        int i = (cmd->flags & 0x10) ? 2 : 1;
        float r = rand() * 4.656613e-10f;
        cmd->value[0] = cmd->value[i] + cmd->value[i + 1] * r;
    }
}

bool zAnimWaitTick(_zanim_cmd_hdr *hdr, float *dT)
{
    zanim_cmd_wait *cmd = (zanim_cmd_wait *)hdr;
    bool done = true;

    if (cmd->flags & 8) {
        float t;
        if (cmd->flags & 4)
            t = ZAnim.m_CurAnim->m_timer;
        else if (cmd->flags & 2)
            t = ZAnim.m_CurSeq->seq_timer;
        else
            t = ZAnim.m_CurSeq->cmd_timer;
        if (!(t + *dT <= cmd->value[0])) {
            float left = cmd->value[0] - t;
            if (left < 0.0f)
                left = 0.0f;
            done = true;
            *dT -= left;
        } else {
            *dT = 0.0f;
            done = false;
        }
    }
    if (cmd->flags & 0x10) {
        cmd->count++;
        if (done) {
            if (cmd->count <= ((int *)cmd->value)[(cmd->flags & 8) != 0])
                done = false;
        }
    }
    return done;
}

/* a command that fails: the sequence stops here */
bool zAnimFailTick(_zanim_cmd_hdr *hdr, float *dT)
{
    return false;
}

/* for each child of a node (SOCOM II): the begin marks the first pass, each pass steps to the
 * next child and leaves past the matching end_for_children when there is none */
struct zanim_cmd_forchildren {
    _zanim_cmd_hdr hdr;
    unsigned char pad;
    unsigned char node;
    unsigned char pad2;
    unsigned char first : 1;
    unsigned short end;             /* past the matching END_FOR_CHILDREN */
};

/* back to the FOR_CHILDREN, which steps on */
struct zanim_cmd_endfor {
    _zanim_cmd_hdr hdr;
    unsigned short pad;
    unsigned short target;
};

extern "C" bool func_0025E2D0(_zanim_cmd_hdr *hdr, float *dT)
{
    ZAnim.CmdSet(((zanim_cmd_endfor *)hdr)->target);
    zanim_cmd_forchildren *cmd = (zanim_cmd_forchildren *)ZAnim.CmdHdr();
    CZAnim *anim = ZAnim.m_CurAnim;
    func_0025E3F0(&cmd->hdr, anim);
    if (!anim->GetNode(cmd->node, false))
        ZAnim.CmdSet(cmd->end);
    return true;
}

extern "C" bool func_0025E360(_zanim_cmd_hdr *hdr, float *dT)
{
    zanim_cmd_forchildren *cmd = (zanim_cmd_forchildren *)hdr;
    CZAnim *anim = ZAnim.m_CurAnim;

    func_0025E3F0(hdr, anim);
    if (!anim->GetNode(cmd->node, false))
        ZAnim.CmdSet(cmd->end);
    return true;
}

extern "C" void func_0025E3D0(_zanim_cmd_hdr *hdr)
{
    ((zanim_cmd_forchildren *)hdr)->first = 1;
}

/* stop a called anim (flag 1: at once) */
struct zanim_cmd_stopanim {
    _zanim_cmd_hdr hdr;
    unsigned char index;
    unsigned char flags;
};

bool zAnimStopAnimTick(_zanim_cmd_hdr *hdr, float *dT)
{
    zanim_cmd_stopanim *cmd = (zanim_cmd_stopanim *)hdr;
    _zanim_call_anim_ref *ref = ZAnim.m_CurAnim->GetCallAnimRef(cmd->index);
    bool now = false;

    if ((cmd->flags & 1) == 1)
        now = true;
    if (ref) {
        if (ref->executing_anim) {
            if (ref->executing_anim->Running())
                func_002707C0(ref->executing_anim, now);
            ref->executing_anim = 0;
        } else if (ref->base_anim && ref->base_anim->Running()) {
            func_002707C0(ref->base_anim, now);
        }
    }
    return true;
}

/* if, elseif, else and endif carry their nesting level */
struct zanim_cmd_if {
    _zanim_cmd_hdr hdr;
    unsigned char level;
};

static inline int NestLevelOf(_zanim_cmd_hdr *hdr)
{
    return ((zanim_cmd_if *)hdr)->level;
}

/* an if: its expression follows the record; false skips to the matching else, elseif (whose
 * expression is tried in turn) or endif. OPEN, 8 bytes long: the target loads m_CurSeq once and
 * re-tests it for each of the three type comparisons, each read the if-assign way (h = 0 in the
 * delay slot, then the load) where CmdHdr's ternary -- which BreakTick, GoToEndIf and the
 * FOR_CHILDREN handler need -- branches over; an if-assign accessor here fixes the comparisons
 * but not the ProcExpr test before them (two delay slots unfilled), still 0x1C8. Every other
 * spelling tried reloads it, or (with the type in a local) shares the read, 0x20 short. */
bool zAnimIfTick(_zanim_cmd_hdr *hdr, float *dT)
{
    bool found;

    func_002681C0(&ZAnim, 1);
    found = false;
    do {
        bool done = false;

        ZAnim.CmdNext(8);
        if (!CZAnimExpression::ProcExpr(dT)) {
            do {
                ZAnim.CmdNext();
                if (!ZAnim.CmdValid()) {
                    done = true;
                } else {
                    if (ZAnim.CmdHdr()->data_type == ZAnim.m_cmd_else.data_type ||
                        ZAnim.CmdHdr()->data_type == ZAnim.m_cmd_elseif.data_type ||
                        ZAnim.CmdHdr()->data_type == ZAnim.m_cmd_endif.data_type) {
                        int level = NestLevelOf(ZAnim.CmdHdr());
                        if (level == func_002681C0(&ZAnim, 0)) {
                            done = true;
                            if (ZAnim.CmdHdr()->data_type == ZAnim.m_cmd_endif.data_type)
                                func_002681C0(&ZAnim, -1);
                        }
                    }
                }
            } while (!done);
            if (ZAnim.CmdHdr()->data_type != ZAnim.m_cmd_elseif.data_type)
                found = true;
        } else {
            found = true;
        }
    } while (!found);
    return true;
}

bool zAnimEndIfTick(_zanim_cmd_hdr *hdr, float *dT)
{
    func_002681C0(&ZAnim, -1);
    return true;
}

/* past the matching endif (the end of a taken branch) */
bool zAnimGoToEndIf(_zanim_cmd_hdr *hdr, float *dT)
{
    bool done = false;

    do {
        ZAnim.CmdNext();
        if (!ZAnim.CmdValid()) {
            done = true;
        } else {
            int type = ZAnim.CmdHdr()->data_type;
            if (type == ZAnim.m_cmd_endif.data_type) {
                int level = NestLevelOf(ZAnim.CmdHdr());
                if (level == func_002681C0(&ZAnim, 0)) {
                    done = true;
                    func_002681C0(&ZAnim, -1);
                }
            }
        }
    } while (!done);
    return true;
}

/* out of a while loop: past its endwhile, closing the ifs on the way */
bool zAnimBreakTick(_zanim_cmd_hdr *hdr, float *dT)
{
    for (;;) {
        ZAnim.CmdNext();
        if (ZAnim.CmdValid()) {
            int type = ZAnim.CmdHdr()->data_type;
            if (type == ZAnim.m_cmd_endif.data_type) {
                int level = NestLevelOf(ZAnim.CmdHdr());
                if (level == func_002681C0(&ZAnim, 0))
                    func_002681C0(&ZAnim, -1);
            }
        }
        if (!ZAnim.CmdValid())
            break;
        int type = ZAnim.CmdHdr()->data_type;
        if (type == ZAnim.m_cmd_endwhile.data_type)
            break;
    }
    return true;
}

bool zAnimWhileTick(_zanim_cmd_hdr *hdr, float *dT)
{
    zanim_cmd_while *cmd = (zanim_cmd_while *)hdr;

    if (!(cmd->done & 1)) {
        ZAnim.CmdNext(sizeof(zanim_cmd_while));
        if (!CZAnimExpression::ProcExpr(dT))
            ZAnim.CmdSet(cmd->target);
    }
    return true;
}

bool zAnimEndWhileTick(_zanim_cmd_hdr *hdr, float *dT)
{
    ZAnim.CmdSet(((zanim_cmd_endwhile *)hdr)->target);
    return false;
}

bool zAnimRandomWeightTick(_zanim_cmd_hdr *hdr, float *dT)
{
    return rand() * 4.656613e-10f <= ((zanim_cmd_weight *)hdr)->weight;
}

bool zAnimCallSequenceTick(_zanim_cmd_hdr *hdr, float *dT)
{
    ZAnim.ActivateSequence(ZAnim.m_CurAnim->GetName(((zanim_cmd_sequence *)hdr)->name));
    return true;
}

bool zAnimStopSequenceTick(_zanim_cmd_hdr *hdr, float *dT)
{
    ZAnim.StopSequence(ZAnim.m_CurAnim->GetName(((zanim_cmd_sequence *)hdr)->name));
    return true;
}
