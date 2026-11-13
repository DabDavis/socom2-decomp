/* sc: overlay=MAIN */
/* The boot ELF's main and its loader (MAIN 0x1C4CC0-0x1C692B, the game's one MW unit after the
 * SDK libraries): the IOP rebooted with DNAS271.IMG and its modules loaded, then DNAS.BIN loaded
 * into OVL2's slot and the two MWo3 overlays FTSCORE and ZSEALETC out of APACHE00.ZDB -- off a
 * memory card holding it, else off the disc (or the host) -- through DNAS.BIN's codec, inflated to
 * their slots (docs/storage.md); full-screen pictures, a card error rebooting the ELF, the GS
 * reset. main scores at its MAIN row through `overlay=MAIN` and the row's `ovl-copy: main`. The
 * original file name is unknown; the functions it calls outside MAIN's libraries keep
 * placeholder names. Source order is free: auto-inlining is off (the `inline`s still expand). */

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

int sceOpen(const char *name, int flag, ...);     /* Sony's: variadic (0x1C6570 passes a third, 0) */
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

int sceGsSyncV(int mode);

#include <eetypes.h>	/* u_char ... u_long, and the one 128-bit type */

/* libgraph's BITBLTBUF (Sony's sceGsBitbltbuf) */
typedef struct {
    unsigned long SBP : 14;
    unsigned long : 2;
    unsigned long SBW : 6;
    unsigned long : 2;
    unsigned long SPSM : 6;
    unsigned long : 2;
    unsigned long DBP : 14;
    unsigned long : 2;
    unsigned long DBW : 6;
    unsigned long : 2;
    unsigned long DPSM : 6;
    unsigned long : 2;
} sceGsBitbltbuf;

/* libgraph's image transfer: a GIF tag, four A+D pairs, the IMAGE tag (0x60 bytes) */
typedef struct {
    u_long128 giftag0;
    sceGsBitbltbuf bitbltbuf;   /* 0x10 */
    long bitbltbufaddr;
    long trxpos;
    long trxposaddr;
    long trxreg;
    long trxregaddr;
    long trxdir;
    long trxdiraddr;
    u_long128 giftag1;
} sceGsLoadImage;

int sceGsSetDefLoadImage(sceGsLoadImage *lp, short dbp, short dbw, short dpsm, short x, short y,
                         short w, short h);
int sceGsExecLoadImage(sceGsLoadImage *lp, u_long128 *srcaddr);
int FlushCache(int op);
int sceDmaGetChan(int id);
int sceDmaSync(int d, int mode, int timeout);

#define GS_PMODE ((volatile unsigned long *)0x12000000)
#define GS_SMODE2 ((volatile unsigned long *)0x12000020)
#define GS_DISPFB1 ((volatile unsigned long *)0x12000070)
#define GS_DISPLAY1 ((volatile unsigned long *)0x12000080)
#define GS_DISPFB2 ((volatile unsigned long *)0x12000090)
#define GS_DISPLAY2 ((volatile unsigned long *)0x120000A0)
#define GS_BGCOLOR ((volatile unsigned long *)0x120000E0)

/* the GS's DISPLAY register (Sony's sceGsDisplay) as two words of bitfields: DY, spanning bytes 1
 * and 2, is then read and written as a word (lw/sw); as u_long fields it is a doubleword */
typedef struct {
    unsigned int DX : 12;
    unsigned int DY : 11;
    unsigned int MAGH : 4;
    unsigned int MAGV : 2;
    unsigned int : 3;
    unsigned int DW : 12;
    unsigned int DH : 11;
    unsigned int : 9;
} sceGsDisplay;

/* libgraph's display environment (the display word a bitfield struct) */
typedef struct {
    unsigned long pmode;
    unsigned long smode2;
    unsigned long dispfb;
    sceGsDisplay display;
    unsigned long bgcolor;
} sceGsDispEnv;

/* libgraph's double buffer; only the two display environments are used here */
typedef struct {
    sceGsDispEnv disp[2];
    char rest[0x1B0];           /* the GIF tags, drawing environments and clears */
} sceGsDBuff;

void sceGsResetPath(void);
int sceGsResetGraph(short mode, short inter, short omode, short ffmode);
int sceGsSetDefDBuff(sceGsDBuff *db, short psm, short w, short h, short ztest, short zpsm,
                     short clear);
void sceDmaReset(int mode);
int sceDmaSendN(int d, void *addr, int size);

