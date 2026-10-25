/* fts_patch.cpp -- the online patch: the UI commands DownloadPatch and DeletePatch and the state
 * machine DownloadPatch runs, CSMDownloadPatch (SOCOM II's, by its RTTI at 0x3F2F58; SOCOM 1's demo
 * has nothing like it, so the file's name and every name below but the class's are ours). OVL1
 * 0x2CA330-0x2CC66F, after CMsgEditor's unit and before fts_dnas.cpp (which starts its own member-
 * pointer pool, 0x3DE8C0; this unit's is 0x3DE7F0-0x3DE8BF).
 *
 * The machine fetches PatchServerURL + "APACHE00.ZDB" over HTTP with DNAS.BIN's HTTP client (it is
 * loaded into the OVL2 slot with the network's modules; its functions are called by address, never
 * by the OVL2 names that cover them) and writes the archive to the memory card entry by entry, each
 * one decompressed first. The calls' roles read off the error texts the code prints (sceHTTPCreate,
 * sceHTTPSetOption, sceHTTPOpen, sceHTTPRequest, sceHTTPClose, sceHTTPTerminate).
 *
 * Defined in reverse of their image order (-inline deferred). */

#include <stdio.h>
#include <string.h>

#include "../zanim/zanim_menu.h"

/* ---- DNAS.BIN's HTTP client (called by address; the shapes are the call sites') ---- */

/* a parsed URI: the path at 0x14 (the shape sceHTTPParseURI's answer has) */
struct HTTPURI {
    unsigned char _0[0x14];
    char *path;   // 0x14
};

/* a response: its headers' list at 0x14, the body at 0x18 and its length at 0x20 */
struct HTTPHeader {
    HTTPHeader *next;   // 0x0
};
struct HTTPResponse {
    unsigned char _0[0x14];
    HTTPHeader *headers;   // 0x14
    unsigned char *body;   // 0x18
    unsigned char _1c[4];
    int length;   // 0x20
};

extern "C" {
int func_00534830();                                 /* (the HTTP library started) */
int func_00544860();                                 /* (sceHTTPTerminate) */
int func_005447D8();                                 /* (the HTTP client's init) */
HTTPURI *func_005448F8(const char *uri, int flags);  /* (sceHTTPParseURI) */
void func_00544CA8(HTTPURI *uri);                    /* (the parsed URI freed) */
void *func_00541C10();                               /* (sceHTTPCreate) */
void func_00541D80(void *client);                    /* (sceHTTPDestroy) */
int func_00545800(void *client, int option, ...);    /* (sceHTTPSetOption) */
int func_005469F8(void *client);                     /* (sceHTTPOpen) */
int func_00544550(void *client);                     /* (sceHTTPRequest) */
long func_00544788(void *client);                    /* (the bytes received so far: a long) */
HTTPResponse *func_00544708(void *client);           /* (the response) */
int func_00546B30(void *client);                     /* (sceHTTPClose) */
int func_004C73F0();                                 /* (DNAS.BIN's shutdown, as fts_dnas.cpp's) */
/* the decompressor: an entry's unpacked size, then the unpacking (OVL2 0x539D00 is the row the seed
 * cut at 0x539D50, MAIN's loader calls both too) */
int func_00539D00(int size, void *data, unsigned int *unpacked);
int func_00539D50(int size, unsigned int unpacked, void *data);
}

/* ---- the engine's pieces called out of line ---- */

extern "C" {
CValve *func_002846F0(C2DAnimCmdHdr *hdr);       /* CValve::Create(ArgName(hdr), 2) */
bool func_003515D0(void *modules, unsigned char set);
extern unsigned char data_0049E7E8[];            /* the IOP's module sets */
extern float data_004A449C;                      /* the frame's time */
/* std::string's copy constructor (another unit's copy, out of line) */
void func_0022E560(void *str, const void *from);
void func_001E7EF0(void *str, int flags);        /* ~basic_string */
}

/* the lobby's file over HTTP (SOCOM II's CLobbyFileIO, the online lane's: its vtable 0x406760; 0x9C
 * bytes, a string at 0x90) */
class CLobbyFileIO : public CIO {
public:
    CLobbyFileIO() : m_handle(0), m_open(0), m_90(0), m_94(0), m_98(0)
    {
        m_path[0] = 0;
    }
    bool Open(const char *name, unsigned int mode);
    void Close();
    bool IsOpen() const;
    int GetSize() const;
    unsigned int GetMode() const;
    int ftell() const;
    int fseek(int offset, int whence);
    int fread(void *buf, int size);
    int fread(int size, void **buf);
    int fwrite(const void *buf, int size);
    int freadAsync(void *buf, int size);
    int fwriteAsync(const void *buf, int size);
    bool fsyncAsync(int mode, int *result);
    void fflush();

