/* zNetMediusFile.cpp -- the Medius file API: CLobbyFileIO, a CIO over Medius's file transfers
 * (SOCOM II's, by its RTTI and vtable 0x406760; the unit and its name are ours). OVL1
 * 0x304610-0x30661F: after the LAN lobby's unit, its static initialiser 0x402570 building the map of
 * transfers under way, std::map<std::string, CLobbyFileIO *> at 0x450070. Open, Close, freadAsync
 * and the callbacks that go on to an upload or a create register the file under the message id
 * the request carries; each callback finds its file by the response's id and erases it once its
 * transfer is over. The map's template tail closes the unit (0x305710-0x3065A0, with the pair's
 * destructor 0x304780 after freadAsync and ~map 0x3056A0 after ReleaseFileTransfers). The vtable's
 * slots follow CIO's (zutil.h): Open 0x304C50, Close 0x304A60, IsOpen 0x304A50, GetSize 0x304A30,
 * GetMode 0x304610, ftell 0x304A20, fseek 0x304A10, the two freads 0x304810/0x304800, fwrite
 * 0x3047F0, freadAsync 0x304660, fwriteAsync 0x304650, fsyncAsync 0x304620, CIO's string fwrite,
 * fflush 0x3047E0. The Medius calls are named by the status strings their callbacks check (OVL2's
 * GCC library: MediusFileListFiles 0x64B7C8, MediusFileCreate 0x64B3F8, MediusFileUpload 0x64B490,
 * MediusFileDownload 0x64B5D8, MediusFileClose 0x64B540; the message id made by 0x649B40, SOCOM 1's
 * iCreateAndAllocateMessageID grown a counter). Defined in reverse of their image order
 * (-inline deferred).
 *
 * What settled the shapes (the online lane's round 8):
 *   - every sender keeps the message id's address in a saved register through the id call and the
 *     insert: a `char *id = req.MessageID;` local, declared after `err` (declared first, err takes
 *     the lower register: s1 against the id's s0 in freadAsync);
 *   - the insert builds `std::pair<std::string, CLobbyFileIO *>(key, file)` from a named key and
 *     converts it to the map's value: two string copies, as the target makes. std::make_pair is
 *     never inlined here (include/utility's or a by-value one: the template function stays a
 *     call, and `this` bound to its const reference is homed on the stack);
 *   - the lookup assigns its iterator from find after taking end() into a local (`iterator i;
 *     iterator e = end(); i = find(key);`): the assigned iterator lives in memory and the
 *     dereference re-reads it (`lw a0` in the branch's delay slot); initialised from find, MW keeps
 *     the compared value; `i != end()` written in the test cuts end() to an out-of-line tail();
 *   - the list callback's create request is an inline of its own (CreateFile), as the upload is:
 *     written in the callback, it took the higher stack slot from the upload's.
 * Still emitted here but not in the image: ~pair<std::string, CLobbyFileIO *> (the insert's
 * temporary's type -- the original built no such pair, or did not emit its destructor), the
 * allocator's out-of-line constructor and an out-of-line ~__tree (both ReleaseFileTransfers'). */

#include <string.h>
#include <string>
#include <map>
#include "zonline.h"
#include "../zutil/zutil.h"
#include "../zsys/zsys.h"
#include "../zsys/zsys_public.h"

/* a file on the Medius server (the SDK's MediusFile, 0xAC bytes: a transfer's descriptor, copied
 * whole) */
struct MediusFile {
    char Filename[128];   // 0x0
    unsigned char ServerChecksum[16];   // 0x80
    int FileID;   // 0x90
    int FileSize;   // 0x94
    int CreationTimeStamp;   // 0x98
    int OwnerID;   // 0x9c
    int GroupID;   // 0xa0
    unsigned short OwnerPermissionRWX;   // 0xa4
    unsigned short GroupPermissionRWX;   // 0xa6
    unsigned short GlobalPermissionRWX;   // 0xa8
    unsigned short ServerOperationID;   // 0xaa
};   // 0xac

/* a file's attributes (the SDK's MediusFileAttributes) */
struct MediusFileAttributes {
    char Description[256];   // 0x0
    unsigned int LastChangedTimeStamp;   // 0x100
    unsigned int LastChangedByUserID;   // 0x104
    unsigned int NumberAccesses;   // 0x108
    unsigned int StreamableFlag;   // 0x10c
    unsigned int StreamingDataRate;   // 0x110
};   // 0x114

