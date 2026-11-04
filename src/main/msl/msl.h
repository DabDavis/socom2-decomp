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
extern "C" void *memmove(void *, const void *, unsigned int);
extern "C" void free(void *);
extern "C" unsigned int strlen(const char *);
extern "C" char *strcpy(char *, const char *);

/* newlib's stdio (MAIN's C library), as far as the file buffers reach: the FILE's head, the
 * reentrancy block's three streams, and the getc/putc macros of its <stdio.h> */
extern "C" {
struct __sbuf {
    unsigned char *_base;
    int _size;
};
struct __sFILE {
    unsigned char *_p;          /* 0x00 */
    int _r;                     /* 0x04 */
    int _w;                     /* 0x08 */
    short _flags;
    short _file;
    struct __sbuf _bf;          /* 0x10 */
    int _lbfsize;               /* 0x18 */
};
typedef struct __sFILE FILE;
struct _reent {
    int _errno;
    FILE *_stdin, *_stdout, *_stderr;
};
/* GCC-built newlib keeps it in .data (0x1CC750, -G0): MW must not reach it through $gp, which a
 * plain 4-byte extern would (the default small-data threshold); the section says so */
extern struct _reent *_impure_ptr __attribute__((section(".data")));
int __srget(FILE *);
int __swbuf(int, FILE *);
int ungetc(int, FILE *);
int fflush(FILE *);
int fseek(FILE *, long, int);
long ftell(FILE *);
}
#define EOF (-1)
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#define stdin (_impure_ptr->_stdin)
#define stdout (_impure_ptr->_stdout)
#define stderr (_impure_ptr->_stderr)
#define __sgetc(p) (--(p)->_r < 0 ? __srget(p) : (int)(*(p)->_p++))
#define __sputc(c, p) \
    (--(p)->_w < 0 ? \
        (p)->_w >= (p)->_lbfsize ? \
            (*(p)->_p = (c)), *(p)->_p != '\n' ? \
                (int)*(p)->_p++ : \
                __swbuf('\n', p) : \
            __swbuf((int)(c), p) : \
        (*(p)->_p = (c), (int)*(p)->_p++))
#define getc(fp) __sgetc(fp)
#define putc(x, fp) __sputc(x, fp)

namespace std {

typedef unsigned int size_t;
typedef long streamsize;
typedef long streamoff;

/* <memory>'s allocator: operator new and delete; with exceptions on, new's failure is its own throw
 * (the game's exceptions-off build tests the result and reports "Memory allocation failure") */
template <class T> class allocator {
public:
    typedef std::size_t size_type;
    T *allocate(size_type n) { return (T *)operator new(n * sizeof(T)); }
    void deallocate(T *p, size_type) { operator delete(p); }
};
template <class charT> struct char_traits;

/* <algorithm>'s min, defined after codecvt<char>'s do_length (iostream.cpp) */
template <class T> inline const T &min(const T &a, const T &b);

template <>
struct char_traits<char> {
    typedef char char_type;
    typedef int int_type;
    static void assign(char_type &c1, const char_type &c2) { c1 = c2; }
    static char_type *copy(char_type *s1, const char_type *s2, unsigned int n)
        { memcpy(s1, s2, n); return s1; }
    static char_type *move(char_type *s1, const char_type *s2, unsigned int n)
        { memmove(s1, s2, n); return s1; }
    static int_type eof() { return -1; }
    static int_type not_eof(const int_type &c) { return eq_int_type(c, eof()) ? 0 : c; }
    static bool eq_int_type(const int_type &c1, const int_type &c2) { return c1 == c2; }
    static char_type to_char_type(const int_type &c) { return (char_type)c; }
    static int_type to_int_type(const char_type &c) { return (unsigned char)c; }
};

/* the wide traits copy element by element through assign (round 6: the streambufs' xsgetn/xsputn
 * <wchar_t>, 0x185EB0/0x185C20, byte-exact so -- the reference parameters keep each `p + 4` in a
 * register of its own in MW's unrolled loop; a plain `*s1++ = *s2++` folds it into the offsets) */
template <>
struct char_traits<wchar_t> {
    typedef wchar_t char_type;
    typedef int int_type;
    static void assign(char_type &c1, const char_type &c2) { c1 = c2; }
    static char_type *copy(char_type *s1, const char_type *s2, unsigned int n)
    {
        char_type *s = s1;
        for (size_t i = 0; i < n; ++i)
            assign(*s1++, *s2++);
        return s;
    }
    static int_type eof() { return -1; }
    static int_type not_eof(const int_type &c) { return eq_int_type(c, eof()) ? 0 : c; }
    static bool eq_int_type(const int_type &c1, const int_type &c2) { return c1 == c2; }
    static char_type to_char_type(const int_type &c) { return (char_type)c; }
    static int_type to_int_type(const char_type &c) { return (int_type)c; }
};

/* fpos<int>: the 64-bit offset, then the conversion state */
template <class stateT>
class fpos {
public:
    fpos(streamoff off) : pos_(off), st_() {}
    operator streamoff() const { return pos_; }
    stateT state() const { return st_; }
    void state(stateT st) { st_ = st; }
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
    /* the copy constructor and the assignment are defined by the units, where each must first be
     * seen (ios.cpp at its top; iostream.cpp after the filebufs, whose constructors call the copy) */
    _RefCountedPtr(const _RefCountedPtr &other);
    ~_RefCountedPtr();
    _RefCountedPtr &operator=(const _RefCountedPtr &other);
    T *get() const { return ptr_; }
    T *operator->() const { return ptr_; }
private:
    T *ptr_;
    int *refCount_;
};

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

/* <iterator>'s tags: the template replace takes the range's category by value (an empty class: a byte
 * loaded off the stack and never stored) */
struct input_iterator_tag {};
struct forward_iterator_tag : public input_iterator_tag {};

/* <iterator>'s distance over pointers, by value (the string's replace; the class iterators' form is
 * below, by lower_bound) */
template <class T>
inline int distance(T *first, T *last)
{
    return last - first;
}

/* the game's (CodeWarrior 3.0's) basic_string<char>, the library's build of it (exceptions on: the
 * length and range checks throw): twelve bytes, a long form (the capacity's word holding the mark in
 * bit 0, the size, the text) over a short form (the mark and the size in the first byte, eleven
 * characters with the terminator in place) -- include/string's shape, its members out of class (MAIN's
 * copy of the destructor is 0x184C10, OVL1's 0x1E7EF0). THE LONG FORM'S BITFIELDS ARE size_type,
 * `typename Allocator::size_type`: MW narrows a bitfield access to a byte only when the field's declared
 * type is not template-dependent -- the mark's tests and stores here are words (lw/sw), the short form's
 * `unsigned char` size a byte (build/scratch/rt/r7/bf4.cpp; OVL1's reserve 0x1E7C90 byte-exact so, the
 * exceptions-off build, as a probe: build/scratch/rt/r7/g1.cpp) */
template <class charT, class traits = char_traits<charT>, class Allocator = allocator<charT> >
class basic_string : private Allocator {
public:
    typedef typename Allocator::size_type size_type;

