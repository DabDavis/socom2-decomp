/* zNetMediusUniverse.cpp -- the online lobby's universe information and accounts, new in SOCOM II:
 * the Medius universe information server's answers (the universes' status list, which the server
 * menu shows and the patch level is checked against, and the universe news), the state machine that
 * asks for them (CSMGetServerList, the 'GetServerList' UI command's), and the accounts kept on the
 * memory card per lobby server -- CZUniversInformationServer::CAcctDB, a vector of 0x1318-byte
 * records that is a save module twice over (CSaveModule for the profile file, CLobbySaveModule for
 * the lobby's profile blob). The class names are the RTTI's (0x3F5CA8, 0x3F5C88, 0x3F5E48); the
 * unit, its name and every member name are ours. OVL1 0x2FAFC0-0x30066F: after the lobby unit
 * (PasswordSecurity 0x2FAF50 its last function) and before the friends unit; ~CAcctDB, an inline
 * virtual, opens it (0x2FAFC0), the vector instances, the deleting destructor 0x300570 and the
 * thunks of CAcctDB's secondary tables close it (0x2FE230-0x30066F). Its static initialiser 0x4022D0
 * builds theAcctDB (0x44FC60) and the two universe vectors (0x44FD50, 0x44FD70) and registers their
 * destructors; the statics are declared here, not defined, as the other lobby units do.
 * Grouped by role, not yet in reverse of their image order (-inline deferred): every function
 * scores alone, but the unit wants reordering before its inline copies are placed. */

#include <string.h>
#include <stdlib.h>
#include <string>
#include <vector>

#include "../zrdr/zrdr.h"
#include "../zsav/zsave.h"
#include "../znet/znet.h"

/* ---- what the unit calls in other units ---- */

/* the UI's variables (zui.h's CUIVarManager, declared again here: zui.h's _zrdr cannot sit beside
 * zrdr.h's) */
enum UIVAR_LONGEVITY {
    UIVL_DORMANT = 0,
    UIVL_TEMPORARY = 1,
    UIVL_PERMANENT = 2,
    UIVL_PERSISTENT = 3
};

class CUIVariable : public _zrdr {
public:
    CUIVariable &operator=(const char *str);
};

class CUIVarManager {
public:
    CUIVariable *Add(const char *name, _zrdr *value, UIVAR_LONGEVITY longevity);
};
extern CUIVarManager theUIVarManager;

void *_malloc(unsigned int size, const char *file, int line);
void _free(void *p, const char *file, int line);
void zSysSleep(int ticks);

/* a valve (zanim.h's CValve, as the chat unit declares it again: its inline setter) */
enum VALVE_TYPE {
    VALVE_TYPE_0,
    VALVE_TYPE_1
};

enum VALVE_STATE {
    VALVE_SAME,
    VALVE_ON,
    VALVE_OFF
};

class CValve {
public:
    static CValve *Create(const char *name, VALVE_TYPE type);
    void MakeCallbacks(VALVE_STATE state);
    bool SetValue(int value)
    {
        VALVE_STATE state = VALVE_SAME;

        if (m_value != value)
            state = value == 0 ? VALVE_OFF : VALVE_ON;
        m_value = value;
        if (state != VALVE_SAME) {
            MakeCallbacks(state);
            return true;
        }
        return false;
    }

    char *m_name;   // 0x0
    unsigned short m_value;   // 0x4
    unsigned short m_flags;   // 0x6
};

/* the UI command that runs the machine below, and what it reads (zanim.h's and zanim_menu.h's
 * classes, only as far as this unit uses them: zanim.h's _zrdr cannot sit beside zrdr.h's) */
class CZAnim {
public:
    const char *GetName(int i);
};

class CZAnimMain {
public:
    unsigned char _0[0x38];
    CZAnim *m_CurAnim;   // 0x38
};
extern CZAnimMain ZAnim;

class C2DAnimCmdHdr {
public:
    unsigned char _0[8];
    unsigned short m_argument_index;   // 0x8
};

/* the lobby, as far as this unit reads it (zonline.h's CZOnlineLobby: the menus' event handler) */
class CZOnlineLobby {
public:
    unsigned char _0[0x58];
    void (*m_event_handler)(const char *event, const char *arg);   // 0x58
    unsigned char _5c[0xa8 - 0x5c];
    unsigned char m_a8;   // 0xa8: the DNAS key is there
};
extern CZOnlineLobby theLobby;

extern "C" {
char *strtok(char *s, const char *delim);   /* (MAIN's) */
extern int myConnectionIndex;   /* the Medius connection, -1 none */
extern unsigned int data_0049E2D4;   /* zSys's ticks per second */
const char *func_00361B40(int id);   /* a localised text */
void func_003A59D0(void *sm, int stack, int priority);   /* (the machine's thread started) */
}

/* a state machine whose states are member functions (zanim_menu.h's CSimpleStateMachine, declared
 * again here for the same reason: the layout and the two inline virtuals alike) */
class CSimpleStateMachine;
typedef void (CSimpleStateMachine::*SSMState)(unsigned char event);

extern "C" {
void func_002852B0(void *queue, SSMState state);    /* the state queue's push */
void func_003A5C30(CSimpleStateMachine *sm);        /* (the machine's tick) */
bool func_003A5C10(CSimpleStateMachine *sm);        /* (whether it is done) */
}

