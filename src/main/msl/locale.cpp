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
 * NOT WRITTEN (round 4): the library's own basic_string<char> instances that classic's "C" pulls in
 * -- the C-string constructor 0x18D2C0 (MAIN's copy of OVL1's 0x2396F0, marked ovl-copy), reserve
 * 0x18D8C0 (0x278: OVL1's is 0x258 -- the library throws length_error through _MSLstring where the
 * game's build has exceptions off), the template replace 0x18D3E0 (0x450, OVL1's 0x1EE780 is 0x3BC)
 * and assign(first, last) 0x18DB40 (its caller) -- and ~out_of_range 0x18D830 (the replace's throw):
 * neither MSL's string internals nor the logic_error family are modelled yet (include/string calls
 * the game's reserve and replace through placeholders). The static initialiser 0x1D4C20 is this
 * unit's: it fills std::__ctype_map's entries 9-127, the multi-bit masks (alpha 0x1, blank 0x2,
 * cntrl 0x4, digit 0x8, graph 0x10, lower 0x20, print 0x40, punct 0x80, space 0x100, upper 0x200,
 * xdigit 0x400: a two-byte enum whose operator| is a function, so only single masks initialise
 * statically), then sets two guards (0x1D5658, 0x1D565C) -- two template static members' ids, not
 * identified; msl.h keeps ctype_base::mask one byte until the wide ctype's rows are renamed with it. */

#include "msl.h"

namespace std {

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

}
