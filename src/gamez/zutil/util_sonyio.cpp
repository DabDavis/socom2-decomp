/* util_sonyio.cpp -- CFileIO (OVL1 0x39CDC0-0x39F150): a file on the host, the CD (through
 * CFileCD's sector reader) or the hard disk, through Sony's sceOpen/sceRead and libcdvd. The file
 * name is SOCOM 1's: the press beta's asserts name util_sonyio.cpp, and SOCOM 1's demo holds
 * CFileIO's and CFileCD's methods in this order between COutput's unit and CSTable's (CBufferIO's
 * IsOpen among them in both builds). SOCOM II added the hard disk, the error handler, the flags
 * word at 0x10, reads through an open package (0x4A15F8/FC: util_main.cpp's CPackage, called here
 * through its placeholders) and a CD file opened by its sector (the DNAS client's).
 *
 * The hard disk's game partition: HddFindFiles 0x39CE80, HddMountGame 0x39D050 (the UI lane's round
 * 31), WriteHddIcon and LoadHddIconFile (OPEN, its note), HddUnmountGame, HddRebootCallback and
 * HddRegisterReboot (the UI lane's round 34). Not written: 0x39DDB0 and
 * 0x39DFC0 (byte-exact as drafts). */

#include <stdio.h>
#include "zutil.h"

extern "C" {
int sceOpen(const char *name, int flags);
int sceRead(int fd, void *buf, int nbyte);
int sceWrite(int fd, const void *buf, int nbyte);
int sceLseek(int fd, int offset, int whence);
int sceClose(int fd);
char *strstr(const char *s, const char *sub);
/* MSL's strtok (MAIN) */
char *strtok(char *s, const char *delim);
/* the CD reads that wait out the sound streams (989snd.c's) */
int snd_StreamSafeCdGetError(void);
int snd_StreamSafeCdSync(int mode);
int snd_StreamSafeCdRead(unsigned int lbn, unsigned int sectors, void *buf, void *mode);
}

void zUtilStrUpr(char *s);

/* the system's public state: whether this is a CD boot (zsys_public.h) */
struct _zsys_public {
    unsigned char _0[0x1d];
    unsigned char isCdBoot;   // 0x1D
};
extern _zsys_public zSys;
/* the hard disk's partition is mounted (zsys_init.cpp's static, the sound/system lane's name) */
extern unsigned char s_hddMounted;

/* a file in the CD's table of contents (SOCOM 1's DWARF) */
struct _FILECD {
    char *name;                 // 0x0
    unsigned int startSector;   // 0x4
    unsigned int endSector;     // 0x8
    unsigned int sectorSize;    // 0xc: the file's size in bytes
};   // 0x10 bytes

/* libcdvd's read mode (Sony's sceCdRMode: the retry count, the spindle speed, the sector size) */
struct sceCdRMode {
    unsigned char trycount;      // 0x0
    unsigned char spindlctrl;    // 0x1
    unsigned char datapattern;   // 0x2
    unsigned char pad;           // 0x3
};

/* the libcdvd read mode the reads pass and the bounce buffer for a destination not 16-byte
 * aligned (SOCOM 1's file statics, 0x4A1778 and 0x4A17B0) */
extern sceCdRMode m_CDmode;
extern char alignedBuffer[0x2000];
/* the lowest sector a file of the TOC starts at (SOCOM II's: BuildTOC keeps it, 0x39DFC0 parks
 * the head there) */
extern int data_004A17A0;

/* a CD file being read: its TOC entry, the position, the next sector to read, the sector buffer
 * and the position in it (SOCOM 1's DWARF: 0x14 bytes). The constructor and destructor are out of
 * line: the DNAS client calls them (0x2CC9D0), with OpenSector */
class CFileCD {
public:
    CFileCD();
    ~CFileCD();
    int Read(char *buf, int size);
    int XferSectorsToCaller(char *buf, int sectors);
    bool OpenSector(unsigned int sector);
    static _FILECD *Find(const char *name);
    static char *CreatePath(char *dir, char *path);
    static void BuildTOC();
    /* the sector the next read starts from (ours: fseek, Open and OpenSector share it) */
    void SetHeadSector(int sector)
    {
        if (m_headSector != sector) {
            m_headSector = sector;
            m_buf = 0;
        }
    }
    /* the next sector read into the buffer, the position found in it (ours) */
    void ReadSector()
    {
        snd_StreamSafeCdSync(0);
        snd_StreamSafeCdRead(m_headSector++, 1, m_sector, &m_CDmode);
        snd_StreamSafeCdGetError();
        m_buf = m_sector + (m_pos & 0x7FF);
        snd_StreamSafeCdSync(0);
    }