/* the boot loader's double buffer (0x1E6410) and the DMA packet that clears the screen */
extern sceGsDBuff data_001E6410;
extern char data_001CE680[];

/* the GS reset to a black 640x448 interlaced screen, both read circuits on frame buffer 0 */
void func_001C67C0(void)
{
    *GS_PMODE = 0;
    *GS_BGCOLOR = 0;
    asm volatile { sync }
    sceGsResetPath();
    sceGsResetGraph(0, 1, 2, 0);
    sceGsSetDefDBuff(&data_001E6410, 0, 640, 448, 2, 58, 1);
    data_001E6410.disp[0].display.DX += 4;
    data_001E6410.disp[1].display.DX += 4;
    data_001E6410.disp[1].display.DY += 1;
    *GS_SMODE2 = 1;
    *GS_DISPLAY1 = *(unsigned long *)&data_001E6410.disp[0].display;
    *GS_DISPLAY2 = *(unsigned long *)&data_001E6410.disp[1].display;
    *GS_DISPFB1 = 0x1400;
    *GS_DISPFB2 = 0x1400;
    *GS_PMODE = 0x7F23;
    sceDmaReset(1);
    sceDmaSendN(sceDmaGetChan(2), data_001CE680, 10);
    sceDmaSync(sceDmaGetChan(2), 0, 0);
}

/* a raw 32-bit picture shown full width: the screen faded out (PMODE's ALP), the picture
 * streamed into GS memory 64 lines at a time, the screen faded back in */
void func_001C6570(const char *path, int w, int h, int x, int y)
{
    float a;
    int fd;
    sceGsLoadImage li;
    u_long128 buf[0xA000];

    a = 1.0f;
    do {
        sceGsSyncV(0);
        *GS_PMODE = ((long)(int)(255.0f * a) << 8) | 0xE1;
        a += -0.1f;
    } while (a >= 0.0f);
    *GS_PMODE = 0xE1;
    fd = sceOpen(path, 1, 0);
    if (fd >= 0) {
        sceGsSetDefLoadImage(&li, (x * 4 + y * 2560) / 256, w / 64, 0, 0, 0, w, 64);
        do {
            sceRead(fd, buf, 0x28000);
            FlushCache(0);
            sceGsExecLoadImage(&li, buf);
            sceDmaSync(sceDmaGetChan(2), 0, 0);
            h -= 64;
            li.bitbltbuf.DBP += 640;
        } while (h != 0);
        sceClose(fd);
    }
    a = 0.0f;
    do {
        sceGsSyncV(0);
        *GS_PMODE = ((long)(int)(255.0f * a) << 8) | 0xE1;
        a += 0.1f;
    } while (a <= 1.0f);
    *GS_PMODE = 0xFFE1;
    *GS_PMODE = 0x7F23;
}
int sceCdInit(int mode);
int LoadExecPS2(const char *filename, int argc, char **argv);
char *strcpy(char *, const char *);
char *strcat(char *, const char *);
int scePad2GetState(int port);                    /* the pad's state */
int scePad2Read(int port, void *data);        /* the pad read */
int scePad2GetButtonInfo(int port, void *data, int n);
void scePad2DeleteSocket(int port);
void scePad2End(void);
void sceDbcEnd(void);

/* the boot ELF's own name, argc and argv (for the reboot) */
extern char *data_001CE650;
extern int data_001E60B0;
extern char **data_001E60B8;
/* the pad's port and its data */
extern int data_001E6300;
extern char data_001E6310[];

/* a memory card error: the "card removed" picture shown until the pad answers, then the
 * libraries shut down and the boot ELF executed again */