    int m_handle;   // 0x4
    unsigned char _8[4];
    char m_path[0x80];   // 0xc
    unsigned char m_open;   // 0x8c
    int m_90;   // 0x90
    int m_94;   // 0x94
    int m_98;   // 0x98
};   // 0x9c bytes

/* the downloaded archive: a ZDB's header (the entry count at 0x98, the entry size at 0x9C) and its
 * entries from 0xA0, each with its data's offset at 0x44 and size at 0x48 */
struct PatchEntry {
    unsigned char _0[0x44];
    int offset;   // 0x44
    unsigned int size;   // 0x48
    unsigned char _4c[0x5c - 0x4c];
};
struct PatchArchive {
    unsigned char _0[0x98];
    unsigned int count;   // 0x98
    int entry_size;   // 0x9c
    PatchEntry entries[1];   // 0xa0
};

/* the patch download as a machine: DNAS.BIN's HTTP library started, the archive's URI parsed,
 * opened, requested and received, then written to the memory card; DownloadState names each step,
 * DownloadStateValve counts them, DownloadByteCount the bytes (SOCOM II's CSMDownloadPatch, its
 * vtable 0x406570; the HTTP steps are virtual, slots 0x14-0x20) */
class CSMDownloadPatch : public CSimpleStateMachine {
public:
    CSMDownloadPatch(CValve *valve, const char *url)
    {
        m_byteCount = func_003857C0(&theUIVarManager, "DownloadByteCount", 0, (UIVAR_LONGEVITY)1);
        m_stateName = func_003857C0(&theUIVarManager, "DownloadState", 0, (UIVAR_LONGEVITY)1);
        m_stateValve = CValve::Create("DownloadStateValve", (VALVE_TYPE)2);
        m_archive = 0;
        m_length = 0;
        m_valve = valve;
        m_stateValve->SetValue(0);
        m_url = url;
        if (m_url[m_url.size() - 1] != '/')
            m_url += "/";
    }
    virtual ~CSMDownloadPatch() {}
    virtual bool Start();
    /* every change of state counted on DownloadStateValve, then the base's push (inlined: its
     * copy of the member pointer is the second one, made with ld/sd).
     * OPEN, 8 instructions: the target loads the valve's value twice -- once for the increment,
     * once for the setter's comparison (lhu v1; lhu v0; addiu v1,v1,1) -- where every form tried
     * shares one load: the setter as a static or as a member of a CValve subclass, the value
     * through an accessor, a local or another pointer, `1 +` first, an Increment() member, the
     * comparison either way round */
    virtual void SetState(const SSMState &state)
    {
        SSMState s = state;

        if (m_stateValve)
            m_stateValve->SetValue(GetValue(m_stateValve) + 1);
        CSimpleStateMachine::SetState(s);
    }
    virtual void ParseURI(unsigned char event);
    virtual void Get(unsigned char event);
    virtual void Receive(unsigned char event);
    virtual void Close(unsigned char event);

    void InitializeDNAS(unsigned char event);
    void Open(unsigned char event);
    void CreateArchive(unsigned char event);
    void WriteTable(unsigned char event);
    void WriteEntries(unsigned char event);
    void RewriteHeader(unsigned char event);
    void RewriteTable(unsigned char event);
    static void EndOfOpen(void *client, int result, void *data);
    static void EndOfTransaction(void *client, int result, void *data);

    /* a valve's value (ours: CValve::GetValue's shape) */
    static int GetValue(CValve *valve) { return valve->m_value; }
    /* the path's last component, for DownloadState */
    const char *FileName()
    {
        const char *name = m_uri->path;
        int i = strlen(name) - 1;

        for (; i > 0; i--) {
            if (name[i] == '/') {
                name += i + 1;
                break;
            }
        }
        return name;
    }
    /* the HTTP library let go of and the machine finished */
    void Shutdown()
    {
        func_003862A0(m_stateName, "Finish");
        if (func_00544860())
            theGame.m_menuState.RunEventAnim("MP_DOWNLOAD_ERROR", "Download Error: sceHTTPTerminate failed\n");
        func_004C73F0();
        func_003515D0(data_0049E7E8, 2);
        SetState(&CSimpleStateMachine::Finish);
    }
    /* a failure: the menu told, the valve off */
    void Error(const char *text, int code)
    {
        char buf[256];

        sprintf(buf, "Download Error: %s (%d)", text, code);
        theGame.m_menuState.RunEventAnim("MP_DOWNLOAD_ERROR", buf);
        SetValveValue(m_valve, 0);
        Shutdown();
    }

