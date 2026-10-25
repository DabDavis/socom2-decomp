/* fts_net_blather.cpp -- the network objects of kind "BlatherState" (the file and class names ours,
 * after the structure's name: SOCOM 1's CZBombState registers "BombState"). SOCOM II's, between
 * fts_net_bomb.cpp and fts_net_persona.cpp: OVL1 0x2C2800-0x2C2BAF -- the local object's creation
 * (0x2C2800, "BlatherObject": the player's SOP messages and taunts), the four remote callbacks and
 * their registration. Complete: 6 functions, all byte-exact. Defined in reverse of their image
 * order (-inline deferred). */

extern "C" {
void *memset(void *p, int c, unsigned int n);
char *strncpy(char *dst, const char *src, unsigned int n);
}

/* a network object as CZNetwork's table hands it out (only its data) */
struct NetObject {
    unsigned char _0[0x1c];
    void *m_data;   // 0x1c
};

/* the DME's ownership and lifespan of an object (their values are not in SOCOM 1's DWARF, only
 * their names in zNetCreateObject's) */
enum NetOwnershipStatus {};
enum NetObjectLifespan {};

class CZNetwork {
public:
    void zNetRegisterRemoteObjectCallback(int index, void (*creation)(int, int),
                                          void (*update)(int, int, int),
                                          void (*postupdate)(int, int, int),
                                          void (*deletion)(int, int));
    int zNetCreateObject(int *index, NetOwnershipStatus own, NetObjectLifespan life, unsigned int a,
                         int b, unsigned int c, int d, void *e, char *name, int structure);
    NetObject *zNetGetObject(int index);
};

/* a UI variable as the blather reads it: its type (3 a string) and the string (zui.h has the
 * whole class) */
class CUIVariable {
public:
    bool IsString() const { return m_type == 3; }
    const char *GetString() const { return m_type == 3 ? m_str : 0; }

    unsigned char m_type;   // 0x0
    const char *m_str;   // 0x4
};

class CUIVarManager {
public:
    CUIVariable *Find(const char *name);
};

extern CUIVarManager theUIVarManager;

/* the object's data: the player's five local SOP messages and five taunts, as the UI variables
 * MPLOCALSOP1-5 and MPTAUNTMSG1-5 hold them (460 bytes; the names ours) */
struct TypeBlatherObject {
    char m_sop[5][46];   // 0x0
    char m_taunt[5][46];   // 0xe6
};

extern CZNetwork theNetwork;

extern "C" {
extern int data_00441598;            /* the structure's index */
extern unsigned char data_004415A0[];   /* (its one field) */

void NetRegisterObjectStart();   /* (the DME's) */
void NetObjectField(void *field, int offset, int a, int b, int size, int type);
void NetRegisterStructure(int *index, const char *name, int size);
int func_002BD0E0();

void func_002C2AF0(int client, int index);
void func_002C2AC0(int client, int index, int size);
void func_002C2A90(int client, int index, int size);
void func_002C2A60(int client, int index);
}

/* the kind registered: one 460-byte field, and its callbacks (OVL1 0x2C2B20) */
extern "C" void func_002C2B20()
{
    NetRegisterObjectStart();
    NetObjectField(data_004415A0, 0, 1, 1, 460, func_002BD0E0());
    NetRegisterStructure(&data_00441598, "BlatherState", 460);
    theNetwork.zNetRegisterRemoteObjectCallback(data_00441598,
                                                func_002C2AF0,
                                                func_002C2AC0,
                                                func_002C2A90,
                                                func_002C2A60);
}

/* the remote callbacks: creation, update, post-update and deletion, each looking its object up
 * and doing nothing with its data (OVL1 0x2C2AF0, 0x2C2AC0, 0x2C2A90, 0x2C2A60). The data's load
 * stays in the target (lw v1,28(v0), as in SOCOM 1's RemoteGrenadePostUpdateCallback); a plain
 * dead local is dropped and the call tail-called, so the read here is volatile. */
extern "C" void func_002C2AF0(int client, int index)
{
    NetObject *obj = theNetwork.zNetGetObject(index);

    if (obj)
        void *data = *(void *volatile *)&obj->m_data;
}

extern "C" void func_002C2AC0(int client, int index, int size)
{
    NetObject *obj = theNetwork.zNetGetObject(index);

    if (obj)
        void *data = *(void *volatile *)&obj->m_data;
}

extern "C" void func_002C2A90(int client, int index, int size)
{
    NetObject *obj = theNetwork.zNetGetObject(index);

    if (obj)
        void *data = *(void *volatile *)&obj->m_data;
}

extern "C" void func_002C2A60(int client, int index)
{
    NetObject *obj = theNetwork.zNetGetObject(index);

    if (obj)
        void *data = *(void *volatile *)&obj->m_data;
}

/* the local player's object: created, its data cleared, and the five SOP messages and the five
 * taunts copied in from their UI variables, each a string or left empty (OVL1 0x2C2800). The
 * object is not tested for null, unlike the bomb's */
extern "C" int func_002C2800()
{
    int index;

    if (theNetwork.zNetCreateObject(&index, (NetOwnershipStatus)1, (NetObjectLifespan)2, 0, 0, 0,
                                    1, 0, "BlatherObject", data_00441598))
        return -1;
    TypeBlatherObject *data = (TypeBlatherObject *)theNetwork.zNetGetObject(index)->m_data;

    memset(data, 0, sizeof(TypeBlatherObject));
    CUIVariable *sop[5];

    sop[0] = theUIVarManager.Find("MPLOCALSOP1");
    sop[1] = theUIVarManager.Find("MPLOCALSOP2");
    sop[2] = theUIVarManager.Find("MPLOCALSOP3");
    sop[3] = theUIVarManager.Find("MPLOCALSOP4");
    sop[4] = theUIVarManager.Find("MPLOCALSOP5");
    char *dst = data->m_sop[0];

    for (int i = 0; i < 5; i++, dst += 46) {
        if (sop[i] && sop[i]->IsString())
            strncpy(dst, sop[i]->GetString(), 46);
    }
    CUIVariable *taunt[5];

    taunt[0] = theUIVarManager.Find("MPTAUNTMSG1");
    taunt[1] = theUIVarManager.Find("MPTAUNTMSG2");
    taunt[2] = theUIVarManager.Find("MPTAUNTMSG3");
    taunt[3] = theUIVarManager.Find("MPTAUNTMSG4");
    taunt[4] = theUIVarManager.Find("MPTAUNTMSG5");
    dst = data->m_taunt[0];
    for (int i = 0; i < 5; i++, dst += 46) {
        if (taunt[i] && taunt[i]->IsString())
            strncpy(dst, taunt[i]->GetString(), 46);
    }
    return index;
}