    _FILECD *m_toc;         // 0x0
    unsigned int m_pos;     // 0x4
    int m_headSector;       // 0x8
    char *m_sector;         // 0xc
    char *m_buf;            // 0x10

    static _FILECD *m_TOC;            // 0x4A1780 (SOCOM 1's)
    static int m_TOCNum;              // 0x4A1788 (SOCOM 1's)
};

/* an open package (the package unit's, SOCOM II's; the names ours): its header and the entry read */
struct PAKENTRY {
    unsigned char _0[0x44];
    int offset;   // 0x44
    int size;     // 0x48
};

/* the package (util_main.cpp's CPackage, ours: its header, then the entry opened; only what this
 * unit reaches) -- its entry by name, its file seeked and the file itself */
class CPackage {
public:
    PAKENTRY *Find(const char *name);
    int Seek(int offset, int whence);
    CFileIO *GetFile();

    void *m_header;   // 0x0
};
/* the package state (SOCOM II's: util_main.cpp's thePackage 0x4A15F8 and its entry opened, 0x4A15FC) */
extern CPackage thePackage;
extern PAKENTRY *data_004A15FC;
/* the root path, the file suffix (";1" on the CD) and the root's directory */
extern char data_003E0520[];
extern char data_004A1750[];
extern char data_004A1650[];
/* the error handler (ours) */
extern void (*data_004A1770)(int error);

/* a file's size, the position put back at its start; -1 unless it was opened for reading (ours) */
static inline int SizeOf(int fd, int read)
{
    int size = -1;

    if (fd != -1 && read) {
        size = sceLseek(fd, 0, 2);
        sceLseek(fd, 0, 0);
    }
    return size;
}

/* a name made the engine's way: its parts joined with backslashes, upper case (ours) */
static inline void MakeFileName(char *out, const char *name)
{
    const char *delim = "/\\";
    char path[512];
    char *tok;

    *out = 0;
    strcpy(path, name);
    for (tok = strtok(path, delim); tok;) {
        strcat(out, tok);
        tok = strtok(0, delim);
        if (tok)
            strcat(out, "\\");
    }
    zUtilStrUpr(out);
}

int CFileIO::fread(int size, void **buf)
{
    *buf = 0;
    return 0;
}

void CFileIO::SetRootPath(const char *path)
{
    char *dir;

    if (strstr(path, "cdrom"))
        strcpy(data_004A1750, ";1");
    else
        strcpy(data_004A1750, "");
    strcpy(data_003E0520, path);
    dir = strstr(data_003E0520, "\\");
    if (dir)
        strcpy(data_004A1650, dir + 1);
}

CFileIO::CFileIO()
{
    m_file = -1;
    m_mode = 0;
    m_filesize = 0;
    m_flags = 0;
}

/* a CD file is open when its reader exists (ours: the && is materialised, an inline's) */
static inline bool CDFileOpen(int file)
{
    return file != -1 && file != 0;
}

bool CFileIO::IsOpen() const
{
    bool open = false;

    if (m_flags) {
        if (m_flags & 1)
            open = CDFileOpen(m_file);
        else
            open = m_file >= 0;
    }
    return open;
}

bool CBufferIO::IsOpen() const
{
    return m_buffer != 0;
}

void CFileIO::Release()
{
    m_file = -1;
}

void CFileIO::PS2FileName(const char *name, char *out, bool cd)
{
    char upath[512];

    MakeFileName(upath, name);
    if (cd && CFileCD::m_TOC) {
        strcpy(out, upath);
        strcat(out, ";1");
    } else {
        strcpy(out, data_003E0520);
        strcat(out, upath);
    }
}

