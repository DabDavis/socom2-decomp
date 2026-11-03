/* msl.h -- the prebuilt MSL C++ library's own declarations (MAIN 0x184600-0x18DB8F), NOT the
 * game's MSL (include/*, CodeWarrior 3.0.1's headers). SOCOM 1's DWARF names two installs: the
 * game's `C:\apps\cw301` and `C:\CodeWarrior\2.4_FE_Based_4.2.5`, whose MSL C++ (Src\ios.cpp,
 * iostream.cpp, locale.cpp; Include\fstream, streambuf, ios, ctype, codecvt, localeimp,
 * RefCountedPtrArray.h) SOCOM 1 links. SOCOM II's library is a later build of it: wchar_t is 4
 * bytes (showmanyc<wchar_t> divides by 4, SOCOM 1's by 2; filebuf<wchar_t>'s flags sit at 0x70).
 * The classes are SOCOM 1's DWARF layouts (`tools/dwarf1.py types`), the members' names its own;
 * only what the written functions use is declared. Our file, in our words. */
#ifndef MAIN_MSL_H
#define MAIN_MSL_H



extern "C" void *memcpy(void *, const void *, unsigned int);
extern "C" void free(void *);
extern "C" unsigned int strlen(const char *);
extern "C" char *strcpy(char *, const char *);

namespace std {

typedef unsigned int size_t;
typedef long streamsize;
typedef long streamoff;

template <class T> class allocator {};
template <class charT> struct char_traits;

template <class T> inline const T &min(const T &a, const T &b) { return b < a ? b : a; }

template <>
struct char_traits<char> {
    typedef char char_type;
    typedef int int_type;
    static char_type *copy(char_type *s1, const char_type *s2, unsigned int n)
        { memcpy(s1, s2, n); return s1; }
    static int_type eof() { return -1; }
    static bool eq_int_type(const int_type &c1, const int_type &c2) { return c1 == c2; }
    static char_type to_char_type(const int_type &c) { return (char_type)c; }
    static int_type to_int_type(const char_type &c) { return (unsigned char)c; }
};

template <>
struct char_traits<wchar_t> {
    typedef wchar_t char_type;
    typedef int int_type;
    static char_type *copy(char_type *s1, const char_type *s2, unsigned int n)
        { memcpy(s1, s2, n * sizeof(char_type)); return s1; }
    static int_type eof() { return -1; }
    static bool eq_int_type(const int_type &c1, const int_type &c2) { return c1 == c2; }
    static char_type to_char_type(const int_type &c) { return (char_type)c; }
    static int_type to_int_type(const char_type &c) { return (int_type)c; }
};

/* fpos<int>: the 64-bit offset, then the conversion state */
template <class stateT>
class fpos {
public:
    fpos(streamoff off) : pos_(off), st_() {}
private:
    streamoff pos_;
    stateT st_;
};

/* the destructor policies of RefCountedPtrArray.h */
template <class T> struct _Single { static void release(T *p) { delete p; } };
template <class T> struct _Array { static void release(T *p) { delete [] p; } };

/* RefCountedPtrArray.h */
template <class T, class Destructor>
class _RefCountedPtr {
public:
    explicit _RefCountedPtr(T *ptr = 0);
    _RefCountedPtr(const _RefCountedPtr &other);
    ~_RefCountedPtr();
    _RefCountedPtr &operator=(const _RefCountedPtr &other);
    T *get() const { return ptr_; }
private:
    T *ptr_;
    int *refCount_;
};

template <class T, class Destructor>
_RefCountedPtr<T, Destructor>::_RefCountedPtr(const _RefCountedPtr &other)
    : ptr_(other.ptr_), refCount_(other.refCount_)
{
    if (refCount_)
        ++*refCount_;
}

template <class T, class Destructor>
_RefCountedPtr<T, Destructor>::_RefCountedPtr(T *ptr)
    : ptr_(ptr), refCount_(0)
{
    try {
        if (ptr_ != 0)
            refCount_ = new int(1);
    } catch (...) {
        Destructor::release(ptr_);
        throw;
    }
}

template <class T, class Destructor>
_RefCountedPtr<T, Destructor>::~_RefCountedPtr()
{
    if (refCount_ != 0 && --*refCount_ == 0) {
        Destructor::release(ptr_);
        delete refCount_;
    }
}

/* the library's string for the exceptions' messages: one counted array */
class _MSLstring {
public:
    explicit _MSLstring(const char *value);
    const char *c_str() const { return data_.get(); }
private:
    _RefCountedPtr<char, _Array<char> > data_;
};

inline _MSLstring::_MSLstring(const char *value)
    : data_(new char[strlen(value) + 1])
{
    strcpy(data_.get(), value);
}

/* the game's (CodeWarrior 3.0's) basic_string<char>, as far as the locale's name needs it: three
 * words, bit 0 of the first the long form's mark, the text the third (include/string's shape; MAIN's
 * copy of its destructor is 0x184C10, OVL1's 0x1E7EF0) */
template <class charT, class traits = char_traits<charT>, class Allocator = allocator<charT> >
class basic_string {
public:
    ~basic_string();
    const charT *c_str() const { return !(w_[0] & 1) ? (const charT *)w_ + 1 : (const charT *)w_[2]; }
private:
    unsigned int w_[3];
};

template <class charT, class traits, class Allocator>
basic_string<charT, traits, Allocator>::~basic_string()
{
    if (w_[0] & 1)
        operator delete((void *)w_[2]);
}

typedef basic_string<char> string;

/* the facets' table: a pointer vector over the pod vector (capacity, size, data), its destructor
 * three this-tested levels */
template <class T>
class __vector_pod {
public:
    ~__vector_pod()
    {
        if (data_)
            operator delete(data_);
    }
    size_t size() const { return size_; }
    T &operator[](size_t i) { return data_[i]; }
protected:
    size_t capacity_;
    size_t size_;
    T *data_;
};

template <class T>
class __vector_imp : public __vector_pod<T> {
public:
    ~__vector_imp() {}
};

template <class T>
class vector : public __vector_imp<T> {
public:
    ~vector() {}
};

class exception {
public:
    exception() {}
    virtual ~exception() {}
    virtual const char *what() const;
};

template <int N> class __locale_imp;

class locale {
public:
    class facet {
        friend class __locale_imp<1>;
    protected:
        virtual ~facet() {}
        unsigned int ref_;
    };
    locale();
    locale &operator=(const locale &rhs) { imp_ = rhs.imp_; return *this; }
    static locale &global();
private:
    _RefCountedPtr<__locale_imp<1>, _Single<__locale_imp<1> > > imp_;
};

inline locale::locale() : imp_(global().imp_) {}

/* localeimp */
template <int N>
class __locale_imp {
public:
    ~__locale_imp() throw();
private:
    vector<locale::facet *> facets_;    /* 0x00 */
    string name_;                       /* 0x0C */
};

template <int N>
__locale_imp<N>::~__locale_imp() throw()
{
    size_t n = facets_.size();
    for (size_t i = 0; i < n; ++i) {
        locale::facet *p = facets_[i];
        if (p != 0)
            delete (--p->ref_ == 0 ? p : 0);
    }
}

class ios_base {
public:
    class failure : public exception {
    public:
        explicit failure(const string &msg);
        virtual ~failure() {}
        virtual const char *what() const;
    private:
        _MSLstring what_;
    };
    enum fmtflags { boolalpha = 0x0001, dec = 0x0002, fixed = 0x0004, hex = 0x0008, internal = 0x0010,
                    left = 0x0020, oct = 0x0040, right = 0x0080, scientific = 0x0100,
                    showbase = 0x0200, showpoint = 0x0400, showpos = 0x0800, skipws = 0x1000,
                    unitbuf = 0x2000, uppercase = 0x4000 };
    enum iostate { goodbit = 0, badbit = 1, eofbit = 2, failbit = 4 };
    enum openmode { app = 0x01, ate = 0x02, binary = 0x04, in = 0x08, out = 0x10, trunc = 0x20 };
    enum seekdir { beg, cur, end };
    enum event { erase_event, imbue_event, copyfmt_event };
    typedef void (*event_callback)(event, ios_base &, int index);

