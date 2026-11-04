/* sc: compiler=mwcps2-3.0b52-030722; flags=-O4,p -sdatathreshold 0 -inline auto,deferred,bottomup,level=5 -enum min -Cpp_exceptions on */
/* NMWException.cp -- Metrowerks' C++ runtime: the array constructor/destructor helpers every
 * `new T[n]`/`delete[]` and static array calls, the terminate/unexpected handlers, the throw's
 * type compare (MAIN 0x181E30-0x18280F).
 *
 * The file name is the PowerPC runtime's (PowerPC_EABI_Support/Runtime/Src/NMWException.cp, as the
 * GameCube/Wii decompilations reconstruct it): the same functions in the same order, which this
 * image emits REVERSED -- deferred code generation, as the game's own units (README "The
 * compiler"); std::set_terminate/set_unexpected are dead-stripped. The MIPS ABI is the image's:
 * a destructor takes (this, short) and runs complete with -1; the element count and size are
 * size_t; the partial-array destructor's five words sit at sp+144 in __construct_array.
 *
 * THE COMPILER (README "MAIN's Metrowerks runtime"): 3.0b50 or 3.0b52 -- __destroy_arr and
 * __destroy_new_array are byte-exact only under 3.0b22-020926, 3.0b38, 3.0b50 and 3.0b52,
 * __initialize_cpp_rts's short-loop nops rule out 3.0b22-020926, and round 7: ExceptionMIPS.cpp's
 * __PopStackFrame 0x184340 rules out 3.0b38 (0x2A8 there against the target's 0x2B8; the whole
 * runtime is alike under b50 and b52, build/scratch/rt/r7/cmpbuilds.py); the game's 3.0b52 here. The
 * game's flags but -Cpp_exceptions ON: with it off, std::terminate and std::unexpected become
 * tail calls (`jr v0`, 9 words off); the target keeps their frames. */

typedef unsigned int size_t;
typedef void *ConstructorDestructor;

#define CTORCALL_COMPLETE(ctor, objptr) (((void (*)(void *, short))ctor)(objptr, 1))
#define DTORCALL_COMPLETE(dtor, objptr) (((void (*)(void *, short))dtor)(objptr, -1))
#define ARRAY_HEADER_SIZE 16

extern "C" void abort(void);
void operator delete[](void *);

namespace std {

typedef void (*terminate_handler)();
typedef void (*unexpected_handler)();
void terminate();
void unexpected();

static void dthandler() { abort(); }               /* 0x1827F0 */

static terminate_handler thandler = dthandler;     /* 0x1CA400 */

static void duhandler() { terminate(); }           /* 0x1827C0: terminate() inlined */

static unexpected_handler uhandler = duhandler;    /* 0x1CA408 */

void terminate()                                   /* 0x182790 */
{
    thandler();
}

void unexpected()                                  /* 0x182760 */
{
    uhandler();
}

}

namespace std {

class exception {
public:
    exception() {}
    virtual ~exception() {}                                         /* 0x182410 */
    virtual const char *what() const { return "exception"; }        /* 0x1824D0 */
};

class bad_cast : public exception {
public:
    bad_cast() {}
    virtual ~bad_cast() {}                                          /* 0x182460 */
    virtual const char *what() const { return "bad_cast"; }         /* 0x1824C0 */
};

}

/* MWRTTI's structures (the PowerPC runtime's MWRTTI.h), the offsets 32-bit as the image loads them */
typedef struct type_info_struct type_info_struct;

typedef struct type_info_base_list {
    type_info_struct *baseTypeInfo;
    int offset;
} type_info_base_list;

typedef struct type_info_ambighead {
    void *baseTypeInfo;
    int offset;
    int bases;
} type_info_ambighead;

struct type_info_struct {
    char *typeName;
    type_info_base_list *baseList;
};

typedef struct RTTIVTableHeader {
    type_info_struct *typeInfo;
    int offset;
} RTTIVTableHeader;

/* inlined at every use: equal names give 1 (the PowerPC's returns the difference) */
/* the increments inside the tests (round 6): MW then steps each pointer in a branch's delay slot and
 * loops back on the first name's byte -- the target's top-tested compare, inlined four times into
 * __dynamic_cast; the PowerPC's `s1++; s2++;` after both tests gives a rotated loop 0x10 longer */
static int strequal(const char *s1, const char *s2)
{
    for (;;) {
        if (*s1 != *s2++)
            return 0;
        if (*s1++ == 0)
            return 1;
    }
}

/* MWRTTI.cp's __dynamic_cast, the PowerPC's adapted (byte-exact since round 6, the name compare
 * respelled above): the s8 frame (the throw), the offset sign-extended to a long before the add
 * (`(long)offset`), the 32-bit table fields (lw), the 64-bit `temp` sum (daddu), the throw
 * (`__throw("!std::exception!!std::bad_cast!!", &obj, ~bad_cast)` at s8+44). */
