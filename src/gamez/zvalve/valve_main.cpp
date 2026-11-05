/* valve_main.cpp -- a valve's name and its ZAnim comparison command (OVL1 0x352960-0x35450F).
 * The file name is SOCOM 1's (its asserts name valve_main.cpp from CValve::FreeName on).
 * Defined in reverse of their image order (-inline deferred). */

#include <stdlib.h>
#include <string.h>

#include "valve.h"
#include "../zsav/zsave.h"

/* atol: newlib (MAIN), through <stdlib.h> */
char *zrdr_findstring(const _zrdr *rdr, const char *tag);
bool zrdr_findint(const _zrdr *rdr, const char *tag, int *out, int n);

/* the rdr lane's CRdrEditor (zrdr.h's, which redefines zanim.h's _zrdr): what the valves' save
 * calls of it, its size (0x4C bytes) */
class CRdrEditor {
public:
    CRdrEditor();
    ~CRdrEditor();
    bool push(const char *tag);
    bool pop();
    bool addint(const char *tag, int i);
    bool addSTRING(const char *tag, const char *s);

    char m_data[0x4C];
};

/* the persistent valves' part of a save (SOCOM 1's class; CSaveModule's slots) */
class CValveSaveManager : public CSaveModule {
public:
    void Serialize(int flags);
    void UnSerialize(const char *name, CRdrFile *file, int flags);
    static void SerializeValves(CRdrEditor *ed);
};

/* SOCOM 1's: every persistent valve written as a NAME and a VAL; SOCOM II's own part of
 * Serialize, which SOCOM 1 inlined (name ours). Pinned: Serialize calls it. Byte-exact since
 * include/list's iterator holds SOCOM 1's `node *` (the world lane's round 6) */
#pragma auto_inline off
void CValveSaveManager::SerializeValves(CRdrEditor *ed)
{
    ed->push(0);
    std::list<CValve *>::iterator i, e = CValve::m_list.end();

    for (i = CValve::m_list.begin(); i != e; ++i) {
        if ((*i)->m_type == VALVE_PERSIST) {
            ed->push(0);
            ed->addSTRING("NAME", (*i)->m_name);
            ed->addint("VAL", (*i)->m_value);
            ed->pop();
        }
    }
    ed->pop();
}
#pragma auto_inline reset

/* the persistent valves saved under "PersistentValves" (SOCOM 1's) */
void CValveSaveManager::Serialize(int flags)
{
    CRdrEditor ed;

    SerializeValves(&ed);
    SaveRdr("PersistentValves", (CRdrFile *)&ed);
}

/* the saved valves made again, persistent, with their values (SOCOM 1's) */
void CValveSaveManager::UnSerialize(const char *name, CRdrFile *file, int flags)
{
    _zrdr *list = (_zrdr *)file;

    for (unsigned int i = 0; i < list->Count(); i++) {
        _zrdr *item = list->Item(i);
        int value;
        char *vname = zrdr_findstring(item, "NAME");

        zrdr_findint(item, "VAL", &value, 1);
        CValve::Create(vname, value, VALVE_PERSIST);
    }
}

/* an operator's name to its code, 0 for none. SOCOM 1's chain (the comparisons 1-6, = += -= *=
 * 7-10, then MOD, NCH, SET and CLR tested with empty arms -- `beqz v0` to the return) with SOCOM
 * II's bit operators: the tests | & |~ &~ take 7-10, the assignments move to 11-14, and |=~ |= &=~
 * &= follow as 17, 15, 18, 16 (the strings' order, 0x3F81C0-0x3F826F) */
int CValve::ParseOperator(const char *op)
{
    int code = 0;

    if (op) {
        if (!strcmp(op, "=="))
            code = 2;
        else if (!strcmp(op, "!="))
            code = 1;
        else if (!strcmp(op, ">="))
            code = 5;
        else if (!strcmp(op, "<="))
            code = 6;
        else if (!strcmp(op, ">"))
            code = 3;
        else if (!strcmp(op, "<"))
            code = 4;
        else if (!strcmp(op, "|"))
            code = 7;
        else if (!strcmp(op, "&"))
            code = 8;
        else if (!strcmp(op, "|~"))
            code = 9;
        else if (!strcmp(op, "&~"))
            code = 10;
        else if (!strcmp(op, "="))
            code = 11;
        else if (!strcmp(op, "+="))
            code = 12;
        else if (!strcmp(op, "-="))
            code = 13;
        else if (!strcmp(op, "*="))
            code = 14;
        else if (!strcmp(op, "|=~"))
            code = 17;
        else if (!strcmp(op, "|="))
            code = 15;
        else if (!strcmp(op, "&=~"))
            code = 18;
        else if (!strcmp(op, "&="))
            code = 16;
        else if (!strcmp(op, "MOD"))
            ;
        else if (!strcmp(op, "NCH"))
            ;
        else if (!strcmp(op, "SET"))
            ;
        else if (!strcmp(op, "CLR"))
            ;
    }
    return code;
}

