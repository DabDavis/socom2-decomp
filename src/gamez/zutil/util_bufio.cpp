/* util_bufio.cpp -- CBufferIO (OVL1 0x39BBC0-0x39C324): a file read whole into memory on Open, or
 * a caller's buffer read and written as a file; falls back on CFileIO while it holds no buffer.
 * The file name is SOCOM 1's (the press beta's asserts name util_bufio.cpp, and SOCOM 1's demo
 * holds CBufferIO's methods in this order, ending at its constructor). SOCOM II made writing grow
 * the buffer, dropped the copy constructor and checks IsOpen before a buffer Open; IsOpen itself
 * is util_sonyio.cpp's in both builds. */

#include "zutil.h"

CBufferIO::CBufferIO()
{
    m_ptr = 0;
    m_buffer = 0;
    m_reserve = 0;
    m_size = 0;
}

bool CBufferIO::Open(void *buf, unsigned int size)
{
    if (IsOpen())
        return true;
    m_size = size;
    m_reserve = size;
    m_buffer = (char *)buf;
    m_ptr = m_buffer;
    return true;
}

void CBufferIO::Release()
{
    m_reserve = 0;
    m_size = 0;
    m_ptr = 0;
    m_buffer = 0;
    CFileIO::Release();
}

bool CBufferIO::Open(const char *name, unsigned int mode)
{
    bool ok;

    if (IsOpen())
        return true;
    ok = CFileIO::Open(name, mode);
    if (CFileIO::IsOpen() && (mode & 1) && !(mode & 0x20))
        ok = LoadBuffer();
    if (!ok)
        Close();
    return ok;
}

bool CBufferIO::LoadBuffer()
{
    bool ok = CFileIO::IsOpen();

    if (ok) {
        int size = GetSize();

        ok = size > 0;
        if (ok) {
            if (m_reserve < size) {
                m_buffer = (char *)_realloc(m_buffer, size, 0, 0);
                m_reserve = size;
            }
            m_size = size;
            if (m_buffer) {
                if (m_size == CFileIO::fread(m_buffer, m_size)) {
                    m_ptr = m_buffer;
                    ok = m_buffer != 0;
                } else {
                    ok = false;
                    _free(m_buffer, 0, 0);
                    m_buffer = 0;
                    m_reserve = 0;
                    m_size = 0;
                }
            }
        }
    }
    return ok;
}

void CBufferIO::Close()
{
    if (m_buffer)
        _free(m_buffer, 0, 0);
    m_buffer = 0;
    m_reserve = 0;
    m_ptr = 0;
    m_size = 0;
    CFileIO::Close();
}

int CBufferIO::ftell() const
{
    if (m_buffer)
        return m_ptr - m_buffer;
    return CFileIO::ftell();
}

int CBufferIO::fseek(int offset, int whence)
{
    int pos;

    if (!m_buffer)
        return CFileIO::fseek(offset, whence);
    pos = 0;
    switch (whence) {
    case 0:
        pos = offset;
        break;
    case 1:
        pos = offset + ftell();
        break;
    case 2:
        pos = m_size + offset;
        break;
    }
    if (ValidPos(pos)) {
        m_ptr = m_buffer + pos;
        return pos;
    }
    return -1;
}

int CBufferIO::fread(void *buf, int size)
{
    int pos, end;

    if (!m_buffer)
        return CFileIO::fread(buf, size);
    pos = ftell();
    end = pos + size;
    if (pos >= m_size)
        return 0;
    if (end >= m_size)
        size = m_size - pos;
    memcpy(buf, m_ptr, size);
    m_ptr += size;
    return size;
}

int CBufferIO::fread(int size, void **buf)
{
    int pos = ftell();
    int end = pos + size;

    if (pos >= m_size)
        return 0;
    if (end >= m_size)
        size = m_size - pos;
    *buf = m_ptr;
    m_ptr += size;
    return size;
}

/* (SOCOM II's, as SOCOM 1's: without a buffer it reads into the pointer itself) */
int CBufferIO::freadchar(char *c)
{
    if (!m_buffer)
        return CFileIO::fread(&c, 1);
    if (m_ptr + 1 >= m_buffer + m_size)
        return 0;
    *c = *m_ptr++;
    return 1;
}

void CBufferIO::fflush()
{
    if (!m_buffer)
        CFileIO::fflush();
}

int CBufferIO::fwrite(const char *str)
{
    if (!m_buffer)
        return CIO::fwrite(str);
    return -1;
}

int CBufferIO::fwrite(const void *buf, int size)
{
    if (!m_buffer)
        return CFileIO::fwrite(buf, size);
    m_size = m_ptr - m_buffer;
    if (m_reserve < m_size + size) {
        m_reserve = m_size + size;
        m_buffer = (char *)_realloc(m_buffer, m_reserve, 0, 0);
        m_ptr = m_buffer + m_size;
    }
    memcpy(m_ptr, buf, size);
    m_ptr += size;
    m_size = m_ptr - m_buffer;
    return 1;
}
