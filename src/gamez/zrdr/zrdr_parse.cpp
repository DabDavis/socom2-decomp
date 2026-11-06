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
#include <vector>
#include <algorithm>
#include <string.h>
#include <stdlib.h>

/* the SDK libc's character table (newlib's _ctype_, MAIN 0x1D08D0): bit 8 is white space */
extern "C" const char _ctype_[];
#define rdr_isspace(c) ((_ctype_ + 1)[c] & 8)

/* newlib's strtod (MAIN 0x19A9B0, SOCOM 1's strtod by name) */
extern "C" double strtod(const char *s, char **end);   /* newlib (MAIN) */
/* the values allocator (zrdr_pack.cpp's unit, 0x335BC0): `n` zeroed values, the first typed */
extern "C" _zrdr *func_00335BC0(unsigned char type, int n);

/* basic_string's erase(pos, n) -- the template replace over (pos, n) with an empty range, as
 * include/string's erase(pos, n) -- and operator[] (inline; include/string has no operator[]) */
static inline std::string &string_erase(std::string &s, unsigned int pos, unsigned int n)
{
    std::forward_iterator_tag category;

    return s.replace(pos, n, (const char *)0, (const char *)0, category);
}
static inline char &string_at(std::string &s, unsigned int pos)
{
    char *p = (char *)s.c_str();

    return p[pos];
}

/* the file being parsed (SOCOM II's; the RTTI names it, 0x3F70F0): a CBufferIO, the name and
 * directory it was opened by, and the preprocessed text (its capacity, the buffer, its length).
 * Its destructor is the vtable's slot 0x4C, after CBufferIO's freadchar */
class CRdrIO : public CBufferIO {
public:
    CRdrIO() : m_capacity(0), m_text(0), m_length(0) {}
    virtual ~CRdrIO();
    bool Open(const char *name, unsigned int mode);
    bool Open(void *buf, unsigned int size);
    /* text appended to the preprocessed buffer, grown by the text's length when it is full (ours) */
    void Append(const char *text)
    {
        int n = text ? strlen(text) : 0;

        if (n > 0) {
            if (m_length + n >= m_capacity) {
                m_text = (char *)_realloc(m_text, n + m_capacity, 0, 0);
                m_capacity += n;
            }
            memcpy(m_text + m_length, text, n);
            m_length += n;
        }
    }

    int m_capacity;      // 0x24
    char *m_text;        // 0x28
    int m_length;        // 0x2c
    std::string m_path;  // 0x30
    std::string m_name;  // 0x3c
};   // 0x48 bytes

/* a preprocessor symbol (SOCOM 1's zrdr_sym, two char pointers there that its destructor freed,
 * kept in a list; SOCOM II's a flag and two strings, 0x1C bytes, kept in a vector): #define sets
 * it, #undef marks it undefined -- the flag set, the symbol hidden from #ifdef -- and a file's own
 * symbols are dropped when it is done. The strings are plain basic_strings -- the vector's clear and
 * erase destroy each with ONE this-test (a CSaferStr member gives two). OPEN, the element copies
 * (__copy 0x332560, __fill_n 0x332A60, __copy_backward 0x332AD0, remove_copy_if 0x333120): code-exact,
 * but they call basic_string's assignment 0x22E4D0 where the target calls 0x32FD70, `j 0x22E4D0`:
 * SOCOM II's MSL has operator=(const basic_string &) INLINE over an out-of-class assign(const
 * basic_string &) (SOCOM 1's MSL has both as 0xC-byte wrappers, 0x39C3A0 and 0x24C020), so 0x22E4D0 is
 * assign and 0x32FD70 operator='s out-of-line copy, which the preprocessor's Define requests first.
 * Measured (the world lane's round 4): include/string so, with 0x22E4D0 renamed, makes all four and
 * 0x32FD70 byte-exact and loses nothing over every hand-written unit -- the rename is outside this
 * lane's range (the integrator's request) */
struct zrdr_sym {
    zrdr_sym(const std::string &name, const std::string &value, int defined)
    {
        m_name = name;
        m_value = value;
        m_undefined = !defined;
    }
    /* written as the constructor is: the strings assigned, then the flag (0x332B50, the insert's
     * construct calls it) */
    zrdr_sym(const zrdr_sym &x)
    {
        m_name = x.m_name;
        m_value = x.m_value;
        m_undefined = x.m_undefined;
    }

    /* (ours, inline) the searches materialise its test (sltu, xori) before they branch */
    bool Defined() const { return !m_undefined; }

    unsigned char m_undefined;   // 0x0
    std::string m_name;          // 0x4
    std::string m_value;         // 0x10
};   // 0x1c bytes

