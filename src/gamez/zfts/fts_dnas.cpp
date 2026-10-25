/* fts_dnas.cpp -- the DNAS authentication: the UI command DNASAuthenticate and the state machine it
 * runs, CSMDNASAuthenticate (SOCOM II's, by its RTTI at 0x3F33C8; SOCOM 1's demo has no DNAS
 * client, so the file's name and every name below but the class's are ours). OVL1 0x2CC670-
 * 0x2CD66F, after fts_patch.cpp (the patch download) and before the unit of 0x2CD670: the unit's
 * own member-pointer pool (0x3DE8C0, CSimpleStateMachine's Finish and Idle again) and its rodata
 * (the two state switches' tables at 0x3F3350/0x3F3380, the error number's "%d", the RTTI's name)
 * start there.
 *
 * DNAS.BIN (OVERLAY/REL, loaded into the OVL2 slot for the authentication) is what the machine
 * drives: its functions are called by address (func_004C6C28 ...), never by the OVL2 names whose
 * functions cover those addresses. Their roles read off the calls -- an initialisation handed the
 * title's authentication data, a start, a status poll, an abort and a shutdown -- match libdnas2's
 * sceDNAS2Init, sceDNAS2AuthNetStart/GetUniqueID, sceDNAS2GetStatus, sceDNAS2Abort and
 * sceDNAS2Shutdown, which stay predictions.
 *
 * Defined in reverse of their image order (-inline deferred). */

#include <stdio.h>
#include <string.h>

#include "../zanim/zanim_menu.h"
#include "../zsys/zsys_public.h"

/* ---- DNAS.BIN (called by address; the shapes are the call sites') ---- */

/* the title's authentication data: the line, the pass phrase and the authentication file
 * (libdnas2's sceDNAS2TitleAuthData_t has this shape: 20 bytes, memset before it is filled) */
struct DNASData {
    char *ptr;
    int size;
};
struct DNASTitleAuthData {
    int line_type;       // 0x0
    DNASData pass_phrase;   // 0x4
    DNASData auth_data;     // 0xc
};

/* what the status poll answers (16 bytes, compared whole with the last answer): the state, and the
 * error when the state says it failed */
struct DNASStatus {
    int code;    // 0x0
    int error;   // 0x4
    int _8;
    int _c;
};

/* the unique-ID request: a product number, the buffer and its size (0x441800, 12 bytes) */
struct DNASUniqueIDReq {
    int product;   // 0x0
    unsigned char *buf;   // 0x4
    int *size;   // 0x8
};

/* the authentication's start: a mode, six bytes zeroed, a buffer and its size (a 20-byte local) */
struct DNASAuthReq {
    int mode;   // 0x0
    unsigned char zero[6];   // 0x4
    unsigned char *buf;   // 0xc
    int *size;   // 0x10
};

extern "C" {
int func_004C6C28(DNASTitleAuthData *data, int priority, int a2, int a3, int a4);  /* (sceDNAS2Init) */
int func_004C7238(DNASUniqueIDReq *req);    /* (the unique ID asked for) */
int func_004C6DF0(DNASAuthReq *req);        /* (the authentication started) */
int func_004C77B8(DNASStatus *status);      /* (sceDNAS2GetStatus) */
int func_004C76A0();                        /* (sceDNAS2Abort) */
int func_004C73F0();                        /* (sceDNAS2Shutdown) */
}

/* ---- the engine's pieces called out of line ---- */

extern "C" {
/* a command's valve: CValve::Create(ArgName(hdr), 2) (zanim_menu.cpp's, out of line) */
CValve *func_002846F0(C2DAnimCmdHdr *hdr);
/* the IOP's modules switched to a set (1, 2 the game's, 3 the network's; the object at 0x49E7E8):
 * whether it went */
bool func_003515D0(void *modules, unsigned char set);
/* a localised string built: the table's text for an id with its arguments substituted */
void func_00363F60(char *out, int size, int id, const char **args, int nargs);
/* the CD file read by sector (util_sonyio.cpp's CFileCD, out of line) */
void func_0039DEA0(void *file);                        /* CFileCD::CFileCD */
bool func_0039DDC0(void *file, int lsn);               /* CFileCD::Open */
void func_0039DDB0(void *file);                        /* CFileCD::Close */
void func_0039DE40(void *file, int flags);             /* CFileCD::~CFileCD */
int Read__7CFileCDFPci(void *file, char *buf, int size);
/* CIOPManager::UnRegisterRebootCallback (zinput.h's CIOPManager declares only the register) */
void UnRegisterRebootCallback__11CIOPManagerFi(CIOPManager *iop, int id);

extern unsigned char data_0049E7E8[];   /* the IOP's module sets */
extern int data_0049E2D4;               /* the ticks one wait sleeps */

int RotateThreadReadyQueue(int priority);   /* the kernel's */
}