class CSimpleStateMachine {
public:
    CSimpleStateMachine();
    ~CSimpleStateMachine();
    virtual bool Start()
    {
        SetState(&CSimpleStateMachine::Idle);
        m_done = 0;
        return false;
    }
    virtual void SetState(const SSMState &state)
    {
        SSMState s = state;

        func_002852B0(m_queue, s);
    }

    void Idle(unsigned char event);
    void Finish(unsigned char event);

    unsigned int m_thread;   // 0x4
    SSMState m_state;   // 0x8
    unsigned char m_queue[0x18];   // 0x14
    unsigned char m_done;   // 0x2c
    int m_30;   // 0x30
    int m_thread_id;   // 0x34
};

/* ---- the universes (the Medius universe information server's answers) ---- */

/* an API call's result (int-sized) */
enum MediusErrorCode {
    MediusSuccess = 0,
    MediusErrorCode_Extent = 0x7FFFFFFF
};

bool CheckMediusError(MediusErrorCode err);

/* a response's status (int-sized) */
enum MediusCallbackStatus {
    MediusCallbackSuccess = 0,
    MediusCallbackStatus_Extent = 0x7FFFFFFF
};

bool CheckMediusCallbackStatus(const char *what, MediusCallbackStatus status);

/* a universe's status, the response kept whole (the Medius SDK's MediusUniverseStatusListResponse:
 * copied member by member, a class with a constructor of its own -- the constructor is ours) */
struct MediusUniverseStatusListResponse {
    MediusUniverseStatusListResponse() {}

    char MessageID[21];   // 0x0
    MediusCallbackStatus StatusCode;   // 0x18
    char UniverseName[128];   // 0x1c
    char DNS[128];   // 0x9c
    int Port;   // 0x11c
    char UniverseDescription[256];   // 0x120
    int Status;   // 0x220: 0 up
    int UserCount;   // 0x224
    int MaxUsers;   // 0x228
    char UniverseBilling[8];   // 0x22c
    char BillingSystemName[128];   // 0x234
    char EndOfList;   // 0x2b4
    char ExtendedInfo[128];   // 0x2b5: "<patch level> <patch server's URL>"
};   // 0x338

/* a universe as the list keeps it: its status, then the patch level and URL its extended
 * information carries */
struct UniverseStatus {
    UniverseStatus()
    {
        m_version[0] = 0;
        m_url[0] = 0;
    }
    /* the response kept, its patch level and URL split off its extended information */
    UniverseStatus(MediusUniverseStatusListResponse *resp)
    {
        m_version[0] = 0;
        m_url[0] = 0;
        if (resp) {
            char buf[0x80];
            char *t;

            m_info = *resp;
            strncpy(buf, m_info.ExtendedInfo, sizeof(buf));
            t = strtok(buf, " \t");
            if (t)
                strncpy(m_version, t, sizeof(m_version));
            t = strtok(0, " \t");
            if (t)
                strncpy(m_url, t, sizeof(m_url));
        }
    }

    MediusUniverseStatusListResponse m_info;   // 0x0
    char m_version[0x20];   // 0x338
    char m_url[0x80];   // 0x358
};   // 0x3d8

/* a line of the universe's news (the SDK's MediusGetUniverseNewsResponse) */
struct MediusGetUniverseNewsResponse {
    MediusGetUniverseNewsResponse() {}

    char MessageID[21];   // 0x0
    MediusCallbackStatus StatusCode;   // 0x18
    char News[256];   // 0x1c
    char EndOfList;   // 0x11c
};   // 0x120

typedef std::vector<UniverseStatus> UniverseStatusList;
typedef std::vector<MediusGetUniverseNewsResponse> UniverseNewsList;

extern UniverseStatusList data_0044FD50;   /* the universes */
extern UniverseNewsList data_0044FD70;   /* the news */
extern bool data_0044FD80;   /* the status list's last entry in */
extern bool data_0044FD88;   /* the news' last line in */
extern bool data_0044FD90;   /* the universe server's connection failed */

/* --- the Medius SDK's universe calls (OVL2, called through their placeholders) ---------------- */

/* what the universe information server is asked for: no session key, the information wanted and
 * the text's encoding and language */
struct MediusGetUniverseInformationRequest {
    char MessageID[21];   // 0x0
    int InfoType;   // 0x18: 3, the status list and the news
    int CharacterEncoding;   // 0x1c
    int Language;   // 0x20
};   // 0x24

typedef void (*MediusUniverseNewsCallback)(int, int, int, MediusGetUniverseNewsResponse *);
typedef void (*MediusUniverseStatusListCallback)(int, int, int, MediusUniverseStatusListResponse *);

extern "C" {
MediusErrorCode func_0064AD98(MediusGetUniverseInformationRequest *req,
                              MediusUniverseNewsCallback news,
                              MediusUniverseStatusListCallback status);   /* MediusGetUniverseInformation */
}

void cbGetUniverseNews(int, int, int, MediusGetUniverseNewsResponse *resp);
void cbGetUniverseStatusList(int, int, int, MediusUniverseStatusListResponse *resp);

/* --- the connection to the universe server (the DME SDK's types, as the chat unit has them) ---- */

/* a server's addresses (a plain struct, copied as words) */
struct NetAddressList {
    int AddressType0;   // 0x0
    char Address0[16];   // 0x4
    int BinaryAddress0;   // 0x14
    int AddressType1;   // 0x18
    char Address1[16];   // 0x1c
    int BinaryAddress1;   // 0x2c
};   // 0x30