    CUIVariable *m_byteCount;   // 0x38: DownloadByteCount
    CUIVariable *m_stateName;   // 0x3c: DownloadState
    CValve *m_stateValve;   // 0x40: DownloadStateValve
    PatchArchive *m_archive;   // 0x44: the response's body
    int m_length;   // 0x48
    CMemCardIO m_card;   // 0x4c
    CLobbyFileIO m_lobbyFile;   // 0x64
    CValve *m_valve;   // 0x100: the command's
    std::string m_url;   // 0x104
    HTTPURI *m_uri;   // 0x110
    void *m_client;   // 0x114
};   // 0x118 bytes

/* the machine the command runs, and the states' statics (held to their addresses) */
extern "C" {
extern CSMDownloadPatch *data_004416D0;
extern int data_00441688;          /* RewriteTable's write */
extern int data_00441690;          /* RewriteHeader's write */
extern int data_00441698;          /* WriteEntries' write */
extern void *data_004416A0;        /* WriteEntries' unpacked entry */
extern unsigned int data_004416A8;   /* WriteEntries' entry */
extern int data_004416B0;          /* WriteTable's write */
extern int data_004416B8;          /* CreateArchive's write */
extern long data_004416C0;         /* Get's last byte count */
extern float data_004416C8;        /* Get's time without progress */
}

/* the last step: the client and the URI freed, the machine finished */
void CSMDownloadPatch::Close(unsigned char event)
{
    switch (event) {
    case 0:
        func_00541D80(m_client);
        m_client = 0;
        func_00544CA8(m_uri);
        m_uri = 0;
        m_archive = 0;
        SetValveValue(m_valve, 1);
        Shutdown();
        break;
    case 1:
        break;
    case 2:
        break;
    }
}

/* the response taken: its body is the archive (written next); left, the connection closed */
void CSMDownloadPatch::Receive(unsigned char event)
{
    switch (event) {
    case 0: {
        char text[256];
        HTTPResponse *response;

        sprintf(text, "Received %s", FileName());
        func_003862A0(m_stateName, text);
        if (!(response = func_00544708(m_client))) {
            Error("Empty response", 0);
        } else {
            HTTPHeader *h;

            for (h = response->headers; h; h = h ? h->next : 0)
                ;
            m_length = response->length;
            m_archive = (PatchArchive *)response->body;
            SetState((SSMState)&CSMDownloadPatch::CreateArchive);
        }
        break;
    }
    case 1:
        break;
    case 2: {
        int err;

        if ((err = func_00546B30(m_client)) < 0)
            Error("sceHTTPClose failed", err);
        break;
    }
    }
}

/* the request's end: the response next */
void CSMDownloadPatch::EndOfTransaction(void *client, int result, void *data)
{
    CSMDownloadPatch *sm = (CSMDownloadPatch *)data;

    if (result == 0)
        sm->SetState((SSMState)&CSMDownloadPatch::Receive);
    else
        sm->Error("sceHTTPO_EndOfTransactionCB (cb) failed", result);
}

/* the request made, then watched: the byte count shown, thirty seconds without progress a failure */
void CSMDownloadPatch::Get(unsigned char event)
{
    switch (event) {
    case 0: {
        char text[256];
        int err;

        sprintf(text, "Get %s", FileName());
        func_003862A0(m_stateName, text);
        if ((err = func_00545800(m_client, 9, 0)) < 0) {
            Error("sceHTTPSetOption(sceHTTPO_BlockingMode) failed", err);
            break;
        }
        if ((err = func_00545800(m_client, 10, EndOfTransaction, this)) < 0) {
            Error("sceHTTPSetOption (sceHTTPO_EndOfTransactionCB) failed", err);
            break;
        }
        if ((err = func_00544550(m_client)) < 0)
            Error("sceHTTPRequest failed", err);
        break;
    }
    case 1: {
        long n = func_00544788(m_client);

        if (n > 0)
            m_byteCount->SetInt(n);
        if (n == data_004416C0) {
            data_004416C8 += data_004A449C;
            if (data_004416C8 > 30.0f)
                Error("Timeout during download", (int)data_004416C8);
        } else {
            data_004416C0 = n;
            data_004416C8 = 0.0f;
        }
        break;
    }
    case 2:
        break;
    }
}

