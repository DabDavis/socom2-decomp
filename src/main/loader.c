/* The boot loader's overlay decode (MAIN 0x1C5DA0): one MWo3 overlay out of
 * APACHE00.ZDB, through DNAS.BIN's codec, inflated to its slot (docs/storage.md).
 * The original file name is unknown; the functions it calls keep placeholder names
 * until their own TUs are written. */

#pragma auto_inline off    /* the image calls 0x1C5DA0 and 0x1C60B0 out of line (0x114/0xEC bytes) */

typedef unsigned long uLong;    /* 64-bit under this compiler: uncompress's 4th argument is lwu-loaded */

int func_001C5EC0(const char *name, void *buf);                        /* read a ZDB entry -> its length */
int func_00539D00(unsigned int len, void *buf, int *key);              /* DNAS.BIN: container header */
int func_00539D50(unsigned int len, int key, void *buf);               /* DNAS.BIN: body decrypt -> length */
int func_00534848(unsigned int len, void *buf, unsigned int *clen);    /* DNAS.BIN: content setup */
int func_00535018(unsigned int len, unsigned int clen, void *buf);     /* DNAS.BIN: content run -> 0 */
void func_004C73F0(void);                                               /* DNAS.BIN: before the second payload */
int uncompress(void *dest, uLong *destLen, const void *source, uLong sourceLen);
void func_00181D80(void *dest, int size);

/* the overlay slots' load addresses, by slot (0x1D52A0: MAIN, OVL2's, OVL1's) */
extern void *data_001D52A0[];
/* nonzero when booted from the host (host0:), zero from the disc */
extern int data_001E60C0;
/* the path the loader builds (0x1E60D0, 256 bytes) */
extern char data_001E60D0[256];
/* the device the overlays load from (the boot unit's word after main) */
extern int data_001E70F4;

int func_00181C90(const char *path, void *ovl);
void func_00534830(void);
int func_001C60B0(int device, const char *name, void *dest, void *buf, int second);
void func_001C6570(const char *path, int w, int h, int x, int y);
char *strncpy(char *, const char *, unsigned int);
char *strncat(char *, const char *, unsigned int);
int func_001C5DA0(const char *name, void *dest, void *buf, int second);

int sceOpen(const char *name, int mode);
int sceClose(int fd);
int sceRead(int fd, void *buf, int n);
int sceLseek(int fd, int offset, int whence);
int strcmp(const char *, const char *);
int strcasecmp(const char *, const char *);

/* APACHE00.ZDB's header (0xA0 bytes) and directory entries (0x5C bytes); the names ours */
typedef struct {
    char pad0[0x14];
    char version[0x84];         /* 0x14: "r0001" */
    unsigned int count;         /* 0x98: the entries */
    char pad9c[0x64];           /* 0xA0 read; the frame holds 0x100 */
} ZdbHeader;

typedef struct {
    char pad0[4];
    char name[0x40];            /* 0x04 */
    unsigned int offset;        /* 0x44: unsigned -- converted to the int parameter, it is loaded ahead of fd */
    unsigned int size;          /* 0x48 */
    char pad4c[0x14];           /* 0x5C read; the frame holds 0x60 */
} ZdbEntry;

/* a ZDB entry read off the disc (or the host) into buf: its length, 0 if absent */
int func_001C5EC0(const char *name, void *buf)
{
    int fd;
    int len = 0;
    unsigned int i;
    ZdbHeader hdr;
    ZdbEntry entry;

    if (data_001E60C0) {
        strncpy(data_001E60D0, "host0:.", 256);
        strncat(data_001E60D0, "\\RUN\\RAW\\", 256);
        strncat(data_001E60D0, "APACHE00.ZDB", 256);
        data_001E60D0[255] = 0;
    } else {
        strncpy(data_001E60D0, "cdrom0:", 256);
        strncat(data_001E60D0, "\\RUN\\RAW\\", 256);
        strncat(data_001E60D0, "APACHE00.ZDB", 256);
        strncat(data_001E60D0, ";1", 256);
        data_001E60D0[255] = 0;
    }
    fd = sceOpen(data_001E60D0, 1);
    if (fd >= 0) {
        sceRead(fd, &hdr, 0xA0);
        if (strcmp(hdr.version, "r0001") == 0) {
            for (i = 0; i < hdr.count; i++) {
                sceRead(fd, &entry, 0x5C);
                if (strcasecmp(name, entry.name) == 0) {
                    sceLseek(fd, entry.offset, 0);
                    len = sceRead(fd, buf, entry.size);
                    break;
                }
            }
        }
        sceClose(fd);
    }
    return len;
}

int sceMcOpen(int port, int slot, const char *name, int mode);
int sceMcClose(int fd);
int sceMcRead(int fd, void *buf, int size);
int sceMcSeek(int fd, int offset, int mode);
int sceMcSync(int mode, int *cmd, int *result);
void func_001C6390(int device, int result);

/* a ZDB entry read off the memory card's APACHE00.ZDB into buf: func_001C5EC0 for the card, each
 * call synchronised and a card error (below -9) handed to func_001C6390, which reboots */
