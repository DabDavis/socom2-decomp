/* util_stable.cpp -- CSTable, the engine's string table (OVL1 0x39F0C0-0x39FEAF), and SOCOM II's
 * UTF-8 helpers around it. The file name is SOCOM 1's: the press beta's asserts name
 * util_stable.cpp, and SOCOM 1's demo holds CSTable's methods as one run between CFileIO's unit
 * and CMemCardIO's (CreateString, FindString, Clear, Destroy, ReleaseBuffer, Pack, Relocate,
 * LoadTable, ~CSTable, CSTable). SOCOM II made Clear, Destroy and FindString virtual, added the
 * two hooks, takes the new buffer's size and owner in Relocate and inlines ReleaseBuffer.
 *
 * The UTF-8 helpers are SOCOM II's (the C2D strings, the teletype and the soft keyboard call
 * them); where SOCOM 1's demo has CBeachBall::Uninit between the two units, SOCOM II has the
 * character decoder and its length, and after the constructor the bounded copy and the append.
 * Their names are ours. */

#include "util_stable.h"
#include <algorithm>
#include <string>
#include "../zsys/zsys.h"

/* __vector_pod<unsigned int>::resize(n, x) (0x28B0E0, not named yet: include/vector has no
 * resize; LoadTable passes the constant zero's address, MW's anonymous static) */
extern "C" void func_0028B0E0(void *v, unsigned int n, const unsigned int &x);

/* a value shifted down by six, unsigned (ours: the append's two six-bit shifts) */
static inline int UTF8Shr6(unsigned int x) { return x >> 6; }

/* a UTF-16 character appended to a string as one to three UTF-8 bytes (ours). Byte-exact since the
 * UI lane's round 32 (open since round 21): the six-bit shifts through an inline taking `c` as an
 * unsigned int -- srl, the middle byte's argument a copy of the int (`move v0,s0`: c's zero
 * extension shared with u, while c itself stays live in s2), the remainders signed on u; the shifts
 * written in place gave sra, the inline on u coalesced the copy and freed s2 */
void UTF8Append(std::string &s, unsigned short c)
{
    int u = c;

    if (u < 0x80)
        s.push_back((unsigned char)c);
    else if (u < 0x800) {
        s.push_back((unsigned char)(UTF8Shr6(c) + 0xC0));
        s.push_back((unsigned char)(u % 64 + 0x80));
    } else {
        s.push_back((unsigned char)((u >> 12) + 0xE0));
        s.push_back((unsigned char)(UTF8Shr6(c) % 64 + 0x80));
        s.push_back((unsigned char)(u % 64 + 0x80));
    }
}

/* the length of the UTF-8 character a lead byte starts (ours; with a signed char every byte is
 * under 0xC0, so the copy below always takes one byte) */
static inline int UTF8LeadLen(char c)
{
    if (c < 0xC0)
        return 1;
    if (c < 0xE0)
        return 2;
    return 3;
}

/* at most n - 1 bytes of src copied and terminated; a copy cut short is cut back to the start of
 * the last whole character (ours) */
void UTF8Strncpy(char *dst, const char *src, int n)
{
    int i, j;

    if (!dst || n <= 0)
        return;
    if (!src) {
        *dst = 0;
        return;
    }
    for (i = 0; (bool)src[i] && i < n - 1; i++)
        dst[i] = src[i];
    if (src[i] == 0) {
        dst[i] = 0;
        return;
    }
    for (j = i - 1; j >= 0; j--) {
        char c = dst[j];

        if ((c & 0xC0) != 0x80) {
            int end = j + UTF8LeadLen(c);

            dst[end < n ? end : j] = 0;
            return;
        }
    }
    *dst = 0;
}

CSTable::CSTable(unsigned int bytes, unsigned int strings)
{
    m_owner = true;
    m_buffer = 0;
    m_bytes = 0;
    m_reserve = 0;
    if (bytes) {
        m_reserve = bytes;
        m_buffer = (char *)_malloc(bytes, 0, 0);
        reserve(strings);
    }
}

CSTable::~CSTable()
{
    Destroy();
}

/* a packed buffer taken as the table: its strings counted, then each one's start recorded */
void CSTable::LoadTable(void *buf, unsigned int bytes, bool owner)
{
    m_buffer = (char *)buf;
    m_bytes = bytes;
    m_owner = owner;
    {
        char *p = m_buffer;
        unsigned int total = 0;
        unsigned int n = 0;

        while (total < bytes) {
            unsigned int len;

            n++;
            len = strlen(p) + 1;
            p += len;
            total += len;
        }
        func_0028B0E0(this, n, 0);
    }
    {
        unsigned int i;
        char *p = m_buffer;
        unsigned int total = 0;

        i = 0;

        while (total < bytes) {
            unsigned int len;

            (*this)[i++] = p;
            len = strlen(p) + 1;
            p += len;
            total += len;
        }
    }
}

/* the buffer copied to buf, which the table then holds (the strings' pointers are not moved: the
 * loop is empty in both builds); the distance moved, or 0 without a buffer */