/* a transfer's progress (int-sized, as the callbacks compare it) */
enum MediusFileXferStatus {
    MediusFileXferStatus_Error = 0,
    MediusFileXferStatus_Initial = 1,
    MediusFileXferStatus_Mid = 2,
    MediusFileXferStatus_End = 3,
    MediusFileXferStatus_Extent = 0x7FFFFFFF
};

/* the files whose names begin so, the owner's, at most PageSize of them (Open's: 0xB0 bytes) */
struct MediusFileListRequest {
    char FileNameBeginsWith[128];   // 0x0
    unsigned int FilesizeGreaterThan;   // 0x80
    unsigned int FilesizeLessThan;   // 0x84
    unsigned int OwnerByID;   // 0x88
    unsigned int NewerThanTimestamp;   // 0x8c
    unsigned int StartingEntryNumber;   // 0x90
    unsigned int PageSize;   // 0x94
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x98
};   // 0xb0

/* a file found (one per callback) */
struct MediusFileListResponse {
    MediusFile MediusFileInfo;   // 0x0
    MediusCallbackStatus StatusCode;   // 0xac
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0xb0
    char EndOfList;   // 0xc5
};

/* a file made on the server */
struct MediusFileCreateRequest {
    MediusFile MediusFileToCreate;   // 0x0
    MediusFileAttributes MediusFileCreateAttributes;   // 0xac
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x1c0
};   // 0x1d8

/* the create's answer: the file as the server made it */
struct MediusFileCreateResponse {
    MediusFile MediusFileInfo;   // 0x0
    MediusCallbackStatus StatusCode;   // 0xac
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0xb0
};

/* a file's contents sent */
struct MediusFileUploadRequest {
    MediusFile MediusFileInfo;   // 0x0
    const void *pFileData;   // 0xac
    int iFileSize;   // 0xb0
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0xb4
};   // 0xcc

/* a packet of it acknowledged */
struct MediusFileUploadResponse {
    unsigned char Data[464];   // 0x0
    int iStartByteIndex;   // 0x1d0
    int iDataSize;   // 0x1d4
    int iPacketNumber;   // 0x1d8
    MediusFileXferStatus iXferStatus;   // 0x1dc
    MediusCallbackStatus StatusCode;   // 0x1e0
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x1e4
};

/* a file fetched */
struct MediusFileDownloadRequest {
    MediusFile MediusFileInfo;   // 0x0
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0xac
};   // 0xc4

/* a packet of it */
struct MediusFileDownloadResponse {
    unsigned char Data[464];   // 0x0
    int iStartByteIndex;   // 0x1d0
    int iDataSize;   // 0x1d4
    int iPacketNumber;   // 0x1d8
    MediusFileXferStatus iXferStatus;   // 0x1dc
    MediusCallbackStatus StatusCode;   // 0x1e0
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0x1e4
};

/* a written file closed on the server */
struct MediusFileCloseRequest {
    MediusFile MediusFileInfo;   // 0x0
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0xac
};   // 0xc4

struct MediusFileCloseResponse {
    MediusFile MediusFileInfo;   // 0x0
    MediusCallbackStatus StatusCode;   // 0xac
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];   // 0xb0
};

extern "C" {
/* the account logged in (OVL2 bss, SOCOM 1's AccountInfo) */
struct AccountInfoType {
    int AccountID;   // 0x0
    char AccountName[32];   // 0x4
};
extern AccountInfoType AccountInfo;

MediusErrorCode MediusCreateMessageID(char *sessionKey, int type, char *messageID);   /* a message's id made */
MediusErrorCode MediusFileListFiles(MediusFileListRequest *req,
    void (*cb)(MediusErrorCode, MediusErrorCode, MediusErrorCode, MediusFileListResponse *));
MediusErrorCode MediusFileCreate(MediusFileCreateRequest *req,
    void (*cb)(MediusErrorCode, MediusErrorCode, MediusErrorCode, MediusFileCreateResponse *));
MediusErrorCode MediusFileUpload(MediusFileUploadRequest *req,
    void (*cb)(MediusErrorCode, MediusErrorCode, MediusErrorCode, MediusFileUploadResponse *));
MediusErrorCode MediusFileDownload(MediusFileDownloadRequest *req,
    void (*cb)(MediusErrorCode, MediusErrorCode, MediusErrorCode, MediusFileDownloadResponse *));
MediusErrorCode MediusFileClose(MediusFileCloseRequest *req,
    void (*cb)(MediusErrorCode, MediusErrorCode, MediusErrorCode, MediusFileCloseResponse *));
}

/* a file transferred through Medius (0x9C bytes, as the patch downloader's member declares it in
 * fts_patch.cpp: the server's descriptor at 4, the mode at 8, the path from 0xC) */