/* a server's key (a plain struct: assigned as eight two-word passes) */
struct NetServerKey {
    unsigned int m_words[16];
};   // 0x40

/* where a server is and how to talk to it (a class with a constructor of its own, so copied
 * member by member) */
struct NetConnectionInfo {
    NetConnectionInfo() {}

    int Type;   // 0x0
    NetAddressList AddressList;   // 0x4
    int WorldID;   // 0x34
    NetServerKey ServerKey;   // 0x38
    char SessionKey[17];   // 0x78
    char AccessKey[17];   // 0x89
};   // 0x9c

/* a connection asked for: the server's info, then the buffer's size and the callback told the
 * connection's index */
struct MediusConnectParams {
    NetConnectionInfo ConnectInfo;   // 0x0
    unsigned char _9c[0xf0 - 0x9c];
    int BufferSize;   // 0xf0: 256
    void (*Callback)(int index);   // 0xf4
    int _f8;   // 0xf8
    int _fc;   // 0xfc
};   // 0x100

extern NetConnectionInfo data_0044FCA0;   /* the universe server */

extern "C" {
void func_0064A750(NetConnectionInfo *info, int type, const char *ip, int port, int flags);   /* (the info built for an address) */
void func_0064F4E0(NetServerKey *key);   /* (the DNAS key) */
int func_00300740(int kind, MediusConnectParams *params);   /* a connection asked for (the friends' unit's) */
void func_0030E3A0(CZNetwork *net, int);   /* (CZNetwork's) */
}

void cbConnectToUniverse(int index);

/* ---- the accounts ---- */

/* a side's equipment for a mission: the skin, the weapons, the three equipment slots (SKIN,
 * PRIMARY, SECONDARY, EQUIP1-3 in the profile) and whether it is set */
struct AcctLoadout {
    AcctLoadout() { m_valid = 0; }

    unsigned char m_skin;   // 0x0
    unsigned short m_primary;   // 0x2
    unsigned short m_secondary;   // 0x4
    unsigned short m_equip1;   // 0x6
    unsigned short m_equip2;   // 0x8
    unsigned short m_equip3;   // 0xa
    unsigned char m_valid;   // 0xc
};   // 0xe

/* a mission's name (a class of its own: the missions' array is copied as one block, so the
 * mission has no constructor of its own, yet each element's name is cleared with its sides' flags) */
struct AcctMissionName {
    AcctMissionName() { m_text[0] = 0; }

    char m_text[8];
};

/* a mission's equipment for each side (PROFILE_INFO's entries: TERR, SEAL, MISSION) */
struct AcctMission {
    AcctLoadout m_terr;   // 0x0
    AcctLoadout m_seal;   // 0xe
    AcctMissionName m_mission;   // 0x1c
};   // 0x24

#define ACCT_MISSIONS 0x80

/* an account: the lobby server it is for (HOST, PORT), the player's name, password, home town and
 * gender, the profile's checksum and the missions' equipment, and whether the password is kept */
struct AcctRecord {
    /* (a constructor of its own: the record is copied member by member -- the char arrays
     * bytewise, the missions as one block) */
    AcctRecord() {}

    /* the missions with a name, the entries a blob of them holds */
    int MissionCount()
    {
        int n = 0;

        for (int i = 0; i < ACCT_MISSIONS; i++)
            if (m_missions[i].m_mission.m_text[0])
                n++;
        return n;
    }

    char m_host[0x80];   // 0x0
    int m_port;   // 0x80
    char m_name[0x20];   // 0x84
    char m_password[0x20];   // 0xa4
    char m_town[0x3f];   // 0xc4
    char m_gender;   // 0x103
    char m_checksum[0x10];   // 0x104
    AcctMission m_missions[ACCT_MISSIONS];   // 0x114
    bool m_savepassword;   // 0x1314
};   // 0x1318

/* the lobby's half of a save (SOCOM II's CLobbySaveModule, its constructor 0x33EFA0 in the save
 * manager's unit: the vtable at 0, its manager, its name; three pure virtuals) */
class CSaferStr : public std::string {
};

class CLobbySaveModule {
public:
    CLobbySaveModule(const char *name, CSaveManager *manager);
    virtual void Save() = 0;
    virtual void Load(const char *blob) = 0;
    virtual void SetChecksum(const char *sum) = 0;

    void Write(const char *blob, int size);   /* 0x33EEA0 */

    CSaveManager *m_manager;   // 0x4
    CSaferStr m_name;   // 0x8
};   // 0x14

class CZUniversInformationServer {
public:
    class CAcctDB : public std::vector<AcctRecord>, public CSaveModule, public CLobbySaveModule {
    public:
        void Serialize(int flags);
        void UnSerialize(const char *name, CRdrFile *file, int flags);
        void Save();
        void Load(const char *blob);
        void SetChecksum(const char *sum);