/* a ZAnim argument instead of a valve: "INPUT_VALVE<n>" names argument n, anything else a valve,
 * created if new (SOCOM 1's; its debug walk of the argument table after the create is gone) */
CValve *VALVE_CMD_DATA::ParseValveName(const char *name, int &index)
{
    if (name && !strncmp(name, "INPUT_VALVE", 11)) {
        index = 0;
        if ((int)strlen(name) > 11)
            index = atol(name + 11);
        return 0;
    }
    return CValve::Create(name, (VALVE_TYPE)2);
}

bool zrdr_isitemtype(const _zrdr *rdr, int index, int type);
void *_calloc(unsigned int n, unsigned int size, const char *file, int line);

/* SOCOM 1's VALVE command: the VALVE_CMD_DATA from +4 (CmdTickCmp compares it). A 16-byte block */
struct VALVE_CMD {
    _zanim_cmd_hdr hdr;   // 0x0
    VALVE_CMD_DATA data;   // 0x4
};   // 0x10 bytes

/* SOCOM 1's: the VALVE command read -- a valve alone (tested true), a valve and an operator, or a
 * valve, an operator and a value: a number, or a valve named as the first is (the one index serving
 * both names, as in SOCOM 1) */
_zanim_cmd_hdr *CValve::CmdParseCmp(_zrdr *rdr)
{
    int index = -1;
    VALVE_CMD *cmd = (VALVE_CMD *)_calloc(1, sizeof(VALVE_CMD), 0, 0);

    cmd->hdr.data_size = sizeof(VALVE_CMD);
    if (rdr) {
        if (rdr->Count() == 1) {
            CValve *valve = VALVE_CMD_DATA::ParseValveName(rdr->Item(0)->String(), index);

            cmd->data.op = 1;
            cmd->data.isArg = index != -1;
            cmd->data.isAnim = true;
            cmd->data.ref = index != -1 ? index : (index = ZAnim.m_CurAnim->AddValve(valve->m_name));
            cmd->data.value = 0;
            cmd->data.SetValueFlags(false, false, false);
        } else if (rdr->Count() == 2) {
            CValve *valve = VALVE_CMD_DATA::ParseValveName(rdr->Item(0)->String(), index);

            cmd->data.op = ParseOperator(rdr->Item(1)->String());
            cmd->data.isArg = index != -1;
            cmd->data.isAnim = true;
            cmd->data.ref = index != -1 ? index : (index = ZAnim.m_CurAnim->AddValve(valve->m_name));
            cmd->data.value = 0;
            cmd->data.SetValueFlags(false, false, false);
        } else if (rdr->Count() >= 3) {
            _zrdr *name = rdr->Item(0);
            _zrdr *v = rdr->Item(2);
            CValve *valve = VALVE_CMD_DATA::ParseValveName(name->String(), index);

            cmd->data.op = ParseOperator(rdr->Item(1)->String());
            cmd->data.isArg = index != -1;
            cmd->data.isAnim = true;
            cmd->data.ref = index != -1 ? index : ZAnim.m_CurAnim->AddValve(valve->m_name);
            if (v->type == 3) {
                CValve *second = VALVE_CMD_DATA::ParseValveName(v->String(), index);

                cmd->data.value = index != -1 ? index : ZAnim.m_CurAnim->AddValve(second->m_name);
                cmd->data.SetValueFlags(index != -1, true, true);
            } else {
                cmd->data.value = v->Int();
                cmd->data.SetValueFlags(false, false, false);
            }
        }
    }
    return (_zanim_cmd_hdr *)cmd;
}

/* SOCOM II's VWATCH command (RegisterCommands' third; the names ours): a valve named once at
 * parse time -- an anim's own (its valve index) or, by its INPUT_VALVE name, an argument's -- and
 * marked when the command runs. A 16-byte block */