/* the unit's statics, held to their addresses by their placeholders */
extern "C" {
extern char data_004416E0[0x100];           /* the error's text */
extern char data_004417E0[0x10];            /* the error's number */
extern DNASStatus data_004417F0;            /* the authentication's last status */
extern DNASUniqueIDReq data_00441800;
extern DNASStatus data_00441810;            /* the unique ID's last status */
extern DNASTitleAuthData data_00441820;
extern unsigned char data_00441840[0x80];   /* the unique ID */
extern unsigned char data_004418C0[];       /* the authentication's buffer */
extern char data_003DE8E0[8];               /* the pass phrase */
extern int data_003DE918;                   /* the unique ID's size (0x80) */
extern int data_003DE920;                   /* the authentication buffer's size (0x9680) */
}

/* the DNAS authentication as a machine: the authentication file read off the disc and handed to
 * DNAS.BIN, the unique ID asked for, then the authentication itself; the command's valve told how
 * it went, DNAS_ERROR_CODE the error (SOCOM II's CSMDNASAuthenticate, its vtable 0x4065A0) */
class CSMDNASAuthenticate : public CSimpleStateMachine {
public:
    CSMDNASAuthenticate(CValve *valve)
    {
        m_reboot = -1;
        m_valve = valve;
        m_error = CValve::Create("DNAS_ERROR_CODE", (VALVE_TYPE)1);
        SetValveValue(m_error, 0);
    }
    virtual ~CSMDNASAuthenticate() {}
    virtual bool Start();

    void LoadAuthFile(unsigned char event);
    void GetUniqueID(unsigned char event);
    void Authenticate(unsigned char event);
    static bool RebootCallback(CIOPManager::RebootPhase phase, CIOPManager::RebootMode mode, void *data);

    /* DNAS.BIN let go of: the reboot callback removed, the network's modules back to the game's */
    void Shutdown()
    {
        UnRegisterRebootCallback__11CIOPManagerFi(&theIOPManager, m_reboot);
        func_004C73F0();
        func_003515D0(data_0049E7E8, 2);
        SetState(&CSimpleStateMachine::Finish);
    }
    /* a failure: its number in DNAS_ERROR_CODE (negated), the valve off, the menu told */
    void Fail(const char *text, const int &error)
    {
        SetValveValue(m_error, -error);
        SetValveValue(m_valve, 0);
        theGame.m_menuState.RunEventAnim("MP_DNAS_ERROR", text);
        Shutdown();
    }

    unsigned char m_auth[0x10000];   // 0x38: the authentication file
    int m_reboot;   // 0x10038: the reboot callback's id
    CValve *m_valve;   // 0x1003C
    CValve *m_error;   // 0x10040: DNAS_ERROR_CODE
};   // 0x10044 bytes

/* the machine the command runs */
extern "C" CSMDNASAuthenticate *data_0044AF40;

/* a DNAS error's text: the localised message for its number, the number substituted (ours) */
const char *DNASErrorString(int error)
{
    const char *args[1];
    int id;

    data_004416E0[0] = 0;
    switch (error) {
    case -101: id = 0x451; break;
    case -102: id = 0x452; break;
    case -103: id = 0x453; break;
    case -104: id = 0x454; break;
    case -105: id = 0x455; break;
    case -106: id = 0x456; break;
    case -107: id = 0x457; break;
    case -108: id = 0x458; break;
    case -201: id = 0x459; break;
    case -202: id = 0x45A; break;
    case -203: id = 0x45B; break;
    case -204: id = 0x45C; break;
    case -401: id = 0x460; break;
    case -402: id = 0x461; break;
    case -403: id = 0x462; break;
    case -404: id = 0x463; break;
    case -601: id = 0x464; break;
    case -602: id = 0x55E; break;
    case -610: id = 0x465; break;
    case -611: id = 0x466; break;
    case -612: id = 0x467; break;
    case -613: id = 0x468; break;
    case -614: id = 0x469; break;
    case -615: id = 0x46A; break;
    case -616: id = 0x46B; break;
    case -617: id = 0x46C; break;
    case -618: id = 0x46D; break;
    case -619: id = 0x46E; break;
    case -620: id = 0x46F; break;
    case -621: id = 0x470; break;
    case -622: id = 0x471; break;
    case -623: id = 0x472; break;
    case -624: id = 0x473; break;
    case -625: id = 0x474; break;
    case -701: id = 0x475; break;
    case -702: id = 0x476; break;
    case -703: id = 0x477; break;
    default: id = 0x478; break;
    }
    if (id > 0) {
        args[0] = data_004417E0;
        sprintf(data_004417E0, "%d", error);
        func_00363F60(data_004416E0, 0x100, id, args, 1);
    }
    return data_004416E0;
}

/* the third state: the authentication started, then polled until it is done (the valve on) or
 * fails */
void CSMDNASAuthenticate::Authenticate(unsigned char event)
{
    switch (event) {
    case 0: {
        DNASAuthReq req;
        int error;

        req.mode = 1;
        memset(req.zero, 0, 6);
        req.buf = data_004418C0;
        req.size = &data_003DE920;
        if ((error = func_004C6DF0(&req)) != 0)
            Fail(DNASErrorString(error), error);
        break;
    }
    case 1: {
        DNASStatus status;

        if (!func_004C77B8(&status)) {
            if (memcmp(&data_004417F0, &status, 16)) {
                switch (status.code) {
                case 0:
                case 1:
                case 2:
                case 3:
                case 4:
                case 9:
                    break;
                case 5:
                case 8:
                    SetValveValue(m_valve, 1);
                    Shutdown();
                    break;
                case 6:
                case 7:
                    Fail(DNASErrorString(status.error), status.error);
                    break;
                }
            }
            memcpy(&data_004417F0, &status, 16);
        }
        RotateThreadReadyQueue(5);
        break;
    }
    case 2:
        break;
    }
}

