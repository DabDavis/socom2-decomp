/* strfile.cpp -- the film read: from the disc through libcdvd's stream functions (the file found in
 * CFileCD's table of contents), or from the host through sceRead, without blocking (Sony's ezmpeg
 * sample strfile.c, compiled as C++: SOCOM 1's demo names the functions and `infile`). OVL1
 * 0x30B9D0-0x30BD3F, the movie code's last unit (the network's start-up follows). Defined in
 * reverse of their image order (-inline deferred). */

#include "zmpeg.h"
#include "../zsys/zsys_public.h"

extern "C" {
int sceOpen(const char *name, int flags);
int sceClose(int fd);
int sceRead(int fd, void *buf, int nbyte);
int sceLseek(int fd, int offset, int whence);
int sceIoctl(int fd, int req, void *arg);
void *sceSifAllocIopHeap(unsigned int size);
int sceSifFreeIopHeap(void *addr);

/* libcdvd's streaming (MAIN, unnamed yet) */
struct sceCdRMode {
    unsigned char trycount;
    unsigned char spindlctrl;
    unsigned char datapattern;
    unsigned char pad;
};
int func_0018EF70(int mode);   /* sceCdSync? */
int func_0018F7D8(unsigned int bufmax, unsigned int bankmax, void *buffer);   /* sceCdStInit */
int func_0018F808(unsigned int lbn, sceCdRMode *mode);   /* sceCdStStart */
int func_0018F870(unsigned int lbn);   /* sceCdStSeek */
int func_0018F8A0(void);   /* sceCdStStop */
int func_0018F8D8(unsigned int size, void *buf, unsigned int mode, unsigned int *err);   /* sceCdStRead */
}

/* a file in the disc's table of contents (only what is read here) */
struct CDFileEntry {
    int _0;
    unsigned int lsn;   // 0x4
    int _8;
    int size;   // 0xc
};

class CFileCD {
public:
    static CDFileEntry *Find(const char *name);
};

class CFileIO {
public:
    static void PS2FileName(const char *name, char *out, bool cd);
};

/* the open film (SOCOM 1's `infile`, 0x14 bytes in both) */
struct StrFile {
    int isOnCD;   // 0x0
    int size;   // 0x4
    CDFileEntry *entry;   // 0x8
    unsigned char *iopBuff;   // 0xc
    int fd;   // 0x10
};

int nonBlockReadReq;   /* 0x455F20: a host read under way (SOCOM 1's) */
StrFile infile;   /* 0x455F30 */

/* the film opened: streamed from the disc on a CD boot, else opened on the host and its size taken
 * (Sony's, CFileCD's table SOCOM II's; OVL1 0x30BB60) */
int strFileOpen(const char *filename)
{
    char fname[1024];
    sceCdRMode mode;
    int status;

    CFileIO::PS2FileName(filename, fname, true);
    if (zSys.isCdBoot) {
        infile.isOnCD = 1;
        func_0018EF70(0);
        infile.iopBuff = (unsigned char *)sceSifAllocIopHeap(0x28010);
        func_0018F7D8(0x50, 5, (void *)(((int)infile.iopBuff + 15) & ~15));
        if (!(infile.entry = CFileCD::Find(fname))) {
            strFileClose();
            return 0;
        }
        infile.size = infile.entry->size;
        mode.trycount = 0;
        mode.spindlctrl = 0;
        mode.datapattern = 0;
        func_0018F808(infile.entry->lsn, &mode);
    } else {
        infile.isOnCD = 0;
        infile.fd = sceOpen(fname, 0x8001);
        do
            sceIoctl(infile.fd, 1, &status);
        while (status == 1);
        if (infile.fd < 0)
            return 0;
        infile.size = sceLseek(infile.fd, 0, 2);
        if (infile.size < 0) {
            sceClose(infile.fd);
            return 0;
        }
        if (sceLseek(infile.fd, 0, 0) < 0) {
            sceClose(infile.fd);
            return 0;
        }
        nonBlockReadReq = 0;
    }
    return 1;
}

/* the film read again from its start (Sony's; OVL1 0x30BAF0) */
int strFileRewind()
{
    int r;

    if (zSys.isCdBoot)
        r = func_0018F870(infile.entry->lsn);
    else
        r = sceLseek(infile.fd, 0, 0);
    if (r < 0) {
        sceClose(infile.fd);
        return 0;
    }
    return 1;
}

/* (Sony's; OVL1 0x30BA90) */
int strFileClose()
{
    if (infile.isOnCD) {
        func_0018F8A0();
        sceSifFreeIopHeap(infile.iopBuff);
    } else
        sceClose(infile.fd);
    return 1;
}

/* the next `size` bytes, when they are there: from the disc's stream in sectors, else a host read
 * started and its completion polled (Sony's; OVL1 0x30B9D0) */
int strFileRead(void *buff, int size)
{
    unsigned int err;
    int status;

    if (infile.isOnCD)
        return func_0018F8D8(size >> 11, buff, 0, &err) << 11;
    if (!nonBlockReadReq) {
        sceRead(infile.fd, buff, size);
        nonBlockReadReq = 1;
    }
    sceIoctl(infile.fd, 1, &status);
    if (!status) {
        nonBlockReadReq = 0;
        return size;
    }
    return 0;
}