    virtual ~ios_base();
    locale imbue(const locale &loc);
    locale getloc() const;
protected:
    void init(void *sb);
private:
    struct callback_pair { event_callback first; int second; };

    void do_callbacks(event e)
    {
        size_t n = cb_siz_;
        while (n > 0) {
            --n;
            cb_vec_[n].first(e, *this, cb_vec_[n].second);
        }
    }

    callback_pair *cb_vec_;     /* 0x04 */
    size_t cb_siz_;
    size_t cb_cap_;
    long *iarray_;              /* 0x10 */
    size_t isize_;
    void **parray_;
    size_t psize_;
    locale *loc_;               /* 0x20 */
    void *rdbuf_;               /* 0x24 */
    long precision_;            /* 0x28 */
    long width_;                /* 0x30 */
    fmtflags fmtflags_;         /* 0x38 */
    unsigned char iostate_;     /* 0x3A */
    unsigned char exceptions_;  /* 0x3B */
};

/* streambuf */
template <class charT, class traits>
class basic_streambuf {
public:
    typedef charT char_type;
    typedef typename traits::int_type int_type;
    typedef fpos<int> pos_type;
    typedef streamoff off_type;

    virtual ~basic_streambuf();
    locale getloc() const { return loc_; }
    int_type sbumpc()
    {
        if (gnext_ < gend_)
            return traits::to_int_type(*gnext_++);
        return uflow();
    }
protected:
    char_type *gptr() const { return gnext_; }
    char_type *egptr() const { return gend_; }
    void gbump(int n) { gnext_ += n; }