    basic_string() { init(); }
    basic_string(const charT *s);
    ~basic_string();
    const charT *c_str() const { return !(w_[0] & 1) ? (const charT *)w_ + 1 : (const charT *)w_[2]; }
    const charT *data() const { return l_.is_long_ ? l_.data_ : s_.data_; }
    size_type size() const { return l_.is_long_ ? l_.size_ : (unsigned char)s_.size_; }
    size_type max_size() const { return 0x7FFFFFFE; }
    void reserve(size_type res_arg = 0);
    template <class InputIterator> basic_string &assign(InputIterator first, InputIterator last, void * = 0);
    template <class ForwardIterator>
    basic_string &replace(size_type pos, size_type n1, ForwardIterator j1, ForwardIterator j2,
                          forward_iterator_tag);
private:
    void init()
    {
        unsigned int (&a)[3] = w_;

        for (unsigned int i = 0; i < 3; ++i)
            a[i] = 0;
    }

    struct __long {
        size_type is_long_ : 1;
        size_type cap_ : 31;
        size_type size_;
        charT *data_;
    };
    struct __short {
        unsigned char is_long_ : 1;
        unsigned char size_ : 7;
        charT data_[11];
    };
    union {
        unsigned int w_[3];
        __long l_;
        __short s_;
    };
};

template <class charT, class traits, class Allocator>
basic_string<charT, traits, Allocator>::~basic_string()
{
    if (w_[0] & 1)
        operator delete((void *)w_[2]);
}

/* from a C string (0x18D2C0, MAIN's copy of OVL1's 0x2396F0, include/string's shape): the words
 * zeroed, the length taken, the room reserved, the text replaced in (locale.cpp's __locale_imp name) */
template <class charT, class traits, class Allocator>
basic_string<charT, traits, Allocator>::basic_string(const charT *s)
{
    init();
    size_type n = strlen(s);
    forward_iterator_tag category;

    reserve(n);
    replace(0, 0, s, s + n, category);
}

typedef basic_string<char> string;

class exception {
public:
    exception() {}
    virtual ~exception() {}
    virtual const char *what() const;
};

/* <stdexcept>: the logic errors carry their message in the library's counted string; the vector's and
 * the string's length and range checks throw them (vtables 0x1D5250, 0x1D5240, 0x1D5290) */
class logic_error : public exception {
public:
    explicit logic_error(const char *what_arg) : what_(what_arg) {}
    virtual const char *what() const { return what_.c_str(); }
private:
    _MSLstring what_;
};

class length_error : public logic_error {
public:
    explicit length_error(const char *what_arg) : logic_error(what_arg) {}
};

class out_of_range : public logic_error {
public:
    explicit out_of_range(const char *what_arg) : logic_error(what_arg) {}
};

/* basic_string's out-of-line members (locale.cpp's template tail: classic()'s "C" instantiates them) */

/* room for res_arg characters (0x18D8C0, MAIN's copy of OVL1's reserve 0x1E7C90): the capacity rounded
 * up to a multiple of 16 or the short form's 11, the text moved to the new room. Growing, the
 * allocation's failure is the caller's; shrinking, it is not (the try around the second allocation:
 * the game's build tests the result instead) */
template <class charT, class traits, class Allocator>
void basic_string<charT, traits, Allocator>::reserve(size_type res_arg)
{
    if (res_arg > max_size())
        throw length_error("basic_string::reserve length_error");
    size_type cap, sz;
    charT *new_data, *old_data;
    bool now_long;
    bool is_long = l_.is_long_;
    if (!is_long) {
        cap = 11;
        sz = (unsigned char)s_.size_;
    } else {
        sz = l_.size_;
        cap = l_.cap_;
    }
    if (res_arg < sz)
        res_arg = sz;
    if (res_arg + 1 < 12)
        res_arg = 11;
    else
        res_arg = (res_arg + 16) & ~15;
    if (res_arg == cap)
        return;
    if (res_arg == 11) {
        old_data = l_.data_;
        new_data = s_.data_;
        now_long = false;
    } else {
        if (cap < res_arg)
            new_data = this->allocate(res_arg);
        else {
            try {
                new_data = this->allocate(res_arg);
            } catch (...) {
                return;
            }
        }
        now_long = true;
        old_data = is_long ? l_.data_ : s_.data_;
    }
    traits::copy(new_data, old_data, sz);
    new_data[sz] = charT();
    if (is_long)
        this->deallocate(old_data, cap);
    l_.is_long_ = now_long;
    if (!now_long)
        s_.size_ = sz;
    else {
        l_.data_ = new_data;
        l_.size_ = sz;
        l_.cap_ = res_arg;
    }
}

/* the whole text replaced by a range (0x18DB40): the template replace over (0, size()) */
template <class charT, class traits, class Allocator>
template <class InputIterator>
basic_string<charT, traits, Allocator> &basic_string<charT, traits, Allocator>::assign(InputIterator first,
                                                                                       InputIterator last, void *)
{
    forward_iterator_tag category;

    return replace(0, size(), first, last, category);
}

/* n1 characters at pos replaced by the range j1..j2 (0x18D3E0, MAIN's copy of OVL1's 0x1EE780): the
 * checks, then either a new buffer (the head, the range, the tail copied in) or the text moved in place
 * -- the range first copied out (assign into a temporary) when the tail's move would overwrite it.
 * Byte-exact (round 7) -- each piece measured: the locals declared at the top, the mark's bool LAST
 * (as reserve's: it takes the highest number, so its spill slot is the first after new_data's -- the
 * target's 176; first, it was the last slot), the short form's size read before its capacity (the
 * byte's load scheduled first, into v1), n2 through distance(j1, j2) by value (written `j2 - j1` the
 * allocator spilled j2 rather than xlen and new_data: 0x438), and the temporary's text read through a
 * reference to it, short form first, assigned in each arm (data() inlined leaves an empty join block,
 * which takes the outer join's alignment nop: `b` one word early; folded onto the frame, the address
 * register goes) -- build/scratch/rt/r7/rp*.py */
template <class charT, class traits, class Allocator>
template <class ForwardIterator>
basic_string<charT, traits, Allocator> &basic_string<charT, traits, Allocator>::replace(size_type pos,
    size_type n1, ForwardIterator j1, ForwardIterator j2, forward_iterator_tag)
{
    charT *data;
    size_type sz, cap, xlen, n2, new_size, tail;
    bool is_long = l_.is_long_;
    if (!is_long) {
        data = s_.data_;
        sz = (unsigned char)s_.size_;
        cap = 11;
    } else {
        data = l_.data_;
        sz = l_.size_;
        cap = l_.cap_;
    }
    if (pos > sz)
        throw out_of_range("basic_string: out_of_range");
    xlen = min(n1, sz - pos);
    n2 = (size_type)distance(j1, j2);
    if (n2 > max_size() || sz - xlen > max_size() - n2)
        throw length_error("basic_string: length_error");
    new_size = n2 + (sz - xlen);
    tail = sz - (pos + xlen);
    if (new_size >= cap) {
        size_type newcap = (cap + 15) & ~15;
        while (newcap < new_size + 1)
            newcap = (2 * newcap + 15) & ~15;
        charT *new_data = this->allocate(newcap);
        if (pos != 0)
            traits::copy(new_data, data, pos);
        charT *p = new_data + pos;
        traits::copy(p, j1, (size_type)(j2 - j1));
        if (tail != 0)
            traits::copy(p + n2, data + pos + xlen, tail);
        traits::assign(new_data[new_size], charT());
        if (is_long)
            this->deallocate(data, cap);
        else
            l_.is_long_ = true;
        l_.data_ = new_data;
        l_.size_ = new_size;
        l_.cap_ = newcap;
    } else {
        basic_string temp;
        if (tail != 0 && data + pos + n2 < j2 && j2 <= data + sz) {
            temp.assign(j1, j2);
            const basic_string &t = temp;
            if (!t.l_.is_long_)
                j1 = t.s_.data_;
            else
                j1 = t.l_.data_;
        }
        if (tail != 0)
            traits::move(data + pos + n2, data + pos + xlen, tail);
        traits::move(data + pos, j1, n2);
        traits::assign(data[new_size], charT());
        if (!is_long)
            s_.size_ = new_size;
        else
            l_.size_ = new_size;
    }
    return *this;
}

/* <vector>, CodeWarrior 3.0's MSL shape (the game's map names its instances so): a vector of
 * pointers is __vector_imp<T, A, 1> over the pod vector of unsigned int (capacity, size, data -- the
 * facets' table, its destructor three this-tested levels, resize on the pod), a vector of a class
 * __vector_imp<T, A, 0> over its deleter (the same three words), with class iterators nested in it
 * (one word: copied through an FPR, passed by value in a register, returned through a hidden
 * pointer -- the range map's walks) and the class insert out of line on the element pointer */
template <class T, class Allocator>
class __vector_pod {
public:
    __vector_pod() : capacity_(0), size_(0), data_(0) {}
    ~__vector_pod()
    {
        if (data_)
            operator delete(data_);
    }
    size_t size() const { return size_; }
    size_t capacity() const { return capacity_; }
    T &operator[](size_t i) { return data_[i]; }
    T *begin() { return data_; }
    T *end() { return data_ + size_; }
    static size_t max_size() { return 0x3FFFFFFF; }
    void resize(size_t sz, const T &c);
    void insert(T *position, size_t n, const T &x);
    T *erase(T *first, T *last);
protected:
    size_t capacity_;
    size_t size_;
    T *data_;
};

/* <algorithm>'s copy, forward over random-access iterators */
template <class InputIterator, class OutputIterator>
inline OutputIterator copy(InputIterator first, InputIterator last, OutputIterator result)
{
    for (; first < last; ++first, ++result)
        *result = *first;
    return result;
}

template <class T, class Allocator>
void __vector_pod<T, Allocator>::resize(size_t sz, const T &c)
{
    if (sz > size())
        insert(end(), sz - size(), c);
    else if (sz < size())
        erase(begin() + sz, end());
}

/* n copies of x at position: in place when the capacity holds them (x itself moved with the tail
 * when it lies there), else into a buffer grown by doubling from the old capacity (SOCOM 1's
 * <vector> 486-530, one block: newcap, newsize, olddata, pos, xp, t1 -- MW's DWARF lists a block's
 * locals in REVERSE declaration order: flush's `failed` before its sentry `ok`). MAIN's copy 0x18B810
 * byte-exact (round 7; rounds 5-6 had it 20 instructions off): the six locals declared at the top --
 * SOCOM 1's order but olddata last, which the class inserts below need (newsize, newcap and olddata in
 * that order; 360 of the 720 orders give the pod's registers, the 120 with that order among them all
 * three inserts': build/scratch/rt/r7/perm.py) -- and the in-place test through capacity() (round 6: every saved register; the
 * accessor makes the growth branch re-read capacity_), `e - (t1 - n) <= xp` (branches, where `!(xp <
 * ...)` made a bool), the doubling loop's if/else (the target's padding) under `while (newsize > newcap)`
 * -- written `newcap < newsize`, MW computed the loop's bottom test into v0 and its hoisted max_size()
 * into v1, where the target branches on at and keeps the constant in v0 -- and the old end taken into
 * a local ahead of the second copy (the target reads size_ before the new data_; the end written in the
 * argument list is evaluated after the third argument) -- build/scratch/rt/r7/pi1.py, pi2.py. The fills
 * step their pointer in the loop's increment (the class insert below needs it; the pod's code is the same
 * either way). Written so that iostream.cpp emits ~logic_error, ~length_error and what(). */
template <class T, class Allocator>
void __vector_pod<T, Allocator>::insert(T *position, size_t n, const T &x)
{
    size_t t1;
    const T *xp;
    size_t pos;
    size_t newsize;
    size_t newcap;
    T *olddata;
    if (n == 0)
        return;
    if (n > max_size() || size_ > max_size() - n)
        throw length_error("vector::insert length error");
    newsize = size_ + n;
    if (newsize <= capacity()) {
        T *e = data_ + size_;
        t1 = (size_t)(e - position);
        xp = &x;
        if (e - (t1 - n) <= xp && xp < e)
            xp += n;
        if (t1 != 0)
            for (T *i = e, *j = e + n; position < i;)
                *--j = *--i;
        for (size_t k = n; k != 0; --k, ++position)
            *position = *xp;
        size_ += n;
    } else {
        olddata = data_;
        pos = (size_t)(position - olddata);
        newcap = capacity_ ? capacity_ : 1;
        while (newsize > newcap) {
            if (newcap < max_size() / 2)
                newcap *= 2;
            else
                newcap = max_size();
        }
        data_ = (T *)operator new(newcap * sizeof(T));
        T *p = data_ + pos;
        for (size_t k = n; k != 0; --k, ++p)
            *p = x;
        if (olddata) {
            copy(olddata, olddata + pos, data_);
            T *oldend = olddata + size_;
            copy(olddata + pos, oldend, data_ + pos + n);
            operator delete(olddata);
        }
        size_ = newsize;
        capacity_ = newcap;
    }
}

template <class T, class Allocator>
T *__vector_pod<T, Allocator>::erase(T *first, T *last)
{
    if (first == last)
        return first;
    T *e = data_ + size_;
    if (e - last != 0)
        copy(last, e, first);
    size_ -= last - first;
    return first;
}

template <class T, class Allocator>
class __vector_deleter {
public:
    __vector_deleter() : capacity_(0), size_(0), data_(0) {}
    ~__vector_deleter()
    {
        if (data_)
            operator delete(data_);
    }
protected:
    size_t capacity_;
    size_t size_;
    T *data_;
};

template <class T, class Allocator, bool IsPOD> class __vector_imp;

template <class T, class Allocator>
class __vector_imp<T, Allocator, true> : public __vector_pod<unsigned int, allocator<unsigned int> > {
    typedef __vector_pod<unsigned int, allocator<unsigned int> > base;
public:
    ~__vector_imp() {}
    T &operator[](size_t i) { return (T &)base::operator[](i); }
};

template <class T, class Allocator>
class __vector_imp<T, Allocator, false> : public __vector_deleter<T, Allocator> {
public:
    class iterator {
    public:
        typedef int difference_type;
        iterator(T *p) : m_p(p) {}
        T &operator*() const { return *m_p; }
        T *operator->() const { return m_p; }
        iterator &operator++() { ++m_p; return *this; }
        iterator &operator+=(difference_type n) { m_p += n; return *this; }
        iterator operator+(difference_type n) const { iterator t(*this); return t += n; }
        difference_type operator-(const iterator &o) const { return m_p - o.m_p; }
        bool operator==(const iterator &o) const { return m_p == o.m_p; }
        T *m_p;
    };
    class const_iterator {
    public:
        typedef int difference_type;
        const_iterator(const T *p) : m_p(p) {}
        const T &operator*() const { return *m_p; }
        const T *operator->() const { return m_p; }
        const_iterator &operator++() { ++m_p; return *this; }
        const_iterator &operator+=(difference_type n) { m_p += n; return *this; }
        difference_type operator-(const const_iterator &o) const { return m_p - o.m_p; }
        bool operator==(const const_iterator &o) const { return m_p == o.m_p; }
        const T *m_p;
    };
    ~__vector_imp() {}
    iterator begin() { return iterator(this->data_); }
    iterator end() { return iterator(this->data_ + this->size_); }
    const_iterator begin() const { return const_iterator(this->data_); }
    const_iterator end() const { return const_iterator(this->data_ + this->size_); }
    size_t capacity() const { return this->capacity_; }
    static size_t max_size() { return size_t(-1) / sizeof(T); }
    void insert(T *position, size_t n, const T &x);
    void insert(iterator position, const T &x) { insert(position.m_p, 1, x); }
};

/* the class insert (0x18BE10 over the mask map's 12-byte entries, 0x18C510 over the case maps' 16-byte
 * ones, both byte-exact, round 7): the pod insert's statements over a class element -- an entry assigned
 * member by member (the wchar_t ends lhu/sh, the mask's halves lh/sh), max_size() the largest count of
 * them. The fills step the pointer in the loop's increment: `*position++ = *xp` over a class element
 * copies the old pointer into a temporary ahead of the member stores (the target stores through the
 * pointer and steps it in the branch's delay slot) */
template <class T, class Allocator>
void __vector_imp<T, Allocator, false>::insert(T *position, size_t n, const T &x)
{
    size_t t1;
    const T *xp;
    size_t pos;
    size_t newsize;
    size_t newcap;
    T *olddata;
    if (n == 0)
        return;
    if (n > max_size() || this->size_ > max_size() - n)
        throw length_error("vector::insert length error");
    newsize = this->size_ + n;
    if (newsize <= capacity()) {
        T *e = this->data_ + this->size_;
        t1 = (size_t)(e - position);
        xp = &x;
        if (e - (t1 - n) <= xp && xp < e)
            xp += n;
        if (t1 != 0)
            for (T *i = e, *j = e + n; position < i;)
                *--j = *--i;
        for (size_t k = n; k != 0; --k, ++position)
            *position = *xp;
        this->size_ += n;
    } else {
        olddata = this->data_;
        pos = (size_t)(position - olddata);
        newcap = this->capacity_ ? this->capacity_ : 1;
        while (newsize > newcap) {
            if (newcap < max_size() / 2)
                newcap *= 2;
            else
                newcap = max_size();
        }
        this->data_ = (T *)operator new(newcap * sizeof(T));
        T *p = this->data_ + pos;
        for (size_t k = n; k != 0; --k, ++p)
            *p = x;
        if (olddata) {
            copy(olddata, olddata + pos, this->data_);
            T *oldend = olddata + this->size_;
            copy(olddata + pos, oldend, this->data_ + pos + n);
            operator delete(olddata);
        }
        this->size_ = newsize;
        this->capacity_ = newcap;
    }
}

template <class T> struct __is_pointer { enum { value = false }; };
template <class T> struct __is_pointer<T *> { enum { value = true }; };

template <class T, class Allocator = allocator<T> >
class vector : public __vector_imp<T, Allocator, __is_pointer<T>::value> {
public:
    ~vector() {}
};

/* <iterator>'s distance and <algorithm>'s lower_bound, over random-access iterators by value */
template <class Iterator>
inline typename Iterator::difference_type distance(Iterator first, Iterator last)
{
    return last - first;
}

template <class ForwardIterator, class T>
ForwardIterator lower_bound(ForwardIterator first, ForwardIterator last, const T &value)
{
    typename ForwardIterator::difference_type len = distance(first, last);
    while (len > 0) {
        ForwardIterator i = first;
        typename ForwardIterator::difference_type len2 = len / 2;
        i += len2;
        if (*i < value) {
            first = ++i;
            len -= len2 + 1;
        } else
            len = len2;
    }
    return first;
}


template <int N> class __locale_imp;
template <bool b> struct __chooser {};
class locale;
template <class Facet> const Facet &__use_facet(const locale &loc, __chooser<true>);

class locale {
public:
    class facet {
        friend class __locale_imp<1>;
    protected:
        explicit facet(size_t refs = 0) : ref_(refs) {}
        virtual ~facet() {}
        unsigned int ref_;
    };
    /* a facet's index in a locale's table, numbered on first use (SOCOM 1's locale::id: one word, the
     * count a static member) */
    class id {
    public:
        id() {}
        operator size_t()
        {
            if (id_ == 0)
                id_ = ++id_count_s;
            return id_;
        }
    private:
        size_t id_;
        static size_t id_count_s;
    };
    locale();
    locale &operator=(const locale &rhs) { imp_ = rhs.imp_; return *this; }
    static locale &global() throw();
    static locale &classic() throw();
private:
    template <class Facet> friend const Facet &__use_facet(const locale &loc, __chooser<true>);
    typedef _RefCountedPtr<__locale_imp<1>, _Single<__locale_imp<1> > > imp_ptr;
    locale(const imp_ptr &imp) : imp_(imp) {}
    imp_ptr imp_;
};

inline locale::locale() : imp_(global().imp_) {}

/* localeimp */
template <int N>
class __locale_imp {
public:
    __locale_imp(const char *name) : name_(name) {}
    ~__locale_imp() throw();
    void replace(const locale::facet &f, size_t id);
private:
    typedef locale::facet facet;
    template <class Facet> friend const Facet &__use_facet(const locale &loc, __chooser<true>);
    vector<locale::facet *> facets_;    /* 0x00 */
    string name_;                       /* 0x0C */
};

/* a facet installed at its id, the table grown to hold it; the facet's count taken first and given
 * back if the growth throws */
template <int N>
void __locale_imp<N>::replace(const locale::facet &f, size_t id)
{
    facet &nf = const_cast<facet &>(f);
    ++nf.ref_;
    if (id >= facets_.size()) {
        try {
            facets_.resize(id + 1, 0);
        } catch (...) {
            delete (--nf.ref_ == 0 ? &nf : 0);
            throw;
        }
    }
    facet *&p = facets_[id];
    facet *old = p;
    if (old != 0)
        delete (--old->ref_ == 0 ? old : 0);
    p = &nf;
}

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
        explicit failure(const char *msg) : what_(msg) {}
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
    enum seekdir { beg = 0x01, cur = 0x02, end = 0x04 };   /* a bitmask: seekoff's compare chain */
    enum event { erase_event, imbue_event, copyfmt_event };
    typedef void (*event_callback)(event, ios_base &, int index);

