/* sc: compiler=mwcps2-3.0b52-030722; flags=-O4,p -sdatathreshold 0 -inline auto -enum min -Cpp_exceptions on -wchar_t on */
/* locale.cpp -- the prebuilt MSL C++ library's locale unit (MAIN 0x18CF20-0x18DB8F): SOCOM 1's
 * DWARF names it (CodeWarrior 2.4's MSL_Common\Src\locale.cpp: locale::global and classic, then
 * ctype<char>, lines 221-291 for its conversions). What is written so far: ctype<char>'s case and
 * width conversions, in SOCOM 1's line order, which is the image's (the library is not deferred).
 * The library's compiler and flags (iostream.cpp's header comment). */

#include "msl.h"

namespace std {

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
