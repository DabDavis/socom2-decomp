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