/* the archive's URI parsed: the server's path and APACHE00.ZDB */
void CSMDownloadPatch::ParseURI(unsigned char event)
{
    switch (event) {
    case 0: {
        unsigned int uri[3];

        func_0022E560(uri, &m_url);
        std::func_001EEB70(uri, "APACHE00.ZDB");
        m_uri = func_005448F8((uri[0] & 1) ? (const char *)uri[2] : (const char *)uri + 1, 0x60);
        if (!m_uri)
            Error("Bad URI", 0);
        SetState((SSMState)&CSMDownloadPatch::Open);
        func_001E7EF0(uri, -1);
        break;
    }
    case 1:
        break;
    case 2:
        break;
    }
}

/* the archive's table written again, now that the entries' offsets are known; left, the file
 * closed and the reserve files made again */
void CSMDownloadPatch::RewriteTable(unsigned char event)
{
    int result;

    switch (event) {
    case 0:
        data_00441688 = m_card.fwriteAsync(m_archive->entries, m_archive->entry_size * m_archive->count);
        break;
    case 1:
        if (m_card.fsyncAsync(data_00441688, &result))
            SetState((SSMState)&CSMDownloadPatch::Close);
        break;
    case 2:
        m_card.Close();
        theMemCard.CreateReserveFiles();
        break;
    }
}

/* back to the file's start, the header written again */
void CSMDownloadPatch::RewriteHeader(unsigned char event)
{
    int result;

    switch (event) {
    case 0:
        m_card.fseek(0, 0);
        data_00441690 = m_card.fwriteAsync(m_archive, 0xa0);
        break;
    case 1:
        if (m_card.fsyncAsync(data_00441690, &result))
            SetState((SSMState)&CSMDownloadPatch::RewriteTable);
        break;
    case 2:
        break;
    }
}

/* the entries written one at a time, each unpacked first and its offset noted */
void CSMDownloadPatch::WriteEntries(unsigned char event)
{
    int result;
    unsigned int unpacked = 0;

    switch (event) {
    case 0: {
        unsigned int size = m_archive->entries[data_004416A8].size;

        data_004416A0 = _malloc(size, 0, 0);
        memcpy(data_004416A0, (char *)m_archive + m_archive->entries[data_004416A8].offset, size);
        if (func_00539D00(size, data_004416A0, &unpacked) >= 0) {
            if (size < unpacked)
                data_004416A0 = _realloc(data_004416A0, unpacked, 0, 0);
            m_archive->entries[data_004416A8].size = func_00539D50(size, unpacked, data_004416A0);
            m_archive->entries[data_004416A8].offset = m_card.ftell();
            data_00441698 = m_card.fwriteAsync(data_004416A0, m_archive->entries[data_004416A8].size);
        }
        break;
    }
    case 1:
        if (m_card.fsyncAsync(data_00441698, &result)) {
            if (++data_004416A8 < m_archive->count) {
                unsigned int size;

                _free(data_004416A0, 0, 0);
                size = m_archive->entries[data_004416A8].size;
                data_004416A0 = _malloc(size, 0, 0);
                memcpy(data_004416A0, (char *)m_archive + m_archive->entries[data_004416A8].offset, size);
                if (func_00539D00(size, data_004416A0, &unpacked) >= 0) {
                    if (size < unpacked)
                        data_004416A0 = _realloc(data_004416A0, unpacked, 0, 0);
                    m_archive->entries[data_004416A8].size = func_00539D50(size, unpacked, data_004416A0);
                    m_archive->entries[data_004416A8].offset = m_card.ftell();
                    data_00441698 = m_card.fwriteAsync(data_004416A0, m_archive->entries[data_004416A8].size);
                }
            } else {
                SetState((SSMState)&CSMDownloadPatch::RewriteHeader);
            }
        }
        break;
    case 2:
        if (data_004416A0) {
            _free(data_004416A0, 0, 0);
            data_004416A0 = 0;
            data_004416A8 = 0;
        }
        break;
    }
}

/* the entries' table written after the header */
void CSMDownloadPatch::WriteTable(unsigned char event)
{
    int result;

    switch (event) {
    case 0:
        data_004416B0 = m_card.fwriteAsync(m_archive->entries, m_archive->entry_size * m_archive->count);
        break;
    case 1:
        if (m_card.fsyncAsync(data_004416B0, &result))
            SetState((SSMState)&CSMDownloadPatch::WriteEntries);
        break;
    case 2:
        break;
    }
}