struct VWATCH_CMD {
    _zanim_cmd_hdr hdr;   // 0x0
    int ref;   // 0x4: the anim's valve index, or the argument's (or a valve)
    int _8;   // 0x8
    unsigned char _c;   // 0xc
    unsigned char isArg : 1;   // 0xd: ref is an argument's index
    unsigned char isAnim : 1;   // 0xd: ref is the anim's valve index
};   // 0x10 bytes

/* an argument's valve of an anim, none when it has no arguments (ours) */
static inline CValve *ArgValve(CZAnim *anim, int ref)
{
    CZAnimArgList *args = anim->m_arg_list;

    return args ? args->GetValve(ref) : 0;
}

/* the valve's value tested against the block's by the operator -- the comparisons and bit tests
 * answer, the assignments set the valve (its callbacks told) and answer true; no valve, false
 * (SOCOM 1's, with SOCOM II's bit operators and a valve for the value). `a` declared ahead of the
 * valve takes s1 below the valve's s0; the jump table's case 0 is the last block, as written last;
 * the clamped difference computed into a local once (passed to the inline SetValue as written, it
 * is substituted into both of its uses) and written `a >= c ? a - c : 0` (the target's slt into
 * v0) */
bool VALVE_CMD_DATA::Compare()
{
    unsigned short a;
    CValve *valve;

    if (isArg)
        valve = ArgValve(ZAnim.m_CurAnim, ref);
    else if (isAnim)
        valve = ZAnim.m_CurAnim->GetValve(ref);
    else
        valve = (CValve *)ref;
    if (!valve)
        return false;

    a = valve->m_value;
    int b = value;

    if (valIsValve) {
        CValve *v;

        if (valIsArg)
            v = ArgValve(ZAnim.m_CurAnim, b);
        else if (valIsAnim)
            v = ZAnim.m_CurAnim->GetValve(b);
        else
            v = (CValve *)b;
        b = v ? v->m_value : 0;
    }
    unsigned short c = b;

    switch (op) {
    case 1:
        return a != c;
    case 2:
        return a == c;
    case 3:
        return a > c;
    case 4:
        return a < c;
    case 5:
        return a >= c;
    case 6:
        return a <= c;
    case 7:
        return (a | c) != 0;
    case 8:
        return (a & c) != 0;
    case 9:
        return (a | ~c) != 0;
    case 10:
        return (a & ~c) != 0;
    case 11:
        valve->SetValue(c);
        return true;
    case 12:
        valve->SetValue(a + c);
        return true;
    case 13: {
        int d = a >= c ? a - c : 0;

        valve->SetValue(d);
        return true;
    }
    case 14:
        valve->SetValue(a * c);
        return true;
    case 15:
        valve->SetValue(a | c);
        return true;
    case 16:
        valve->SetValue(a & c);
        return true;
    case 17:
        valve->SetValue(a | ~c);
        return true;
    case 18:
        valve->SetValue(a & ~c);
        return true;
    case 0:
        return true;
    }
    return false;
}

/* SOCOM II's VBIT command (RegisterCommands' second, "VBIT"; the names ours): a valve's bits
 * tested, set or cleared -- the valve named as VWATCH's, the operator ParseOperator's code for
 * "get" (`&`), "set" (`|=`) or "clr" (`&=~`), the bits the items after it. The block from +4 is the
 * VALVE command's data (the tick compares it as CmdTickCmp does). A 16-byte block */
struct VBIT_CMD {
    _zanim_cmd_hdr hdr;   // 0x0
    int ref;   // 0x4: the anim's valve index, or the argument's
    unsigned int bits;   // 0x8
    unsigned char op;   // 0xc
    unsigned char isArg : 1;   // 0xd
    unsigned char isAnim : 1;   // 0xd
    unsigned char _d2 : 1;   // 0xd: cleared (the parse's last three stores)
    unsigned char _d3 : 1;   // 0xd
    unsigned char _d4 : 1;   // 0xd

    /* the three flags the parse clears: through a setter `false` stays `(0 & 1) << n` at run time */
    void SetFlags(bool a, bool b, bool c)
    {
        _d2 = a;
        _d3 = b;
        _d4 = c;
    }
};   // 0x10 bytes

/* the block's data compared, as the VALVE command's tick (0x353260; the name ours) */
bool CValve::CmdTickVBit(_zanim_cmd_hdr *hdr, float *dT)
{
    return ((VALVE_CMD_DATA *)((char *)hdr + 4))->Compare();
}

