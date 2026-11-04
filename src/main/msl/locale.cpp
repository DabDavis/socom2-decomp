/* sc: compiler=mwcps2-3.0b52-030722; flags=-O4,p -inline auto,bottomup,level=8 -enum min -Cpp_exceptions on -wchar_t on -str readonly */
/* locale.cpp -- the prebuilt MSL C++ library's locale unit (MAIN 0x18CF20-0x18DB8F): SOCOM 1's
 * DWARF names it (CodeWarrior 2.4's MSL_Common\Src\locale.cpp: locale::global and classic, then
 * ctype<char>, lines 221-291 for its conversions). Written: global and classic, ctype<char>'s
 * constructor, destructor, case and width conversions, in SOCOM 1's line order, which is the
 * image's (the library is not deferred). The library's compiler and flags (iostream.cpp's header
 * comment), plus `-str readonly` (round 4: classic's "C" is addressed absolutely, not as small data;
 * no other library function moves with it).
 *
 * global and classic: function-local statics behind their guards (data.txt's init$724/G$722 and
 * init$795/C$792), each registered with ~locale (ios.cpp's 0x184600) on __register_global_object;
 * both are `throw()` (the __unexpected pad and its spin after the return). classic builds
 * __locale_imp<1>("C") -- the facet vector zeroed inline, the name through basic_string(const char *)
 * out of line -- and hands it to the static through a counted-pointer temporary (its constructor
 * 0x18D340, the copy 0x1848C0, the temporary's destructor 0x184A00).
 *
 * The vector destructors this unit emits (no-target) are referenced only from exception tables,
 * which the link discarded (the image's table is empty): the image has no copy of them.
 *
 * The library's own basic_string<char> instances that classic's "C" pulls in: the C-string
 * constructor 0x18D2C0 (MAIN's copy of OVL1's 0x2396F0, marked ovl-copy) is byte-exact since round 6
 * (msl.h); NOT WRITTEN: reserve 0x18D8C0 (0x278: OVL1's is 0x258 -- the library throws length_error
 * through _MSLstring where the game's build has exceptions off), the template replace 0x18D3E0 (0x450,
 * OVL1's 0x1EE780 is 0x3BC) and assign(first, last) 0x18DB40 (its caller) -- and ~out_of_range
 * 0x18D830 (the replace's throw, emitted only once the replace is written): MSL's string internals
 * are modelled nowhere (include/string calls the game's reserve and replace through placeholders,
 * msl.h MAIN's). The static initialiser 0x1D4C20 is this
 * unit's: it fills std::__ctype_map's entries 9-127, the multi-bit masks (alpha 0x1, blank 0x2,
 * cntrl 0x4, digit 0x8, graph 0x10, lower 0x20, print 0x40, punct 0x80, space 0x100, upper 0x200,
 * xdigit 0x400: a two-byte enum whose operator| is a function, so only single masks initialise
 * statically), then sets two guards (0x1D5658, 0x1D565C) -- two template static members' ids, not
 * identified. Round 5: the mask IS that two-byte enum (msl.h; ctype<wchar_t>'s do_is masks again after
 * its and), so ctype<char>'s constructor takes `const mask *` and its row is renamed with it
 * (__ct__Q23std8ctype<c>FPCQ33std10ctype_base4maskbUi). The initialiser stays unwritten: its two
 * guards and iostream.cpp's two (0x1D4BF0) look like the facets' template ids' (ctype<char>/codecvt<char>
 * after their ids at 0x1D5650/0x1D5654, the wchar_t pair's at 0x1D5620/0x1D5624) -- guarded template
 * statics, which msl.h's explicit specialisations do not make; modelling the four facets as instances
 * of their primary templates is the next step there. */

#include "msl.h"

