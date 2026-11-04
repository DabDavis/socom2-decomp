/* sc: compiler=mwcps2-3.0b52-030722; flags=-O4,p -inline auto,bottomup,level=8 -enum min -Cpp_exceptions on -wchar_t on -str readonly */
/* iostream.cpp -- the prebuilt MSL C++ library's standard streams unit (MAIN 0x184C80-0x18CF1F):
 * SOCOM 1's DWARF names it (CodeWarrior 2.4's MSL_Common\Src\iostream.cpp) and puts there the
 * stream objects' Init classes (__nInit, __wInit) and every template the streams instantiate --
 * streambuf, filebuf, istream/ostream, codecvt, ctype<wchar_t> -- each in its header's lines.
 * What is written so far: the small virtual slots (named by the vtables at 0x1D5050-0x1D5200),
 * uflow, xsgetn<char>, the filebufs' setbuf/showmanyc, the getlocs, codecvt's stubs and
 * ctype<wchar_t>'s widen/narrow; round 3: the Init classes (__nInit/__wInit), xsputn, and
 * basic_filebuf's bodies but overflow and seekoff (placeholders) -- SOCOM 1's DWARF names the
 * members and their locals (fstream lines 203-863).
 *
 * THE COMPILER (README "MAIN's Metrowerks runtime"): 3.0b38/3.0b50/3.0b52 -- showmanyc<wchar_t>
 * and setbuf<wchar_t> need a 4-byte wchar_t (3.0b22-020926 and later); ctype<wchar_t>::do_widen's
 * six-instruction loop keeps an empty delay slot and two nops only under 3.0b38+ (3.0b22-020926
 * fills the slot, 7 words off); the game's 3.0b52 here. -Cpp_exceptions ON (getloc keeps its frame
 * around the copy constructor's call; off, it is `j`), -wchar_t on (the `w` in every name), and
 * NOT deferred: SOCOM 1's DWARF lines rise with the addresses in this library (locale.cpp's
 * ctype<char> 221..288), the order SOCOM II keeps. The inline depth is explicit (`bottomup`): ios.cpp's
 * ~__locale_imp needs it (round 2); and it is at least 6, not the game's 5 (round 5): the ostream
 * sentries inline setstate whole -- clear, the failure, its counted string and the counted pointer's
 * constructor, six high -- which level=5 cuts to one level (rdstate, operator| and clear called);
 * 6, 7, 8 and 9 give the same 142 over the library, so the units carry 8, the level the game's
 * deepest witnesses ask for (README "Open, and next"). */

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

/* xsputn<char> matches; OPEN, xsputn<wchar_t> (0x185C20: 0x13C against 0x21C): as xsgetn<wchar_t>,
 * the target's wide copy is a loop, the traits' memcpy here */
template <class charT, class traits>
streamsize
basic_streambuf<charT, traits>::xsputn(const char_type *s, streamsize n)
{
    streamsize i = min(n, streamsize(epptr() - pptr()));
    if (i > 0) {
        traits::copy(pptr(), s, (unsigned int)i);
        s += i;
        pbump((int)i);
        n -= i;
    }
    while (n > 0) {
        if (traits::eq_int_type(sputc(*s++), traits::eof()))
            break;
        ++i;
        --n;
    }
    return i;
}

/* INLINE (round 6): the image generates ~basic_filebuf<char> right after __nInit's constructor, which
 * takes its address for the static filebufs' registration -- what MW does with an inline function
 * referenced and not inlined (an out-of-line template member waits for the unit's end: probed in
 * build/scratch/rt/p2*.cpp). So generated, ahead of _RefCountedPtr's instantiation, it inlines
 * ~basic_streambuf and ~locale and calls ~_RefCountedPtr (0x184A00), as the target's both do. */
template <class charT, class traits>
inline basic_filebuf<charT, traits>::~basic_filebuf()
{
}

template <class charT, class traits>
basic_streambuf<charT, traits> *basic_filebuf<charT, traits>::setbuf(charT *, streamsize n)
{
    buffered_ = n != 0;
    return this;
}

/* NOT WRITTEN (round 3): a placeholder body so the class instantiates -- the target's seekoff is 0x268
 * (char) / 0x278 (wchar_t): encoding(), the flush-out test, unshift, fseek/ftell by way */