    virtual ~ios_base();
    fmtflags setf(fmtflags fmtfl)
    {
        fmtflags old = fmtflags_;
        fmtflags_ = fmtflags(fmtflags_ | fmtfl);
        return old;
    }
    locale imbue(const locale &loc);
    locale getloc() const;
    fmtflags flags() const { return fmtflags_; }
    iostate rdstate() const { return iostate(iostate_); }
    iostate exceptions() const { return iostate(exceptions_); }
    bool good() const { return iostate_ == 0; }
    bool fail() const;
    void clear(iostate state = goodbit);
    void setstate(iostate state);
protected:
    ios_base() {}
    void init(void *sb);
public:     /* the state words are the stream classes' and their guards' (MSL reaches them directly) */
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

/* <ios>'s bitmask operators: the result is the one-byte enum again (`andi 0xff` after the mask) */
inline ios_base::openmode operator&(ios_base::openmode x, ios_base::openmode y)
    { return ios_base::openmode(int(x) & int(y)); }
inline ios_base::openmode operator|(ios_base::openmode x, ios_base::openmode y)
    { return ios_base::openmode(int(x) | int(y)); }
inline ios_base::iostate operator&(ios_base::iostate x, ios_base::iostate y)
    { return ios_base::iostate(int(x) & int(y)); }
inline ios_base::iostate operator|(ios_base::iostate x, ios_base::iostate y)
    { return ios_base::iostate(int(x) | int(y)); }
inline ios_base::fmtflags operator&(ios_base::fmtflags x, ios_base::fmtflags y)
    { return ios_base::fmtflags(int(x) & int(y)); }

inline bool ios_base::fail() const
{
    return (rdstate() & (badbit | failbit)) != 0;
}

/* clear's check: a stream without a buffer is bad; a state the exceptions mask names throws */
inline void ios_base::clear(iostate state)
{
    iostate_ = (unsigned char)state;
    if (rdbuf_ == 0)
        iostate_ |= badbit;
    if (rdstate() & exceptions())
        throw failure("ios_base failure in clear");
}

inline void ios_base::setstate(iostate state)
{
    clear(rdstate() | state);
}

/* streambuf */
template <class charT, class traits>
class basic_streambuf {
public:
    typedef charT char_type;
    typedef typename traits::int_type int_type;
    typedef fpos<int> pos_type;
    typedef streamoff off_type;

