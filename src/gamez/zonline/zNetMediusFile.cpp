/* zNetMediusFile.cpp -- the Medius file API: CLobbyFileIO, a CIO over Medius's file transfers
 * (SOCOM II's, by its RTTI and vtable 0x406760; the unit and its name are ours). OVL1
 * 0x304610-0x30661F: after the LAN lobby's unit, its static initialiser 0x402570 building the map of
 * transfers under way, std::map<std::string, CLobbyFileIO *> at 0x450070 (Open, Close and freadAsync
 * insert the file under the key Medius hands back, the transfers' callbacks find and erase it); the
 * map's template tail closes it (0x305710-0x3065A0, byte-exact as the MSL lane's probe). The
 * vtable's slots follow CIO's (zutil.h): Open 0x304C50, Close 0x304A60, IsOpen 0x304A50, GetSize
 * 0x304A30, GetMode 0x304610, ftell 0x304A20, fseek 0x304A10, the two freads 0x304810/0x304800,
 * fwrite 0x3047F0, freadAsync 0x304660, fwriteAsync 0x304650, fsyncAsync 0x304620, CIO's string
 * fwrite, fflush 0x3047E0. Still unwritten: Open, Close, freadAsync and the transfers' callbacks
 * (0x304820, 0x304B90, 0x304DE0, 0x304F40: each finds its file by the response's key), the
 * initialiser and the map's tail. Defined in reverse of their image order (-inline deferred). */

#include <string.h>
#include <string>
#include <map>
#include "../zutil/zutil.h"

/* a file transferred through Medius (0x9C bytes, as the patch downloader's member declares it in
 * fts_patch.cpp: the transfer's descriptor at 4, the mode at 8, the path from 0xC) */
class CLobbyFileIO : public CIO {
public:
    bool Open(const char *name, unsigned int mode);
    void Close();
    bool IsOpen() const;
    int GetSize() const;
    unsigned int GetMode() const;
    int ftell() const;
    int fseek(int offset, int whence);
    int fread(void *buf, int size);
    int fread(int size, void **buf);
    int fwrite(const void *buf, int size);
    int freadAsync(void *buf, int size);
    int fwriteAsync(const void *buf, int size);
    bool fsyncAsync(int mode, int *result);
    void fflush();

    unsigned char *m_handle;   // 0x4: Medius's descriptor (its size at 0x94)
    unsigned int m_mode;   // 0x8: 2 and 4 the writes
    char m_path[0x80];   // 0xc
    unsigned char m_open;   // 0x8c
    int m_result;   // 0x90: the last transfer's result
    const void *m_buf;   // 0x94: the asynchronous transfer's buffer
    int m_size;   // 0x98: and its size
};   // 0x9c

/* the transfers under way, by Medius's key (0x450070, the unit's) */
extern "C" std::map<std::string, CLobbyFileIO *> data_00450070;

/* the transfers done: the last one's result handed back, true once none is under way (OVL1
 * 0x304620) */
bool CLobbyFileIO::fsyncAsync(int mode, int *result)
{
    bool done = data_00450070.empty();

    if (result)
        *result = m_result;
    return done;
}

/* an asynchronous write kept for the transfer (OVL1 0x304650) */
int CLobbyFileIO::fwriteAsync(const void *buf, int size)
{
    m_buf = buf;
    m_size = size;
    return -1;
}

/* nothing to flush (OVL1 0x3047E0) */
void CLobbyFileIO::fflush()
{
}

/* no synchronous writes (OVL1 0x3047F0) */
int CLobbyFileIO::fwrite(const void *buf, int size)
{
    return -1;
}

/* no synchronous reads (OVL1 0x304800) */
int CLobbyFileIO::fread(int size, void **buf)
{
    return -1;
}

/* (OVL1 0x304810) */
int CLobbyFileIO::fread(void *buf, int size)
{
    return -1;
}

/* no seeking (OVL1 0x304A10) */
int CLobbyFileIO::fseek(int offset, int whence)
{
    return -1;
}

/* (OVL1 0x304A20) */
int CLobbyFileIO::ftell() const
{
    return -1;
}

/* the transfer's size, -1 when none is open (OVL1 0x304A30) */
int CLobbyFileIO::GetSize() const
{
    int size = -1;

    if (m_handle)
        size = *(int *)(m_handle + 0x94);
    return size;
}

/* a transfer open (OVL1 0x304A50) */
bool CLobbyFileIO::IsOpen() const
{
    return m_handle != 0;
}

/* the mode it was opened with (OVL1 0x304610) */
unsigned int CLobbyFileIO::GetMode() const
{
    return m_mode;
}