int CSTable::Relocate(void *buf, unsigned int reserve, bool owner)
{
    int delta;
    iterator i;

    if (size() == 0 || !IsPacked())
        return 0;
    delta = (char *)buf - m_buffer;
    memcpy(buf, m_buffer, m_bytes);
    for (i = begin(); i != end(); i++)
        ;
    ReleaseBuffer(m_owner);
    m_owner = owner;
    m_buffer = (char *)buf;
    m_reserve = reserve;
    return delta;
}

/* the strings copied into one buffer of their own (fn told, before the old strings are freed),
 * which the table then owns; its size */
unsigned int CSTable::Pack(void (*fn)(CSTable *, void *), void *data)
{
    unsigned int total;
    iterator i;
    char *buf, *p;
    char **old;
    unsigned int n;

    if (size() == 0)
        return 0;
    if (m_buffer)
        return m_bytes;
    total = 0;
    for (i = begin(); i != end(); ++i)
        total += strlen(*i) + 1;
    p = buf = (char *)_malloc(total, 0, 0);
    old = (char **)_malloc(size() * sizeof(char *), 0, 0);
    n = 0;
    for (i = begin(); i != end(); ++i) {
        char *s = *i;
        int len = strlen(s);

        memcpy(p, s, len + 1);
        old[n] = s;
        *i = p;
        p += len + 1;
        n++;
    }
    if (fn)
        fn(this, data);
    while (n--)
        _free(old[n], 0, 0);
    _free(old, 0, 0);
    if (m_buffer)
        _free(m_buffer, 0, 0);
    m_buffer = 0;
    m_buffer = buf;
    m_reserve = total;
    m_bytes = total;
    return m_bytes;
}

/* a string freed, its slot cleared (ours) */
struct FreeString {
    FreeString() {}
    char *operator()(char *s) const
    {
        _free(s, 0, 0);
        return 0;
    }
};

/* every string and the buffer let go of, the storage given up */
void CSTable::Destroy()
{
    if (IsPacked()) {
        if (m_owner)
            _free(m_buffer, 0, 0);
        m_buffer = 0;
        m_bytes = 0;
        m_reserve = 0;
    } else
        std::transform(begin(), end(), begin(), FreeString());
    m_reserve = 0;
    clear();
    FreeVector<std::vector<char *> >(*this);
}

/* every string forgotten: the buffer zeroed, or each string freed */
void CSTable::Clear()
{
    if (IsPacked()) {
        memset(m_buffer, 0, m_reserve);
        m_bytes = 0;
    } else
        std::transform(begin(), end(), begin(), FreeString());
    clear();
}

char *CSTable::FindString(const char *str)
{
    iterator i;

    if (str) {
        for (i = begin(); i != end(); ++i) {
            char *s = *i;

            if (s && strlen(str) == strlen(s) && !strcmp(s, str))
                return s;
        }
    }
    return 0;
}

/* the table's copy of a string: the one it holds, or a new one -- copied into the buffer while
 * it has room, else duplicated */
char *CSTable::CreateString(const char *str)
{
    char *s;

    if (!str)
        return 0;
    s = FindString(str);
    if (!s) {
        if (m_buffer) {
            if (m_owner) {
                unsigned int len = strlen(str) + 1;

                if (m_bytes + len <= m_reserve) {
                    char *p = m_buffer + m_bytes;

                    memcpy(p, str, len);
                    m_bytes += len;
                    if (size() == capacity()) {
                        s = OnTableFull(str);
                        if (!s)
                            return s;
                    }
                    push_back(p);
                    OnCreateString(p);
                    return p;
                } else
                    return OnTableFull(str);
            }
        } else {
            char *p = _strdup(str, 0, 0);

            push_back(p);
            OnCreateString(p);
            s = p;
        }
    }
    return s;
}

char *CSTable::OnTableFull(const char *str)
{
    return 0;
}

/* the length in bytes of the UTF-8 character at s (ours) */
int UTF8CharLen(const char *&s)
{
    if ((unsigned char)s[0] < 0xC0)
        return 1;
    if ((unsigned char)s[0] < 0xE0)
        return 2;
    return s[1] ? 3 : 1;
}

/* the UTF-8 character at s as UTF-16 (ours). Byte-exact since the UI lane's round 32 (open since
 * round 21): the three-byte form an accumulator -- the high part first, the third byte read into a
 * local assigned twice (so its load is numbered after the high part's shift and before the middle
 * part's: `igreplay --movers` gave 52 -> 58.5..61.5, the pointer then free for it in a0), the
 * middle part added, then the low part with `+=` (the sum first in the add; `ch = lo + ch`, `ch =
 * ... + lo` in one statement and a low part assigned once put it second). Round 30's shape (the
 * third byte first, one expression after) was 5 off; the single expression 2 instructions short */
unsigned short UTF8GetChar(const char *&s)
{
    const unsigned char *p = (const unsigned char *)s;
    unsigned short c = p[0];
    unsigned char c1 = p[1];

    if (c < 0xC0)
        return c;
    if (c < 0xE0)
        return c1 + ((c - 0xC0) << 6) - 0x80;
    if (c1) {
        unsigned short ch = (unsigned short)((c - 0xE0) << 12);
        unsigned short lo = p[2];
        lo = (unsigned short)(lo - 0x80);
        ch = (unsigned short)(ch + (unsigned short)((c1 - 0x80) << 6));
        ch += lo;
        return ch;
    }
    return c;
}