bool CFileIO::Open(const char *name, unsigned int mode)
{
    char upath[512];
    char full[512];
    int flags = 0;

    if (mode & 1)
        flags |= 1;
    if (mode & 2)
        flags |= 2;
    if (mode & 0x10)
        flags |= 0x400;
    if (mode & 8)
        flags |= 0x200;
    m_mode = mode;
    m_file = -1;
    m_flags = 0;
    MakeFileName(upath, name);
    if (thePackage.m_header) {
        PAKENTRY *e = thePackage.Find(upath);

        if (e) {
            CFileIO *pak;

            thePackage.Seek(e->offset, 0);
            pak = thePackage.GetFile();
            m_file = pak->m_file;
            m_flags = pak->m_flags | 0x100;
            m_filesize = e->size;
            data_004A15FC = e;
        }
    } else if (s_hddMounted) {
        char *d = full;
        char *s = upath;

        strcpy(d, "pfs0:/");
        for (d += 6; *s != 0; s++, d++) {
            if (*s == '\\')
                *d = '/';
            else
                *d = *s;
        }
        *d = 0;
        m_file = sceOpen(full, flags);
        if (m_file < 0)
            m_file = -1;
        else {
            m_filesize = SizeOf(m_file, mode & 1);
            m_flags = 2;
        }
    }
    if (m_file == -1) {
        if (zSys.isCdBoot) {
            CFileCD *cd;

            strcat(upath, ";1");
            m_cd = new CFileCD;
            cd = m_cd;
            cd->m_toc = CFileCD::Find(upath);
            if (cd->m_toc) {
                cd->m_pos = 0;
                cd->SetHeadSector(cd->m_toc->startSector);
            }
            if (cd->m_toc) {
                m_filesize = m_cd->m_toc->sectorSize;
                m_flags = 1;
            } else {
                delete m_cd;
                m_cd = 0;
            }
        } else {
            strcpy(full, data_003E0520);
            strcat(full, upath);
            m_file = sceOpen(full, flags);
            if (m_file >= 0) {
                m_filesize = SizeOf(m_file, mode & 1);
                m_flags = 4;
            }
        }
    }
    return CFileIO::IsOpen();
}

void CFileIO::Close()
{
    if (thePackage.m_header && (m_flags & 0x100)) {
        m_file = -1;
        data_004A15FC = 0;
        return;
    }
    if (CFileIO::IsOpen()) {
        if (m_flags == 1 && m_cd)
            delete m_cd;
        else
            sceClose(m_file);
        m_file = -1;
    }
}

/* the read position moved (SOCOM 1's CFileIO::fseek, the package's offset added: SOCOM II's).
 * Byte-exact since the UI's round 23 (open since round 2, 2 instructions short): the package's offset
 * read as a plain if/else into a local declared first (the target's `beqz; move t0,zero; b; lw t0;
 * move t0,zero` -- the ternary, an inline returning it and a zeroed local all if-convert it with the
 * registers swapped), the host path's 2-1-default chain a SWITCH with the default written first
 * and case 1 before case 2 (MW tests the higher value first and lays the blocks out in source
 * order: the if/else-if chain tests 2 then 1 but puts the default last), and the sector's sum
 * through a local shifted value added first (`s + start`; in place, either order, MW emits the
 * start sector first). */
int CFileIO::fseek(int offset, int whence)
{
    int pos;

    if (m_flags & 1) {
        int base;
        CFileCD *cd;
        int sector;

        if (data_004A15FC)
            base = data_004A15FC->offset;
        else
            base = 0;
        cd = m_cd;
        switch (whence) {
        case 0:
            cd->m_pos = offset;
            break;
        case 1:
            cd->m_pos += offset;
            break;
        case 2:
            cd->m_pos = offset + cd->m_toc->sectorSize;
            break;
        }
        unsigned int s = (cd->m_pos + base) >> 11;

        sector = s + cd->m_toc->startSector;
        if (cd->m_headSector != sector) {
            cd->m_headSector = sector;
            cd->m_buf = 0;
        }
        pos = cd->m_pos;
    } else {
        int mode;

        switch (whence) {
        default:
            mode = 0;
            if (data_004A15FC)
                offset += data_004A15FC->offset;
            break;
        case 1:
            mode = 1;
            break;
        case 2:
            mode = 0;
            offset += m_filesize;
            if (data_004A15FC)
                offset += data_004A15FC->offset;
            break;
        }
        pos = sceLseek(m_file, offset, mode);
    }
    if (pos < 0 && (m_flags & 2))
        data_004A1770(2);
    return pos;
}

int CFileIO::fread(void *buf, int size)
{
    int n;

    if (size == 0)
        return 0;
    if (m_flags & 1)
        n = m_cd->Read((char *)buf, size);
    else
        n = sceRead(m_file, buf, size);
    if (n < 0 && (m_flags & 2))
        data_004A1770(2);
    return n;
}

