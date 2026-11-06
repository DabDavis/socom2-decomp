/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* The Medius response handlers' file upload row (OVL2 0x64D490): in the run of response handlers RegisterPackets registers
 * (the rest stand as drafts). The server asks for the next block of an upload (0x686AD4-0x686ADC, mediusfile.c's); the block
 * goes back with the request's fields and up to 464 bytes of the data. Round 26b. */

#include "medius.h"

extern int GlobalInitFlag;              /* a word here, as mediusfile.c's */
extern void *memcpy(void *d, const void *s, unsigned int n);

typedef struct MediusUploadSession { int w[4]; int nField20; char bField24; } MediusUploadSession;  /* 21 bytes copied */

typedef struct MediusUploadAsk {        /* the server's request */
    unsigned int nOffset;               /* 0x00 */
    int nField04;                       /* 0x04 */
    int nField08;                       /* 0x08: more to come */
    int nStatus;                        /* 0x0C */
    MediusUploadSession session;        /* 0x10 */
} MediusUploadAsk;

typedef struct MediusUploadBlock {      /* the block (508 bytes) */
    char abData[464];                   /* 0x000 */
    unsigned int nOffset;               /* 0x1D0 */
    int nLength;                        /* 0x1D4 */
    int nField04;                       /* 0x1D8 */
    int nField08;                       /* 0x1DC */
    int nStatus;                        /* 0x1E0 */
    MediusUploadSession session;        /* 0x1E4 */
} MediusUploadBlock;

extern void (*data_00686AD4)(int a0, int a1, int a2, MediusUploadBlock *pBlock);    /* the upload's callback */
extern const char *data_00686AD8;       /* the data being uploaded */
extern int data_00686ADC;               /* and its size */
extern int data_00686B20;               /* the block's message type */

int func_0064D490(int a0, int a1, int a2, MediusUploadAsk *pAsk)
{
    MediusUploadBlock block;

    block.nLength = 0;
    block.nOffset = pAsk->nOffset;
    block.nField04 = pAsk->nField04;
    block.nStatus = pAsk->nStatus;
    block.nField08 = pAsk->nField08;
    memcpy(&block.session, &pAsk->session, 21);
    if (block.nField08 != 0) {
        int nLeft = data_00686ADC - block.nOffset;

        if (nLeft > 464)
            nLeft = 464;
        block.nLength = nLeft;
        memcpy(block.abData, data_00686AD8 + block.nOffset, nLeft);
    }
    if (GlobalInitFlag != 1)
        block.nStatus = -966;
    if (MyConnectionIndex < 0)
        block.nStatus = -966;
    if (data_00686AD4 != 0)
        data_00686AD4(a0, a1, a2, &block);
    NetSendMessage(MEDIUS_MSG_CLASS, MyConnectionIndex, MEDIUS_TO_SERVER, 1, data_00686B20, sizeof(block), &block);
    return 40;
}