/* the second state: the unique ID asked for, then polled until it is there */
void CSMDNASAuthenticate::GetUniqueID(unsigned char event)
{
    switch (event) {
    case 0: {
        int error;

        data_00441800.product = 1000000003;
        data_00441800.buf = data_00441840;
        data_00441800.size = &data_003DE918;
        if ((error = func_004C7238(&data_00441800)) != 0)
            Fail(DNASErrorString(error), error);
        break;
    }
    case 1: {
        DNASStatus status;

        if (!func_004C77B8(&status)) {
            if (memcmp(&data_00441810, &status, 16)) {
                switch (status.code) {
                case 0:
                case 1:
                case 2:
                case 3:
                case 4:
                case 9:
                    break;
                case 5:
                case 8:
                    SetState((SSMState)&CSMDNASAuthenticate::Authenticate);
                    break;
                case 6:
                case 7:
                    Fail(DNASErrorString(status.error), status.error);
                    break;
                }
            }
            memcpy(&data_00441810, &status, 16);
        }
        RotateThreadReadyQueue(5);
        break;
    }
    case 2:
        break;
    }
}

/* the first state: the authentication file read off the disc (sector 10110) and handed to DNAS.BIN
 * with the pass phrase */
void CSMDNASAuthenticate::LoadAuthFile(unsigned char event)
{
    switch (event) {
    case 0: {
        unsigned char file[0x14];
        int error = 0;

        func_0039DEA0(file);
        memset(m_auth, 0, 0x10000);
        memset(&data_00441820, 0, 0x14);
        if (func_0039DDC0(file, 10110)) {
            Read__7CFileCDFPci(file, (char *)m_auth, 0x10000);
            func_0039DDB0(file);
            data_00441820.line_type = 1;
            data_00441820.auth_data.ptr = (char *)m_auth;
            data_00441820.auth_data.size = 0x10000;
            data_00441820.pass_phrase.ptr = data_003DE8E0;
            data_00441820.pass_phrase.size = 8;
            if ((error = func_004C6C28(&data_00441820, 5, 0, 0, 0)) != 0)
                Fail(DNASErrorString(error), error);
            else
                SetState((SSMState)&CSMDNASAuthenticate::GetUniqueID);
        } else {
            Fail("Cannot open DNAS Authfile", error);
        }
        func_0039DE40(file, -1);
        break;
    }
    case 1:
        break;
    case 2:
        break;
    }
}

/* the unique ID DNAS gave, and its size (the lobby's, 0x2EEDE0, asks for it; ours) */
unsigned char *GetDNASUniqueID(int *size)
{
    if (size)
        *size = data_003DE918;
    return data_00441840;
}

/* the IOP about to reboot: DNAS aborted and waited for, shut down and waited for, then let go of
 * (ten waits each at most) */
bool CSMDNASAuthenticate::RebootCallback(CIOPManager::RebootPhase phase, CIOPManager::RebootMode mode, void *data)
{
    CSMDNASAuthenticate *sm = (CSMDNASAuthenticate *)data;

    if (phase == CIOPManager::REBOOT_BEGIN) {
        DNASStatus status;
        int i;

        func_004C76A0();
        for (i = 0; i < 10; i++) {
            func_004C77B8(&status);
            if (status.code == 2)
                break;
            zSysSleep(data_0049E2D4);
        }
        func_004C73F0();
        for (i = 0; i < 10; i++) {
            func_004C77B8(&status);
            if (status.code == 1)
                break;
            zSysSleep(data_0049E2D4);
        }
        sm->Shutdown();
    }
    return true;
}

/* the machine started: the network's modules loaded, the reboot callback registered, the file read */
bool CSMDNASAuthenticate::Start()
{
    CSimpleStateMachine::Start();
    if (func_003515D0(data_0049E7E8, 3)) {
        m_reboot = theIOPManager.RegisterRebootCallback(RebootCallback, this);
        SetState((SSMState)&CSMDNASAuthenticate::LoadAuthFile);
    }
    return true;
}

/* the command DNASAuthenticate: the machine made and started, then run until it is done (as
 * UIInitNetwork runs CSMInitNetwork) */
bool UIDNASAuthenticate(C2DAnimCmdHdr *hdr, float *dT)
{
    bool done = false;
    CValve *valve = func_002846F0(hdr);

    if (!data_0044AF40) {
        data_0044AF40 = new CSMDNASAuthenticate(valve);
        data_0044AF40->Start();
    } else {
        func_003A5C30(data_0044AF40);
        if (func_003A5C10(data_0044AF40)) {
            delete data_0044AF40;
            data_0044AF40 = 0;
            done = true;
        }
    }
    return done;
}