/* the valve named by the first item, the operator by the second, the bits by the rest (0x353270;
 * the name ours) */
_zanim_cmd_hdr *CValve::CmdParseVBit(_zrdr *rdr)
{
    VBIT_CMD *cmd = 0;
    int n = rdr->Count();

    if (n > 2 && zrdr_isitemtype(rdr, 0, 3) && zrdr_isitemtype(rdr, 1, 3)) {
        int index = -1;
        CValve *valve = VALVE_CMD_DATA::ParseValveName(rdr->Item(0)->String(), index);
        char *op = rdr->Item(1)->String();

        if (valve) {
            cmd = (VBIT_CMD *)_calloc(1, sizeof(VBIT_CMD), 0, 0);
            cmd->hdr.data_size = sizeof(VBIT_CMD);
            cmd->isArg = index != -1;
            cmd->isAnim = true;
            cmd->ref = index != -1 ? index : (index = ZAnim.m_CurAnim->AddValve(valve->m_name));
            unsigned int bits = 0;

            for (unsigned int i = 2; (int)i < n; i++)
                bits |= 1 << rdr->Item(i)->Int();
            if (!strcmp(op, "get"))
                cmd->op = 8;
            else if (!strcmp(op, "set"))
                cmd->op = 15;
            else if (!strcmp(op, "clr"))
                cmd->op = 18;
            cmd->bits = bits;
            cmd->SetFlags(false, false, false);
        }
    }
    return (_zanim_cmd_hdr *)cmd;
}

/* the watched valve marked (its bit 26), whichever way the command names it; always done */
bool CValve::CmdTickVWatch(_zanim_cmd_hdr *hdr, float *dT)
{
    VWATCH_CMD *cmd = (VWATCH_CMD *)hdr;
    CValve *valve;

    if (cmd->isArg)
        valve = ArgValve(ZAnim.m_CurAnim, cmd->ref);
    else if (cmd->isAnim)
        valve = ZAnim.m_CurAnim->GetValve(cmd->ref);
    else
        valve = (CValve *)cmd->ref;
    if (valve)
        valve->m_socom2_bit26 = true;
    return true;
}

/* the command's block: the valve named by the first item -- an INPUT_VALVE name makes none --
 * added to the current anim's valves */
_zanim_cmd_hdr *CValve::CmdParseVWatch(_zrdr *rdr)
{
    VWATCH_CMD *cmd = 0;

    if (zrdr_isitemtype(rdr, 0, 3)) {
        int index = -1;
        CValve *valve = VALVE_CMD_DATA::ParseValveName(rdr->Item(0)->String(), index);

        if (valve) {
            cmd = (VWATCH_CMD *)_calloc(1, sizeof(VWATCH_CMD), 0, 0);
            cmd->hdr.data_size = sizeof(VWATCH_CMD);
            cmd->isArg = index != -1;
            cmd->isAnim = true;
            cmd->ref = index != -1 ? index : (index = ZAnim.m_CurAnim->AddValve(valve->m_name));
        }
    }
    return (_zanim_cmd_hdr *)cmd;
}

/* SOCOM 1's: the name freed, the callbacks deleted, the list emptied (byte-exact since include/list's
 * begin() reads the sentinel through a reference: &end_.next_ kept in a saved register) */
CValve::~CValve()
{
    FreeName();
    CValveCBList::iterator e = m_callbacks.end();

    for (CValveCBList::iterator i = m_callbacks.begin(); i != e; ++i)
        delete *i;
    m_callbacks.clear();
}

/* SOCOM 1's: the bit-fields set in the initialiser list (each read, masked and written back as a
 * word, as the target's are), the name copied */
CValve::CValve(const char *name, unsigned int value, VALVE_TYPE type)
    : m_name(0), m_value(value), m_type(type), m_name_pooled(0), m_pooled(0), m_socom2_bit26(0),
      m_unused(0)
{
    SetName(name);
}

/* the name taken as it is, owned by the caller's pool */
void CValve::AssignName(char *name)
{
    FreeName();
    m_name = name;
    m_name_pooled = 1;
}

void CValve::SetName(const char *name)
{
    FreeName();
    m_name = name ? _strdup(name, 0, 0) : 0;
    m_name_pooled = 0;
}

void CValve::FreeName()
{
    if (m_name_pooled)
        m_name = 0;
    else {
        if (m_name)
            _free(m_name, 0, 0);
        m_name = 0;
    }
    m_name_pooled = 0;
}
