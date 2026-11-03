/* sc: compiler=mwcps2-3.0b52-030722; flags=-O4,p -inline auto,bottomup,level=5 -enum min -Cpp_exceptions on -wchar_t on */
/* iostream.cpp -- the prebuilt MSL C++ library's standard streams unit (MAIN 0x184C80-0x18CF1F):
 * SOCOM 1's DWARF names it (CodeWarrior 2.4's MSL_Common\Src\iostream.cpp) and puts there the
 * stream objects' Init classes (__nInit, __wInit) and every template the streams instantiate --
 * streambuf, filebuf, istream/ostream, codecvt, ctype<wchar_t> -- each in its header's lines.
 * What is written so far: the small virtual slots (named by the vtables at 0x1D5050-0x1D5200),
 * uflow, xsgetn<char>, the filebufs' setbuf/showmanyc, the getlocs, codecvt's stubs and
 * ctype<wchar_t>'s widen/narrow.
 *
 * THE COMPILER (README "MAIN's Metrowerks runtime"): 3.0b38/3.0b50/3.0b52 -- showmanyc<wchar_t>
 * and setbuf<wchar_t> need a 4-byte wchar_t (3.0b22-020926 and later); ctype<wchar_t>::do_widen's
 * six-instruction loop keeps an empty delay slot and two nops only under 3.0b38+ (3.0b22-020926
 * fills the slot, 7 words off); the game's 3.0b52 here. -Cpp_exceptions ON (getloc keeps its frame
 * around the copy constructor's call; off, it is `j`), -wchar_t on (the `w` in every name), and
 * NOT deferred: SOCOM 1's DWARF lines rise with the addresses in this library (locale.cpp's
 * ctype<char> 221..288), the order SOCOM II keeps. The inline depth is explicit (`bottomup,level=5`,
 * the game's): ios.cpp's ~__locale_imp needs it (round 2). */

#include "msl.h"

