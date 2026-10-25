/* fts_net_blather.cpp -- the network objects of kind "BlatherState" (the file and class names ours,
 * after the structure's name: SOCOM 1's CZBombState registers "BombState"). SOCOM II's, between
 * fts_net_bomb.cpp and fts_net_persona.cpp: OVL1 0x2C2A60-0x2C2BAF, the four remote callbacks and
 * their registration. Defined in reverse of their image order (-inline deferred). */

/* a network object as CZNetwork's table hands it out (only its data) */
struct NetObject {
    unsigned char _0[0x1c];
    void *m_data;   // 0x1c
};

class CZNetwork {
public:
    void zNetRegisterRemoteObjectCallback(int index, void (*creation)(int, int),
                                          void (*update)(int, int, int),
                                          void (*postupdate)(int, int, int),
                                          void (*deletion)(int, int));
    NetObject *zNetGetObject(int index);
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