        void Add(const char *host, int port, const char *name, const char *password,
                 const char *town, int gender, _zrdr *profiles, bool savepassword);
        void SetAcctInfo(const char *host, int port, const char *name, const char *password,
                         const char *town, int gender, bool savepassword);
        const char *GetChecksum();
        AcctRecord *Find(const char *host, int port, const char *name);
        AcctLoadout *GetLoadout(const char *host, int port, const char *name, const char *mission,
                                int side);
        void SetLoadout(const char *host, int port, const char *name, const char *mission,
                        AcctLoadout *loadout, int side);
        int Count(const char *host, int port);
        int IndexOf(const char *host, int port, const char *name);
        void Delete(const char *host, int port, int index);
        int GetGender(const char *host, int port, int index);
        bool GetSavePassword(const char *host, int port, int index);
        const char *GetTown(const char *host, int port, int index);
        const char *GetPassword(const char *host, int port, int index);
        const char *GetName(const char *host, int port, int index);
        const char *GetPatchURL(const char *level);
        bool VerifyPatchLevel(const char *level);
        int GetUniverseStatus(const char *name);
        int GetUniversePort(const char *name);
        const char *GetUniverseDNS(const char *name);
        const char *GetUniverseDescription(unsigned int i);
        const char *GetUniverseName(unsigned int i);
        void ConnectToUniverse(const char *host, int port);
        int GetNewsCount();
        const char *GetNewsLine(int i);

        /* the account by its lobby server and name, 0 none (inlined everywhere but Find itself) */
        AcctRecord *FindAcct(const char *host, int port, const char *name)
        {
            AcctRecord *r = 0;

            for (iterator i = begin(); i != end(); ++i) {
                if (!strcmp((*i).m_host, host) && port == (*i).m_port && !strcmp((*i).m_name, name)) {
                    r = &*i;
                    break;
                }
            }
            return r;
        }
        /* the index-th account on a lobby server, 0 none */
        AcctRecord *FindAcct(const char *host, int port, int index)
        {
            AcctRecord *r = 0;
            int n = 0;

            for (iterator i = begin(); i != end(); ++i) {
                if (!strcmp((*i).m_host, host) && port == (*i).m_port) {
                    if (n == index) {
                        r = &*i;
                        break;
                    }
                    n++;
                }
            }
            return r;
        }
    };
};

typedef CZUniversInformationServer::CAcctDB CAcctDB;

/* the lobby's blob written (CLobbySaveModule's, the save manager's unit: its placeholder, called on
 * the module by reference -- a pointer conversion would test the account database for null) */
extern "C" void func_0033EEA0(CLobbySaveModule &module, const char *blob, int size);

/* the current account's lobby server and name, as the lobby's UI variables hold them */
#define CURRENT_ACCOUNT()                                                                          \
    const char *host = theUIVarManager.Add("LOBBY_HOST", 0, UIVL_PERMANENT)->String();             \
    int port = theUIVarManager.Add("LOBBY_PORT", 0, UIVL_PERMANENT)->Int();                         \
    const char *name = theUIVarManager.Add("PLAYERNAMEVAR", 0, UIVL_TEMPORARY)->String()

/* --- the lobby's blob (CLobbySaveModule) and the profile file (CSaveModule) ------------------ */

/* the current account's missions' equipment written as the lobby's blob: a count byte, then the
 * named entries as they are.
 * OPEN, 4 instructions: two pairs of argument moves come out in parameter order where the target
 * has them the other way round -- _malloc(total, 0, 0)'s `move a0,s1` ahead of `move a1,zero`, and
 * memcpy(out, ...)'s `move a0,s3` ahead of the source's `addiu a1` (the target loads the plain
 * variable last both times). No change from: total unsigned, total assigned after its declaration,
 * no total (the size + 1 then lands in a0 straight), blob and out declared apart, a char * cursor,
 * `r->m_missions + i`, memcpy(out++, ...) */
void CAcctDB::Save()
{
    CURRENT_ACCOUNT();
    AcctRecord *r = FindAcct(host, port, name);

    if (r) {
        int size = r->MissionCount() * sizeof(AcctMission);
        int total = size + 1;

        if (size) {
            char *blob = (char *)_malloc(total, 0, 0);
            AcctMission *out = (AcctMission *)(blob + 1);

            if (blob) {
                memset(blob, 0, total);
                *blob = r->MissionCount();
                for (int i = 0; i < ACCT_MISSIONS; i++) {
                    if (r->m_missions[i].m_mission.m_text[0]) {
                        memcpy(out, &r->m_missions[i], sizeof(AcctMission));
                        out++;
                    }
                }
                func_0033EEA0(*this, blob, size + 1);
                if (blob)
                    _free(blob, 0, 0);
            }
        }
    }
}

/* the current account's profile checksum set */
void CAcctDB::SetChecksum(const char *sum)
{
    CURRENT_ACCOUNT();
    AcctRecord *r = FindAcct(host, port, name);

    if (r && r->m_checksum && sum)
        memcpy(r->m_checksum, sum, sizeof(r->m_checksum));
}

/* the current account's profile checksum, 0 none */
const char *CAcctDB::GetChecksum()
{
    const char *sum = 0;
    CURRENT_ACCOUNT();
    AcctRecord *r = FindAcct(host, port, name);

    if (r)
        sum = r->m_checksum;
    return sum;
}

/* the lobby's blob read back into the current account's missions */
void CAcctDB::Load(const char *blob)
{
    CURRENT_ACCOUNT();

    if (blob) {
        char n = *blob;
        AcctMission *m = (AcctMission *)(blob + 1);

        for (int i = 0; i < n; i++, m++) {
            if (m->m_seal.m_valid) {
                AcctLoadout l;

                memcpy(&l, &m->m_seal, sizeof(l));
                SetLoadout(host, port, name, m->m_mission.m_text, &l, 0);
            }
            if (m->m_terr.m_valid) {
                AcctLoadout l;

                memcpy(&l, &m->m_terr, sizeof(l));
                SetLoadout(host, port, name, m->m_mission.m_text, &l, 1);
            }
        }
    }
}