int CFileIO::fwrite(const void *buf, int size)
{
    int n;

    if (!(m_flags & 1))
        n = sceWrite(m_file, buf, size);
    if (n < 0 && (m_flags & 2))
        data_004A1770(2);
    return n;
}

int CFileIO::ftell() const
{
    return 0;
}

void CFileIO::fflush()
{
}

/* a directory entered on a path: "." leaves it, ".." strips its last part, any other name is
 * appended after a backslash (SOCOM 1's; BuildTOC walks the disc with it) */
char *CFileCD::CreatePath(char *dir, char *path)
{
    char *p = path;

    if (!strcmp(dir, "."))
        return path;
    if (!strcmp(dir, "..")) {
        for (p += strlen(path) - 1; *p != '\\' && p != path; p--)
            *p = 0;
        if (strcmp(path, "\\"))
            *p = 0;
    } else {
        if (strlen(path))
            strcpy(path + strlen(path), "\\");
        strcpy(path + strlen(path), dir);
    }
    return path;
}

/* the disc's table of contents built: the primary volume descriptor read from sector 16, its root
 * directory pushed, then each directory popped and its sectors' records walked -- "." and ".."
 * skipped, a file entered in the TOC under its path, a directory pushed under its own (SOCOM 1's;
 * SOCOM II's TOC holds 360 entries, a directory's sectors round up and the lowest file sector is
 * kept). The UI lane's round 28, by MW's colouring (tools/igreplay.py --want, then tools/igvar.py):
 * off and sector declared uninitialised at the top, ahead of buf (s5 / s4 / s3 as the target);
 * the record read as buf[off + N], no pointer of its own (MW's CSE temporary takes s0, the name's
 * address `buf + (off + 33)`); ONE sectors for the root and the walk -- MW splits the second live
 * range into a temporary numbered after the loop optimiser's (its spill slot below the walk's
 * pointers, sp 176); the lowest sector a signed int (lw, then sltu against the 64-bit sector);
 * the name taken after the entry's stores, the lowest sector set first */
void CFileCD::BuildTOC()
{
    _FILECD dirs[40];
    char path[512];
    int count;
    int done;
    int off;
    unsigned long sector;
    int sectors;

    data_004A17A0 = 999999999;
    m_CDmode.trycount = 20;
    m_CDmode.spindlctrl = 1;
    m_CDmode.datapattern = 0;
    path[0] = 0;
    count = 0;
    done = 0;
    m_TOC = (_FILECD *)_malloc(360 * sizeof(_FILECD), 0, 0);
    m_TOCNum = 0;
    unsigned char *buf = (unsigned char *)_memalign(64, 2064, 0, 0);
    CFileCD cd;

    cd.SetHeadSector(16);
    cd.Read((char *)buf, 2048);
    {
        int size = buf[166] + (buf[167] << 8) + (buf[168] << 16) + (buf[169] << 24);
        sectors = (size + 2047) / 2048;

        dirs[count].startSector = buf[158] + (buf[159] << 8) + (buf[160] << 16) + (buf[161] << 24);
        dirs[count].sectorSize = sectors;
        dirs[count].name = (char *)_malloc(1, 0, 0);
        strcpy(dirs[count].name, "");
        count++;
    }
    while (done != count) {
        _FILECD *dir = &dirs[done++];
        unsigned int extent = dir->startSector;
        int i;

        sectors = dir->sectorSize;
        strcpy(path, dir->name);
        for (i = 0; i < sectors; i++) {
            off = 0;

            cd.SetHeadSector(extent + i);
            cd.Read((char *)buf, 2048);
            for (; buf[off]; off += buf[off]) {
                sector = buf[off + 2] + (buf[off + 3] << 8) + (buf[off + 4] << 16) + (buf[off + 5] << 24);
                unsigned long size = buf[off + 10] + (buf[off + 11] << 8) + (buf[off + 12] << 16) + (buf[off + 13] << 24);

                if (buf[off + 32] == 1 && buf[off + 33] == 0)
                    continue;
                if (buf[off + 32] == 1 && buf[off + 33] == 1)
                    continue;
                if (!(buf[off + 25] & 2)) {
                    _FILECD *f = &m_TOC[m_TOCNum++];

                    f->startSector = sector;
                    f->sectorSize = size;
                    char *name = (char *)&buf[off + 33];
                    f->name = (char *)_malloc(strlen(path) + 3 + strlen(name), 0, 0);
                    f->name[0] = 0;
                    if (sector < data_004A17A0)
                        data_004A17A0 = sector;
                    if (strlen(path)) {
                        strcpy(f->name, path);
                        strcat(f->name, "\\");
                    }
                    strcat(f->name, name);
                } else {
                    dirs[count].startSector = sector;
                    dirs[count].sectorSize = (size + 2047) / 2048;
                    dirs[count].name = _strdup(CreatePath((char *)&buf[off + 33], path), 0, 0);
                    CreatePath("..", path);
                    count++;
                }
            }
        }
    }
    for (int j = 0; j < done; j++)
        _free(dirs[j].name, 0, 0);
}

