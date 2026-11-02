/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* The Medius file API (OVL2 0x64B3F8-0x64BC44): SCEA's Medius client library, prebuilt with
 * ee-gcc 2.96 at -O2 -G0 (README "The GCC front"). New in SOCOM II (SOCOM 1's library has no file
 * requests) and its own object: it reads GlobalInitFlag as a word, as the clan object does, where
 * the API object before it (mediusapi.c, which defines the flag) reads a halfword, and its
 * callback slots (0x686AD0-0x686B08) follow the clan object's in .bss, apart from the API
 * object's (0x6561C0-0x6562E0). Each request ends in its MessageID; the shape is the clan
 * API's (mediusclan.c). The names are the online lane's (zNetMediusFile.cpp calls each with the
 * callback that checks the call's own status string); the record layouts are medius.h's. An
 * upload keeps the data pointer and size for the transfer that follows. */

#include "medius.h"

extern int GlobalInitFlag;              /* a word here (the API object's is a halfword) */

typedef struct MediusFileUploadRequest {
    MediusFile MediusFileInfo;
    const void *pFileData;
    int iFileSize;
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
} MediusFileUploadRequest;

extern MediusCallback data_00686AD4;    /* the upload's callback */
extern const void *data_00686AD8;       /* the data being uploaded */
extern int data_00686ADC;               /* and its size */
extern int data_00686EA4;

typedef struct MediusFileCreateRequest {
    MediusFile MediusFileToCreate;
    MediusFileAttributes MediusFileCreateAttributes;
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
} MediusFileCreateRequest;

typedef struct MediusFileCloseRequest {
    MediusFile MediusFileInfo;
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
} MediusFileCloseRequest;

typedef struct MediusFileDownloadRequest {
    MediusFile MediusFileInfo;
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
} MediusFileDownloadRequest;

typedef struct MediusFileRequest_0064B670 {
    MediusFile MediusFileInfo;
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
} MediusFileRequest_0064B670;

typedef struct MediusFileRequest_0064B708 {
    MediusFile MediusFileInfo;
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
} MediusFileRequest_0064B708;

typedef struct MediusFileListRequest {
    char FileNameBeginsWith[128];
    unsigned int FilesizeGreaterThan;
    unsigned int FilesizeLessThan;
    unsigned int OwnerByID;
    unsigned int NewerThanTimestamp;
    unsigned int StartingEntryNumber;
    unsigned int PageSize;
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
} MediusFileListRequest;

typedef struct MediusFileRequest_0064B888 {
    MediusFile MediusFileInfo;
    MediusFileAttributes MediusFileAttributes;
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
} MediusFileRequest_0064B888;

typedef struct MediusFileRequest_0064B920 {
    MediusFile MediusFileInfo;
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
} MediusFileRequest_0064B920;

typedef struct MediusFileRequest_0064B9B8 {
    int Data0[121];
    char String484[21];
    char Data505[3];
} MediusFileRequest_0064B9B8;

typedef struct MediusFileRequest_0064BA50 {
    int Data0[121];
    char String484[21];
    char Data505[3];
} MediusFileRequest_0064BA50;

typedef struct MediusFileRequest_0064BAE8 {
    char Data0[312];
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
} MediusFileRequest_0064BAE8;

typedef struct MediusFileRequest_0064BBB0 {
    MediusFile MediusFileInfo;
    char MessageID[MEDIUS_MESSAGEID_MAXLEN];
} MediusFileRequest_0064BBB0;

extern MediusCallback data_00686AD0;
extern int data_00686E9C;
extern MediusCallback data_00686AE0;
extern int data_00686EAC;
extern MediusCallback data_00686AE4;
extern int data_00686EB4;
extern MediusCallback data_00686AE8;
extern int data_00686EBC;
extern MediusCallback data_00686AEC;
extern int data_00686EC4;
extern MediusCallback data_00686AF0;
extern int data_00686ECC;
extern MediusCallback data_00686AF4;
extern int data_00686ED4;
extern MediusCallback data_00686AF8;
extern int data_00686EDC;
extern MediusCallback data_00686AFC;
extern int data_00686EE4;
extern MediusCallback data_00686B00;
extern int data_00686EEC;
extern MediusCallback data_00686B04;
extern int data_00686EF4;
extern MediusCallback data_00686B08;
extern int data_00686EFC;

MediusErrorCode MediusFileCreate(MediusFileCreateRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    data_00686AD0 = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686E9C,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusFileUpload(MediusFileUploadRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    data_00686AD4 = callback;
    data_00686AD8 = request->pFileData;
    data_00686ADC = request->iFileSize;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686EA4,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusFileClose(MediusFileCloseRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    data_00686AE0 = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686EAC,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusFileDownload(MediusFileDownloadRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    data_00686AE4 = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686EB4,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode func_0064B670(MediusFileRequest_0064B670 *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    data_00686AE8 = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686EBC,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode func_0064B708(MediusFileRequest_0064B708 *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    data_00686AEC = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686EC4,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode MediusFileListFiles(MediusFileListRequest *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    data_00686AF0 = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686ECC,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode func_0064B888(MediusFileRequest_0064B888 *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    data_00686AF4 = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686ED4,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode func_0064B920(MediusFileRequest_0064B920 *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    data_00686AF8 = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686EDC,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode func_0064B9B8(MediusFileRequest_0064B9B8 *request, MediusCallback callback)
{
    iTerminateString(request->String484, MEDIUS_MESSAGEID_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    data_00686AFC = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686EE4,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode func_0064BA50(MediusFileRequest_0064BA50 *request, MediusCallback callback)
{
    iTerminateString(request->String484, MEDIUS_MESSAGEID_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    data_00686B00 = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686EEC,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode func_0064BAE8(MediusFileRequest_0064BAE8 *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    data_00686B04 = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686EF4,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}

MediusErrorCode func_0064BBB0(MediusFileRequest_0064BBB0 *request, MediusCallback callback)
{
    iTerminateString(request->MessageID, MEDIUS_MESSAGEID_MAXLEN);
    if (GlobalInitFlag != 1)
        return MediusErrorNotInitialized;
    if (MyConnectionIndex < 0)
        return MediusErrorNotConnected;
    data_00686B08 = callback;
    if (NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686EFC,
                       sizeof(*request), request) != 0)
        return MediusErrorSendingMessage;
    return MediusErrorNone;
}