    virtual void imbue(const locale &loc) { loc_ = loc; }
    virtual basic_streambuf *setbuf(char_type *, streamsize) { return this; }
    virtual pos_type seekoff(off_type, ios_base::seekdir, ios_base::openmode)
        { return pos_type(off_type(-1)); }
    virtual pos_type seekpos(pos_type, ios_base::openmode) { return pos_type(off_type(-1)); }
    virtual int sync() { return 0; }
    virtual streamsize showmanyc() { return 0; }
    virtual streamsize xsgetn(char_type *s, streamsize n);
    virtual int_type underflow() { return traits::eof(); }
    virtual int_type uflow();
    virtual int_type pbackfail(int_type) { return traits::eof(); }
    virtual streamsize xsputn(const char_type *s, streamsize n);
    virtual int_type overflow(int_type) { return traits::eof(); }

    char_type *gbeg_;           /* 0x04 */
    char_type *gnext_;          /* 0x08 */
    char_type *gend_;           /* 0x0C */
    char_type *pbeg_;           /* 0x10 */
    char_type *pnext_;          /* 0x14 */
    char_type *pend_;           /* 0x18 */
    locale loc_;                /* 0x1C */
};

/* fstream */
template <class charT, class traits>
class basic_filebuf : public basic_streambuf<charT, traits> {
protected:
    virtual basic_streambuf<charT, traits> *setbuf(charT *, streamsize n);
    virtual streamsize showmanyc();

    void *file_;                /* 0x24 */
    int state_;                 /* 0x28 */
    void *a_codecvt_;           /* 0x2C */
    charT input_buffer_[16];    /* 0x30 */
    bool overflow_called_last_;
    bool always_noconv_;
    bool buffered_;             /* 0x42 (char), 0x72 (wchar_t) */
};

/* codecvt */
class codecvt_base {
public:
    enum result { ok, partial, error, noconv };
};

template <class internT, class externT, class stateT> class codecvt;

template <>
class codecvt<char, char, int> : public locale::facet, public codecvt_base {
protected:
    virtual result do_out(int &state, const char *from, const char *from_end, const char *&from_next,
                          char *to, char *to_end, char *&to_next) const;
    virtual result do_in(int &state, const char *from, const char *from_end, const char *&from_next,
                         char *to, char *to_end, char *&to_next) const;
    virtual result do_unshift(int &state, char *to, char *to_end, char *&to_next) const;
    virtual int do_encoding() const;
    virtual bool do_always_noconv() const;
    virtual int do_max_length() const;
};

template <>
class codecvt<wchar_t, char, int> : public locale::facet, public codecvt_base {
protected:
    virtual result do_unshift(int &state, char *to, char *to_end, char *&to_next) const;
    virtual int do_encoding() const;
    virtual bool do_always_noconv() const;
    virtual int do_max_length() const;
};

/* ctype */
struct ctype_base {
    typedef unsigned char mask;
};

template <class charT> class ctype;

template <>
class ctype<char> : public locale::facet, public ctype_base {
protected:
    virtual char do_toupper(char c) const;
    virtual const char *do_toupper(char *low, const char *high) const;
    virtual char do_tolower(char c) const;
    virtual const char *do_tolower(char *low, const char *high) const;
    virtual char do_widen(char c) const;
    virtual const char *do_widen(const char *low, const char *high, char *to) const;
    virtual char do_narrow(char c, char dfault) const;
    virtual const char *do_narrow(const char *low, const char *high, char dfault, char *to) const;

    const unsigned char *__table_;          /* 0x08 */
    const unsigned char *__lower_map_;      /* 0x0C */
    const unsigned char *__upper_map_;      /* 0x10 */
    bool __owns_;                           /* 0x14 */
};

template <>
class ctype<wchar_t> : public locale::facet, public ctype_base {
protected:
    virtual bool do_is(mask m, wchar_t c) const;
    virtual const wchar_t *do_is(const wchar_t *low, const wchar_t *high, mask *vec) const;
    virtual const wchar_t *do_scan_is(mask m, const wchar_t *low, const wchar_t *high) const;
    virtual const wchar_t *do_scan_not(mask m, const wchar_t *low, const wchar_t *high) const;
    virtual wchar_t do_toupper(wchar_t c) const;
    virtual const wchar_t *do_toupper(wchar_t *low, const wchar_t *high) const;
    virtual wchar_t do_tolower(wchar_t c) const;
    virtual const wchar_t *do_tolower(wchar_t *low, const wchar_t *high) const;
    virtual wchar_t do_widen(char c) const;
    virtual const char *do_widen(const char *low, const char *high, wchar_t *to) const;
    virtual char do_narrow(wchar_t c, char dfault) const;
    virtual const wchar_t *do_narrow(const wchar_t *low, const wchar_t *high, char dfault, char *to) const;
};

}

#endif