extern "C" void *__dynamic_cast(void *obj, int offset, type_info_struct *typeInfo, type_info_struct *subtypeInfo,
                                bool isReference)
{
    RTTIVTableHeader *vtable_header;
    type_info_base_list *list;
    void *result;
    int i;
    int bases;
    long temp;

    if (obj == 0)
        return 0;

    vtable_header = *(RTTIVTableHeader **)((char *)obj + (long)offset);

    if (vtable_header->typeInfo != 0) {
        result = (char *)obj + vtable_header->offset;
        if (typeInfo == 0 || strequal(vtable_header->typeInfo->typeName, typeInfo->typeName))
            return result;

        list = vtable_header->typeInfo->baseList;

        if (list != 0) {
            for (; list->baseTypeInfo; list++) {
                if (list->offset & 0x80000000) {
                    temp = (list->offset & 0x7FFFFFFF);
                    bases = ((type_info_ambighead *)list)->bases;

                    if (vtable_header->offset + temp == 0
                        && strequal(list->baseTypeInfo->typeName, typeInfo->typeName)) {
                        list = (type_info_base_list *)((type_info_ambighead *)list + 1);

                        for (i = 0; i < bases; i++) {
                            if (vtable_header->offset + list->offset == 0
                                && strequal(list->baseTypeInfo->typeName, subtypeInfo->typeName))
                                return (char *)result + list->offset;
                            list++;
                        }
                        break;
                    } else {
                        list = (type_info_base_list *)((type_info_ambighead *)list + 1);
                        list += bases - 1;
                    }
                } else {
                    if (strequal(list->baseTypeInfo->typeName, typeInfo->typeName))
                        return (char *)result + list->offset;
                }
            }
        }
    }

    if (isReference)
        throw std::bad_cast();

    return 0;
}

extern "C" char __throw_catch_compare(const char *throwtype, const char *catchtype, long *offset_result)
{
    const char *cptr1, *cptr2;

    *offset_result = 0;

    if ((cptr2 = catchtype) == 0)
        return true;

    cptr1 = throwtype;

    if (*cptr2 == 'P') {
        cptr2++;
        if (*cptr2 == 'C')
            cptr2++;
        if (*cptr2 == 'V')
            cptr2++;
        if (*cptr2 == 'v') {
            if (*cptr1 == 'P' || *cptr1 == '*')
                return true;
        }
        cptr2 = catchtype;
    }

    switch (*cptr1) {
    case '*':
    case '!':
        if (*cptr1++ != *cptr2++)
            return false;
        for (;;) {
            if (*cptr1 == *cptr2++) {
                if (*cptr1++ == '!') {
                    long offset;

                    for (offset = 0; *cptr1 != '!';)
                        offset = offset * 10 + *cptr1++ - '0';
                    *offset_result = offset;
                    return true;
                }
            } else {
                while (*cptr1++ != '!')
                    ;
                while (*cptr1++ != '!')
                    ;
                if (*cptr1 == 0)
                    return false;

                cptr2 = catchtype + 1;
            }
        }
        return false;
    }

    while ((*cptr1 == 'P' || *cptr1 == 'R') && *cptr1 == *cptr2) {
        cptr1++;
        cptr2++;

        if (*cptr2 == 'C') {
            if (*cptr1 == 'C')
                cptr1++;
            cptr2++;
        }
        if (*cptr1 == 'C')
            return false;

        if (*cptr2 == 'V') {
            if (*cptr1 == 'V')
                cptr1++;
            cptr2++;
        }
        if (*cptr1 == 'V')
            return false;
    }

    for (; *cptr1 == *cptr2; cptr1++, cptr2++) {
        if (*cptr1 == 0)
            return true;
    }

    return false;
}

class __partial_array_destructor {
private:
    void *p;
    size_t size;
    size_t n;
    ConstructorDestructor dtor;

public:
    size_t i;

    __partial_array_destructor(void *array, size_t elementsize, size_t nelements, ConstructorDestructor destructor)
    {
        p = array;
        size = elementsize;
        n = nelements;
        dtor = destructor;
        i = n;
    }

    ~__partial_array_destructor()
    {
        char *ptr;

        if (i < n && dtor) {
            for (ptr = (char *)p + size * i; i > 0; i--) {
                ptr -= size;
                DTORCALL_COMPLETE(dtor, ptr);
            }
        }
    }
};

extern "C" void *__construct_new_array(void *block, ConstructorDestructor ctor, ConstructorDestructor dtor,
                                       size_t size, size_t n)
{
    char *ptr;

    if ((ptr = (char *)block) != 0) {
        size_t *p = (size_t *)ptr;

        p[0] = size;
        p[1] = n;
        ptr += ARRAY_HEADER_SIZE;

        if (ctor) {
            __partial_array_destructor pad(ptr, size, n, dtor);
            char *p;

            for (pad.i = 0, p = (char *)ptr; pad.i < n; pad.i++, p += size)
                CTORCALL_COMPLETE(ctor, p);
        }
    }
    return ptr;
}

extern "C" void __construct_array(void *ptr, ConstructorDestructor ctor, ConstructorDestructor dtor, size_t size, size_t n)
{
    __partial_array_destructor pad(ptr, size, n, dtor);
    char *p;

    for (pad.i = 0, p = (char *)ptr; pad.i < n; pad.i++, p += size)
        CTORCALL_COMPLETE(ctor, p);
}

extern "C" void __destroy_arr(void *block, ConstructorDestructor *dtor, size_t size, size_t n)
{
    char *p;

    for (p = (char *)block + size * n; n > 0; n--) {
        p -= size;
        DTORCALL_COMPLETE(dtor, p);
    }
}

extern "C" void __destroy_new_array(void *block, ConstructorDestructor dtor)
{
    if (block) {
        if (dtor) {
            size_t i, objects, objectsize;
            char *p;

            objectsize = *(size_t *)((char *)block - ARRAY_HEADER_SIZE);
            objects = ((size_t *)((char *)block - ARRAY_HEADER_SIZE))[1];
            p = (char *)block + (objectsize * objects);

            for (i = 0; i < objects; i++) {
                p -= objectsize;
                DTORCALL_COMPLETE(dtor, p);
            }
        }
        ::operator delete[]((char *)block - ARRAY_HEADER_SIZE);
    }
}