template <class charT, class traits>
typename basic_filebuf<charT, traits>::pos_type
basic_filebuf<charT, traits>::seekoff(off_type off, ios_base::seekdir way, ios_base::openmode which)
{
    return pos_type(off_type(-1));
}

/* seekpos<wchar_t> matches; OPEN, seekpos<char> (0x186B20, 0x340 against 0x1C8): it inlines
 * unshift<char> here, which the explicit instantiation generates ahead of it (after overflow), where
 * the wide one's seekpos comes first and calls it, as the target's both do */
template <class charT, class traits>
typename basic_filebuf<charT, traits>::pos_type
basic_filebuf<charT, traits>::seekpos(pos_type sp, ios_base::openmode which)
{
    if (file_ == 0 || (which & (ios_base::in | ios_base::out)) == 0)
        return pos_type(off_type(-1));
    int width = a_codecvt_->encoding();
    if (this->pptr() > this->pbase() && traits::eq_int_type(overflow(traits::eof()), traits::eof()))
        return pos_type(off_type(-1));
    if ((which & ios_base::out) && overflow_called_last_ && width < 0) {
        if (!unshift())
            return pos_type(off_type(-1));
    }
    if (fseek(file_, sp, SEEK_SET) != 0)
        return pos_type(off_type(-1));
    this->setg(0, 0, 0);
    this->setp(0, 0);
    overflow_called_last_ = false;
    return sp;
}

template <class charT, class traits>
int basic_filebuf<charT, traits>::sync()
{
    return -(file_ == 0
             || (this->pptr() > this->pbase() && traits::eq_int_type(overflow(traits::eof()), traits::eof()))
             || fflush(file_) != 0);
}

template <class charT, class traits>
streamsize basic_filebuf<charT, traits>::showmanyc()
{
    return this->gend_ - this->gnext_;
}

template <class charT, class traits>
typename basic_filebuf<charT, traits>::int_type
basic_filebuf<charT, traits>::underflow()
{
    if (file_ == 0)
        return traits::eof();
    if (buffered_ && this->gptr() == 0) {
        if (this->pptr() > this->pbase() && traits::eq_int_type(overflow(traits::eof()), traits::eof()))
            return traits::eof();
        this->setp(0, 0);
        this->setg(input_buffer_, input_buffer_, input_buffer_);
    }
    overflow_called_last_ = false;
    if (this->gptr() < this->egptr())
        return traits::to_int_type(*this->gptr());
    int_type ci;
    if (always_noconv_)
        ci = read1(this->gptr() == 0);
    else {
        int width = a_codecvt_->encoding();
        if (width > 0)
            ci = read2(width, this->gptr() == 0);
        else
            ci = read3(this->gptr() == 0);
    }
    if (traits::eq_int_type(ci, traits::eof()))
        return traits::eof();
    char_type c = traits::to_char_type(ci);
    if (this->gptr() != 0) {
        size_t es = (size_t)(this->egptr() - this->eback());
        if (es == sizeof(input_buffer_) / sizeof(char_type)) {
            *(int *)input_buffer_ = *(int *)(input_buffer_ + sizeof(input_buffer_) / sizeof(char_type)
                                             - sizeof(int) / sizeof(char_type));
            es = sizeof(int) / sizeof(char_type);
        }
        ++es;
        this->setg(input_buffer_, input_buffer_ + es - 1, input_buffer_ + es);
        *this->gptr() = c;
    }
    return ci;
}

template <class charT, class traits>
typename basic_filebuf<charT, traits>::int_type
basic_filebuf<charT, traits>::uflow()
{
    if (buffered_ && this->gptr() == 0) {
        if (this->pptr() > this->pbase() && traits::eq_int_type(overflow(traits::eof()), traits::eof()))
            return traits::eof();
        this->setp(0, 0);
        this->setg(input_buffer_, input_buffer_, input_buffer_);
    }
    if (this->gptr() != 0) {
        if (traits::eq_int_type(underflow(), traits::eof()))
            return traits::eof();
        int_type c = traits::to_int_type(*this->gptr());
        this->gbump(1);
        return c;
    }
    overflow_called_last_ = false;
    if (file_ == 0)
        return traits::eof();
    int_type ci;
    if (always_noconv_)
        ci = read1(false);
    else {
        int width = a_codecvt_->encoding();
        if (width > 0)
            ci = read2(width, false);
        else
            ci = read3(false);
    }
    if (traits::eq_int_type(ci, traits::eof()))
        return traits::eof();
    return ci;
}