/* the file's TOC entry by name, under the root's directory (SOCOM 1's, the directory SOCOM II's) */
_FILECD *CFileCD::Find(const char *name)
{
    char path[256];
    _FILECD *found;
    int i;
    int off;

    if (!m_TOC)
        return 0;
    found = 0;
    strcpy(path, data_004A1650);
    strcat(path, name);
    for (i = 0, off = 0; i < m_TOCNum && !found; off += 16, i++)
        if (!strcmp(path, *(char **)((char *)m_TOC + off)))
            found = (_FILECD *)((char *)m_TOC + off);
    return found;
}

CFileCD::CFileCD()
{
    m_headSector = -1;
    m_toc = 0;
    m_pos = 0;
    m_buf = 0;
    m_sector = (char *)_memalign(0x40, 0x800, 0, 0);
}

CFileCD::~CFileCD()
{
    if (m_sector)
        _free(m_sector, 0, 0);
    m_sector = 0;
}

/* a file opened by its first sector, a TOC entry made for it (SOCOM II's; the DNAS client's) */
bool CFileCD::OpenSector(unsigned int sector)
{
    m_toc = &m_TOC[m_TOCNum++];
    m_toc->name = "";
    m_toc->startSector = sector;
    m_toc->endSector = 0;
    m_toc->sectorSize = 0;
    SetHeadSector(sector);
    return true;
}

/* whole sectors read straight to the caller, through the bounce buffer one at a time when the
 * destination is not 16-byte aligned (SOCOM 1's) */
int CFileCD::XferSectorsToCaller(char *buf, int sectors)
{
    int bytes = sectors << 11;

    snd_StreamSafeCdSync(0);
    if ((int)buf & 0xF) {
        int i;

        for (i = 0; i < sectors; i++) {
            snd_StreamSafeCdRead(m_headSector++, 1, alignedBuffer, &m_CDmode);
            snd_StreamSafeCdGetError();
            snd_StreamSafeCdSync(0);
            memcpy(buf, alignedBuffer, 0x800);
            buf += 0x800;
        }
    } else {
        snd_StreamSafeCdRead(m_headSector, sectors, buf, &m_CDmode);
        snd_StreamSafeCdGetError();
        m_headSector += sectors;
    }
    m_pos += bytes;
    snd_StreamSafeCdSync(0);
    return bytes;
}

/* size bytes read from the position: whole sectors straight to the caller, partial ones through
 * the sector buffer (SOCOM 1's) */
int CFileCD::Read(char *buf, int size)
{
    int n;
    int sectors = size >> 11;

    if (!(m_pos & 0x7FF)) {
        if (!(size & 0x7FF))
            n = XferSectorsToCaller(buf, sectors);
        else if (size >= 0x800) {
            int rest;

            n = XferSectorsToCaller(buf, sectors);
            ReadSector();
            rest = size - (sectors << 11);
            memcpy(buf + n, m_buf, rest);
            n += rest;
            m_buf += rest;
            m_pos += rest;
        } else {
            ReadSector();
            memcpy(buf, m_buf, size);
            n = size;
            m_buf += size;
            m_pos += size;
        }
    } else {
        if (!m_buf || m_buf == m_sector + 0x800)
            ReadSector();
        n = m_sector + 0x800 - m_buf;
        if (size <= n) {
            memcpy(buf, m_buf, size);
            n = size;
            m_buf += size;
            m_pos += size;
        } else {
            memcpy(buf, m_buf, n);
            size -= n;
            buf += n;
            m_buf += n;
            m_pos += n;
            if (size >= 0x800) {
                int x = XferSectorsToCaller(buf, size >> 11);

                buf += x;
                size -= x;
                n += x;
            }
            if (size) {
                ReadSector();
                memcpy(buf, m_buf, size);
                n += size;
                m_buf += size;
                m_pos += size;
            } else
                m_buf = 0;
        }
    }
    return n;
}

