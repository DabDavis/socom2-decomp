/* zrdr_parse.cpp -- an .rdr read from its text: the file preprocessed into a buffer, then parsed
 * into a tree of values, a parenthesised list an array (OVL1 0x32F600-0x3333EF). The unit name is
 * SOCOM 1's (the press beta's asserts and SOCOM 1's __sinit_zrdr_parse.cpp name it; SOCOM 1's demo
 * holds zrdr_read, ReadArray, ReadToken, the preprocessor and MakeUnion in it). SOCOM II reads
 * through CRdrIO, a CBufferIO that keeps the file's name and directory as strings and the
 * preprocessed text in a buffer of its own, where SOCOM 1 kept a stack of open files; the
 * parser's functions take it as an argument. Defined in reverse of their image order (-inline
 * deferred). */

#include "zrdr.h"
#include <string>
#include <list>
#include <string.h>
#include <stdlib.h>

/* the SDK libc's character table (newlib's _ctype_, MAIN 0x1D08D0): bit 8 is white space */
extern "C" const char _ctype_[];
#define rdr_isspace(c) ((_ctype_ + 1)[c] & 8)

/* newlib's strtod (MAIN 0x19A9B0, SOCOM 1's strtod by name) */
extern "C" double func_0019A9B0(const char *s, char **end);
/* the values allocator (zrdr_pack.cpp's unit, 0x335BC0): `n` zeroed values, the first typed */
extern "C" _zrdr *func_00335BC0(unsigned char type, int n);

/* basic_string's members the map has not named (placeholders, `this` first): find_last_of and
 * find over a C string, substr, and the assignment from a string */
extern "C" unsigned int func_00331F30(const std::string *s, const char *chars, unsigned int pos);
extern "C" unsigned int func_00331ED0(const std::string *s, const char *str, unsigned int pos);
extern "C" std::string func_0032FC40(const std::string *s, unsigned int pos, unsigned int n);
extern "C" void *func_0022E4D0(std::string *s, const std::string &x);

/* basic_string's erase(pos, n) and operator[] (inline; include/string has neither) */
static inline std::string &string_erase(std::string &s, unsigned int pos, unsigned int n)
{
    std::forward_iterator_tag category;

    return *(std::string *)func_001EE780(&s, pos, n, 0, 0, category);
}
static inline char &string_at(std::string &s, unsigned int pos)
{
    char *p = (char *)s.c_str();

    return p[pos];
}

/* the file being parsed (SOCOM II's; the RTTI names it, 0x3F70F0): a CBufferIO, the name and
 * directory it was opened by, and the preprocessed text (its capacity, the buffer, its length) */
class CRdrIO : public CBufferIO {
public:
    virtual ~CRdrIO();
    bool Open(const char *name, unsigned int mode);
    bool Open(void *buf, unsigned int size);

    int m_capacity;      // 0x24
    char *m_text;        // 0x28
    int m_length;        // 0x2c
    std::string m_path;  // 0x30
    std::string m_name;  // 0x3c
};   // 0x48 bytes

/* SOCOM 1's: a token made a value -- an integer when all of it reads as one, else a real, else
 * (or quoted) a string in the file's table */
_zrdr *CRdrFile::MakeUnion(const char *token, bool quoted)
{
    _zrdr *u = func_00335BC0(0, 1);
    int len = strlen(token);

    if (!quoted && len > 0) {
        char *end;
        long i = strtol(token, &end, 10);

        if (len == end - token) {
            u->type = ZRDR_INTEGER;
            u->integer = i;
            return u;
        }
        float f = func_0019A9B0(token, &end);

        if (len == end - token) {
            u->type = ZRDR_REAL;
            u->real = f;
            return u;
        }
    }
    u->type = ZRDR_STRING;
    u->string = m_strings.CreateString(token);
    return u;
}

/* SOCOM 1's: the next token read up to a parenthesis or white space, a comment skipped to the
 * line's end, a quoted string read whole; the character that ended it returned (0 at the end) */