/* the symbol table (0x488630; the class and its names ours): the symbols, and the count there was
 * before the file being read, whose own symbols it drops after (Pop) */
class zrdr_symtab : public std::vector<zrdr_sym> {
public:
    zrdr_symtab() : m_base(0) {}
    /* the newest symbol of that name -- from the file's own (local) or from all, an undefined one
     * only when asked for (any) */
    zrdr_sym *Find(const std::string &name, bool any, bool local)
    {
        zrdr_sym *found = 0;
        int i = size();
        int first = local ? m_base : 0;

        for (i--; i >= first; i--) {
            zrdr_sym *sym = &(*this)[i];

            if (sym->m_name != name)
                continue;
            if (any || sym->Defined())
                found = sym;
            break;
        }
        return found;
    }
    void Define(const std::string &name, const std::string &value)
    {
        zrdr_sym *sym = Find(name, true, true);

        if (sym) {
            sym->m_value = value;
            sym->m_undefined = 0;
        } else
            push_back(zrdr_sym(name, value, 1));
    }
    void Undefine(const std::string &name)
    {
        zrdr_sym *sym = Find(name, true, true);

        if (sym)
            sym->m_undefined = 1;
        else
            push_back(zrdr_sym(name, "", 0));
    }
    /* a walk with its body compiled out (0x32FB10) */
    void Dump()
    {
        for (iterator i = begin(); i != end(); i++)
            ;
    }
    /* the file's own symbols dropped, then every undefined one: too tall to inline whole, zrdr_read
     * inlines its body and calls each piece out of line (0x32FB80-0x32FC30, the erase 0x32FB90) */
    void Pop()
    {
        iterator last;
        iterator first;

        if (size() > m_base) {
            iterator i = begin();

            for (unsigned int n = 0; n < m_base; n++, i++)
                ;
            /* both bound to references: a temporary a reference binds takes its slot ahead of
             * the loop's postfix ++ temporary (zrdr_read's 0x140/0x144/0x148, as the target);
             * written `erase(i, end())` the three slots come out in the reverse order */
            const iterator &e = end();
            const iterator &r = erase(i, e);
        }
        last = end();
        first = std::remove_if(begin(), last, IsUndefined);
        erase(first, last);
        m_base = 0;
        Dump();
    }
    static bool IsUndefined(const zrdr_sym &sym);

    unsigned int m_base;   // 0xc
};   // 0x10 bytes

/* MEASURED (the MSL headers round 14, build/p14/zs1.py, untracked): with 0x333330 named
 * __dt__Q23std54__vector_imp<8zrdr_sym,...,0>Fv and 0x333390 __dt__11zrdr_symtabFv, this unit's
 * symbol-table destructor and __sinit_zrdr_parse.cpp are byte-exact (+2) -- but its ~__vector_imp
 * copy inlines the deleter's destructor (0x108 against 0x58: generated before it in this tail), so
 * 0x333330 would stand on its draft alone (an `sc: as=` line, an eleventh draft-only row): left to
 * the integrator. A dead-stripped stand-in that builds a symbol table (`new zrdr_symtab`, anywhere in
 * the unit: the constructor emits the implicit destructor levels at once) makes ~__vector_imp
 * byte-exact too, and costs the class insert 0x3325E0, which then inlines its temporary's deleter
 * destructor where the target calls it (0x3331F0) -- no order found that keeps both */
static zrdr_symtab g_symbols;