/* the profile file's accounts read: each one's details and profiles added */
void CAcctDB::UnSerialize(const char *name, CRdrFile *file, int flags)
{
    for (int i = 0; i < file->Count(); i++) {
        _zrdr *acct = file->Item(i);

        if (acct) {
            bool savepassword = true;
            int port = 0;
            int gender = 0;
            const char *host = zrdr_findstring(acct, "HOST");
            const char *pname = zrdr_findstring(acct, "NAME");
            const char *town = zrdr_findstring(acct, "TOWN");
            const char *password = zrdr_findstring(acct, "PASSWORD");

            zrdr_findint(acct, "PORT", &port, 1);
            zrdr_findint(acct, "GENDER", &gender, 1);
            _zrdr *profiles = zrdr_findtag(acct, "PROFILES");

            zrdr_findbool(acct, "SAVEPASSWORD", &savepassword);
            Add(host, port, pname, password, town, gender, profiles, savepassword);
        }
    }
}

/* the accounts written as the profile file (AcctInfo): each one's details, its checksum and its
 * missions' equipment, the password only when it is kept */
void CAcctDB::Serialize(int flags)
{
    CRdrEditor ed;

    ed.push(0);
    for (iterator i = begin(); i != end(); ++i) {
        ed.push(0);
        ed.addSTRING("HOST", (*i).m_host);
        ed.addSTRING("NAME", (*i).m_name);
        ed.addSTRING("TOWN", (*i).m_town);
        ed.addint("GENDER", (*i).m_gender);
        ed.addint("PORT", (*i).m_port);
        if ((*i).m_password[0] && (*i).m_savepassword)
            ed.addSTRING("PASSWORD", (*i).m_password);
        ed.addbool("SAVEPASSWORD", (*i).m_savepassword);
        ed.addstringnode("PROFILES");
        ed.push(0);
        ed.addstringnode("PROFILE_CHECKSUM");
        ed.push(0);
        for (int j = 0; j < 0x10; j++)
            ed.addintnode((*i).m_checksum[j]);
        ed.pop();
        ed.addstringnode("PROFILE_INFO");
        ed.push(0);
        for (int k = 0; k < ACCT_MISSIONS; k++) {
            AcctMission *m = &(*i).m_missions[k];

            if (m->m_mission.m_text[0]) {
                ed.push(0);
                ed.addSTRING("MISSION", m->m_mission.m_text);
                if (m->m_terr.m_valid) {
                    ed.addstringnode("TERR");
                    ed.push(0);
                    ed.addint("SKIN", m->m_terr.m_skin);
                    ed.addint("PRIMARY", m->m_terr.m_primary);
                    ed.addint("SECONDARY", m->m_terr.m_secondary);
                    ed.addint("EQUIP1", m->m_terr.m_equip1);
                    ed.addint("EQUIP2", m->m_terr.m_equip2);
                    ed.addint("EQUIP3", m->m_terr.m_equip3);
                    ed.pop();
                }
                if (m->m_seal.m_valid) {
                    ed.addstringnode("SEAL");
                    ed.push(0);
                    ed.addint("SKIN", m->m_seal.m_skin);
                    ed.addint("PRIMARY", m->m_seal.m_primary);
                    ed.addint("SECONDARY", m->m_seal.m_secondary);
                    ed.addint("EQUIP1", m->m_seal.m_equip1);
                    ed.addint("EQUIP2", m->m_seal.m_equip2);
                    ed.addint("EQUIP3", m->m_seal.m_equip3);
                    ed.pop();
                }
                ed.pop();
            }
        }
        ed.pop();
        ed.pop();
        ed.pop();
    }
    ed.pop();
    SaveRdr("AcctInfo", &ed);
}

/* --- the universes ---------------------------------------------------------------------------- */

/* the universe server's connection made or refused (the index negative) */
void cbConnectToUniverse(int index)
{
    if (index < 0)
        data_0044FD90 = 1;
}

/* the universe server looked up and connected to, with the DNAS key when there is one */
void CAcctDB::ConnectToUniverse(const char *host, int port)
{
    char ip[64];

    if (!theNetwork.zNetLookupHost(host, ip, sizeof(ip))) {
        MediusConnectParams params;
        NetServerKey key;

        func_0064A750(&params.ConnectInfo, 0, ip, port, 1);
        memcpy(&data_0044FCA0, &params.ConnectInfo, sizeof(NetConnectionInfo));
        if (theLobby.m_a8)
            func_0064F4E0(&key);
        else
            memset(&key, 0, sizeof(key));
        data_0044FCA0.ServerKey = key;
        params.ConnectInfo = data_0044FCA0;
        params.BufferSize = 256;
        params.Callback = cbConnectToUniverse;
        params._f8 = 0;
        params._fc = 0;
        data_0044FD90 = 0;
        if (func_00300740(2, &params))
            func_0030E3A0(&theNetwork, 1);
        else
            data_0044FD90 = 1;
    } else {
        data_0044FD90 = 1;
    }
}