int func_001C61A0(int device, const char *name, void *buf)
{
    int fd = -1;
    int len = 0;
    int cmd;
    int result;
    unsigned int i;
    ZdbHeader hdr;
    ZdbEntry entry;

    sceMcOpen(device, 0, "APACHE00.ZDB", 1);
    sceMcSync(0, &cmd, &fd);
    if (fd >= 0) {
        sceMcRead(fd, &hdr, 0xA0);
        sceMcSync(0, &cmd, &result);
        if (result < -9)
            func_001C6390(device, result);
        if (strcmp(hdr.version, "r0001") == 0) {
            for (i = 0; i < hdr.count; i++) {
                sceMcRead(fd, &entry, 0x5C);
                sceMcSync(0, &cmd, &result);
                if (result < -9)
                    func_001C6390(device, result);
                if (strcasecmp(name, entry.name) == 0) {
                    sceMcSeek(fd, entry.offset, 0);
                    sceMcSync(0, &cmd, &result);
                    sceMcRead(fd, buf, entry.size);
                    sceMcSync(0, &cmd, &len);
                    if (len < -9)
                        func_001C6390(device, len);
                    break;
                }
            }
        }
        sceMcClose(fd);
        sceMcSync(0, &cmd, &result);
        if (result < -9)
            func_001C6390(device, result);
    }
    return len;
}

/* an overlay read off the device (the memory card) and inflated into its slot: loader.c's
 * func_001C5DA0 for the device, whose entry comes decrypted */
int func_001C60B0(int device, const char *name, void *dest, void *buf, int second)
{
    uLong destLen = 0x5000000;
    int len = 0;                /* before ok: its zero is set first, in v0 (after it, below the store) */
    int ok = 0;
    unsigned int clen;

    if (device >= 0)
        len = func_001C61A0(device, name, buf);
    if (len > 0) {
        if (func_00534848(len, buf, &clen) == 0) {
            if (func_00535018(len, clen, buf) == 0) {
                if (second)
                    func_004C73F0();
                if (uncompress(dest, &destLen, buf, clen) == 0) {
                    if (destLen != 0) {
                        func_00181D80(dest, destLen);
                        ok = 1;
                    }
                }
            }
        }
    }
    return ok;
}

/* the device's boot: DNAS.BIN into OVL2's slot, then FTSCORE and ZSEALETC out of the device */
unsigned char func_001C5B30(int device)
{
    unsigned char ok = 0;
    const char *pic;
    void *ovl2 = data_001D52A0[1];
    void *ovl1 = data_001D52A0[2];
    char buf[0x180000];

    if (data_001E60C0) {
        strncpy(data_001E60D0, "host0:./OVERLAY/REL/", 256);
        strncat(data_001E60D0, "DNAS.BIN", 256);
        data_001E60D0[255] = 0;
    } else {
        strncpy(data_001E60D0, "cdrom0:\\OVERLAY\\REL\\", 256);
        strncat(data_001E60D0, "DNAS.BIN", 256);
        strncat(data_001E60D0, ";1", 256);
        data_001E60D0[255] = 0;
    }
    if (func_00181C90(data_001E60D0, ovl2)) {
        func_00534830();
        if (device == 0)
            pic = "PUNOTREM.RAW";
        else
            pic = "PUNOTRE2.RAW";
        if (data_001E60C0) {
            strncpy(data_001E60D0, "host0:.", 256);
            strncat(data_001E60D0, "\\RUN\\RAW\\", 256);
            strncat(data_001E60D0, pic, 256);
            data_001E60D0[255] = 0;
        } else {
            strncpy(data_001E60D0, "cdrom0:", 256);
            strncat(data_001E60D0, "\\RUN\\RAW\\", 256);
            strncat(data_001E60D0, pic, 256);
            strncat(data_001E60D0, ";1", 256);
            data_001E60D0[255] = 0;
        }
        func_001C6570(data_001E60D0, 640, 128, 0, 160);
        if (func_001C60B0(device, "FTSCORE", ovl1, buf, 0) && func_001C60B0(device, "ZSEALETC", ovl2, buf, 1)) {
            ok = 1;
            data_001E70F4 = device;
        }
    }
    return ok;
}

/* the disc's boot: DNAS.BIN into OVL2's slot, then FTSCORE and ZSEALETC out of APACHE00.ZDB;
 * DNAS.BIN shut down on a failure */
unsigned char func_001C59C0(void)
{
    unsigned char ok = 0;
    void *ovl2 = data_001D52A0[1];
    void *ovl1 = data_001D52A0[2];
    char buf[0x180000];

    if (data_001E60C0) {
        strncpy(data_001E60D0, "host0:./OVERLAY/REL/", 256);
        strncat(data_001E60D0, "DNAS.BIN", 256);
        data_001E60D0[255] = 0;
    } else {
        strncpy(data_001E60D0, "cdrom0:\\OVERLAY\\REL\\", 256);
        strncat(data_001E60D0, "DNAS.BIN", 256);
        strncat(data_001E60D0, ";1", 256);
        data_001E60D0[255] = 0;
    }
    if (func_00181C90(data_001E60D0, ovl2)) {
        func_00534830();
        if (func_001C5DA0("FTSCORE", ovl1, buf, 0) && func_001C5DA0("ZSEALETC", ovl2, buf, 1))
            ok = 1;
        if (ok == 0)        /* `!ok` tests the register; `== 0` masks it first (andi) */
            func_004C73F0();
    }
    return ok;
}

int func_001C5DA0(const char *name, void *dest, void *buf, int second)
{
    uLong destLen = 0x5000000;
    int ok = 0;
    int len;
    unsigned int clen;
    int key;

    if ((len = func_001C5EC0(name, buf)) > 0) {
        key = 0;
        if (func_00539D00(len, buf, &key) >= 0) {
            if ((len = func_00539D50(len, key, buf)) > 0) {
                if (func_00534848(len, buf, &clen) == 0) {
                    if (func_00535018(len, clen, buf) == 0) {
                        if (second)
                            func_004C73F0();
                        if (uncompress(dest, &destLen, buf, clen) == 0) {
                            if (destLen != 0) {
                                func_00181D80(dest, destLen);
                                ok = 1;
                            }
                        }
                    }
                }
            }
        }
    }
    return ok;
}