namespace std {

/* RefCountedPtrArray.h's copy constructor, at the top as the header has it (msl.h leaves it to the
 * units: iostream.cpp must see it late); classic() calls it, the unit emits its copy (ios.cpp's kept) */
template <class T, class Destructor>
_RefCountedPtr<T, Destructor>::_RefCountedPtr(const _RefCountedPtr &other)
    : ptr_(other.ptr_), refCount_(other.refCount_)
{
    if (refCount_)
        ++*refCount_;
}

locale &locale::global() throw()
{
    static locale global_locale(classic());
    return global_locale;
}

locale &locale::classic() throw()
{
    static locale classic_locale(imp_ptr(new __locale_imp<1>("C")));
    return classic_locale;
}

ctype<char>::ctype(const mask *tab, bool del, size_t refs)
    : locale::facet(refs), __table_(tab), __lower_map_(__lower_map), __upper_map_(__upper_map), __owns_(del)
{
    if (__table_ == 0) {
        __table_ = __ctype_map;
        __owns_ = false;
    }
}

ctype<char>::~ctype()
{
    if (__owns_)
        delete [] __table_;
}

char ctype<char>::do_toupper(char c) const
{
    return (char)__upper_map_[(unsigned char)c];
}

const char *ctype<char>::do_toupper(char *low, const char *high) const
{
    for (; low < high; ++low)
        *low = (char)__upper_map_[(unsigned char)*low];
    return high;
}

char ctype<char>::do_tolower(char c) const
{
    return (char)__lower_map_[(unsigned char)c];
}

const char *ctype<char>::do_tolower(char *low, const char *high) const
{
    for (; low < high; ++low)
        *low = (char)__lower_map_[(unsigned char)*low];
    return high;
}

char ctype<char>::do_widen(char c) const
{
    return c;
}

const char *ctype<char>::do_widen(const char *low, const char *high, char *to) const
{
    memcpy(to, low, (unsigned int)(high - low));
    return high;
}

char ctype<char>::do_narrow(char c, char) const
{
    return c;
}

const char *ctype<char>::do_narrow(const char *low, const char *high, char, char *to) const
{
    memcpy(to, low, (unsigned int)(high - low));
    return high;
}

/* The classic table (0x1CA410): the C locale's class of each of the first 128 characters, none for
 * the rest. The masks are built with the mask's operator| (msl.h), a function, so an entry naming
 * more than one class is no constant expression: MW lays the single-class entries down as data and
 * stores the other 100 in this unit's static initialiser, __sinit_locale.cpp (0x1D4C20, round 6:
 * byte-exact as written, the stores in table order before the facets' guards). __lower_map and
 * __upper_map (0x1CA610, 0x1CA710: identities but for the 26 letters, fully constant) stay extern. */
#define A ctype_base::alpha
#define B ctype_base::blank
#define C ctype_base::cntrl
#define D ctype_base::digit
#define G ctype_base::graph
#define L ctype_base::lower
#define P ctype_base::print
#define U ctype_base::punct
#define S ctype_base::space
#define H ctype_base::upper
#define X ctype_base::xdigit

const ctype_base::mask __ctype_map[256] = {
    /* 0x00 */ C, C, C, C, C, C, C, C, C, C | S | B, C | S, C | S, C | S, C | S, C, C,
    /* 0x10 */ C, C, C, C, C, C, C, C, C, C, C, C, C, C, C, C,
    /* 0x20 */ S | B | P, U | G | P, U | G | P, U | G | P, U | G | P, U | G | P, U | G | P, U | G | P,
               U | G | P, U | G | P, U | G | P, U | G | P, U | G | P, U | G | P, U | G | P, U | G | P,
    /* 0x30 */ D | X | G | P, D | X | G | P, D | X | G | P, D | X | G | P, D | X | G | P, D | X | G | P,
               D | X | G | P, D | X | G | P, D | X | G | P, D | X | G | P,
               U | G | P, U | G | P, U | G | P, U | G | P, U | G | P, U | G | P,
    /* 0x40 */ U | G | P, H | X | A | G | P, H | X | A | G | P, H | X | A | G | P, H | X | A | G | P,
               H | X | A | G | P, H | X | A | G | P, H | A | G | P, H | A | G | P, H | A | G | P,
               H | A | G | P, H | A | G | P, H | A | G | P, H | A | G | P, H | A | G | P, H | A | G | P,
    /* 0x50 */ H | A | G | P, H | A | G | P, H | A | G | P, H | A | G | P, H | A | G | P, H | A | G | P,
               H | A | G | P, H | A | G | P, H | A | G | P, H | A | G | P, H | A | G | P,
               U | G | P, U | G | P, U | G | P, U | G | P, U | G | P,
    /* 0x60 */ U | G | P, L | X | A | G | P, L | X | A | G | P, L | X | A | G | P, L | X | A | G | P,
               L | X | A | G | P, L | X | A | G | P, L | A | G | P, L | A | G | P, L | A | G | P,
               L | A | G | P, L | A | G | P, L | A | G | P, L | A | G | P, L | A | G | P, L | A | G | P,
    /* 0x70 */ L | A | G | P, L | A | G | P, L | A | G | P, L | A | G | P, L | A | G | P, L | A | G | P,
               L | A | G | P, L | A | G | P, L | A | G | P, L | A | G | P, L | A | G | P,
               U | G | P, U | G | P, U | G | P, U | G | P, C
};

#undef A
#undef B
#undef C
#undef D
#undef G
#undef L
#undef P
#undef U
#undef S
#undef H
#undef X

/* The char facets' ids (0x1D5650, 0x1D5654). A static member of a class template's instance with a
 * constructor -- locale::id's, empty -- gets a guard byte, `@GUARD@<member>` (SOCOM 1's 2.4 build:
 * `__init__<member>`), which the unit's initialiser tests and sets after the table: 0x1D5658 and
 * 0x1D565C, in definition order. iostream.cpp defines the wide pair the same way. */
locale::id ctype<char>::id;
locale::id codecvt<char, char, int>::id;

}