    virtual ~basic_streambuf();
    locale getloc() const;
    basic_streambuf *pubsetbuf(char_type *s, streamsize n) { return setbuf(s, n); }
    int pubsync() { return sync(); }
    int_type sbumpc()
    {
        if (gnext_ < gend_)
            return traits::to_int_type(*gnext_++);
        return uflow();
    }
    int_type sputc(char_type c)
    {
        if (pnext_ < pend_)
            return traits::to_int_type(*pnext_++ = c);
        return overflow(traits::to_int_type(c));
    }
protected:
    basic_streambuf() : gbeg_(0), gnext_(0), gend_(0), pbeg_(0), pnext_(0), pend_(0) {}
    char_type *eback() const { return gbeg_; }
    char_type *gptr() const { return gnext_; }
    char_type *egptr() const { return gend_; }
    void gbump(int n) { gnext_ += n; }
    void setg(char_type *gbeg, char_type *gnext, char_type *gend)
    {
        gbeg_ = gbeg;
        gnext_ = gnext;
        gend_ = gend;
    }
    char_type *pbase() const { return pbeg_; }
    char_type *pptr() const { return pnext_; }
    char_type *epptr() const { return pend_; }
    void pbump(int n) { pnext_ += n; }
    void setp(char_type *pbeg, char_type *pend)
    {
        pbeg_ = pnext_ = pbeg;
        pend_ = pend;
    }

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

/* codecvt */
class codecvt_base {
public:
    enum result { ok, partial, error, noconv };
};

template <class internT, class externT, class stateT> class codecvt;

/* the conversion facets' public face (MSL's codecvt): each forwards to its virtual */
#define _MSL_CODECVT_PUBLIC(internT, externT, stateT) \
public: \
    typedef internT intern_type; \
    typedef externT extern_type; \
    typedef stateT state_type; \
    result out(state_type &state, const intern_type *from, const intern_type *from_end, \
               const intern_type *&from_next, extern_type *to, extern_type *to_limit, \
               extern_type *&to_next) const \
        { return do_out(state, from, from_end, from_next, to, to_limit, to_next); } \
    result unshift(state_type &state, extern_type *to, extern_type *to_limit, \
                   extern_type *&to_next) const \
        { return do_unshift(state, to, to_limit, to_next); } \
    result in(state_type &state, const extern_type *from, const extern_type *from_end, \
              const extern_type *&from_next, intern_type *to, intern_type *to_limit, \
              intern_type *&to_next) const \
        { return do_in(state, from, from_end, from_next, to, to_limit, to_next); } \
    int encoding() const throw() { return do_encoding(); } \
    bool always_noconv() const throw() { return do_always_noconv(); } \
    int length(state_type &state, const extern_type *from, const extern_type *end, size_t max) const \
        { return do_length(state, from, end, max); } \
    int max_length() const throw() { return do_max_length(); }

template <>
class codecvt<char, char, int> : public locale::facet, public codecvt_base {
    _MSL_CODECVT_PUBLIC(char, char, int)
    explicit codecvt(size_t refs = 0) : locale::facet(refs) {}
    static locale::id id;
protected:
    virtual result do_out(int &state, const char *from, const char *from_end, const char *&from_next,
                          char *to, char *to_end, char *&to_next) const;
    virtual result do_in(int &state, const char *from, const char *from_end, const char *&from_next,
                         char *to, char *to_end, char *&to_next) const;
    virtual result do_unshift(int &state, char *to, char *to_end, char *&to_next) const;
    virtual int do_encoding() const;
    virtual bool do_always_noconv() const;
    virtual int do_length(int &state, const char *from, const char *from_end, size_t max) const;
    virtual int do_max_length() const;
};

template <>
class codecvt<wchar_t, char, int> : public locale::facet, public codecvt_base {
    _MSL_CODECVT_PUBLIC(wchar_t, char, int)
    explicit codecvt(size_t refs = 0) : locale::facet(refs) {}
    static locale::id id;
protected:
    virtual result do_out(int &state, const wchar_t *from, const wchar_t *from_end,
                          const wchar_t *&from_next, char *to, char *to_limit, char *&to_next) const;
    virtual result do_in(int &state, const char *from, const char *from_end, const char *&from_next,
                         wchar_t *to, wchar_t *to_limit, wchar_t *&to_next) const;
    virtual result do_unshift(int &state, char *to, char *to_end, char *&to_next) const;
    virtual int do_encoding() const;
    virtual bool do_always_noconv() const;
    virtual int do_length(int &state, const char *from, const char *from_end, size_t max) const;
    virtual int do_max_length() const;
};

/* ctype: the classification mask is a two-byte enum (-enum min) whose bitmask operators are
 * functions -- inline, so a mask built with them folds in code but is no constant expression (the
 * classic table fills its multi-bit entries in locale.cpp's static initialiser), and their result is
 * the enum again (`andi 0xffff` after the and) */
struct ctype_base {
    enum mask {
        alpha = 0x0001, blank = 0x0002, cntrl = 0x0004, digit = 0x0008, graph = 0x0010, lower = 0x0020,
        print = 0x0040, punct = 0x0080, space = 0x0100, upper = 0x0200, xdigit = 0x0400
    };
};

inline ctype_base::mask operator&(ctype_base::mask x, ctype_base::mask y)
    { return ctype_base::mask(int(x) & int(y)); }
inline ctype_base::mask operator|(ctype_base::mask x, ctype_base::mask y)
    { return ctype_base::mask(int(x) | int(y)); }

template <class charT> class ctype;

/* the classic tables (locale.cpp's; SOCOM 1's std::__ctype_map, __lower_map, __upper_map) */
extern const ctype_base::mask __ctype_map[256];
extern const unsigned char __lower_map[256];
extern const unsigned char __upper_map[256];

template <>
class ctype<char> : public locale::facet, public ctype_base {
public:
    explicit ctype(const mask *tab = 0, bool del = false, size_t refs = 0);
    char widen(char c) const { return do_widen(c); }
    static locale::id id;
protected:
    ~ctype();
    virtual char do_toupper(char c) const;
    virtual const char *do_toupper(char *low, const char *high) const;
    virtual char do_tolower(char c) const;
    virtual const char *do_tolower(char *low, const char *high) const;
    virtual char do_widen(char c) const;
    virtual const char *do_widen(const char *low, const char *high, char *to) const;
    virtual char do_narrow(char c, char dfault) const;
    virtual const char *do_narrow(const char *low, const char *high, char dfault, char *to) const;