class CLobbyFileIO : public CIO {
public:
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
    const unsigned char *GetChecksum() const;

    /* the server's descriptor replaced by a copy of the one given */
    void SetFile(const MediusFile *f)
    {
        if (m_handle)
            _free(m_handle, 0, 0);
        if (f) {
            m_handle = (MediusFile *)_malloc(sizeof(MediusFile), 0, 0);
            memcpy(m_handle, f, sizeof(MediusFile));
        } else {
            m_handle = 0;
        }
    }

    MediusFile *m_handle;   // 0x4: the server's descriptor
    unsigned int m_mode;   // 0x8: 2 and 4 the writes, 8 a create when the file is missing
    char m_path[0x80];   // 0xc
    unsigned char m_open;   // 0x8c
    int m_result;   // 0x90: the last transfer's result
    const void *m_buf;   // 0x94: the asynchronous transfer's buffer
    int m_size;   // 0x98: and its size
};   // 0x9c

typedef std::map<std::string, CLobbyFileIO *> CLobbyFileMap;

/* the transfers under way, by their message ids (0x450070) */
CLobbyFileMap data_00450070;

/* a transfer under way, by its message id */
inline CLobbyFileIO *FindTransfer(const char *id)
{
    CLobbyFileIO *file = 0;
    std::string key(id);
    CLobbyFileMap::iterator i;
    CLobbyFileMap::iterator e = data_00450070.end();

    i = data_00450070.find(key);
    if (i != e)
        file = (*i).second;
    return file;
}

/* a transfer registered under its message id */
inline void AddTransfer(char *id, CLobbyFileIO *file)
{
    std::string key(id);

    data_00450070.insert(std::pair<std::string, CLobbyFileIO *>(key, file));
}

/* a transfer over */
inline void RemoveTransfer(const char *id)
{
    data_00450070.erase(std::string(id));
}

void cbFileListFiles(MediusErrorCode, MediusErrorCode, MediusErrorCode, MediusFileListResponse *resp);
void cbCreateFile(MediusErrorCode, MediusErrorCode, MediusErrorCode, MediusFileCreateResponse *resp);
void cbUploadFile(MediusErrorCode, MediusErrorCode, MediusErrorCode, MediusFileUploadResponse *resp);
void cbFileClose(MediusErrorCode, MediusErrorCode, MediusErrorCode, MediusFileCloseResponse *resp);
void cbFileDownload(MediusErrorCode, MediusErrorCode, MediusErrorCode, MediusFileDownloadResponse *resp);

/* a written file's contents sent, once the server has its descriptor */
inline void UploadFile(CLobbyFileIO *file)
{
    MediusFileUploadRequest req;

    memset(&req, 0, sizeof(req));
    if (file->m_handle)
        memcpy(&req.MediusFileInfo, file->m_handle, sizeof(MediusFile));
    req.pFileData = file->m_buf;
    req.iFileSize = file->m_size;
    MediusErrorCode err;
    char *id = req.MessageID;
    err = MediusCreateMessageID(SessionKey, 109, id);
    if (CheckMediusError(err))
        AddTransfer(id, file);
    if (CheckMediusError(err))
        CheckMediusError(MediusFileUpload(&req, cbUploadFile));
}

/* a missing file made on the server, for a write that asked for it */
inline void CreateFile(CLobbyFileIO *file)
{
    MediusFileCreateRequest req;

    memset(&req, 0, sizeof(req));
    if (file->m_path)
        memcpy(req.MediusFileToCreate.Filename, file->m_path, sizeof(req.MediusFileToCreate.Filename));
    req.MediusFileToCreate.FileSize = 2000;
    req.MediusFileToCreate.OwnerPermissionRWX = 6;
    req.MediusFileToCreate.OwnerID = AccountInfo.AccountID;
    req.MediusFileCreateAttributes.StreamableFlag = 0;
    req.MediusFileCreateAttributes.StreamingDataRate = 0;
    MediusErrorCode err;
    char *id = req.MessageID;
    err = MediusCreateMessageID(SessionKey, 108, id);
    if (CheckMediusError(err))
        AddTransfer(id, file);
    if (CheckMediusError(err))
        CheckMediusError(MediusFileCreate(&req, cbCreateFile));
}

/* every transfer forgotten (OVL1 0x305640: the lobby's ReleaseUIResources calls it). Byte-exact
 * (the comment had kept round 8's OPEN: 2 instructions then, FreeVector's temporary map calling
 * include/memory's out-of-line allocator() where the image passes a bare stack slot -- closed in
 * the MSL headers since); the tree's constructor, clear, swap and the inlined destructor's
 * destroy */