/* the old patch and scratch pad deleted, APACHE00.ZDB created on the card and its header written */
void CSMDownloadPatch::CreateArchive(unsigned char event)
{
    int result;

    switch (event) {
    case 0:
        theMemCard.DeleteScratchpad();
        theMemCard.DeletePatchFile();
        if (m_card.Open("APACHE00.ZDB", 10)) {
            data_004416B8 = m_card.fwriteAsync(m_archive, 0xa0);
        } else {
            theMemCard.CreateReserveFiles();
            Error("Cannot create patch archive", 0);
        }
        break;
    case 1:
        if (m_card.fsyncAsync(data_004416B8, &result))
            SetState((SSMState)&CSMDownloadPatch::WriteTable);
        break;
    case 2:
        break;
    }
}

/* the open's end: the request next */
void CSMDownloadPatch::EndOfOpen(void *client, int result, void *data)
{
    CSMDownloadPatch *sm = (CSMDownloadPatch *)data;

    if (result == 0)
        sm->SetState((SSMState)&CSMDownloadPatch::Get);
    else
        sm->Error("sceHTTPOpen (cb) failed", result);
}

/* the client made, told the URI, GET, a non-blocking open with a callback and a thirty-second
 * timeout, and opened */
void CSMDownloadPatch::Open(unsigned char event)
{
    switch (event) {
    case 0: {
        char text[256];
        int err;

        sprintf(text, "Open %s", FileName());
        func_003862A0(m_stateName, text);
        if (!(m_client = func_00541C10())) {
            Error("sceHTTPCreate failed\n", 0);
            break;
        }
        if ((err = func_00545800(m_client, 3, m_uri)) < 0) {
            Error("sceHTTPSetOption(sceHTTPO_ParsedURI) failed", err);
            break;
        }
        if ((err = func_00545800(m_client, 2, 1)) < 0) {
            Error("sceHTTPSetOption(sceHTTPO_Method, sceHTTPM_GET) failed", err);
            break;
        }
        if ((err = func_00545800(m_client, 22, 0)) < 0) {
            Error("sceHTTPSetOption(sceHTTPO_BlockingOpenMode) failed", err);
            break;
        }
        if ((err = func_00545800(m_client, 23, EndOfOpen, this) < 0)) {   /* (sic: the comparison assigned) */
            Error("sceHTTPO_EndOfOpenCB failed", err);
            break;
        }
        if ((err = func_00545800(m_client, 21, 30)) < 0) {
            Error("sceHTTPO_OpenTimeout failed", err);
            break;
        }
        if ((err = func_005469F8(m_client)) < 0) {
            Error("sceHTTPOpen failed", err);
            return;
        }
        break;
    }
    case 1:
        break;
    case 2:
        break;
    }
}

/* the first step: DNAS.BIN's HTTP library started */
void CSMDownloadPatch::InitializeDNAS(unsigned char event)
{
    switch (event) {
    case 0:
        func_003862A0(m_stateName, "Initialize DNAS");
        break;
    case 1: {
        int err;

        if ((err = func_00534830()) == 0)
            SetState((SSMState)&CSMDownloadPatch::ParseURI);
        else
            Error("DNAS Initialization Failure", err);
        break;
    }
    case 2:
        break;
    }
}

/* the machine started: the network's modules loaded, DNAS.BIN's library started next */
bool CSMDownloadPatch::Start()
{
    CSimpleStateMachine::Start();
    func_003862A0(m_stateName, "Start");
    if (func_003515D0(data_0049E7E8, 3))
        SetState((SSMState)&CSMDownloadPatch::InitializeDNAS);
    if (func_005447D8() < 0)
        return false;
    return true;
}

/* the command DownloadPatch: the machine made from PatchServerURL and started, then run until it is
 * done (as UIInitNetwork runs CSMInitNetwork) */
bool UIDownloadPatch(C2DAnimCmdHdr *hdr, float *dT)
{
    bool done = false;
    CValve *valve = func_002846F0(hdr);

    if (!data_004416D0) {
        CUIVariable *url = func_003857C0(&theUIVarManager, "PatchServerURL", 0, (UIVAR_LONGEVITY)1);

        data_004416D0 = new CSMDownloadPatch(valve, url->String());
        data_004416D0->Start();
    } else {
        func_003A5C30(data_004416D0);
        if (func_003A5C10(data_004416D0)) {
            delete data_004416D0;
            data_004416D0 = 0;
            done = true;
        }
    }
    return done;
}

/* the command DeletePatch: nothing left to do in SOCOM II */
bool UIDeletePatch(C2DAnimCmdHdr *hdr, float *dT)
{
    return true;
}