/* the IOP's reboot registration (util_main.cpp's CIOPManager: the callback's type and the call) */
class CIOPManager {
public:
    enum RebootPhase { PreReboot = 0, PostReboot = 1 };
    enum RebootMode { FirstBoot = 0 };
    typedef bool (*RebootCallback)(RebootPhase phase, RebootMode mode, void *data);

    int RegisterRebootCallback(RebootCallback cb, void *data);
};
extern CIOPManager theIOPManager;

int HddMountGame();
int HddUnmountGame();

/* whether the menu opened the game's partition (0x4A37B0), and the reboot callback's id (0x3E0640,
 * -1 until registered; both ours) */
extern unsigned char s_hddOpen;
extern int s_hddRebootId;

/* the game's partition across an IOP reboot (0x39D910, SOCOM II's; the name ours): while the menu
 * has it open, let go before the reboot and mounted again after; true. HddUnmountGame inlined */
bool HddRebootCallback(CIOPManager::RebootPhase phase, CIOPManager::RebootMode mode, void *data)
{
    if (s_hddOpen) {
        if (phase == CIOPManager::PreReboot)
            HddUnmountGame();
        else
            HddMountGame();
    }
    return true;
}

/* the reboot callback registered once (0x39D8C0, SOCOM II's: CMenuState::Init calls it; the name
 * ours) */
void HddRegisterReboot()
{
    if (s_hddRebootId == -1)
        s_hddRebootId = theIOPManager.RegisterRebootCallback(HddRebootCallback, 0);
}

/* a file read whole for the hard disk's icon (ours) */
struct HDDICONFILE {
    void *data;
    int size;
};

/* the hard disk icon's header (PS2ICON3D, then each file's offset and size: ours) */
struct HDDICONHEADER {
    char magic[9];
    char pad[3];
    int reserved;
    struct {
        int offset;
        int size;
    } files[4];
    char unused[0x220 - 0x30];
};

int LoadHddIconFile(const char *dir, const char *name, HDDICONFILE *out);

/* a file read whole into a 64-aligned buffer, rounded up to 512 bytes and zeroed past its end;
 * the buffer and its rounded size handed back, the file's size returned (SOCOM II's; the name
 * ours).
 * OPEN, 2 instructions short (0x260 of 0x268): the inlined Close's IsOpen tests the CD reader
 * through its address (`addiu v0,sp,372; lw v1,0(v0)`) and materialises `!= -1 && != 0` with two
 * branches and an assigned true (daddiu); CDFileOpen by value gives nor/sltu. Taking it by `const
 * int &` gives the target's size (8 instructions off) but costs IsOpen and Close their matches;
 * `if (...) return true; return false;` and a bool local cost them too. The rest matches.
 * Round 23, measured again: the helper by `const int &` or `const int *` gives LoadHddIconFile the
 * target's size and its jump form, 3 instructions off (the field read folded, `lw v1,372(sp)`,
 * where the target builds the address), and costs IsOpen 0x39EF80 and Close 0x39E8A0 7 each (both
 * keep the value form in the target); the reference read into a local first keeps those two and
 * changes nothing here. Everywhere else in the target the file's m_file and m_flags go through
 * saved registers (sp+372 into s0, sp+384 into s1, set up with the inlined constructor): only the
 * inlined IsOpen reads them afresh, so its copy here is not the out-of-line one's source.
 * Round 24: the CD test over the union's pointer (`CFileCD *const &`, m_cd) or through the object
 * (`CDFileOpen(const CFileIO *)` binding a reference to m_file) is 3 off the same way and reads
 * `lw v1,0(s0)` -- MW merges the reference's address with the constructor's hoisted &m_file where
 * the target builds it afresh (`addiu v0,sp,372`); a reference accessor over m_file passed to the
 * by-value test, or a by-value pointer, stays 8 off (the value form). */