void ReleaseFileTransfers()
{
    FreeVector(data_00450070);
}

/* the file looked for by Open: found, its descriptor kept and a waiting write sent; missing, made
 * when the mode asks for it (OVL1 0x305230) */
void cbFileListFiles(MediusErrorCode, MediusErrorCode, MediusErrorCode, MediusFileListResponse *resp)
{
    if (CheckMediusCallbackStatus("MediusFileListFiles", resp->StatusCode)) {
        CLobbyFileIO *file = FindTransfer(resp->MessageID);

        if (file) {
            if (resp->EndOfList) {
                RemoveTransfer(resp->MessageID);
                file->m_result = resp->MediusFileInfo.FileSize;
            }
            file->SetFile(&resp->MediusFileInfo);
            if ((file->m_mode & 2) && file->m_buf && file->m_size)
                UploadFile(file);
        }
    } else {
        LobbyEvent("MP_MEDIUS_FILE_DOES_NOT_EXIST", 0);
        CLobbyFileIO *file = FindTransfer(resp->MessageID);

        if (file) {
            RemoveTransfer(resp->MessageID);
            if (file->m_handle)
                _free(file->m_handle, 0, 0);
            file->m_handle = 0;
            file->m_result = -1;
            if (file->m_mode & 8)
                CreateFile(file);
        }
    }
}

/* the server's checksum of the file, when one is open (OVL1 0x3051E0: the save module's) */
const unsigned char *CLobbyFileIO::GetChecksum() const
{
    const unsigned char *sum = 0;

    if (IsOpen())
        sum = m_handle->ServerChecksum;
    return sum;
}

/* the file made: its descriptor kept and a waiting write sent (OVL1 0x304F40) */
void cbCreateFile(MediusErrorCode, MediusErrorCode, MediusErrorCode, MediusFileCreateResponse *resp)
{
    if (CheckMediusCallbackStatus("MediusCreateFile", resp->StatusCode)) {
        CLobbyFileIO *file = FindTransfer(resp->MessageID);

        if (file) {
            RemoveTransfer(resp->MessageID);
            file->SetFile(&resp->MediusFileInfo);
            if ((file->m_mode & 2) && file->m_buf && file->m_size)
                UploadFile(file);
        }
    } else {
        CLobbyFileIO *file = FindTransfer(resp->MessageID);

        if (file)
            RemoveTransfer(resp->MessageID);
    }
}

/* a written file's packet acknowledged: the last one closes the file (OVL1 0x304DE0) */
void cbUploadFile(MediusErrorCode, MediusErrorCode, MediusErrorCode, MediusFileUploadResponse *resp)
{
    if (CheckMediusCallbackStatus("MediusUploadFile", resp->StatusCode) && resp->iXferStatus) {
        CLobbyFileIO *file = FindTransfer(resp->MessageID);

        if (file && resp->iXferStatus == MediusFileXferStatus_End) {
            RemoveTransfer(resp->MessageID);
            file->Close();
        }
    } else {
        CLobbyFileIO *file = FindTransfer(resp->MessageID);

        if (file)
            RemoveTransfer(resp->MessageID);
    }
}

/* the file looked for on the server: kept once found, made when the mode says so (OVL1 0x304C50) */
bool CLobbyFileIO::Open(const char *name, unsigned int mode)
{
    m_mode = mode;
    MediusFileListRequest req;
    bool ok = false;

    memset(&req, 0, sizeof(req));
    req.FilesizeGreaterThan = 0;
    req.FilesizeLessThan = 5000000;
    req.OwnerByID = AccountInfo.AccountID;
    req.PageSize = 1;
    if (name)
        memcpy(req.FileNameBeginsWith, name, sizeof(req.FileNameBeginsWith));
    MediusErrorCode err;
    char *id = req.MessageID;
    err = MediusCreateMessageID(SessionKey, 114, id);
    if (CheckMediusError(err))
        AddTransfer(id, this);
    if (CheckMediusError(err)) {
        if (name && strlen(name))
            strcpy(m_path, name);
        else
            m_path[0] = 0;
        ok = CheckMediusError(MediusFileListFiles(&req, cbFileListFiles));
    }
    return ok;
}

/* the close answered: the result kept (OVL1 0x304B90) */
void cbFileClose(MediusErrorCode, MediusErrorCode, MediusErrorCode, MediusFileCloseResponse *resp)
{
    CheckMediusCallbackStatus("MediusFileClose", resp->StatusCode);
    CLobbyFileIO *file = FindTransfer(resp->MessageID);

    if (file) {
        RemoveTransfer(resp->MessageID);
        file->m_result = resp->StatusCode;
    }
}