template <class charT, class traits>
typename basic_filebuf<charT, traits>::int_type
basic_filebuf<charT, traits>::pbackfail(int_type c)
{
    if (file_ == 0)
        return traits::eof();
    if (!buffered_ && always_noconv_) {
        if (traits::eq_int_type(c, traits::eof()))
            return traits::eof();
        int result = ungetc(c, file_);
        if (traits::eq_int_type(result, traits::eof()))
            return traits::eof();
    } else {
        if (this->gptr() <= this->eback())
            return traits::eof();
        this->gbump(-1);
        if (!traits::eq_int_type(c, traits::eof()))
            *this->gptr() = traits::to_char_type(c);
    }
    return traits::not_eof(c);
}

/* NOT WRITTEN (round 3): a placeholder body -- the target's overflow is 0x474 (char) / 0x4C4
 * (wchar_t): the put area, unshift on the switch to output, write2 when converting */
template <class charT, class traits>
typename basic_filebuf<charT, traits>::int_type
basic_filebuf<charT, traits>::overflow(int_type c)
{
    return traits::eof();
}

template <class charT, class traits>
typename basic_filebuf<charT, traits>::int_type
basic_filebuf<charT, traits>::read1(bool peek)
{
    int_type c = getc(file_);
    if (c == EOF)
        return traits::eof();
    if (peek) {
        if (ungetc(c, file_) == EOF)
            return traits::eof();
    }
    return c;
}

template <class charT, class traits>
bool basic_filebuf<charT, traits>::unshift()
{
    codecvt_base::result r = codecvt_base::partial;
    char to[4];
    while (r == codecvt_base::partial) {
        char *to_next = to;
        r = a_codecvt_->unshift(state_, to, to + sizeof(to), to_next);
        switch (r) {
        case codecvt_base::error:
            return false;
        case codecvt_base::noconv:
            break;
        case codecvt_base::ok:
        case codecvt_base::partial:
            for (char *t = to; t < to_next;)
                if (putc(*t++, file_) == EOF)
                    return false;
            break;
        }
    }
    return true;
}

/* a fixed-width encoding: n bytes in, one character out (put back again when peeking) */
template <class charT, class traits>
typename basic_filebuf<charT, traits>::int_type
basic_filebuf<charT, traits>::read2(int n, bool peek)
{
    char extern_buf[12];
    char_type intern_buf;
    const char *extern_end;
    char_type *intern_end;
    if ((unsigned int)n > sizeof(extern_buf))
        return traits::eof();
    for (int i = 0; i < n; ++i) {
        int c = getc(file_);
        if (c == EOF)
            return traits::eof();
        extern_buf[i] = (char)c;
    }
    switch (a_codecvt_->in(state_, extern_buf, extern_buf + n, extern_end,
                           &intern_buf, &intern_buf + 1, intern_end)) {
    case codecvt_base::error:
    case codecvt_base::partial:
        return traits::eof();
    case codecvt_base::noconv:
        intern_buf = *(char_type *)extern_buf;
        break;
    case codecvt_base::ok:
        break;
    }
    if (peek) {
        while (n > 0)
            if (ungetc(extern_buf[--n], file_) == EOF)
                return traits::eof();
    }
    return traits::to_int_type(intern_buf);
}

/* a variable-width encoding: bytes in one at a time until a character comes out */
template <class charT, class traits>
typename basic_filebuf<charT, traits>::int_type
basic_filebuf<charT, traits>::read3(bool peek)
{
    char extern_buf[12];
    const char *extern_end;
    char_type intern_buf;
    char_type *intern_end = &intern_buf;
    size_t i = 0;
    size_t offset = 0;
    while (intern_end < &intern_buf + 1) {
        if (i == sizeof(extern_buf))
            return traits::eof();
        int c = getc(file_);
        if (c == EOF)
            return traits::eof();
        extern_buf[i++] = (char)c;
        switch (a_codecvt_->in(state_, extern_buf + offset, extern_buf + i, extern_end,
                               &intern_buf, &intern_buf + 1, intern_end)) {
        case codecvt_base::error:
            return traits::eof();
        case codecvt_base::ok:
        case codecvt_base::partial:
            offset = (size_t)(extern_end - extern_buf);
            break;
        case codecvt_base::noconv:
            intern_buf = *(char_type *)extern_buf;
            intern_end = &intern_buf + 1;
            break;
        }
    }
    if (peek)
        while (i != 0)
            if (ungetc(extern_buf[--i], file_) == EOF)
                return traits::eof();
    return traits::to_int_type(intern_buf);
}

