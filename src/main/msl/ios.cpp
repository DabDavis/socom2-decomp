/* sc: compiler=mwcps2-3.0b52-030722; flags=-O4,p -inline auto,bottomup,level=8 -enum min -Cpp_exceptions on -wchar_t on -str readonly */
/* ios.cpp -- the prebuilt MSL C++ library's ios_base unit (MAIN 0x184600-0x184C7F): SOCOM 1's DWARF
 * names it (CodeWarrior 2.4's MSL_Common\Src\ios.cpp: ~ios_base at lines 112-120, init at 180-211)
 * and links after it the templates it instantiates (RefCountedPtrArray.h, localeimp). SOCOM II's
 * build adds MAIN's copy of the game's ~basic_string (0x184C10, scored there through its ovl-copy
 * row): its __locale_imp names itself with a CodeWarrior 3.0 string. Every row of the range is
 * byte-exact (the two basic_streambuf setbufs, 0x1847C0/0x1847D0, are written in iostream.cpp).
 *
 * The failure constructor and imbue come first and are dead-stripped in the image: they are what
 * makes the unit emit ~locale (imbue's local, cleaned up by address), ~failure, _MSLstring's
 * RefCountedPtr<char> constructor and destructor ahead of ~ios_base, as the image holds them; their
 * own bodies (and _MSLstring's out-of-line destructor) are MSL's shape, unverifiable. what() is
 * the class's key function, defined after init where the image has it.
 *
 * THE FLAGS (iostream.cpp's header comment has the build): ~__locale_imp inlines the facet vector's
 * three destructor levels only with an explicit inline depth (`level=3` and up, or `bottomup`;
 * plain `-inline auto` calls the pod's destructor) -- the library's flags now carry the game's
 * `bottomup,level=5`, nothing else in the library moving. And ~ios_base's callback loop is NOT
 * strength-reduced in the target (n*8 recomputed each pass) where ~__locale_imp's facet loop is:
 * byte-exact only with `#pragma opt_strength_reduction off` around ~ios_base -- no loop shape
 * tried reproduces it (down/up counting, pointer walks, a reference, int/size_t), and SOCOM 1's
 * copy (0x1449B0) has the same unreduced loop. */

#include "msl.h"

namespace std {

/* RefCountedPtrArray.h's copy constructor, defined here at the top as the header has it (msl.h leaves
 * it to the units: iostream.cpp must see it late) */
template <class T, class Destructor>
_RefCountedPtr<T, Destructor>::_RefCountedPtr(const _RefCountedPtr &other)
    : ptr_(other.ptr_), refCount_(other.refCount_)
{
    if (refCount_)
        ++*refCount_;
}

ios_base::failure::failure(const string &msg)
    : what_(msg.c_str())
{
}

locale ios_base::imbue(const locale &loc)
{
    locale old = *loc_;
    *loc_ = loc;
    do_callbacks(imbue_event);
    return old;
}

#pragma opt_strength_reduction off
ios_base::~ios_base()
{
    do_callbacks(erase_event);
    delete loc_;
    free(parray_);
    free(iarray_);
    free(cb_vec_);
}
#pragma opt_strength_reduction reset

void ios_base::init(void *sb)
{
    rdbuf_ = sb;
    exceptions_ = goodbit;
    iostate_ = badbit;
    iostate_ = sb ? goodbit : badbit;
    fmtflags_ = fmtflags(skipws | dec);
    width_ = 0;
    precision_ = 6;
    loc_ = 0;
    try {
        loc_ = new locale;
    } catch (...) {
    }
    if (loc_ == 0)
        iostate_ = badbit;
    cb_vec_ = 0;
    cb_siz_ = 0;
    cb_cap_ = 0;
    iarray_ = 0;
    isize_ = 0;
    parray_ = 0;
    psize_ = 0;
}

const char *ios_base::failure::what() const
{
    return what_.c_str();
}

}