/* the file let go: a written one closed on the server first (OVL1 0x304A60) */
void CLobbyFileIO::Close()
{
    if (m_handle) {
        if (m_mode & 6) {
            MediusFileCloseRequest req;

            if (m_handle)
                memcpy(&req.MediusFileInfo, m_handle, sizeof(MediusFile));
            MediusErrorCode err;
            char *id = req.MessageID;
            err = MediusCreateMessageID(SessionKey, 110, id);
            if (CheckMediusError(err))
                AddTransfer(id, this);
            if (CheckMediusError(err))
                CheckMediusError(MediusFileClose(&req, cbFileClose));
        }
        _free(m_handle, 0, 0);
    }
    m_handle = 0;
    m_path[0] = 0;
}

/* (OVL1 0x304A50) */
bool CLobbyFileIO::IsOpen() const
{
    return m_handle != 0;
}

/* the transfer's size, -1 when none is open (OVL1 0x304A30) */
int CLobbyFileIO::GetSize() const
{
    int size = -1;

    if (m_handle)
        size = m_handle->FileSize;
    return size;
}

/* (OVL1 0x304A20) */
int CLobbyFileIO::ftell() const
{
    return -1;
}

/* no seeking (OVL1 0x304A10) */
int CLobbyFileIO::fseek(int offset, int whence)
{
    return -1;
}

/* a read's packet: copied into the buffer, the last one ending the transfer (OVL1 0x304820) */
void cbFileDownload(MediusErrorCode, MediusErrorCode, MediusErrorCode, MediusFileDownloadResponse *resp)
{
    if (CheckMediusCallbackStatus("MediusFileDownload", resp->StatusCode)) {
        CLobbyFileIO *file = FindTransfer(resp->MessageID);

        if (file) {
            switch (resp->iXferStatus) {
            case MediusFileXferStatus_End:
                RemoveTransfer(resp->MessageID);
                file->m_result = resp->StatusCode;
            case MediusFileXferStatus_Initial:
            case MediusFileXferStatus_Mid:
                if (resp->iStartByteIndex + resp->iDataSize <= file->m_size) {
                    memcpy((char *)file->m_buf + resp->iStartByteIndex, resp->Data, resp->iDataSize);
                    file->m_result = resp->iStartByteIndex;
                }
                break;
            case MediusFileXferStatus_Error:
                RemoveTransfer(resp->MessageID);
                file->m_result = -1;
                break;
            }
        }
    } else {
        CLobbyFileIO *file = FindTransfer(resp->MessageID);

        if (file) {
            RemoveTransfer(resp->MessageID);
            file->m_result = resp->StatusCode;
        }
    }
}

/* (OVL1 0x304810) */
int CLobbyFileIO::fread(void *buf, int size)
{
    return -1;
}

/* no synchronous reads (OVL1 0x304800) */
int CLobbyFileIO::fread(int size, void **buf)
{
    return -1;
}

/* no synchronous writes (OVL1 0x3047F0) */
int CLobbyFileIO::fwrite(const void *buf, int size)
{
    return -1;
}

/* nothing to flush (OVL1 0x3047E0) */
void CLobbyFileIO::fflush()
{
}

/* a read started: the buffer kept and the download asked for (OVL1 0x304660) */
int CLobbyFileIO::freadAsync(void *buf, int size)
{
    if (m_handle) {
        m_buf = buf;
        m_size = size;
        MediusFileDownloadRequest req;

        if (m_handle)
            memcpy(&req.MediusFileInfo, m_handle, sizeof(MediusFile));
        MediusErrorCode err;
        char *id = req.MessageID;
        err = MediusCreateMessageID(SessionKey, 111, id);
        if (CheckMediusError(err))
            AddTransfer(id, this);
        if (CheckMediusError(err))
            CheckMediusError(MediusFileDownload(&req, cbFileDownload));
    }
    return -1;
}

/* an asynchronous write kept for the transfer (OVL1 0x304650) */
int CLobbyFileIO::fwriteAsync(const void *buf, int size)
{
    m_buf = buf;
    m_size = size;
    return -1;
}

/* the transfers done: the last one's result handed back, true once none is under way (OVL1
 * 0x304620) */
bool CLobbyFileIO::fsyncAsync(int mode, int *result)
{
    bool done = data_00450070.empty();

    if (result)
        *result = m_result;
    return done;
}

/* the mode it was opened with (OVL1 0x304610) */
unsigned int CLobbyFileIO::GetMode() const
{
    return m_mode;
}
