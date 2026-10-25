/* util_sonyio.cpp -- CFileIO (OVL1 0x39CDC0-0x39F150): a file on the host, the CD (through
 * CFileCD's sector reader) or the hard disk, through Sony's sceOpen/sceRead and libcdvd. The file
 * name is SOCOM 1's: the press beta's asserts name util_sonyio.cpp, and SOCOM 1's demo holds
 * CFileIO's and CFileCD's methods in this order between COutput's unit and CSTable's (CBufferIO's
 * IsOpen among them in both builds). SOCOM II added the hard disk, the error handler, the flags
 * word at 0x10 and reads through an open package (0x4A15F8/FC; the package's own unit is not
 * written yet).
 *
 * OPEN, not written yet: Open (0x39E9B0, 0x4A0), CFileCD::Read (0x39D960), BuildTOC (0x39DFD0),
 * Find (0x39DEF0), XferSectorsToCaller, CreatePath, PS2FileName and the hard disk's mount and
 * reboot handlers (0x39CE80-0x39D910). */

#include "zutil.h"

extern "C" {
int sceRead(int fd, void *buf, int nbyte);
int sceWrite(int fd, const void *buf, int nbyte);
int sceLseek(int fd, int offset, int whence);
int sceClose(int fd);
char *strstr(const char *s, const char *sub);
}

/* a CD file being read: its TOC entry, the position, the sector in the buffer (SOCOM 1's DWARF:
 * 0x14 bytes; SOCOM II's constructor allocates the sector buffer) */
struct _FILECD {
    int _0;
    int lsn;    // 0x4
    int _8;
    int size;   // 0xc
};

class CFileCD {
public:
    CFileCD()
    {
        m_headSector = -1;
        m_toc = 0;
        m_pos = 0;
        m_buf = 0;
        m_sector = (char *)_memalign(0x40, 0x800, 0, 0);
    }
    ~CFileCD()
    {
        if (m_sector)
            _free(m_sector, 0, 0);
        m_sector = 0;
    }
    int Read(char *buf, int size);

    _FILECD *m_toc;         // 0x0
    unsigned int m_pos;     // 0x4
    int m_headSector;       // 0x8
    char *m_sector;         // 0xc
    char *m_buf;            // 0x10
};

/* an open package's entry: reads are offset by its start (SOCOM II's; the package unit's) */
struct PAKENTRY {
    unsigned char _0[0x44];
    int offset;   // 0x44
};

/* the package state (SOCOM II's; the package unit's statics) */
extern int data_004A15F8;
extern PAKENTRY *data_004A15FC;
/* the root path, the file suffix (";1" on the CD) and the root's directory */
extern char data_003E0520[];
extern char data_004A1750[];
extern char data_004A1650[];
/* the error handler (ours) */
extern void (*data_004A1770)(int error);

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
            cd->m_pos = offset + cd->m_toc->size;
            break;
        }
        sector = cd->m_toc->lsn + ((cd->m_pos + base) >> 11);
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

/* the handler told of a failed read, write or seek (ours) */
void CFileIO::SetErrorHandler(void (*handler)(int error))
{
    data_004A1770 = handler;
}

int CFileIO::fwrite(const char *str)
{
    return CIO::fwrite(str);
}