    const mask *__table_;                   /* 0x08 */
    const unsigned char *__lower_map_;      /* 0x0C */
    const unsigned char *__upper_map_;      /* 0x10 */
    bool __owns_;                           /* 0x14 */
};

}

/* localeimp's range map (Metrowerks::range_map, SOCOM 1's DWARF: a vector of entries, each a range
 * x1_..x2_ mapped linearly, y = m_ * x + b_) -- ctype<wchar_t>'s classification and case tables */
namespace Metrowerks {

template <class T, class U>
class range_map {
public:
    void insert(const T &x1, const T &x2, const U &y1, const U &y2);
    void insert(const T &x1, const T &x2, const U &y) { insert(x1, x2, y, y); }
    void insert(const T &x, const U &y) { insert(x, x, y, y); }
    U operator[](const T &x) const;
private:
    struct entry {
        bool contains(const T &x) const { return !(x < x1_) && !(x2_ < x); }
        /* the lookup's miss test: one inline level more keeps the negation a value (sltu, xori,
         * then the branch), as the target's operator[]s have it */
        bool operator!=(const T &x) const { return !contains(x); }
        bool operator<(const entry &e) const { return x2_ < e.x1_; }
        bool operator<(const T &x) const { return x2_ < x; }
        T x1_;
        T x2_;
        U m_;
        U b_;
    };
    std::vector<entry> table_;
};

/* a range mapped linearly, installed over what the table held there: the entries it covers whole
 * replaced, one it cuts split around it (0x18B0C0, 0x18B320, both byte-exact since round 6). THE
 * ENTRY HAS NO CONSTRUCTOR: MW copies a class with a user constructor member by member (lhu/sh),
 * one without as words through the FPRs on copy construction -- the target's `oldi = *i` -- and
 * member by member on assignment (`*i = e`), so e is filled in field by field here
 * (build/scratch/rt/pe3.cpp); then the overlap test is the entries' operator< (`e < *i` re-reads the
 * iterator), the slope `x1 != x2 ? ... : 0`, ++pos ahead of the call (the target's delay slot) and
 * the last position assigned to i first (its slot reused) -- build/scratch/rt/ri3.py */
template <class T, class U>
void range_map<T, U>::insert(const T &x1, const T &x2, const U &y1, const U &y2)
{
    entry e;
    e.x1_ = x1;
    e.x2_ = x2;
    e.m_ = x1 != x2 ? U((y2 - y1) / (x2 - x1)) : U(0);
    e.b_ = U(y1 - e.m_ * x1);
    typename std::vector<entry>::iterator i = std::lower_bound(table_.begin(), table_.end(), e);
    if (i == table_.end() || e < *i) {
        table_.insert(i, e);
        return;
    }
    std::size_t pos = std::size_t(i - table_.begin());
    entry oldi = *i;
    if (oldi.x1_ < e.x1_) {
        i->x2_ = T(e.x1_ - 1);
        ++pos;
        table_.insert(++i, e);
    } else
        *i = e;
    if (e.x2_ < oldi.x2_) {
        i = table_.begin() + (pos + 1);
        oldi.x1_ = T(e.x2_ + 1);
        table_.insert(i, oldi);
    }
}

/* the value at x: the entry lower_bound finds, if its range holds x (0x18CB70, 0x18CC60, both
 * byte-exact since round 6: `*i != x` through the entry's operator!= -- the dereference re-reads the
 * found iterator's word for x1_, the extra inline level keeps the negation a value; `!i->contains(x)`,
 * `contains(x) == false`, an int-returning contains and the test written out were 0x8-0x20 off) */
template <class T, class U>
U range_map<T, U>::operator[](const T &x) const
{
    typename std::vector<entry>::const_iterator i = std::lower_bound(table_.begin(), table_.end(), x);
    if (i == table_.end() || *i != x)
        return U(0);
    return U(i->m_ * x + i->b_);
}

}

namespace std {

template <>
class ctype<wchar_t> : public locale::facet, public ctype_base {
public:
    explicit ctype(size_t refs = 0);
    wchar_t widen(char c) const { return do_widen(c); }
    static locale::id id;
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