/* a line of the universe's news kept; the news is in at its last line */
void cbGetUniverseNews(int, int, int, MediusGetUniverseNewsResponse *resp)
{
    if (CheckMediusCallbackStatus("UniverseNews", resp->StatusCode)) {
        data_0044FD70.push_back(*resp);
        if (resp->EndOfList)
            data_0044FD88 = 1;
    } else {
        data_0044FD88 = 1;
    }
}

/* the number of lines of news */
int CAcctDB::GetNewsCount()
{
    return data_0044FD70.size();
}

/* a line of the news */
const char *CAcctDB::GetNewsLine(int i)
{
    return data_0044FD70[i].News;
}

/* a universe kept (a temporary built from the response, below the editor in the frame); at the
 * list's last entry the server menu's SERVER_LIST variable built from them (each universe's name,
 * whether it is up, its users and its limit) */
void cbGetUniverseStatusList(int, int, int, MediusUniverseStatusListResponse *resp)
{
    if (CheckMediusCallbackStatus("UniverseStatusList", resp->StatusCode)) {
        data_0044FD50.push_back(UniverseStatus(resp));
        if (resp->EndOfList) {
            CRdrEditor ed;

            ed.push(0);
            for (UniverseStatusList::iterator i = data_0044FD50.begin(); i != data_0044FD50.end(); ++i) {
                ed.push(0);
                ed.addstringnode((*i).m_info.UniverseName);
                ed.addintnode(!(*i).m_info.Status);
                ed.addintnode((*i).m_info.UserCount);
                ed.addintnode((*i).m_info.MaxUsers);
                ed.pop();
            }
            ed.pop();
            theUIVarManager.Add("SERVER_LIST", &ed, UIVL_TEMPORARY);
            data_0044FD80 = 1;
        }
    } else {
        data_0044FD80 = 1;
    }
}

/* a universe's name by its place in the list, "" past its end */
const char *CAcctDB::GetUniverseName(unsigned int i)
{
    if (i < data_0044FD50.size())
        return data_0044FD50[i].m_info.UniverseName;
    return "";
}

/* a universe's description by its place in the list, "" past its end */
const char *CAcctDB::GetUniverseDescription(unsigned int i)
{
    if (i < data_0044FD50.size())
        return data_0044FD50[i].m_info.UniverseDescription;
    return "";
}

/* the named universe's address, 0 when it is not listed */
const char *CAcctDB::GetUniverseDNS(const char *name)
{
    for (UniverseStatusList::iterator i = data_0044FD50.begin(); i != data_0044FD50.end(); ++i)
        if (!strcmp((*i).m_info.UniverseName, name))
            return (*i).m_info.DNS;
    return 0;
}

/* the named universe's port, 0 when it is not listed */
int CAcctDB::GetUniversePort(const char *name)
{
    for (UniverseStatusList::iterator i = data_0044FD50.begin(); i != data_0044FD50.end(); ++i)
        if (!strcmp((*i).m_info.UniverseName, name))
            return (*i).m_info.Port;
    return 0;
}

/* the named universe's status, 0 when it is not listed */
int CAcctDB::GetUniverseStatus(const char *name)
{
    for (UniverseStatusList::iterator i = data_0044FD50.begin(); i != data_0044FD50.end(); ++i)
        if (!strcmp((*i).m_info.UniverseName, name))
            return (*i).m_info.Status;
    return 0;
}

/* whether the build is the patch level the named universe asks for (yes when it asks none) */
bool CAcctDB::VerifyPatchLevel(const char *level)
{
    bool ok = true;

    for (UniverseStatusList::iterator i = data_0044FD50.begin(); i != data_0044FD50.end(); ++i) {
        if (!strcmp((*i).m_info.UniverseName, level)) {
            if ((*i).m_version[0])
                ok = !strcmp("r0001", (*i).m_version);
            break;
        }
    }
    return ok;
}

/* the named universe's patch server (the last one listed under the name), 0 none */
const char *CAcctDB::GetPatchURL(const char *level)
{
    const char *url = 0;

    for (UniverseStatusList::iterator i = data_0044FD50.begin(); i != data_0044FD50.end(); ++i)
        if (!strcmp((*i).m_info.UniverseName, level))
            url = (*i).m_url;
    return url;
}

/* --- the accounts --------------------------------------------------------------------------- */

/* the index-th account's name on a lobby server, 0 none */
const char *CAcctDB::GetName(const char *host, int port, int index)
{
    AcctRecord *r = FindAcct(host, port, index);

    return r ? r->m_name : 0;
}

const char *CAcctDB::GetPassword(const char *host, int port, int index)
{
    AcctRecord *r = FindAcct(host, port, index);

    return r ? r->m_password : 0;
}

const char *CAcctDB::GetTown(const char *host, int port, int index)
{
    AcctRecord *r = FindAcct(host, port, index);

    return r ? r->m_town : 0;
}

bool CAcctDB::GetSavePassword(const char *host, int port, int index)
{
    AcctRecord *r = FindAcct(host, port, index);

    return r ? r->m_savepassword : false;
}

int CAcctDB::GetGender(const char *host, int port, int index)
{
    AcctRecord *r = FindAcct(host, port, index);

    return r ? r->m_gender : 0;
}

/* an account added or updated: its details, then the profile's checksum and each mission's
 * equipment from the profile file's PROFILES list */
