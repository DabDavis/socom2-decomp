/* util_sonyio.cpp -- CFileIO (OVL1 0x39CDC0-0x39F150): a file on the host, the CD (through
 * CFileCD's sector reader) or the hard disk, through Sony's sceOpen/sceRead and libcdvd. The file
 * name is SOCOM 1's: the press beta's asserts name util_sonyio.cpp, and SOCOM 1's demo holds
 * CFileIO's and CFileCD's methods in this order between COutput's unit and CSTable's (CBufferIO's
 * IsOpen among them in both builds). SOCOM II added the hard disk, the error handler, the flags
 * word at 0x10, reads through an open package (0x4A15F8/FC: the package unit's, 0x39C330-0x39C4FB)
 * and a CD file opened by its sector (the DNAS client's).
 *
 * OPEN, not written yet: BuildTOC (0x39DFD0: the ISO 9660 walk, 0x520 -- m2c's draft is a start), the
 * hard disk's mount and
 * reboot handlers (0x39CE80-0x39D910), 0x39DDB0 and 0x39DFC0. */

#include "zutil.h"

extern "C" {
int sceOpen(const char *name, int flags);
int sceRead(int fd, void *buf, int nbyte);
int sceWrite(int fd, const void *buf, int nbyte);
int sceLseek(int fd, int offset, int whence);
int sceClose(int fd);
char *strstr(const char *s, const char *sub);
/* MSL's strtok (MAIN, not named in the map yet) */
char *func_0019AA00(char *s, const char *delim);
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

/* the libcdvd read mode the reads pass and the bounce buffer for a destination not 16-byte
 * aligned (SOCOM 1's file statics, 0x4A1778 and 0x4A17B0) */
extern int m_CDmode;
extern char alignedBuffer[0x2000];

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

/* the package state (SOCOM II's; the package unit's statics) */
extern int data_004A15F8;
extern PAKENTRY *data_004A15FC;
/* the package's entry by name, its file seeked and the file itself (the package unit's, not named
 * yet) */
extern "C" PAKENTRY *func_0039C330(void *pak, const char *name);
extern "C" int func_0039C3D0(void *pak, int offset, int whence);
extern "C" CFileIO *func_0039C3E0(void *pak);
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
    for (tok = func_0019AA00(path, delim); tok;) {
        strcat(out, tok);
        tok = func_0019AA00(0, delim);
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
    if (data_004A15F8) {
        PAKENTRY *e = func_0039C330(&data_004A15F8, upath);

        if (e) {
            CFileIO *pak;

            func_0039C3D0(&data_004A15F8, e->offset, 0);
            pak = func_0039C3E0(&data_004A15F8);
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
    if (data_004A15F8 && (m_flags & 0x100)) {
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

/* where the open package's file starts, 0 with none (ours) */
static inline int PakOffset()
{
    return data_004A15FC ? data_004A15FC->offset : 0;
}

/* OPEN, 2 instructions short (0x170 of 0x178): the target reads the package's offset as a full
 * if/else -- `beqz; move t0,zero; b; lw t0,0x44; move t0,zero` -- and keeps it in t0 with the
 * reader in a3; every spelling tried (the ternary initialising or assigned, an inline helper
 * returning it, an if/return, a zeroed local, the reader declared first) if-converts it into
 * `beqz; move; lw` with the two registers swapped. The rest (the CD switch, the sector sum's
 * operand order, the host's 2-1-default chain) matches. */
int CFileIO::fseek(int offset, int whence)
{
    int pos;

    if (m_flags & 1) {
        int base = PakOffset();
        CFileCD *cd;
        int sector;

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
        sector = cd->m_toc->startSector + ((cd->m_pos + base) >> 11);
        if (cd->m_headSector != sector) {
            cd->m_headSector = sector;
            cd->m_buf = 0;
        }
        pos = cd->m_pos;
    } else {
        int mode;

        if (whence == 2) {
            mode = 0;
            offset += m_filesize;
            if (data_004A15FC)
                offset += data_004A15FC->offset;
        } else if (whence == 1) {
            mode = 1;
        } else {
            mode = 0;
            if (data_004A15FC)
                offset += data_004A15FC->offset;
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

/* the handler told of a failed read, write or seek (ours) */
void CFileIO::SetErrorHandler(void (*handler)(int error))
{
    data_004A1770 = handler;
}

int CFileIO::fwrite(const char *str)
{
    return CIO::fwrite(str);
}