int LoadHddIconFile(const char *dir, const char *name, HDDICONFILE *out)
{
    CFileIO file;
    char path[256];
    int alloc;
    int size = 0;

    strcpy(path, dir);
    strcat(path, name);
    if (file.Open(path, 1) != false) {
        size = file.GetSize();
        alloc = (size + 511) / 512 * 512;
        void *buf = _memalign(64, alloc, 0, 0);

        memset(buf, 0, alloc);
        file.fread(buf, size);
        file.Close();
        out->data = buf;
        out->size = alloc;
    }
    return size;
}

/* the installed game's icon written as one file: the header, system.cnf and the three icons
 * (SOCOM II's; the name ours) */
bool WriteHddIcon(const char *path)
{
    HDDICONFILE files[4];
    HDDICONHEADER header;
    int r;
    int i;

    strncpy(header.magic, "PS2ICON3D", 9);
    memset(header.pad, 0, 3);
    header.reserved = 0;
    header.files[0].size = LoadHddIconFile("", "system.cnf", &files[0]);
    header.files[1].size = LoadHddIconFile("RUN/ICONS/HDD/", "icon.sys", &files[1]);
    header.files[2].size = LoadHddIconFile("RUN/ICONS/HDD/", "list.ico", &files[2]);
    header.files[3].size = LoadHddIconFile("RUN/ICONS/HDD/", "deleting.ico", &files[3]);
    header.files[0].offset = 512;
    header.files[1].offset = header.files[0].offset + files[0].size;
    header.files[2].offset = header.files[1].offset + files[1].size;
    header.files[3].offset = header.files[2].offset + files[2].size;
    int fd = sceOpen(path, 3);

    if (fd >= 0) {
        r = sceWrite(fd, &header, 512);
        for (i = 0; i < 4 && r >= 0; i++)
            r = sceWrite(fd, files[i].data, files[i].size);
        r = sceClose(fd);
    }
    for (i = 0; i < 4; i++) {
        if (files[i].data)
            _free(files[i].data, 0, 0);
        files[i].data = 0;
    }
    return r >= 0;
}

/* the hard disk's state (zsys's: sceDevctl("hdd0:")'s answer, 0 when it is usable) */
extern int s_hddStatus;

extern "C" {
/* the IOP's directory calls (sifdev.h) */
struct sce_stat {
    unsigned int st_mode;
    unsigned int st_attr;
    unsigned int st_size;
    unsigned char st_ctime[8];
    unsigned char st_atime[8];
    unsigned char st_mtime[8];
    unsigned int st_hisize;
    unsigned int st_private[6];
};
struct sce_dirent {
    struct sce_stat d_stat;
    char d_name[256];
    void *d_private;
};
int sceDopen(const char *dirname);
int sceDread(int fd, struct sce_dirent *buf);
int sceDclose(int fd);
}

extern "C" {
/* the IOP's file system calls (sifdev.h) */
int sceMount(const char *fsname, const char *devname, int flag, const char *arg, int arglen);
int sceUmount(const char *name);
int sceFormat(const char *path, const char *blkdevname, void *arg, int arglen);
int sceMkdir(const char *name, int mode);
int sceDevctl(const char *devname, int cmd, const void *arg, unsigned int arglen, void *buf,
              unsigned int buflen);
}

/* the hard disk's modules loaded and started, a mount point let go (zsys_init.cpp's) */
bool zSysHddInit();
bool zSysHddUnmount(const char *name);

/* the game's partition passwords, each byte inverted in the low byte of a word (ours: "b00mr972") */
static const int s_hddPassword[8] = {
    0x70928A9D, (int)0x935A89CF, 0x6C9E76CF, 0x656B9E92, 0x6C37C78D, (int)0xCBCA36C6, (int)0xCF31CCC8,
    0x3DD42DCD
};

/* the scratchpad's strings: the partition's path, a size and the create string, a password */
#define HDD_PATH ((char *)0x70000000)
#define HDD_SIZE ((char *)0x70000100)
#define HDD_MAKE ((char *)0x70000200)
#define HDD_PASS ((char *)0x70000400)

/* ",password" written to the scratchpad (ours) */
static inline char *HddPassword()
{
    char *pw = HDD_PASS;

    char *p = pw;

    *p++ = ',';
    for (int i = 0; i < 8; i++)
        *p++ = ~(char)s_hddPassword[i];
    *p = 0;
    return pw;
}