template <class charT, class traits>
typename basic_filebuf<charT, traits>::int_type
basic_filebuf<charT, traits>::write2(int n, char_type intern_buf)
{
    char extern_buf[12];
    char *extern_end;
    const char_type *intern_end;
    switch (a_codecvt_->out(state_, &intern_buf, &intern_buf + 1, intern_end,
                            extern_buf, extern_buf + sizeof(extern_buf), extern_end)) {
    case codecvt_base::error:
    case codecvt_base::partial:
        return traits::eof();
    case codecvt_base::noconv:
        n = sizeof(char_type);
        memcpy(extern_buf, &intern_buf, (unsigned int)n);
        break;
    case codecvt_base::ok:
        n = extern_end - extern_buf;
        break;
    }
    for (int i = 0; i < n; ++i) {
        if (putc(extern_buf[i], file_) == EOF)
            return traits::eof();
    }
    return traits::to_int_type(intern_buf);
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
/* The standard streams' Init classes (SOCOM 1's DWARF: iostream.cpp lines 107-145 for __nInit): the
 * first Init object builds the streams in place over three static filebufs on the C library's
 * stdin/stdout/stderr, unbuffered; the last one flushes and destroys them. */
int __nInit::ninit_cnt_s;

__nInit::__nInit()
{
    static filebuf fin(stdin);
    static filebuf fout(stdout);
    static filebuf ferr(stderr);
    if (ninit_cnt_s++ == 0) {
        new (&cin) istream(&fin);
        new (&cout) ostream(&fout);
        new (&cerr) ostream(&ferr);
        new (&clog) ostream(&ferr);
        cin.tie(&cout);
        cerr.setf(ios_base::unitbuf);
        fin.pubsetbuf(0, 0);
        fout.pubsetbuf(0, 0);
        ferr.pubsetbuf(0, 0);
    }
}

__nInit::~__nInit()
{
    if (--ninit_cnt_s == 0) {
        cout.flush();
        cerr.flush();
        clog.flush();
        cin.~istream();
        cout.~ostream();
        cerr.~ostream();
        clog.~ostream();
    }
}

int __wInit::winit_cnt_s;

__wInit::__wInit()
{
    static wfilebuf wfin(stdin);
    static wfilebuf wfout(stdout);
    static wfilebuf wferr(stderr);
    if (winit_cnt_s++ == 0) {
        new (&wcin) wistream(&wfin);
        new (&wcout) wostream(&wfout);
        new (&wcerr) wostream(&wferr);
        new (&wclog) wostream(&wferr);
        wcin.tie(&wcout);
        wcerr.setf(ios_base::unitbuf);
        wfin.pubsetbuf(0, 0);
        wfout.pubsetbuf(0, 0);
        wferr.pubsetbuf(0, 0);
    }
}

__wInit::~__wInit()
{
    if (--winit_cnt_s == 0) {
        wcout.flush();
        wcerr.flush();
        wclog.flush();
        wcin.~wistream();
        wcout.~wostream();
        wcerr.~wostream();
        wclog.~wostream();
    }
}

template class _RefCountedPtr<__locale_imp<1>, _Single<__locale_imp<1> > >;

/* The filebufs' FILE constructors and imbue, defined after the locale pointer's instance so that
 * their temporary locale's destructor inlines it, as the target's do -- and ahead of the copy
 * constructor's definition (after the class instantiations below), which they call. All four match
 * (round 6; the constructors were round 3's open: 0x160 against 0x134). THE RULE, measured: MW
 * auto-inlines a non-inline function only once it has been GENERATED (a later one is called, and
 * the call decided afresh at each inline level), while an inline function is expanded wherever its
 * definition is known; an inline function referenced and not expanded is generated right after the
 * top-level function that needs it, a non-inline template member at the unit's end -- the target's
 * destructors are the first kind (generated after __nInit's and __wInit's constructors), its
 * constructors the second (after showmanyc, in the end pass). */
template <class charT, class traits>
basic_filebuf<charT, traits>::basic_filebuf(FILE *file)
    : file_(file), state_(0), overflow_called_last_(false), buffered_(true)
{
    a_codecvt_ = &use_facet<cvt>(this->getloc());
    always_noconv_ = a_codecvt_->always_noconv();
}

template <class charT, class traits>
void basic_filebuf<charT, traits>::imbue(const locale &loc)
{
    base::imbue(loc);
    a_codecvt_ = &use_facet<cvt>(this->getloc());
    always_noconv_ = a_codecvt_->always_noconv();
}

/* the stream classes' constructors (basic_ios::init inlined) */
template <class charT, class traits>
basic_istream<charT, traits>::basic_istream(basic_streambuf<charT, traits> *sb)
    : gcount_(0)
{
    this->init(sb);
}

template <class charT, class traits>
basic_ostream<charT, traits>::basic_ostream(basic_streambuf<charT, traits> *sb)
{
    this->init(sb);
}

/* the output guard (SOCOM 1's ostream lines): any exception on the way caught and turned into the
 * stream's bad state, rethrown when the stream's mask asks */
template <class charT, class traits>
basic_ostream<charT, traits>::sentry::sentry(basic_ostream &os)
    : ok_(false), uncaught_exception_(false), os_(os)
{
    try {
        if (os_.good()) {
            if (os_.tie())
                os_.tie()->flush();
            if (os_.good())
                ok_ = true;
            else
                os_.setstate(ios_base::failbit);
        } else
            os_.setstate(ios_base::failbit);
    } catch (...) {
        if (!(os_.rdstate() & ios_base::failbit))
            os_.iostate_ |= ios_base::badbit;
        if (os_.exceptions() & os_.rdstate())
            throw;
    }
}

/* SOCOM 1's ostream 1046-1071: the buffer synced under the guard, its failure the stream's bad state */
template <class charT, class traits>
basic_ostream<charT, traits> &basic_ostream<charT, traits>::flush()
{
    sentry ok(*this);
    ok.uncaught_exception_ = true;
    bool failed = false;
    if (this->rdbuf()) {
        try {
            if (this->rdbuf()->pubsync() == -1)
                failed = true;
        } catch (...) {
            this->iostate_ |= ios_base::badbit;
            if (this->exceptions() & ios_base::badbit)
                throw;
        }
        if (failed)
            this->setstate(ios_base::badbit);
    }
    return *this;
}

template class basic_filebuf<char, char_traits<char> >;
template class basic_filebuf<wchar_t, char_traits<wchar_t> >;

/* RefCountedPtrArray.h's copy constructor (0x1848C0, ios.cpp's copy kept), defined only now: MW
 * auto-inlines a non-inline template member only once it has been generated, so the filebufs'
 * FILE constructors, generated above, call it (their default locale's imp_(global().imp_)) while
 * inlining ~_RefCountedPtr, instantiated ahead of them -- as the target's do (round 6) */
template <class T, class Destructor>
_RefCountedPtr<T, Destructor>::_RefCountedPtr(const _RefCountedPtr &other)
    : ptr_(other.ptr_), refCount_(other.refCount_)
{
    if (refCount_)
        ++*refCount_;
}

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
/* the wide conversion is no conversion at all: each wchar_t's four bytes as they lie in memory.
 * OPEN, 2 x 40 instructions (0x98 against 0xA0 each): the byte loop is MW's to unroll here (four
 * lb/sb pairs) and the target keeps it a loop (counter in a1, the source pointer stepped in the
 * delay slot); and the closing `?:` is a select here, branches in the target. Tried: int/size_t
 * counters, while/do forms, a variable bound, `*p++`; -O3,p and `-opt nounroll` change nothing;
 * only `#pragma opt_unroll_loops off` keeps the loop (then 2 instructions off in the scheduling)
 * -- SOCOM 1's copies (0x14A630, 0x14A6C0) have the loop too. */
codecvt_base::result
codecvt<wchar_t, char, int>::do_out(int &, const wchar_t *from, const wchar_t *from_end, const wchar_t *&from_next,
                                   char *to, char *to_limit, char *&to_next) const
{
    from_next = from;
    to_next = to;
    for (; from_next < from_end && to_next < to_limit - (sizeof(wchar_t) - 1); ++from_next, to_next += sizeof(wchar_t)) {
        const char *p = (const char *)from_next;
        for (size_t i = 0; i < sizeof(wchar_t); ++i)
            to_next[i] = p[i];
    }
    return from_next < from_end ? partial : ok;
}

codecvt_base::result
codecvt<wchar_t, char, int>::do_in(int &, const char *from, const char *from_end, const char *&from_next,
                                  wchar_t *to, wchar_t *to_limit, wchar_t *&to_next) const
{
    from_next = from;
    to_next = to;
    for (; from_next < from_end - (sizeof(wchar_t) - 1) && to_next < to_limit; from_next += sizeof(wchar_t), ++to_next) {
        char *p = (char *)to_next;
        for (size_t i = 0; i < sizeof(wchar_t); ++i)
            p[i] = from_next[i];
    }
    return from_next < from_end ? partial : ok;
}

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

/* the classic classification of the first 128 characters, and the case maps over 0-0xFFFF */
ctype<wchar_t>::ctype(size_t refs)
    : locale::facet(refs)
{
    __table_.insert(0, 8, cntrl);
    __table_.insert(9, cntrl | space | blank);
    __table_.insert(10, 13, cntrl | space);
    __table_.insert(14, 31, cntrl);
    __table_.insert(32, space | blank | print);
    __table_.insert(33, 47, punct | graph | print);
    __table_.insert(48, 57, digit | xdigit | graph | print);
    __table_.insert(58, 64, punct | graph | print);
    __table_.insert(65, 70, upper | xdigit | alpha | graph | print);
    __table_.insert(71, 90, upper | alpha | graph | print);
    __table_.insert(91, 96, punct | graph | print);
    __table_.insert(97, 102, lower | xdigit | alpha | graph | print);
    __table_.insert(103, 122, lower | alpha | graph | print);
    __table_.insert(123, 126, punct | graph | print);
    __table_.insert(127, cntrl);
    __lower_map_.insert(0, 0xFFFF, 0, 0xFFFF);
    __lower_map_.insert('A', 'Z', 'a', 'z');
    __upper_map_.insert(0, 0xFFFF, 0, 0xFFFF);
    __upper_map_.insert('a', 'z', 'A', 'Z');
}

bool ctype<wchar_t>::do_is(mask m, wchar_t c) const
{
    return (__table_[c] & m) != 0;
}

const wchar_t *ctype<wchar_t>::do_is(const wchar_t *low, const wchar_t *high, mask *vec) const
{
    for (const wchar_t *p = low; p < high; ++p, ++vec)
        *vec = __table_[*p];
    return high;
}

const wchar_t *ctype<wchar_t>::do_scan_is(mask m, const wchar_t *low, const wchar_t *high) const
{
    const wchar_t *p;
    for (p = low; p < high; ++p) {
        mask t = __table_[*p];
        if ((t & m) != 0)
            break;
    }
    return p;
}

const wchar_t *ctype<wchar_t>::do_scan_not(mask m, const wchar_t *low, const wchar_t *high) const
{
    const wchar_t *p;
    for (p = low; p < high; ++p) {
        mask t = __table_[*p];
        if ((t & m) == 0)
            break;
    }
    return p;
}

wchar_t ctype<wchar_t>::do_toupper(wchar_t c) const
{
    return __upper_map_[c];
}

const wchar_t *ctype<wchar_t>::do_toupper(wchar_t *low, const wchar_t *high) const
{
    for (; low < high; ++low)
        *low = __upper_map_[*low];
    return high;
}

wchar_t ctype<wchar_t>::do_tolower(wchar_t c) const
{
    return __lower_map_[c];
}

const wchar_t *ctype<wchar_t>::do_tolower(wchar_t *low, const wchar_t *high) const
{
    for (; low < high; ++low)
        *low = __lower_map_[*low];
    return high;
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

/* The wide facets' ids (0x1D5620, 0x1D5624) and so this unit's static initialiser, __sinit_iostream.cpp
 * (0x1D4BF0, round 6): locale::id has a constructor, so each id of a class template's instance gets a
 * guard byte (`@GUARD@id__...`, 0x1D5628 and 0x1D562C after the ids, in their order), which the
 * initialiser tests and sets -- the empty constructor itself leaves nothing (SOCOM 1's guard-only
 * `__sinit_iostream.cpp`, whose 2.4 build names the guard `__init__id__Q23std8ctype<w>`). locale.cpp
 * defines the char pair after its classic table. */
locale::id codecvt<wchar_t, char, int>::id;
locale::id ctype<wchar_t>::id;

}