/* remove_if's test: a symbol #undef'd (0x331E20, the unit's first function) */
bool zrdr_symtab::IsUndefined(const zrdr_sym &sym)
{
    return sym.m_undefined != 0;
}

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
        float f = strtod(token, &end);

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
 * Byte-exact since round 34 (17 off since round 3): the depth test is `if (depth == 0) array = this; else array =
 * ...` with `array` left uninitialised -- the copy is what coalescing removes after the branches are laid out, so the
 * target's `b` over the allocation and the two filled delay slots are an arm emptied late. `!depth` swaps the arms and
 * loses the coalescing (47, the HUD's round 65 rule: `!x` swaps ternary and if arms, `x == 0` keeps them), and every
 * spelling that keeps `array = this` at the top falls through into the allocation. include/list's ~__list_deleter
 * written as its constructor is, `node_base *t = &end_; erase(iterator(t->next_), iterator(t));`, is what the list's
 * destructor needs (s1, the sentinel's address, the end iterator stored first). */
_zrdr *CRdrFile::ReadArray(CRdrIO *io, int depth)
{
    _zrdr *rdr = 0;
    int token = 1;
    int done = 0;
    std::list<_zrdr *> list;
    _zrdr *array;

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
    if (depth == 0) {
        array = this;
    } else {
        array = func_00335BC0(ZRDR_ARRAY, 1);
    }
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

/* the preprocessed text freed with the name and directory (the vtable's slot 0x4C) */
CRdrIO::~CRdrIO()
{
    if (m_text)
        _free(m_text, 0, 0);
    m_text = 0;
    m_capacity = 0;
    m_length = 0;
}

/* the file opened by name, its directory split off it with forward slashes */
bool CRdrIO::Open(const char *name, unsigned int mode)
{
    m_name = name;
    m_path.erase();
    unsigned int pos = m_name.find_last_of("\\/", (unsigned int)std::string::npos);

    if (pos != (unsigned int)std::string::npos) {
        m_path = m_name.substr(0, pos);
        string_erase(m_name, 0, pos + 1);
        unsigned int i;
        while ((i = m_path.find_first_of("\\", 0)) != (unsigned int)std::string::npos)
            string_at(m_path, i) = '/';
    }
    return CBufferIO::Open(name, mode);
}

/* a buffer parsed in place: no name, no directory */
bool CRdrIO::Open(void *buf, unsigned int size)
{
    m_name.erase();
    m_path.erase();
    return CBufferIO::Open(buf, size);
}

static inline void line_erase(std::string &s) { s.erase(); }
static inline std::string &string_erase2(std::string &s, unsigned int pos, unsigned int n) { return string_erase(s, pos, n); }

/* a string trimmed of the characters given at both ends (ours, inline) */
static inline std::string &zrdr_trim(std::string &s, const char *set)
{
    unsigned int i = s.find_first_not_of(set, 0);

    if (i)
        string_erase(s, 0, i);
    i = s.find_last_not_of(set, (unsigned int)std::string::npos);
    if (i + 1 != s.size())
        string_erase(s, i + 1, (unsigned int)std::string::npos);
    return s;
}

/* the preprocessor's directives (SOCOM 1's _get_pptoken, its codes 1, 2 and 4 the same; SOCOM II
 * compares each directive's length with strncmp where SOCOM 1 called strcmp) */
enum {
    ZRDR_PP_DEFINE = 1,
    ZRDR_PP_UNDEF = 2,
    ZRDR_PP_IFDEF = 4,
    ZRDR_PP_ELSE = 8,
    ZRDR_PP_ENDIF = 16,
    ZRDR_PP_INCLUDE = 32
};

int _get_pptoken(const char *line)
{
    int token = 0;

    if (!strncmp(line, "#ifdef", 6))
        token = ZRDR_PP_IFDEF;
    else if (!strncmp(line, "#else", 5))
        token = ZRDR_PP_ELSE;
    else if (!strncmp(line, "#endif", 6))
        token = ZRDR_PP_ENDIF;
    else if (!strncmp(line, "#define", 7))
        token = ZRDR_PP_DEFINE;
    else if (!strncmp(line, "#include", 8))
        token = ZRDR_PP_INCLUDE;
    else if (!strncmp(line, "#undef", 6))
        token = ZRDR_PP_UNDEF;
    return token;
}

/* SOCOM 1's ValidateFormat: the parentheses balance, outside quotes and ';' comments (SOCOM II reads
 * the file given, rewound before and after) */
bool CRdrFile::ValidateFormat(CRdrIO *io)
{
    int depth = 0;
    bool comment = false;
    bool quoted = false;
    char c;

    io->fseek(0, 0);
    while (io->fread(&c, 1) == 1) {
        switch (c) {
        case '"':
            if (!comment)
                quoted = !quoted;
            break;
        case ';':
            if (!quoted)
                comment = true;
            break;
        case '\r':
        case '\n':
            if (!quoted)
                comment = false;
            break;
        case '(':
            if (!quoted && !comment)
                depth++;
            break;
        case ')':
            if (!quoted && !comment)
                depth--;
            break;
        }
    }
    io->fseek(0, 0);
    return depth == 0;
}

/* SOCOM 1's _preproc_filter (a buffer there): the file read line by line into its preprocessed
 * text -- #include'd files preprocessed and spliced in, #define and #undef kept in the symbol table,
 * #ifdef/#else/#endif skipping lines (eight levels), every other line's tokens replaced by the
 * values of the symbols they name -- and the text then made the file's buffer.
 * The symbol table's Define and Undefine are too tall to inline whole and inlined one level here,
 * their pieces called out of line -- the copies 0x32FC50-0x32FD80, which sit before this recursive
 * function in reverse of their first use; #ifdef's and the substitution's Find inline whole. Written
 * so, the unit's template tail comes out as the target's: the element's copy constructor 0x332B50,
 * the insert 0x3325E0 and its helpers.
 * Byte-exact since round 34 (open since round 4). What settled it: the skip stack `= {0}`, MW's null-tested byte clear; the
 * depth defined ahead of it; zrdr_trim returning its string and CALLED IN THE ARGUMENTS of Define, Undefine and Find (`&name`
 * in a saved register after each trim, an arm's value frame-relative where it is not trimmed: an inline's by-reference
 * argument that is a call is not substituted, so the parameter is a variable; the two-argument Define's trims still run
 * left to right); the empty value a temporary of the call (its slot the last of the strings), two small wrappers
 * (line_erase, string_erase2: ours) a pass deeper for the frame slots of the first two erases' tags; the directive's
 * code an int narrowed at the switch; a line's character an int (no sign extension of a char variable); the word's
 * marks `start = mark = i`; a directive inside a string ends the file's loop (`break`). */
bool _preproc_filter(CRdrIO *io)
{
    std::string white(" \t");
    std::string newline("\n");
    std::string comment(";");
    std::string quote("\"");
    std::string delims(" \t()");
    std::string space(" \t");

    if (!CRdrFile::ValidateFormat(io))
        return false;
    io->fseek(0, 0);
    int depth = 0;
    bool skip[8] = {0};
    bool skipping = false;
    int error = 0;

    io->m_capacity = io->m_filesize;
    bool quoted = false;

    io->m_text = (char *)_malloc(io->m_capacity, 0, 0);
    bool eof = false;

    io->m_length = 0;
    std::string line;

    while (!error && !eof) {
        line_erase(line);
        bool end = false;
        char c;

        for (;;) {
            if (!io->fread(&c, 1)) {
                line.push_back('\n');
                end = true;
                break;
            }
            line.push_back(c);
            if (c == '\n')
                break;
        }
        eof = end;
        unsigned int pos = line.find_first_not_of(" \t", 0);
        bool directive = pos != (unsigned int)std::string::npos && line.at(pos) == '#';

        if (directive && pos)
            string_erase2(line, 0, pos);
        if (directive) {
            int token = _get_pptoken(line.c_str());

            if (quoted)
                break;
            unsigned int end = line.find_first_of(comment, 0);

            if (end != (unsigned int)std::string::npos)
                string_erase(line, end, (unsigned int)std::string::npos);
            switch ((unsigned short)token) {
            case ZRDR_PP_INCLUDE:
                if (!skipping) {
                    std::string name(line, line.find_first_of(white, 0), (unsigned int)std::string::npos);

                    zrdr_trim(name, " \t\n\r\f\"");
                    CRdrIO inc;
                    const char *found = zrdr_findfile(name.c_str(), io->m_path.c_str());

                    if (inc.Open(found, 1)) {
                        if (_preproc_filter(&inc)) {
                            io->Append(";; RDR_INCLUDE: ");
                            io->Append(name.c_str());
                            io->Append("\n");
                            int n = inc.GetSize();

                            if (n > 0) {
                                io->m_text = (char *)_realloc(io->m_text, n + io->m_capacity, 0, 0);
                                io->m_capacity += n;
                                memcpy(io->m_text + io->m_length, inc.m_buffer, n);
                                io->m_length += n;
                                io->Append("\n\n");
                            }
                        }
                        inc.Close();
                    }
                }
                break;
            case ZRDR_PP_DEFINE:
                if (!skipping) {
                    unsigned int first = line.find_first_of(white, 0);
                    unsigned int last = line.find_first_of(white, first + 1);
                    std::string name(line, first, last - first);

                    if (last != (unsigned int)std::string::npos) {
                        std::string value(line, last, (unsigned int)std::string::npos);

                        g_symbols.Define(zrdr_trim(name, " \t\n\r\f"), zrdr_trim(value, " \t\n\r\f"));
                    } else {
                        g_symbols.Define(zrdr_trim(name, " \t\n\r\f"), std::string(""));
                    }
                }
                break;
            case ZRDR_PP_UNDEF:
                if (!skipping) {
                    std::string name(line, line.find_first_of(white, 0), (unsigned int)std::string::npos);

                    g_symbols.Undefine(zrdr_trim(name, " \t\n\r\f"));
                }
                break;
            case ZRDR_PP_IFDEF: {
                std::string name(line, line.find_first_of(white, 0), (unsigned int)std::string::npos);

                depth++;
                if (!skipping) {
                    if (g_symbols.Find(zrdr_trim(name, " \t\n\r\f"), false, false))
                        skip[depth] = false;
                    else
                        skip[depth] = true;
                    skipping = skip[depth];
                } else
                    skip[depth] = true;
                break;
            }
            case ZRDR_PP_ELSE:
                if (!depth)
                    error = ZRDR_PP_ELSE;
                else if (!skip[depth - 1]) {
                    skip[depth] = !skip[depth];
                    skipping = skip[depth];
                }
                break;
            case ZRDR_PP_ENDIF:
                if (!depth)
                    error = ZRDR_PP_ENDIF;
                depth--;
                skipping = skip[depth];
                break;
            }
        } else {
            unsigned int i = 0;

            if (skipping)
                continue;
            if (!quoted) {
                i = line.find_first_not_of(space, 0);
                if (i) {
                    string_erase(line, 0, i);
                    i = 0;
                }
            }
            unsigned int start = (unsigned int)std::string::npos;
            unsigned int mark = (unsigned int)std::string::npos;

            for (; i < line.size(); i++) {
                int ch = line[i];

                if (quoted) {
                    if (ch == '"')
                        quoted = false;
                } else if (ch == ';') {
                    if (!i)
                        line.erase();
                    else
                        string_erase(line, i, line.size() - i - 1);
                } else if (ch == '"')
                    quoted = true;
                else if (rdr_isspace(ch) || ch == '(' || ch == ')') {
                    if (start == mark)
                        start = mark = i;
                    else {
                        std::string word = line.substr(start, i - start);

                        zrdr_sym *sym = g_symbols.Find(zrdr_trim(word, " \t\n\r\f"), false, false);

                        if (sym && sym->m_value.size()) {
                            string_erase(line, start, word.size());
                            line.replace(start, 0, sym->m_value);
                            i = start + sym->m_value.size();
                        }
                        start = mark = i;
                    }
                } else if (start == mark)
                    start = i;
            }
            if (line.size())
                io->Append(line.c_str());
        }
    }
    char *old = io->m_buffer;

    io->m_buffer = io->m_text;
    io->m_reserve = io->m_capacity;
    io->m_size = io->m_length;
    io->m_filesize = io->m_length;
    io->m_ptr = io->m_buffer + io->m_size;
    _free(old, 0, 0);
    io->fseek(0, 0);
    io->m_text = 0;
    io->m_capacity = 0;
    io->m_length = 0;
    return true;
}

/* an .rdr read: from the archives unless the flags say not (1), else from its file, preprocessed
 * (its text dumped beside it as "<name>pp" when the flags ask, 2) and parsed; the symbols the file
 * defined are dropped after. CRdrIO's GetSize and Close are called through the vtable on the local
 * (MW devirtualises a member only when the object's own class declares it: CFileIO's are direct).
 * Byte-exact once Pop binds end() and erase's result to const references (its comment): every
 * plain form -- the step in the body or the for's third clause, a while, a block, the counter
 * declared first, end() alone bound -- left the inlined Pop's temporaries in another order */
_zrdr *zrdr_read(const char *name, const char *path, int flags)
{
    CRdrFile *file = 0;

    if (!(flags & 1)) {
        file = CRdrArchive::FindRdr(name, flags);
        if (file)
            return file;
    }
    CRdrIO io;
    const char *found = zrdr_findfile(name, path);

    if (io.Open(found, 1)) {
        g_symbols.m_base = g_symbols.size();
        if (_preproc_filter(&io)) {
            if (flags & 2) {
                std::string pp(name);

                pp.append("pp");
                CFileIO out;

                if (out.Open(pp.c_str(), 26)) {
                    char *buf = io.m_buffer;

                    out.fwrite(buf, io.GetSize());
                    out.Close();
                }
            }
            file = new CRdrFile;
            file->ReadArray(&io, 0);
            if (!file->array) {
                delete file;
                file = 0;
            }
        }
        g_symbols.Pop();
        io.Close();
    }
    return file;
}

/* insert<zrdr_sym> 0x3325E0's end numbered before m: include/vector's __insert_end_pointer, a marked modelling device (the AI
 * core's round 37 measured this opt-in, +1 and nothing lost in this unit; what the original wrote is not found) */
namespace std { template <> struct __insert_end_pointer<zrdr_sym> { enum { value = 1 }; }; }

/* 0x3323F0's end as the erase's last parameter: include/vector's __erase_end_object, a marked modelling device (the MSL's round 40 measured it byte-exact);
 * what the original wrote is not known. */
namespace std { template <> struct __erase_end_object<zrdr_sym> { enum { value = 1 }; }; }