    Metrowerks::range_map<wchar_t, mask> __table_;          /* 0x08 */
    Metrowerks::range_map<wchar_t, wchar_t> __lower_map_;   /* 0x14 */
    Metrowerks::range_map<wchar_t, wchar_t> __upper_map_;   /* 0x20 */
};

/* localeimp: the facet a locale holds, made and installed on first use (0x189470-0x1897EB, the four
 * instances iostream.cpp's streams use) */
template <class Facet>
const Facet &__use_facet(const locale &loc, __chooser<true>)
{
    size_t i = Facet::id;
    const locale::facet *f;
    if (i >= loc.imp_->facets_.size() || (f = loc.imp_->facets_[i]) == 0) {
        Facet *nf = new Facet;
        loc.imp_->replace(*nf, Facet::id);
        f = loc.imp_->facets_[i];
    }
    return static_cast<const Facet &>(*f);
}

template <class Facet>
inline const Facet &use_facet(const locale &loc)
{
    return __use_facet<Facet>(loc, __chooser<true>());
}

/* fstream: the C library's FILE under a stream buffer (SOCOM 1's DWARF layout and members) */
template <class charT, class traits>
class basic_filebuf : public basic_streambuf<charT, traits> {
    typedef basic_streambuf<charT, traits> base;
public:
    typedef charT char_type;
    typedef typename traits::int_type int_type;
    typedef typename base::pos_type pos_type;
    typedef typename base::off_type off_type;