namespace std {

locale ios_base::getloc() const
{
    return *loc_;
}

template <class charT, class traits>
basic_streambuf<charT, traits>::~basic_streambuf()
{
}

template <class charT, class traits>
locale basic_streambuf<charT, traits>::getloc() const
{
    return loc_;
}

template <class charT, class traits>
typename basic_streambuf<charT, traits>::int_type
basic_streambuf<charT, traits>::uflow()
{
    if (traits::eq_int_type(underflow(), traits::eof()))
        return traits::eof();
    return traits::to_int_type(*gnext_++);
}

template <class charT, class traits>
streamsize
basic_streambuf<charT, traits>::xsgetn(char_type *s, streamsize n)
{
    streamsize i = min(n, streamsize(egptr() - gptr()));
    if (i > 0) {
        traits::copy(s, gptr(), (unsigned int)i);
        s += i;
        gbump((int)i);
        n -= i;
    }
    while (n > 0) {
        int_type c = sbumpc();
        if (traits::eq_int_type(c, traits::eof()))
            break;
        *s = traits::to_char_type(c);
        ++i;
        ++s;
        --n;
    }
    return i;
}

template <class charT, class traits>
basic_streambuf<charT, traits> *basic_filebuf<charT, traits>::setbuf(charT *, streamsize n)
{
    buffered_ = n != 0;
    return this;
}

template <class charT, class traits>
streamsize basic_filebuf<charT, traits>::showmanyc()
{
    return this->gend_ - this->gnext_;
}


/* The stream classes (round 2): basic_istream/basic_ostream over a virtual basic_ios (MW's layout:
 * the virtual base pointer first, the vptr after the members, the base's displacement word at its
 * +72), their constructors (basic_ios::init inlined: ios_base::init, the tie cleared, the fill
 * widened through the ctype facet), the implicit destructors and MW's thunks for them
 * (`@8@72@__dt__...`, 0x18CEE0-0x18CF1F, emitted at the unit's end). ~RefCountedPtr<__locale_imp>
 * is instantiated ahead of them so that their temporary locale's destructor inlines it.
 *
 * The two codecvt destructors (0x18A550, 0x18A5B0, 0x5C each) are implicit: they match once
 * locale::facet's destructor is inline (msl.h), its vtable store folded into theirs.
 * OPEN: xsgetn<wchar_t> (0x185EB0, 0x214) instantiates from the char body 0x134: the wide traits'
 * copy is a loop there, not memcpy -- not written. ~ctype<char> (locale.cpp) is generated, not
 * written.
 *
 * codecvt<char, char, int>: no conversion */
codecvt_base::result codecvt<char, char, int>::do_out(int &, const char *from, const char *, const char *&from_next,
                                                     char *to, char *, char *&to_next) const
{
    from_next = from;
    to_next = to;
    return noconv;
}

codecvt_base::result codecvt<char, char, int>::do_in(int &, const char *from, const char *, const char *&from_next,
                                                    char *to, char *, char *&to_next) const
{
    from_next = from;
    to_next = to;
    return noconv;
}

codecvt_base::result codecvt<char, char, int>::do_unshift(int &, char *to, char *, char *&to_next) const
{
    to_next = to;
    return noconv;
}

int codecvt<char, char, int>::do_encoding() const
{
    return 1;
}

bool codecvt<char, char, int>::do_always_noconv() const
{
    return true;
}

int codecvt<char, char, int>::do_max_length() const
{
    return 1;
}

/* min<int> is called here before its definition (below) and generated after this, its first user
 * (0x18A530); do_length<wchar_t> inlines it */
int codecvt<char, char, int>::do_length(int &, const char *from, const char *from_end, size_t max) const
{
    return min(int(from_end - from), int(max));
}

template <class T>
inline const T &min(const T &a, const T &b)
{
    return b < a ? b : a;
}

template class fpos<int>;        /* generated first: seekoff/seekpos inline its constructor */
template class basic_streambuf<char, char_traits<char> >;
template class basic_streambuf<wchar_t, char_traits<wchar_t> >;
template class basic_filebuf<char, char_traits<char> >;
template class basic_filebuf<wchar_t, char_traits<wchar_t> >;
template class _RefCountedPtr<__locale_imp<1>, _Single<__locale_imp<1> > >;
template class basic_istream<char, char_traits<char> >;
template class basic_ostream<char, char_traits<char> >;
template class basic_istream<wchar_t, char_traits<wchar_t> >;
template class basic_ostream<wchar_t, char_traits<wchar_t> >;

/* RefCountedPtrArray.h's assignment (0x188B50): defined here, after the streams' imbues, which
 * call it (locale's inline operator= forwarding to it) */
template <class T, class Destructor>
_RefCountedPtr<T, Destructor> &_RefCountedPtr<T, Destructor>::operator=(const _RefCountedPtr &other)
{
    if (ptr_ != other.ptr_) {
        if (refCount_ != 0 && --*refCount_ == 0) {
            Destructor::release(ptr_);
            delete refCount_;
        }
        ptr_ = other.ptr_;
        refCount_ = other.refCount_;
        if (refCount_ != 0)
            ++*refCount_;
    }
    return *this;
}


/* codecvt<wchar_t, char, int> */
codecvt_base::result codecvt<wchar_t, char, int>::do_unshift(int &, char *to, char *, char *&to_next) const
{
    to_next = to;
    return noconv;
}

int codecvt<wchar_t, char, int>::do_encoding() const
{
    return 4;
}

bool codecvt<wchar_t, char, int>::do_always_noconv() const
{
    return false;
}

/* OPEN, 0x38 vs 0x3C: the target inlines min<int> here (generated after do_length<char>, its first
 * user, which calls it); this unit calls it. With min defined ahead of both, both inline it (and
 * do_length<char>, 0x34, misses instead); defined between them, neither does -- under -inline
 * auto, auto,level=5 and auto,bottomup,level=5 alike. Neither order of generation reproduces
 * "call it first, inline it after". */
int codecvt<wchar_t, char, int>::do_length(int &, const char *from, const char *from_end, size_t max) const
{
    return min(int((from_end - from) / sizeof(wchar_t)), int(max)) * sizeof(wchar_t);
}

int codecvt<wchar_t, char, int>::do_max_length() const
{
    return 4;
}

/* ctype<wchar_t> */
wchar_t ctype<wchar_t>::do_widen(char c) const
{
    return (wchar_t)c;
}

const char *ctype<wchar_t>::do_widen(const char *low, const char *high, wchar_t *to) const
{
    for (; low < high; ++low, ++to)
        *to = (wchar_t)*low;
    return high;
}

char ctype<wchar_t>::do_narrow(wchar_t c, char dfault) const
{
    if (c >= 255)
        return dfault;
    return (char)c;
}

const wchar_t *ctype<wchar_t>::do_narrow(const wchar_t *low, const wchar_t *high, char dfault, char *to) const
{
    while (low < high)
        *to++ = do_narrow(*low++, dfault);
    return high;
}

}