void CAcctDB::Add(const char *host, int port, const char *name, const char *password,
                  const char *town, int gender, _zrdr *profiles, bool savepassword)
{
    AcctRecord blank;
    AcctRecord *r = FindAcct(host, port, name);

    if (!r) {
        push_back(blank);
        r = &back();
        memset(r, 0, sizeof(*r));
    }
    strncpy(r->m_host, host, sizeof(r->m_host));
    r->m_port = port;
    strncpy(r->m_name, name, sizeof(r->m_name));
    if (password)
        strncpy(r->m_password, password, sizeof(r->m_password));
    if (town)
        strncpy(r->m_town, town, sizeof(r->m_town));
    r->m_gender = gender;
    r->m_savepassword = savepassword;
    _zrdr *info = profiles ? zrdr_findtag(profiles, "PROFILE_INFO") : 0;

    if (info) {
        _zrdr *sum = zrdr_findtag(profiles, "PROFILE_CHECKSUM");
        int j = 0;

        if (sum)
            for (; j < 0x10 && (unsigned int)j < sum->Count(); j++)
                r->m_checksum[j] = sum->Item(j)->Int();
        for (int k = 0; k < info->Count(); k++) {
            _zrdr *entry = info->Item(k);

            if (entry) {
                const char *mission = zrdr_findstring(entry, "MISSION");
                AcctLoadout l;
                _zrdr *side = zrdr_findtag(entry, "SEAL");

                if (side) {
                    int v;

                    memset(&l, 0, sizeof(l));
                    l.m_valid = 1;
                    v = 0;
                    zrdr_findint(side, "SKIN", &v, 1);
                    l.m_skin = v;
                    zrdr_findint(side, "PRIMARY", &v, 1);
                    l.m_primary = v;
                    zrdr_findint(side, "SECONDARY", &v, 1);
                    l.m_secondary = v;
                    zrdr_findint(side, "EQUIP1", &v, 1);
                    l.m_equip1 = v;
                    zrdr_findint(side, "EQUIP2", &v, 1);
                    l.m_equip2 = v;
                    zrdr_findint(side, "EQUIP3", &v, 1);
                    l.m_equip3 = v;
                    SetLoadout(host, port, name, mission, &l, 0);
                }
                side = zrdr_findtag(entry, "TERR");
                if (side) {
                    int v;

                    memset(&l, 0, sizeof(l));
                    l.m_valid = 1;
                    v = 0;
                    zrdr_findint(side, "SKIN", &v, 1);
                    l.m_skin = v;
                    zrdr_findint(side, "PRIMARY", &v, 1);
                    l.m_primary = v;
                    zrdr_findint(side, "SECONDARY", &v, 1);
                    l.m_secondary = v;
                    zrdr_findint(side, "EQUIP1", &v, 1);
                    l.m_equip1 = v;
                    zrdr_findint(side, "EQUIP2", &v, 1);
                    l.m_equip2 = v;
                    zrdr_findint(side, "EQUIP3", &v, 1);
                    l.m_equip3 = v;
                    SetLoadout(host, port, name, mission, &l, 1);
                }
            }
        }
    }
}

/* a side's equipment for a mission set on an account (0 the SEALs, 1 the terrorists), the mission
 * added in the first free entry when the account has none for it yet */
void CAcctDB::SetLoadout(const char *host, int port, const char *name, const char *mission,
                         AcctLoadout *loadout, int side)
{
    AcctRecord *r = FindAcct(host, port, name);

    if (r && mission && *mission && side != 2) {
        int i;

        for (i = 0; i < ACCT_MISSIONS; i++) {
            AcctMission *m = &r->m_missions[i];

            if (!strcasecmp(m->m_mission.m_text, mission)) {
                if (side == 0) {
                    m->m_seal = *loadout;
                    m->m_seal.m_valid = 1;
                } else if (side == 1) {
                    m->m_terr = *loadout;
                    m->m_terr.m_valid = 1;
                }
                return;
            }
        }
        AcctMission add;

        memset(&add, 0, sizeof(add));
        if (mission)
            memcpy(add.m_mission.m_text, mission, sizeof(add.m_mission.m_text));
        if (side == 0) {
            add.m_seal = *loadout;
            add.m_seal.m_valid = 1;
        } else if (side == 1) {
            add.m_terr = *loadout;
            add.m_terr.m_valid = 1;
        }
        for (i = 0; i < ACCT_MISSIONS; i++) {
            AcctMission *m = &r->m_missions[i];

            if (!m->m_mission.m_text[0]) {
                if (m)
                    memcpy(m, &add, sizeof(add));
                break;
            }
        }
    }
}

/* a side's equipment for a mission kept on an account (0 the SEALs, 1 the terrorists), 0 none */
AcctLoadout *CAcctDB::GetLoadout(const char *host, int port, const char *name, const char *mission,
                                 int side)
{
    AcctLoadout *l = 0;

    if (side != 2 && mission && *mission) {
        AcctRecord *r = FindAcct(host, port, name);
        int i = 0;

        if (r) {
            for (; i < ACCT_MISSIONS; i++) {
                AcctMission *m = &r->m_missions[i];

                if (!strcasecmp(m->m_mission.m_text, mission)) {
                    if (side == 0 && m->m_seal.m_valid)
                        l = &m->m_seal;
                    else if (side == 1 && m->m_terr.m_valid)
                        l = &m->m_terr;
                    break;
                }
            }
        }
    }
    return l;
}