    explicit basic_filebuf(FILE *file);
    virtual ~basic_filebuf();
protected:
    virtual void imbue(const locale &loc);
    virtual basic_streambuf<charT, traits> *setbuf(charT *, streamsize n);
    virtual pos_type seekoff(off_type off, ios_base::seekdir way, ios_base::openmode which);
    virtual pos_type seekpos(pos_type sp, ios_base::openmode which);
    virtual int sync();
    virtual streamsize showmanyc();
    virtual int_type underflow();
    virtual int_type uflow();
    virtual int_type pbackfail(int_type c);
    virtual int_type overflow(int_type c);
private:
    typedef codecvt<charT, char, int> cvt;

    bool unshift();
    /* one character put as it is (overflow's no-conversion path; always expanded) */
    int_type write1(char_type c) { return putc(c, file_) == EOF ? traits::eof() : traits::to_int_type(c); }
    int_type read1(bool peek);
    int_type read2(int n, bool peek);
    int_type read3(bool peek);
    int_type write2(int n, char_type intern_buf);

    FILE *file_;                /* 0x24 */
    int state_;                 /* 0x28 */
    const cvt *a_codecvt_;      /* 0x2C */
    charT input_buffer_[16];    /* 0x30 */
    bool overflow_called_last_; /* 0x40 (char), 0x70 (wchar_t) */
    bool always_noconv_;
    bool buffered_;
};

/* ios, istream, ostream: the stream classes over a virtual basic_ios */
template <class charT, class traits> class basic_ostream;

template <class charT, class traits>
class basic_ios : public ios_base {
public:
    typedef charT char_type;
    char_type widen(char c) const { return use_facet<ctype<char_type> >(getloc()).widen(c); }
    basic_ostream<charT, traits> *tie() const { return tiestr_; }
    basic_streambuf<charT, traits> *rdbuf() const { return (basic_streambuf<charT, traits> *)rdbuf_; }
    basic_ostream<charT, traits> *tie(basic_ostream<charT, traits> *tiestr)
    {
        basic_ostream<charT, traits> *old = tiestr_;
        tiestr_ = tiestr;
        return old;
    }
protected:
    basic_ios() {}
    void init(basic_streambuf<charT, traits> *sb)
    {
        ios_base::init(sb);
        tiestr_ = 0;
        fill_ = widen(' ');
    }
private:
    basic_ostream<charT, traits> *tiestr_;  /* 0x40 */
    char_type fill_;                        /* 0x44 */
};

template <class charT, class traits>
class basic_istream : virtual public basic_ios<charT, traits> {
public:
    explicit basic_istream(basic_streambuf<charT, traits> *sb);
private:
    streamsize gcount_;                     /* 0x08 */
};

template <class charT, class traits>
class basic_ostream : virtual public basic_ios<charT, traits> {
public:
    explicit basic_ostream(basic_streambuf<charT, traits> *sb);
    basic_ostream &flush();
    /* the output guard: the tied stream flushed, the state checked; on the way out a unit-buffered
     * stream is flushed unless the guard's flag says not to (flush sets it) */
    class sentry {
    public:
        explicit sentry(basic_ostream &os);
        ~sentry()
        {
            if (!os_.fail() && (os_.flags() & ios_base::unitbuf) && !uncaught_exception_)
                os_.flush();
        }
        operator bool() const { return ok_; }
        bool ok_;
        bool uncaught_exception_;
        basic_ostream &os_;
    };
};

typedef basic_streambuf<char, char_traits<char> > streambuf;
typedef basic_filebuf<char, char_traits<char> > filebuf;
typedef basic_istream<char, char_traits<char> > istream;
typedef basic_ostream<char, char_traits<char> > ostream;
typedef basic_filebuf<wchar_t, char_traits<wchar_t> > wfilebuf;
typedef basic_istream<wchar_t, char_traits<wchar_t> > wistream;
typedef basic_ostream<wchar_t, char_traits<wchar_t> > wostream;

/* iostream: the standard streams, built in place by the first Init object (the game's units each
 * hold one: include/iostream) */
extern istream cin;
extern ostream cout;
extern ostream clog;
extern ostream cerr;
extern wistream wcin;
extern wostream wcout;
extern wostream wclog;
extern wostream wcerr;

class __nInit {
public:
    __nInit();
    ~__nInit();
private:
    static int ninit_cnt_s;
};

class __wInit {
public:
    __wInit();
    ~__wInit();
private:
    static int winit_cnt_s;
};

}

/* <new>'s placement forms */
inline void *operator new(std::size_t, void *p) { return p; }
inline void operator delete(void *, void *) {}

#endif
