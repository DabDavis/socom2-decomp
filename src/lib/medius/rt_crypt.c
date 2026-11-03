/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* rt_crypt: the RT library's key master (OVL2 0x62EBF8-0x631008, rt_crypt/src/rt_crypt.c 1.70, "rt_crypt
 * version: 1.01.0023"; SOCOM 1's object at 0x40A580: IsNullRSAKey .. KM_Decrypt), SCEA's Medius/DME
 * network stack, prebuilt with ee-gcc 2.96 at -O2 -G0 (README "The GCC front"). A key master (one per
 * id, on a circular list from gpKeyMaster) holds the local RSA key pair, the game session's RC key
 * and its two cipher states, and a chain of per-client keys: a client's RSA public key, its RC key
 * and that key's two cipher states. Keys are 64 bytes; a cipher state is 0x10C. Names are SOCOM 1's
 * (s1:exact/calls/align: the same object, in the same order, SOCOM II's additions between).
 *
 * Byte-exact: 23 of 25 (KM_Initialize, round 8b; the four 0x200-byte cipher functions at
 * 0x630690.. are not written yet). The vendor's shapes, each measured: a leaf's loop wrapped in its
 * positive test (IsNullRSAKey/IsNullRCKey: guard clauses give a second `jr ra`); the null tests of a
 * signature's inputs one `||` (KM_DigiSign/Verify); a positive test around the copy-out with the
 * clearing after it (KM_GetGameSessionKey); one key-master variable reused for both ends
 * (KM_CopyKeys); `i = 64; while (i-- > 0)` (func_0062FDD8: the target tests the old index with
 * bgtz); the cipher state is word-aligned (KM_CopyKeys moves its last word with lw/sw).
 * OPEN: KM_Create (8 words: the target puts `move a0,s0` in the `nKMID < 0` test's bgez slot,
 * shared by both calls; 2.96 here makes it a bgezl carrying the lui), func_0062F818 (8 bytes short:
 * the target sets the result to 0 inside each of the three branches, a copy 2.96 here sinks into
 * IsNullRSAKey's delay slot -- result first, last, or set in each branch all give the same). */

typedef struct RSAKey {
    unsigned int n[16];
} RSAKey;                               /* 0x40 */

typedef struct RSAKeyPair {
    RSAKey pub;                         /* 0x00 */
    RSAKey priv;                        /* 0x40 */
} RSAKeyPair;                           /* 0x80 */

typedef struct RCKey {
    unsigned char k[64];
} RCKey;                                /* 0x40 */

typedef struct RCState {
    unsigned char s[256];               /* the permutation */
    unsigned int i;                     /* 0x100 */
    unsigned int j;                     /* 0x104 */
    unsigned int n;                     /* 0x108 */
} RCState;                              /* 0x10C: word-aligned (KM_CopyKeys copies its last word with lw/sw) */

typedef struct KeyChain {
    struct KeyChain *prev;              /* 0x00 */
    struct KeyChain *next;              /* 0x04 */
    int nID;                            /* 0x08: the client's id */
    RSAKey *pPublicKey;                 /* 0x0C */
    RCState *pEncrypt;                  /* 0x10 */
    RCState *pDecrypt;                  /* 0x14 */
    RCKey *pSymmetricKey;               /* 0x18 */
} KeyChain;                             /* 0x1C */

typedef struct KeyMaster {
    struct KeyMaster *prev;             /* 0x00: the list's head holds its tail here */
    struct KeyMaster *next;             /* 0x04 */
    KeyChain *pHead;                    /* 0x08 */
    KeyChain *pTail;                    /* 0x0C */
    int nID;                            /* 0x10 */
    int nLocalClientID;                 /* 0x14 */
    int nGameSessionID;                 /* 0x18 */
    RSAKey *pLocalPublicKey;            /* 0x1C */
    RSAKey *pLocalPrivateKey;           /* 0x20 */
    RCState *pSessionEncrypt;           /* 0x24 */
    RCState *pSessionDecrypt;           /* 0x28 */
    RCKey *pGameSessionKey;             /* 0x2C */
} KeyMaster;                            /* 0x30 */

extern KeyMaster *gpKeyMaster;
extern RSAKeyPair data_00676240;        /* the key pair of the ids below zero */
extern RSAKeyPair data_006762C0;        /* the key pair of the ids from zero */
extern RSAKeyPair data_00676340;        /* the key pair of id -1 */

extern void *memset(void *p, int c, unsigned int n);
extern int rt_memory_allocate(void **ppMemory, unsigned long nSize);
extern int rt_memory_free(void **ppMemory);
extern unsigned int randomMT(void);
extern void RCInitialize(RCState *pState, RCKey *pKey);
extern void func_0062BC10(RSAKey *pPub, RSAKey *pPriv, void *pIn, void *pOut, int nLength);
extern void func_0062BBE0(RSAKey *pKey, void *pIn, void *pOut, int nLength);

void InitializePRNG(void);
void func_0062EEC0(void *pData, int nLength, void *pDigest, int nDigestLength);
int KM_SetPublicKey(int nKMID, int nClientID, RSAKey *pKey);
int KM_SetLocalKeyPair(int nKMID, RSAKeyPair *pKeyPair);
int KM_AddSymmetricKey(int nKMID, int nClientID, RCKey *pKey);
void KM_Destroy(int nKMID);
int func_0062FCD8(int nKMID, int nClientID, RCKey *pKey);

int IsNullRSAKey(RSAKey *pKey)
{
    unsigned int i;

    if (pKey != 0) {
        for (i = 0; i < 16; i++) {
            if (pKey->n[i] != 0)
                return 0;
        }
    }
    return 1;
}

int IsNullRCKey(RCKey *pKey)
{
    unsigned int i;

    if (pKey != 0) {
        for (i = 0; i < 64; i++) {
            if (pKey->k[i] != 0)
                return 0;
        }
    }
    return 1;
}

KeyMaster *GetKeyMaster(int nID)
{
    KeyMaster *pKM = gpKeyMaster;

    while (pKM != 0 && pKM->nID != nID) {
        pKM = pKM->next;
        if (pKM == gpKeyMaster)
            pKM = 0;
    }
    return pKM;
}

KeyChain *NewKeyChain(void)
{
    KeyChain *pKC = 0;

    rt_memory_allocate((void **)&pKC, sizeof(KeyChain));
    if (pKC != 0) {
        pKC->prev = 0;
        pKC->next = 0;
        pKC->pPublicKey = 0;
        pKC->pEncrypt = 0;
        pKC->pDecrypt = 0;
        pKC->pSymmetricKey = 0;
        pKC->nID = 0;
    }
    return pKC;
}

void DeleteKeyChain(KeyChain *pKC)
{
    if (pKC != 0) {
        if (pKC->pPublicKey != 0)
            rt_memory_free((void **)&pKC->pPublicKey);
        if (pKC->pSymmetricKey != 0)
            rt_memory_free((void **)&pKC->pSymmetricKey);
        if (pKC->pEncrypt != 0)
            rt_memory_free((void **)&pKC->pEncrypt);
        if (pKC->pDecrypt != 0)
            rt_memory_free((void **)&pKC->pDecrypt);
        rt_memory_free((void **)&pKC);
    }
}

void AppendKeyChain(KeyMaster *pKM, KeyChain *pKC)
{
    if (pKM != 0) {
        pKC->next = 0;
        if (pKM->pHead != 0) {
            pKC->prev = pKM->pTail;
            pKC->prev->next = pKC;
        } else {
            pKM->pHead = pKC;
            pKC->prev = 0;
        }
        pKM->pTail = pKC;
    }
}

KeyChain *GetKeyChain(KeyMaster *pKM, int nID)
{
    KeyChain *pKC = 0;

    nID &= 0x1FFFFFFF;
    if (pKM != 0)
        pKC = pKM->pHead;
    while (pKC != 0 && pKC->nID != nID)
        pKC = pKC->next;
    return pKC;
}

void RemoveKeyChain(KeyMaster *pKM, KeyChain *pKC)
{
    if (pKM != 0 && pKC != 0) {
        if (pKC->next != 0)
            pKC->next->prev = pKC->prev;
        else
            pKM->pTail = pKC->prev;
        if (pKC->prev != 0)
            pKC->prev->next = pKC->next;
        else
            pKM->pHead = pKC->next;
    }
}

/* the software key pairs (round 8b): SOCOM 1's KM_Initialize kept the server's (id 0x80000000) and
 * the client's; SOCOM II tests a third, id -1's, first. A null pair clears the slot. */
void KM_Initialize(int nKMID, RSAKeyPair *pKeyPair)
{
    if (nKMID == -1) {
        if (pKeyPair != 0)
            data_00676340 = *pKeyPair;
        else
            memset(&data_00676340, 0, sizeof(RSAKeyPair));
    } else if (nKMID == (int)0x80000000) {
        if (pKeyPair != 0)
            data_00676240 = *pKeyPair;
        else
            memset(&data_00676240, 0, sizeof(RSAKeyPair));
    } else {
        if (pKeyPair != 0)
            data_006762C0 = *pKeyPair;
        else
            memset(&data_006762C0, 0, sizeof(RSAKeyPair));
    }
}

int KM_Create(int nKMID, int nLocalClientID, RSAKey *pServerKey)
{
    KeyMaster *pKM = GetKeyMaster(nKMID);

    if (pKM != 0)
        return 0;
    rt_memory_allocate((void **)&pKM, sizeof(KeyMaster));
    memset(pKM, 0, sizeof(KeyMaster));
    if (pKM == 0)
        return -1;
    pKM->nID = nKMID;
    pKM->nLocalClientID = nLocalClientID;
    pKM->nGameSessionID = 0;
    if (gpKeyMaster != 0) {
        pKM->prev = gpKeyMaster->prev;
        pKM->prev->next = pKM;
    } else {
        gpKeyMaster = pKM;
    }
    gpKeyMaster->prev = pKM;
    pKM->next = gpKeyMaster;
    if (nKMID == -1)
        KM_SetLocalKeyPair(nKMID, &data_00676340);
    else if (nKMID < 0)
        KM_SetLocalKeyPair(nKMID, &data_00676240);
    else
        KM_SetLocalKeyPair(nKMID, &data_006762C0);
    if (nLocalClientID != 0xFFFF && nKMID != -1)
        KM_SetPublicKey(nKMID, 0xFFFF, pServerKey);
    return 0;
}

int KM_SetPublicKey(int nKMID, int nClientID, RSAKey *pKey)
{
    KeyMaster *pKM = GetKeyMaster(nKMID);
    KeyChain *pKC;

    if (pKM == 0)
        return -1;
    pKC = GetKeyChain(pKM, nClientID);
    if (IsNullRSAKey(pKey)) {
        if (pKC != 0 && pKC->pPublicKey != 0)
            memset(pKC->pPublicKey, 0, sizeof(RSAKey));
    } else {
        if (pKC == 0) {
            pKC = NewKeyChain();
            pKC->nID = nClientID;
            AppendKeyChain(pKM, pKC);
        }
        if (pKC->pPublicKey == 0)
            rt_memory_allocate((void **)&pKC->pPublicKey, sizeof(RSAKey));
        *pKC->pPublicKey = *pKey;
    }
    return 0;
}

int KM_GetPublicKey(int nKMID, int nClientID, RSAKey *pKey)
{
    KeyMaster *pKM = GetKeyMaster(nKMID);
    KeyChain *pKC;

    if (pKM != 0) {
        pKC = GetKeyChain(pKM, nClientID);
        if (pKC != 0 && !IsNullRSAKey(pKC->pPublicKey)) {
            *pKey = *pKC->pPublicKey;
            return 0;
        }
    }
    if (pKey != 0)
        memset(pKey, 0, sizeof(RSAKey));
    return -1;
}

int KM_SetLocalKeyPair(int nKMID, RSAKeyPair *pKeyPair)
{
    KeyMaster *pKM = GetKeyMaster(nKMID);

    if (pKM == 0)
        return -1;
    if (IsNullRSAKey(&pKeyPair->pub) || IsNullRSAKey(&pKeyPair->priv)) {
        if (pKM->pLocalPublicKey != 0)
            memset(pKM->pLocalPublicKey, 0, sizeof(RSAKey));
        if (pKM->pLocalPrivateKey != 0)
            memset(pKM->pLocalPrivateKey, 0, sizeof(RSAKey));
    } else {
        if (pKM->pLocalPublicKey == 0)
            rt_memory_allocate((void **)&pKM->pLocalPublicKey, sizeof(RSAKey));
        if (pKM->pLocalPrivateKey == 0)
            rt_memory_allocate((void **)&pKM->pLocalPrivateKey, sizeof(RSAKey));
        *pKM->pLocalPublicKey = pKeyPair->pub;
        *pKM->pLocalPrivateKey = pKeyPair->priv;
    }
    return 0;
}

int func_0062F818(int nKMID, RSAKey *pKey)
{
    RSAKeyPair *pPair;
    int result = 0;

    if (nKMID == (int)0x80000000)
        pPair = &data_00676240;
    else if (nKMID == -1)
        pPair = &data_00676340;
    else
        pPair = &data_006762C0;
    if (IsNullRSAKey(&pPair->pub))
        result = -3;
    if (pKey != 0) {
        *pKey = pPair->pub;
        if (result != 0)
            memset(pKey, 0, sizeof(RSAKey));
    }
    return result;
}

int KM_SetGameSessionKey(int nKMID, int nGameSessionID, RCKey *pKey)
{
    KeyMaster *pKM = GetKeyMaster(nKMID);

    if (pKM == 0)
        return -1;
    if (IsNullRCKey(pKey))
        return -1;
    if (pKM->pGameSessionKey == 0)
        rt_memory_allocate((void **)&pKM->pGameSessionKey, sizeof(RCKey));
    if (pKM->pSessionEncrypt == 0)
        rt_memory_allocate((void **)&pKM->pSessionEncrypt, sizeof(RCState));
    if (pKM->pSessionDecrypt == 0)
        rt_memory_allocate((void **)&pKM->pSessionDecrypt, sizeof(RCState));
    pKM->nGameSessionID = nGameSessionID;
    *pKM->pGameSessionKey = *pKey;
    RCInitialize(pKM->pSessionEncrypt, pKey);
    RCInitialize(pKM->pSessionDecrypt, pKey);
    return 0;
}

int KM_GetGameSessionKey(int nKMID, int nGameSessionID, RCKey *pKey)
{
    KeyMaster *pKM = GetKeyMaster(nKMID);

    if (pKey != 0) {
        if (pKM != 0 && !IsNullRCKey(pKM->pGameSessionKey)) {
            *pKey = *pKM->pGameSessionKey;
            return 0;
        }
        memset(pKey, 0, sizeof(RCKey));
    }
    return -1;
}

int KM_AddSymmetricKey(int nKMID, int nClientID, RCKey *pKey)
{
    KeyMaster *pKM = GetKeyMaster(nKMID);
    KeyChain *pKC;

    if (pKM == 0)
        return -1;
    if (IsNullRCKey(pKey))
        return -1;
    pKC = GetKeyChain(pKM, nClientID);
    if (pKC == 0) {
        pKC = NewKeyChain();
        pKC->nID = nClientID;
        AppendKeyChain(pKM, pKC);
    }
    if (pKC->pSymmetricKey == 0)
        rt_memory_allocate((void **)&pKC->pSymmetricKey, sizeof(RCKey));
    if (pKC->pEncrypt == 0)
        rt_memory_allocate((void **)&pKC->pEncrypt, sizeof(RCState));
    if (pKC->pDecrypt == 0)
        rt_memory_allocate((void **)&pKC->pDecrypt, sizeof(RCState));
    *pKC->pSymmetricKey = *pKey;
    RCInitialize(pKC->pEncrypt, pKey);
    RCInitialize(pKC->pDecrypt, pKey);
    return 0;
}

int func_0062FCD8(int nKMID, int nClientID, RCKey *pKey)
{
    KeyMaster *pKM = GetKeyMaster(nKMID);
    KeyChain *pKC;

    if (pKM == 0 || pKey == 0)
        return -1;
    pKC = GetKeyChain(pKM, nClientID);
    if (pKC == 0 || IsNullRCKey(pKC->pSymmetricKey)) {
        memset(pKey, 0, sizeof(RCKey));
        return -1;
    }
    *pKey = *pKC->pSymmetricKey;
    return 0;
}

int func_0062FDD8(int nKMID, int nClientID, RCKey *pKey)
{
    RCKey key;
    int i;

    if (IsNullRCKey(pKey))
        return -1;
    if (func_0062FCD8(nKMID, nClientID, &key) != 0)
        return 1;
    i = 64;
    while (i-- > 0) {
        if (pKey->k[i] > key.k[i])
            return 1;
        if (pKey->k[i] < key.k[i])
            return -1;
    }
    return 0;
}

int func_0062FE80(int nKMID, int nClientID)
{
    KeyMaster *pKM = GetKeyMaster(nKMID);
    KeyChain *pKC;

    if (pKM != 0) {
        pKC = GetKeyChain(pKM, nClientID);
        if (pKC != 0) {
            if (pKC->pSymmetricKey != 0)
                memset(pKC->pSymmetricKey, 0, sizeof(RCKey));
            if (pKC->pEncrypt != 0)
                memset(pKC->pEncrypt, 0, sizeof(RCState));
            if (pKC->pDecrypt == 0)
                memset(pKC->pDecrypt, 0, sizeof(RCState));
        }
    }
}

int KM_CopyKeys(int nFromKMID, int nFromClientID, int nToKMID, int nToClientID)
{
    KeyMaster *pKM;
    KeyChain *pFrom;
    KeyChain *pTo;

    pKM = GetKeyMaster(nFromKMID);
    if (pKM == 0)
        return -1;
    pFrom = GetKeyChain(pKM, nFromClientID);
    if (pFrom == 0)
        return -1;
    pKM = GetKeyMaster(nToKMID);
    if (pKM == 0)
        return -1;
    pTo = GetKeyChain(pKM, nToClientID);
    if (pTo == 0) {
        pTo = NewKeyChain();
        pTo->nID = nToClientID;
        AppendKeyChain(pKM, pTo);
    }
    if (!IsNullRSAKey(pFrom->pPublicKey))
        KM_SetPublicKey(nToKMID, nToClientID, pFrom->pPublicKey);
    if (!IsNullRCKey(pFrom->pSymmetricKey)) {
        KM_AddSymmetricKey(nToKMID, nToClientID, pFrom->pSymmetricKey);
        *pTo->pEncrypt = *pFrom->pEncrypt;
        *pTo->pDecrypt = *pFrom->pDecrypt;
    }
    return 0;
}

int KM_DigiSign(int nKMID, int nClientID, RSAKey *pPub, RSAKey *pPriv, void *pSignature)
{
    KeyMaster *pKM;
    KeyChain *pKC;

    if (pSignature == 0 || IsNullRSAKey(pPub) || IsNullRSAKey(pPriv))
        return -1;
    pKM = GetKeyMaster(nKMID);
    if (pKM == 0)
        return -1;
    pKC = GetKeyChain(pKM, nClientID);
    if (pKC == 0)
        return -1;
    if (IsNullRCKey(pKC->pSymmetricKey))
        return -1;
    func_0062EEC0(pKC->pSymmetricKey, 4, pSignature, 64);
    func_0062BC10(pPub, pPriv, pSignature, pSignature, 64);
    return 0;
}

int KM_DigiVerify(int nKMID, int nClientID, RSAKey *pKey, void *pSignature)
{
    KeyMaster *pKM;
    KeyChain *pKC;
    unsigned int digest[16];
    unsigned int decrypted[16];
    unsigned int i;

    if (pSignature == 0 || IsNullRSAKey(pKey))
        return -1;
    pKM = GetKeyMaster(nKMID);
    if (pKM == 0)
        return -1;
    pKC = GetKeyChain(pKM, nClientID);
    if (pKC == 0)
        return -1;
    if (IsNullRCKey(pKC->pSymmetricKey))
        return -1;
    func_0062EEC0(pKC->pSymmetricKey, 4, digest, 64);
    func_0062BBE0(pKey, pSignature, decrypted, 64);
    for (i = 0; i < 16; i++)
        if (digest[i] != decrypted[i])
            return -1;
    return 0;
}

void KM_Destroy(int nKMID)
{
    KeyMaster *pKM = GetKeyMaster(nKMID);
    KeyChain *pKC;

    if (pKM != 0) {
        while ((pKC = pKM->pTail) != 0) {
            RemoveKeyChain(pKM, pKC);
            DeleteKeyChain(pKC);
        }
        if (pKM->pLocalPrivateKey != 0)
            rt_memory_free((void **)&pKM->pLocalPrivateKey);
        if (pKM->pLocalPublicKey != 0)
            rt_memory_free((void **)&pKM->pLocalPublicKey);
        if (pKM->pGameSessionKey != 0)
            rt_memory_free((void **)&pKM->pGameSessionKey);
        if (pKM->pSessionEncrypt != 0)
            rt_memory_free((void **)&pKM->pSessionEncrypt);
        if (pKM->pSessionDecrypt != 0)
            rt_memory_free((void **)&pKM->pSessionDecrypt);
        if (gpKeyMaster == pKM)
            gpKeyMaster = pKM->next;
        pKM->next->prev = pKM->prev;
        pKM->prev->next = pKM->next;
        if (gpKeyMaster == pKM)
            gpKeyMaster = 0;
        rt_memory_free((void **)&pKM);
    }
}

/* SOCOM 1's KM_Cleanup destroyed every key manager; SOCOM II's destroys a class of them: -1 the
 * local one, 0x80000000 those with the top bit set, 0 those without it, starting over from the
 * head after each destroy (the ring's walk ends back at gpKeyMaster) */
void KM_Cleanup(int nKMID)
{
    KeyMaster *pKM = gpKeyMaster;
    int nID;

    while (pKM != 0) {
        nID = pKM->nID;
        if ((nKMID == -1 && nID == nKMID)
            || (nKMID == (int)0x80000000 && nID == (nID & 0x7FFFFFFF) + nKMID)
            || (nKMID == 0 && nID == (nID & 0x7FFFFFFF))) {
            KM_Destroy(nID);
            pKM = gpKeyMaster;
        } else {
            pKM = pKM->next;
            if (pKM == gpKeyMaster)
                pKM = 0;
        }
    }
}

void KM_CreateRandomRCKey(RCKey *pKey)
{
    unsigned char i;

    InitializePRNG();
    for (i = 0; i < 16; i++)
        ((unsigned int *)pKey)[i] = randomMT();
}