/* the game's partition mounted at pfs0: -- made the first time (256 MB, formatted, its RUN directory
 * and the game's icon written) -- and the disk told; 0 when mounted, 2 when it could not be, 1 with
 * no usable disk (0x39D050, SOCOM II's: UIHardDriveOperation and 0x39D910 call it; the name ours).
 * Byte-exact on the UI lane's round 31 (written then): the password built three times by one inline,
 * through a pointer stepped with `*p++` (MW unrolls it in pairs, the target's v0 += 2) -- indexed,
 * the stores fold into the scratchpad's absolute addresses (0x20 longer) -- each word narrowed with
 * `(char)` (the target's lb; plain, a whole lw); the seven later words' addresses are CSE'd across the
 * three copies into s0-s6; the success arm sets the result first (its `move s0,zero` in sceDevctl's
 * delay slot), then the flag, then the byte -- the result last, it lands in the `b`'s slot */
int HddMountGame()
{
    int result = 1;

    zSysHddInit();
    if (!s_hddStatus) {
        strcpy(HDD_PATH, "hdd0:");
        strcat(HDD_PATH, "PP.SCUS-97275..SOCOM_II");
        strcat(HDD_PATH, HddPassword());
        int err = sceMount("pfs0:", HDD_PATH, 1, 0, 0);

        if (err) {
            strcpy(HDD_MAKE, "hdd0:");
            strcat(HDD_MAKE, "PP.SCUS-97275..SOCOM_II");
            strcat(HDD_MAKE, HddPassword());
            strcat(HDD_MAKE, HddPassword());
            sprintf(HDD_SIZE, ",%dM", 256);
            strcat(HDD_MAKE, HDD_SIZE);
            strcat(HDD_MAKE, ",PFS");
            int fd = sceOpen(HDD_MAKE, 512);

            if (fd >= 0) {
                sceClose(fd);
                unsigned int args[3] = {0x2000, 0x2D66, 0xF0F0F0F0};
                int r = sceFormat("pfs:", HDD_PATH, args, 4);

                if (!r) {
                    sceMount("pfs0:", HDD_PATH, 0, 0, 0);
                    sceMkdir("pfs0:/RUN", 0777);
                    r = sceUmount("pfs0:");
                }
                if (!r && WriteHddIcon(HDD_PATH))
                    err = sceMount("pfs0:", HDD_PATH, 1, 0, 0);
            }
        }
        if (!err) {
            result = 0;
            s_hddMounted = 1;
            char c = 60;

            sceDevctl("hdd0:", 0x4803, &c, 1, 0, 0);
        } else
            result = 2;
    }
    return result;
}

/* the installed game's partition let go: HddMountGame's pfs0: unmounted, 0 (0x39D020, SOCOM II's:
 * UIHardDriveOperation and CIOPManager::RebootIOP call it; the name ours, after HddMountGame and the
 * "pfs0:" it passes) */
int HddUnmountGame()
{
    zSysHddUnmount("pfs0:");
    return 0;
}

/* the files of the installed game's RUN directory whose names hold the pattern, counted; the one
 * the index names has the given bytes of its header (from offset 20) copied out (SOCOM II's, for
 * UIHardDriveOperation; the name ours). The file is closed with sceDclose, as the target does */
int HddFindFiles(const char *pattern, void *out, int size, unsigned int index)
{
    char path[512];
    sce_dirent ent;
    char header[252];
    bool found = false;
    const char *root = "pfs0:RUN";
    unsigned int n = 0;

    if (!s_hddStatus) {
        int dir = sceDopen(root);

        if (dir >= 0) {
            while ((bool)sceDread(dir, &ent) && !(index && n > index)) {
                if (pattern && !strstr(ent.d_name, pattern))
                    continue;
                if (out && n == index) {
                    sprintf(path, "%s/%s", root, ent.d_name);
                    found = true;
                }
                n++;
            }
            sceDclose(dir);
        }
        if (found) {
            int fd;
            int read = 0;

            fd = sceOpen(path, 1);

            if (fd >= 0) {
                read = sceRead(fd, header, sizeof(header));
                memcpy(out, header + 20, size);
                sceDclose(fd);
            }
            if (fd < 0 || read < 0)
                data_004A1770(2);
        }
    }
    return n;
}

/* the handler told of a failed read, write or seek (ours) */
void CFileIO::SetErrorHandler(void (*handler)(int error))
{
    data_004A1770 = handler;
}

int CFileIO::fwrite(const char *str)
{
    return CIO::fwrite(str);
}
