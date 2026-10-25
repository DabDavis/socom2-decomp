/* zutil.h -- the engine's file classes: CIO (the abstract file), CFileIO (a host, CD or HDD
 * file), CBufferIO (a CFileIO read whole into memory), CMemCardIO (a file on the memory card) and
 * the string table CSTable. SOCOM 1 kept them in gamez\zutil (its DWARF names zutil.h; the press
 * beta's asserts name util_bufio.cpp, util_sonyio.cpp and util_stable.cpp). Layouts are SOCOM 1's
 * where its DWARF has the class, moved to SOCOM II's offsets as the code reads them.
 *
 * SOCOM II's CIO grew three virtuals between fwrite(const void *, int) and fwrite(const char *):
 * the memory card's asynchronous read and write and the wait for them (CMemCardIO overrides all
 * three; CIO's defaults, 0x2CBAB0/C0/D0, return 0, 0 and true). Their names are ours. */
#ifndef ZUTIL_H
#define ZUTIL_H

#include <string.h>

/* the allocators, with a file and line SOCOM II passes as 0 (zsys_memory.cpp; MAIN's _free) */
void *_malloc(unsigned int size, const char *file, int line);
void *_realloc(void *p, unsigned int size, const char *file, int line);
void _free(void *p, const char *file, int line);
char *_strdup(const char *s, const char *file, int line);

/* the engine's file interface (SOCOM 1's CIO: no data, its vtable all pure but fwrite(const
 * char *); slots 0x30-0x38 SOCOM II's) */
class CIO {
public:
    virtual bool Open(const char *name, unsigned int mode) = 0;   // 0x08
    virtual void Close() = 0;                                     // 0x0C
    virtual bool IsOpen() const = 0;                              // 0x10
    virtual int GetSize() const = 0;                              // 0x14
    virtual unsigned int GetMode() const = 0;                     // 0x18
    virtual int ftell() const = 0;                                // 0x1C
    virtual int fseek(int offset, int whence) = 0;                // 0x20
    virtual int fread(void *buf, int size) = 0;                   // 0x24
    virtual int fread(int size, void **buf) = 0;                  // 0x28
    virtual int fwrite(const void *buf, int size) = 0;            // 0x2C
    /* 0x30-0x38: SOCOM II's; the names ours */
    virtual int freadAsync(void *buf, int size) { return 0; }           // 0x30
    virtual int fwriteAsync(const void *buf, int size) { return 0; }    // 0x34
    virtual bool fsyncAsync(int mode, int *result) { return true; }     // 0x38
    /* a string: its length, then its characters (SOCOM 1's; out of line at 0x2CBA10) */
    virtual int fwrite(const char *str)                           // 0x3C
    {
        int len = str ? strlen(str) : 0;
        int n = fwrite(&len, 4);

        if (str)
            n += fwrite(str, len);
        return n;
    }
    virtual void fflush() = 0;                                    // 0x40
};

class CFileCD;

/* a file on the host, the CD (through CFileCD) or the hard disk (SOCOM 1's DWARF: 0x10 bytes,
 * m_file and m_cd sharing 4; SOCOM II added the flags at 0x10) */
class CFileIO : public CIO {
public:
    CFileIO();
    bool Open(const char *name, unsigned int mode);
    void Close();
    bool IsOpen() const;
    int GetSize() const { return m_filesize; }
    unsigned int GetMode() const { return m_mode; }
    int ftell() const;
    int fseek(int offset, int whence);
    int fread(void *buf, int size);
    int fread(int size, void **buf);
    int fwrite(const void *buf, int size);
    int fwrite(const char *str);
    void fflush();
    void Release();

    static bool PS2FileName(const char *name, char *out, bool cd);
    static void SetRootPath(const char *path);

    union {
        int m_file;          // 0x4
        CFileCD *m_cd;       // 0x4
    };
    int m_filesize;          // 0x8
    unsigned int m_mode;     // 0xc
    unsigned int m_flags;    // 0x10: SOCOM II's -- 1 the CD, 2 report errors (the name ours)
};   // 0x14 bytes

/* a file read whole into a buffer, or a buffer read as a file (SOCOM 1's CBufferIO, no DWARF;
 * the members' names ours) */
class CBufferIO : public CFileIO {
public:
    CBufferIO();
    bool Open(const char *name, unsigned int mode);
    void Close();
    bool IsOpen() const;
    int GetSize() const { return m_buffer ? m_size : CFileIO::GetSize(); }
    int ftell() const;
    int fseek(int offset, int whence);
    int fread(void *buf, int size);
    int fread(int size, void **buf);
    int fwrite(const void *buf, int size);
    int fwrite(const char *str);
    void fflush();
    virtual bool Open(void *buf, unsigned int size);   // 0x44
    virtual int freadchar(char *c);                     // 0x48
    bool LoadBuffer();
    void Release();

    /* the position is inside the buffer (ours: fseek materialises the test) */
    bool ValidPos(int pos) const { return m_size >= pos && pos >= 0; }

    char *m_buffer;     // 0x14
    int m_reserve;      // 0x18
    int m_size;         // 0x1c
    char *m_ptr;        // 0x20
};   // 0x24 bytes

#endif
