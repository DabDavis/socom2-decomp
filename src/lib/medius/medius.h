/* medius.h: SCEA's Medius client library (OVL2 0x645710-0x64F778, "Medius Client Library Version
 * 1.50.0013"), as far as the hand-written units use it. The error codes are the library's own:
 * MediusGetErrorCodeString (0x64A5D8) names each value. Request layouts are from the code (each
 * API terminates its strings and sends sizeof the request); field names follow the online lane's
 * zonline.h where it has the type. The library's globals carry SOCOM 1's names where data.txt
 * has them (GlobalInitFlag, MyConnectionIndex, the requests' callback slots), placeholders
 * elsewhere. */
#ifndef MEDIUS_H
#define MEDIUS_H

#define MEDIUS_MESSAGEID_MAXLEN     21
#define MEDIUS_SESSIONKEY_MAXLEN    17
#define MEDIUS_CLANNAME_MAXLEN      32
#define MEDIUS_ACCOUNTNAME_MAXLEN   32
#define MEDIUS_CLANMSG_MAXLEN       200
#define MEDIUS_CLANSTATS_MAXLEN     256
#define MEDIUS_PASSWORD_MAXLEN      32
#define MEDIUS_ACCOUNTSTATS_MAXLEN  256

typedef enum MediusErrorCode {
    MediusErrorCallMediusCloseBareBeforeReinitializing = -19,
    MediusErrorCallMediusCloseBeforeReinitializing = -18,
    MediusErrorAlreadyConnected = -17,
    MediusErrorNotConnected = -16,
    MediusErrorNotInitialized = -15,
    MediusErrorNoResult = -14,
    MediusErrorItemAlreadyExists = -13,
    MediusErrorItemNotFound = -12,
    MediusErrorSessionInactive = -11,
    MediusErrorGeneral = -10,
    MediusErrorDirectoryNotFound = -8,
    MediusErrorDeviceNotFound = -7,
    MediusErrorFileNotFound = -6,
    MediusErrorSendingMessage = -5,
    MediusErrorClosing = -4,
    MediusErrorDisconnecting = -3,
    MediusErrorConnecting = -2,
    MediusErrorInitializing = -1,
    MediusErrorNone = 0,
    MediusErrorWorldCreatedSizeReduced = 3   /* named MediusWorldCreatedSizeReduced */
} MediusErrorCode;

/* a response's handler: the request's own callback type in the vendor's headers; one here */
typedef void (*MediusCallback)(void);

/* the file API's records (field names as the online lane's zNetMediusFile.cpp has them) */
typedef struct MediusFile {
    char Filename[128];
    unsigned char ServerChecksum[16];
    int FileID;
    int FileSize;
    int CreationTimeStamp;
    int OwnerID;
    int GroupID;
    unsigned short OwnerPermissionRWX;
    unsigned short GroupPermissionRWX;
    unsigned short GlobalPermissionRWX;
    unsigned short ServerOperationID;
} MediusFile;                           /* 0xAC */

typedef struct MediusFileAttributes {
    char Description[256];
    unsigned int LastChangedTimeStamp;
    unsigned int LastChangedByUserID;
    unsigned int NumberAccesses;
    unsigned int StreamableFlag;
    unsigned int StreamingDataRate;
} MediusFileAttributes;                 /* 0x114 */

/* the connection: GlobalInitFlag is 1 once MediusInitialize has run (each unit declares it as
 * its own object reads it: the API object, which defines it, a halfword -- SOCOM 1's symbol is 2
 * bytes -- the clan object a word), MyConnectionIndex the DME connection to the lobby server
 * (negative before MediusConnect) */
extern int MyConnectionIndex;

/* NetSendMessage's message class for every Medius request, and its broadcast target */
#define MEDIUS_MSG_CLASS    64
#define MEDIUS_TO_SERVER    0xFFFF

/* DME: send a message to a connection (0x621268, SOCOM 1's NetSendMessage) */
extern int NetSendMessage(char msgClass, int connIndex, int toClient, int reliable, int msgType,
                          int len, void *msg);
extern int iTerminateString(char *s, int len);

#endif