/* the account by its lobby server and name, 0 none */
AcctRecord *CAcctDB::Find(const char *host, int port, const char *name)
{
    return FindAcct(host, port, name);
}

/* an account's details set, the account added when the lobby server has none of its name (the
 * password kept only when it is to be saved) */
void CAcctDB::SetAcctInfo(const char *host, int port, const char *name, const char *password,
                          const char *town, int gender, bool savepassword)
{
    if (name && *name)
        Add(host, port, name, savepassword ? password : 0, town, gender, 0, savepassword);
}

/* the index-th account on a lobby server forgotten */
void CAcctDB::Delete(const char *host, int port, int index)
{
    int n = 0;

    for (iterator i = begin(); i != end(); ++i) {
        if (!strcmp((*i).m_host, host) && port == (*i).m_port) {
            if (n == index) {
                erase(i);
                return;
            }
            n++;
        }
    }
}

/* the named account's place among the lobby server's (their count when it is not there) */
int CAcctDB::IndexOf(const char *host, int port, const char *name)
{
    int n = 0;

    for (iterator i = begin(); i != end(); ++i) {
        if (!strcmp((*i).m_host, host) && port == (*i).m_port) {
            if (!strcmp((*i).m_name, name))
                break;
            n++;
        }
    }
    return n;
}

/* the accounts on a lobby server */
int CAcctDB::Count(const char *host, int port)
{
    int n = 0;

    for (iterator i = begin(); i != end(); ++i)
        if (!strcmp((*i).m_host, host) && port == (*i).m_port)
            n++;
    return n;
}

/* --- the server list's machine -------------------------------------------------------------- */

/* the universe server connected to, then asked for its status list and news: the command's valve
 * set when both are in (SOCOM II's CSMGetServerList, its vtable 0x406730: Start and the base's
 * SetState) */
class CSMGetServerList : public CSimpleStateMachine {
public:
    CSMGetServerList(CValve *valve) : m_valve(valve) {}
    virtual bool Start();
    void Connect(unsigned char event);
    void GetInfo(unsigned char event);

    /* how the connection went: -1 failed, 1 connected, 0 not yet */
    static int ConnectStatus()
    {
        int s = 0;

        if (data_0044FD90)
            s = -1;
        else if (myConnectionIndex != -1)
            s = 1;
        return s;
    }

    CValve *m_valve;   // 0x38
};   // 0x3c

extern CAcctDB data_0044FC60;   /* theAcctDB */
extern char data_0045A14C[];   /* theNetwork's universe server address */
extern int data_0045A18C;   /* and port */
extern CSMGetServerList *data_0044FD98;   /* the 'GetServerList' command's machine */

/* the status list and the news asked for; done when both have come in */
void CSMGetServerList::GetInfo(unsigned char event)
{
    switch (event) {
    case 0: {
        MediusGetUniverseInformationRequest req;

        data_0044FD50.clear();
        data_0044FD80 = 0;
        data_0044FD88 = 0;
        memset(req.MessageID, 0, sizeof(req.MessageID));
        req.InfoType = 3;
        req.CharacterEncoding = 2;
        req.Language = 1;
        CheckMediusError(func_0064AD98(&req, cbGetUniverseNews, cbGetUniverseStatusList));
        break;
    }
    case 1: {
        bool done = data_0044FD80 && data_0044FD88;

        if (done) {
            m_valve->SetValue(1);
            SetState(&CSimpleStateMachine::Finish);
        }
        break;
    }
    case 2:
        break;
    }
}

/* the universe server connected to; on a failure the menus told and the valve off */
void CSMGetServerList::Connect(unsigned char event)
{
    switch (event) {
    case 0:
        data_0044FC60.ConnectToUniverse(data_0045A14C, data_0045A18C);
        break;
    case 1:
        if (ConnectStatus() > 0) {
            SetState((SSMState)&CSMGetServerList::GetInfo);
        } else if (ConnectStatus() < 0) {
            const char *text = func_00361B40(450);

            m_valve->SetValue(0);
            if (theLobby.m_event_handler)
                theLobby.m_event_handler("MP_MEDIUS_ERROR", text);
            SetState(&CSimpleStateMachine::Finish);
        }
        break;
    case 2:
        break;
    }
}

/* the machine started at the connection */
bool CSMGetServerList::Start()
{
    CSimpleStateMachine::Start();
    SetState((SSMState)&CSMGetServerList::Connect);
    return true;
}

/* the 'GetServerList' UI command: the machine made and run on its thread, ticked each frame until
 * it is done (the argument names the valve it sets) */
bool UIGetServerList(C2DAnimCmdHdr *hdr, float *dT)
{
    bool done = false;
    CValve *valve = CValve::Create(ZAnim.m_CurAnim->GetName(hdr->m_argument_index), (VALVE_TYPE)2);

    if (!data_0044FD98) {
        data_0044FD98 = new CSMGetServerList(valve);
        func_003A59D0(data_0044FD98, 0x19000, 6);
    } else {
        func_003A5C30(data_0044FD98);
        if (func_003A5C10(data_0044FD98)) {
            delete data_0044FD98;
            data_0044FD98 = 0;
            done = true;
        } else {
            zSysSleep(data_0049E2D4 / 60);
        }
    }
    return done;
}
