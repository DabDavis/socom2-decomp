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

namespace std {

typedef long streamsize;
typedef long streamoff;

template <class T> inline const T &min(const T &a, const T &b) { return b < a ? b : a; }

template <class charT> struct char_traits;

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

template <class T> struct _Single {};
template <class T> struct _Array {};

/* RefCountedPtrArray.h */
template <class T, class Destructor>
class _RefCountedPtr {
public:
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

template <int N> class __locale_imp;

class locale {
public:
    class facet {
    protected:
        virtual ~facet();
        unsigned int ref_;
    };
    locale &operator=(const locale &rhs) { imp_ = rhs.imp_; return *this; }
private:
    _RefCountedPtr<__locale_imp<1>, _Single<__locale_imp<1> > > imp_;
};

class ios_base {
public:
    enum seekdir {};
    enum openmode {};
    virtual ~ios_base();
    locale getloc() const;
private:
    void *cb_vec_;              /* 0x04 */
    unsigned int cb_siz_;
    unsigned int cb_cap_;
    long *iarray_;              /* 0x10 */
    unsigned int isize_;
    void **parray_;
    unsigned int psize_;
    locale *loc_;               /* 0x20 */
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