void func_001C6390(int device, int result)
{
    const char *pic;
    char path[256];
    char exec[256];
    int ok;

    if (result < -9) {
        if (device == 0)
            pic = "PUPUTBAK.RAW";
        else
            pic = "PUPUTBA2.RAW";
        if (data_001E60C0) {
            strncpy(path, "host0:.", 256);
            strncat(path, "\\RUN\\RAW\\", 256);
            strncat(path, pic, 256);
            path[255] = 0;
        } else {
            strncpy(path, "cdrom0:", 256);
            strncat(path, "\\RUN\\RAW\\", 256);
            strncat(path, pic, 256);
            strncat(path, ";1", 256);
            path[255] = 0;
        }
        func_001C6570(path, 640, 128, 0, 160);
        for (;;) {
            sceGsSyncV(0);
            ok = 0;
            if (scePad2GetState(data_001E6300) == 1)
                ok = scePad2Read(data_001E6300, data_001E6310);
            if (ok == 0)
                continue;
            if (scePad2GetButtonInfo(data_001E6300, data_001E6310, 14) == 0)
                continue;
            scePad2DeleteSocket(data_001E6300);
            scePad2End();
            sceDbcEnd();
            sceCdInit(5);
            if (data_001E60C0 == 0) {
                LoadExecPS2("cdrom0:\\SCUS_972.75;1", data_001E60B0, data_001E60B8);
            } else {
                strcpy(exec, "host0:");
                strcat(exec, data_001CE650);
                LoadExecPS2(exec, data_001E60B0, data_001E60B8);
            }
        }
    }
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

void func_00181E00(void);
int sceSifInitRpc(int mode);
int sceSifRebootIop(const char *img);
int sceSifSyncIop(void);
int sceSifInitIopHeap(void);
int sceSifLoadFileReset(void);
int sceSifLoadModule(const char *filename, int args, const char *argp);
int sceCdGetDiskType(void);
int sceCdMmode(int media);
int sceFsReset(void);
int sceDbcInit(void);
int scePad2Init(int mode);

/* libpad2's socket parameters (src/lib/sdk/libpad2.c's) */
typedef struct {
    int f0, f4, f8, f12;
    char name[16];
} PAD2SOCKETPARAM;

int scePad2CreateSocket(PAD2SOCKETPARAM *param, void *addr);
extern char data_001E6200[];

typedef struct {
    unsigned char Sec, Min, Hour, Day, Month;
    unsigned short Year;
} sceMcStDateTime;

typedef struct {
    sceMcStDateTime _Create;
    sceMcStDateTime _Modify;
    unsigned int FileSizeByte;
    unsigned short AttrFile;
    unsigned short Reserve1;
    unsigned int Reserve2;
    unsigned int PdaAplNo;
    unsigned char EntryName[32];
} sceMcTblGetDir;

int sceMcInit(void);
int sceMcGetInfo(int port, int slot, int *type, int *free, int *format);
int sceMcChdir(int port, int slot, const char *newdir, char *olddir);
int sceMcGetDir(int port, int slot, const char *name, unsigned int mode, int maxent,
                sceMcTblGetDir *table);
void func_004C53C0(int argc, char **argv);

/* the CD/DVD drive started: the media mode set from the disc's type; nonzero when there is no
 * PlayStation 2 disc (booted from the host) */
inline int InitCd(void)
{
    sceCdInit(0);
    switch (sceCdGetDiskType()) {
    case 0x12:
        sceCdMmode(1);
        return 0;
    case 0x14:
        sceCdMmode(2);
        return 0;
    }
    return 1;
}

/* a formatted PS2 memory card in the port */
inline unsigned char CardReady(int port)
{
    int cmd;
    int result;
    int type;
    int format;
    unsigned char ok = 0;

    sceMcGetInfo(port, 0, &type, 0, &format);
    sceMcSync(0, &cmd, &result);
    if (result > -10 && type == 2 && format)     /* `> -10`: slti at / bnez at; `>= -9` computes the test into v0 */
        ok = 1;
    return ok;
}

/* the card holds SOCOM II's save directory with APACHE00.ZDB in it */
inline unsigned char CardHasZdb(int port)
{
    int cmd;
    int result;
    sceMcTblGetDir dir;
    unsigned char ok = 0;

    sceMcChdir(port, 0, "BASCUS-97275SOCOMII", 0);
    sceMcSync(0, &cmd, &result);
    if (result == 0) {
        sceMcGetDir(port, 0, "APACHE00.ZDB", 0, 1, &dir);
        sceMcSync(0, &cmd, &result);
        if (result > 0)
            ok = 1;
    }
    return ok;
}

#define LOAD_IRX(file) \
    do { \
        if (data_001E60C0) { \
            strncpy(data_001E60D0, "host0:.", 256); \
            strncat(data_001E60D0, "\\RUN\\IRX\\", 256); \
            strncat(data_001E60D0, file, 256); \
            data_001E60D0[255] = 0; \
        } else { \
            strncpy(data_001E60D0, "cdrom0:", 256); \
            strncat(data_001E60D0, "\\RUN\\IRX\\", 256); \
            strncat(data_001E60D0, file, 256); \
            strncat(data_001E60D0, ";1", 256); \
            data_001E60D0[255] = 0; \
        } \
    } while (sceSifLoadModule(data_001E60D0, 0, 0) < 0)

/* the boot ELF's main: the IOP rebooted with DNAS's image and its modules loaded, the GS reset,
 * the pad and the memory cards started; the overlays loaded off a card holding APACHE00.ZDB, else
 * off the disc, and ZSEALETC's entry run */
int main(int argc, char **argv)
{
    int device = -1;
    unsigned char ok;
    int cmd;
    int result;
    PAD2SOCKETPARAM pad;

    data_001CE650 = argv[0];
    data_001E60B0 = argc - 1;
    data_001E60B8 = argv + 1;
    func_00181E00();
    sceSifInitRpc(0);
    data_001E60C0 = InitCd();
    do {
        if (data_001E60C0) {
            strncpy(data_001E60D0, "host0:.", 256);
            strncat(data_001E60D0, "\\RUN\\IRX\\", 256);
            strncat(data_001E60D0, "DNAS271.IMG", 256);
            data_001E60D0[255] = 0;
        } else {
            strncpy(data_001E60D0, "cdrom0:", 256);
            strncat(data_001E60D0, "\\RUN\\IRX\\", 256);
            strncat(data_001E60D0, "DNAS271.IMG", 256);
            strncat(data_001E60D0, ";1", 256);
            data_001E60D0[255] = 0;
        }
    } while (!sceSifRebootIop(data_001E60D0));
    while (!sceSifSyncIop())
        ;
    sceSifInitRpc(0);
    sceSifInitIopHeap();
    sceSifLoadFileReset();
    InitCd();
    sceFsReset();
    func_001C67C0();
    LOAD_IRX("SIO2MAN.IRX");
    LOAD_IRX("CDVDSTM.IRX");
    if (data_001E60C0) {
        strncpy(data_001E60D0, "host0:.", 256);
        strncat(data_001E60D0, "\\RUN\\RAW\\", 256);
        strncat(data_001E60D0, "PULOADGP.RAW", 256);
        data_001E60D0[255] = 0;
    } else {
        strncpy(data_001E60D0, "cdrom0:", 256);
        strncat(data_001E60D0, "\\RUN\\RAW\\", 256);
        strncat(data_001E60D0, "PULOADGP.RAW", 256);
        strncat(data_001E60D0, ";1", 256);
        data_001E60D0[255] = 0;
    }
    func_001C6570(data_001E60D0, 640, 128, 0, 160);
    LOAD_IRX("SIO2D.IRX");
    LOAD_IRX("DBCMAN.IRX");
    LOAD_IRX("DS2U_S1.IRX");
    LOAD_IRX("MCMAN.IRX");
    LOAD_IRX("MCSERV.IRX");
    sceDbcInit();
    scePad2Init(0);
    pad.f0 = 2;
    pad.f4 = 0;
    pad.f8 = 0;
    pad.f12 = 0;
    pad.name[0] = 0;
    data_001E6300 = scePad2CreateSocket(&pad, data_001E6200);
    sceMcInit();
    sceMcSync(0, &cmd, &result);
    if (CardReady(0)) {
        if (CardHasZdb(0))
            device = 0;
    }
    if (device == -1) {
        if (CardReady(1)) {
            if (CardHasZdb(1))
                device = 1;
        }
    }
    ok = 0;
    if (device >= 0)
        ok = func_001C5B30(device);
    if (data_001E60C0) {
        strncpy(data_001E60D0, "host0:.", 256);
        strncat(data_001E60D0, "\\RUN\\RAW\\", 256);
        strncat(data_001E60D0, "LOADING.RAW", 256);
        data_001E60D0[255] = 0;
    } else {
        strncpy(data_001E60D0, "cdrom0:", 256);
        strncat(data_001E60D0, "\\RUN\\RAW\\", 256);
        strncat(data_001E60D0, "LOADING.RAW", 256);
        strncat(data_001E60D0, ";1", 256);
        data_001E60D0[255] = 0;
    }
    func_001C6570(data_001E60D0, 640, 448, 0, 0);
    if (!ok && !data_001E60C0)
        ok = func_001C59C0();
    scePad2DeleteSocket(data_001E6300);
    scePad2End();
    sceDbcEnd();
    if (ok)
        func_004C53C0(argc, argv);
    return 0;
}