int CRdrFile::ReadToken(CRdrIO *io, _zrdr **out)
{
    char token[800];
    char c;
    int n = 0;
    bool quoted = false;

    while (io->fread(&c, 1)) {
        if (c == ';') {
            do {
                if (c == '\n')
                    break;
            } while (io->fread(&c, 1));
        } else if (c == '(' || c == ')' || rdr_isspace(c)) {
            if (n)
                *out = MakeUnion(token, quoted);
            else
                *out = 0;
            return c;
        } else if (n >= 800) {
            *out = 0;
            break;
        } else if (c == '"') {
            quoted = true;
            io->fread(&c, 1);
            while (c != '"') {
                token[n++] = c;
                io->fread(&c, 1);
            }
            token[n++] = 0;
        } else {
            token[n++] = c;
            token[n] = 0;
        }
    }
    return 0;
}

/* SOCOM 1's: the values up to the closing parenthesis gathered, a nested list read as an array,
 * then copied into an array of their own (at the top, the file's own value). The front is taken
 * off with erase(begin()), MSL's own pop_front: include/list's pop_front builds the iterator a
 * level shallower and its temporaries then sit below the push_backs' (the target's are above) --
 * an inline height: include/list's pop_front may serve once the headers are refitted for level 5.
 * OPEN, 17 instructions (23 with include/list as it stands): (1) the list's destructor, inlined
 * down to the range erase, reads end_.next_ through the sentinel's address held in s1 and stores
 * the end iterator first -- include/list's ~__list_deleter written as its constructor is, `node_base
 * *t = &end_; erase(iterator(t->next_), iterator(t));`, gives exactly that (measured with a shadow
 * <list>, build/scratch/rdr/inc/list); (2) the depth test: the target branches on `depth != 0` to
 * the allocation placed after a `b` over it (the depth-0 path an empty then-block), where every
 * spelling tried here falls through into it (`if (!depth) {} else`, if/else with `array = this`
 * or `array = array`, ternaries -- the forms that leave `array` uninitialised at its declaration
 * lose the coalescing of `array` with `this` in s0 and move every register); the loop label's
 * alignment nop follows from it */
_zrdr *CRdrFile::ReadArray(CRdrIO *io, int depth)
{
    _zrdr *rdr = 0;
    int token = 1;
    int done = 0;
    std::list<_zrdr *> list;
    _zrdr *array = this;

    while (!done && token) {
        token = ReadToken(io, &rdr);
        if (token) {
            if (rdr)
                list.push_back(rdr);
            if (token == '(') {
                rdr = ReadArray(io, depth + 1);
                list.push_back(rdr);
            } else if (token == ')')
                done = 1;
        }
    }
    if (depth)
        array = func_00335BC0(ZRDR_ARRAY, 1);
    int n = list.size();
    array->length = n;
    array->type = ZRDR_ARRAY;
    array->array = func_00335BC0(ZRDR_INTEGER, n);
    int i = 0;
    while (list.size()) {
        rdr = list.front();
        list.erase(list.begin());
        array->array[i++] = *rdr;
        _free(rdr, 0, 0);
    }
    return array;
}

/* a buffer parsed in place: no name, no directory */
bool CRdrIO::Open(void *buf, unsigned int size)
{
    m_name.erase();
    m_path.erase();
    return CBufferIO::Open(buf, size);
}

/* the file opened by name, its directory split off it with forward slashes */
bool CRdrIO::Open(const char *name, unsigned int mode)
{
    m_name = name;
    m_path.erase();
    unsigned int pos = func_00331F30(&m_name, "\\/", (unsigned int)std::string::npos);

    if (pos != (unsigned int)std::string::npos) {
        func_0022E4D0(&m_path, func_0032FC40(&m_name, 0, pos));
        string_erase(m_name, 0, pos + 1);
        unsigned int i;
        while ((i = func_00331ED0(&m_path, "\\", 0)) != (unsigned int)std::string::npos)
            string_at(m_path, i) = '/';
    }
    return CBufferIO::Open(name, mode);
}
