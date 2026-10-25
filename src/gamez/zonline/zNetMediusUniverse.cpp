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
 * Defined in reverse of their image order (-inline deferred). */

#include <string.h>
#include <stdlib.h>
#include <string>
#include <vector>

#include "../zrdr/zrdr.h"
#include "../zsav/zsave.h"

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

/* ---- the universes (the Medius universe information server's answers) ---- */

/* a universe's status, the response kept whole (the Medius SDK's MediusUniverseStatusListResponse:
 * copied member by member, a class with a constructor of its own -- the constructor is ours) */
struct MediusUniverseStatusListResponse {
    MediusUniverseStatusListResponse() {}

    char MessageID[21];   // 0x0
    int StatusCode;   // 0x18
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

    MediusUniverseStatusListResponse m_info;   // 0x0
    char m_version[0x20];   // 0x338
    char m_url[0x80];   // 0x358
};   // 0x3d8

/* a line of the universe's news (the SDK's MediusGetUniverseNewsResponse) */
struct MediusGetUniverseNewsResponse {
    MediusGetUniverseNewsResponse() {}

    char MessageID[21];   // 0x0
    int StatusCode;   // 0x18
    char News[256];   // 0x1c
    char EndOfList;   // 0x11c
};   // 0x120

typedef std::vector<UniverseStatus> UniverseStatusList;
typedef std::vector<MediusGetUniverseNewsResponse> UniverseNewsList;

extern UniverseStatusList data_0044FD50;   /* the universes */
extern UniverseNewsList data_0044FD70;   /* the news */

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

/* --- the universes ---------------------------------------------------------------------------- */

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
