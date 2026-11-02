/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* The Medius client API (OVL2 0x645000-0x64F3D8, SOCOM 1's objects at 0x3B20E8 and the request
 * wrappers'), SCEA's Medius, prebuilt with ee-gcc 2.96 at -O2 -G0 (README "The GCC front").
 * Names are SOCOM 1's. Only the functions written so far are here, in image order. */

#define MEDIUS_SESSIONKEY_MAXLEN    17
#define MEDIUS_MESSAGEID_MAXLEN     21

typedef struct NetAddress {
    int AddressType;
    char Address[16];
    unsigned short Port;
} NetAddress;                           /* 24 bytes */

typedef struct NetAddressList {
    NetAddress aAddressList[2];
} NetAddressList;                       /* 48 bytes */

extern NetAddress gstNatServiceAddress;
extern NetAddressList gstClientAddressList;
extern int data_006562F8;               /* the message counter */

extern int iTerminateString(char *s, int len);
extern int iCreateAndAllocateMessageID(char *key, int counter, int type, char *out);

void MediusSetNatServiceAddress(NetAddress addr)
{
    gstNatServiceAddress = addr;
}

void MediusGetNatServiceAddress(NetAddress *addr)
{
    *addr = gstNatServiceAddress;
}

void MediusGetClientAddrList(NetAddressList *list)
{
    *list = gstClientAddressList;
}

int MediusCreateMessageID(char *key, int type, char *out)
{
    iTerminateString(key, MEDIUS_SESSIONKEY_MAXLEN);
    iTerminateString(out, MEDIUS_MESSAGEID_MAXLEN);
    iCreateAndAllocateMessageID(key, data_006562F8, type, out);
    if (data_006562F8 >= 999)
        data_006562F8 = 0;
    else
        data_006562F8++;
    return 0;
}
