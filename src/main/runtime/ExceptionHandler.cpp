/* sc: compiler=mwcps2-3.0b52-030722; flags=-O4,p -sdatathreshold 0 -inline auto,deferred,bottomup,level=5 -enum min -Cpp_exceptions on */
/* ExceptionHandler.cpp -- Metrowerks' C++ exception handling, the table walk (MAIN 0x182930-0x18411B):
 * find the function a return address belongs to, step through its actions, destroy what a throw
 * unwinds past, find the handler, honour exception specifications. The PowerPC runtime's
 * ExceptionPPC.cp is the same algorithm (its names are used here); MIPS encodes the tables
 * differently -- every number a variable-length field (__DecodeUnsignedNumber/__DecodeSignedNumber),
 * every function pointer four unaligned bytes -- so each action is decoded before it is used. SOCOM
 * 1's runtime names every function (0x142C90-0x1443F0, the same twelve in the same order; no DWARF:
 * the runtime was prebuilt); the file name is ours. The frames and registers are ExceptionMIPS.cpp's.
 *
 * The image emits the unit reversed (deferred), as the runtime's other units. */

#include "ExceptionMIPS.h"

namespace std {

class exception {
public:
    exception() {}
    virtual ~exception() {}
    virtual const char *what() const { return "exception"; }
};

class bad_exception : public exception {
public:
    bad_exception() {}
    virtual ~bad_exception() {}
    virtual const char *what() const;
};

void terminate();
void unexpected();

}

extern "C" char __throw_catch_compare(const char *throwtype, const char *catchtype, long *offset_result);

#define EXACTION_ENDBIT 0x80
#define EXACTION_MASK 0x1F

#define EXACTION_ENDOFLIST 0
#define EXACTION_BRANCH 1
#define EXACTION_DESTROYLOCAL 2
#define EXACTION_DESTROYLOCALCOND 3
#define EXACTION_DESTROYLOCALPOINTER 4
#define EXACTION_DESTROYLOCALARRAY 5
#define EXACTION_DESTROYBASE 6
#define EXACTION_DESTROYMEMBER 7
#define EXACTION_DESTROYMEMBERCOND 8
#define EXACTION_DESTROYMEMBERARRAY 9
#define EXACTION_DELETEPOINTER 10
#define EXACTION_DELETEPOINTERCOND 11
#define EXACTION_CATCHBLOCK 12
#define EXACTION_ACTIVECATCHBLOCK 13
#define EXACTION_TERMINATE 14
#define EXACTION_SPECIFICATION 15

/* the action byte's flags: the condition, the object pointer, lives in a register */
#define EXACTION_CONDINREG 0x40
#define EXACTION_PTRINREG 0x20

typedef unsigned char exaction_type;

typedef void (*DeleteFunc)(void *);
typedef void (*DestroyFunc)(void *, short);

/* an exception table entry: a function's range, its unwind information (in place when the size's
 * low bit is set) */
typedef struct ExceptionTableIndex {
    char *function;
    unsigned int size;
    char *unwind;
} ExceptionTableIndex;

typedef struct ex_catchblock {
    char *catch_type;
    unsigned int catch_pcoffset;
    int cinfo_ref;
} ex_catchblock;

typedef struct ex_specification {
    unsigned int specs;
    unsigned int pcoffset;
    int cinfo_ref;
    char *spec;
} ex_specification;

/* a function pointer in an action: four unaligned bytes */
#define GET_ADDRESS(p) \
    (((unsigned char *)(p))[0] | (((unsigned char *)(p))[1] << 8) | (((unsigned char *)(p))[2] << 16) | (((unsigned char *)(p))[3] << 24))

#define DTORCALL_COMPLETE(dtor, objptr) (((DestroyFunc)(dtor))(objptr, -1))
#define DTORCALL_PARTIAL(dtor, objptr) (((DestroyFunc)(dtor))(objptr, 0))

const char *std::bad_exception::what() const
{
    return "bad_exception";
}

/* the table entry whose function holds the address (binary search), or 0 */
static inline ExceptionTableIndex *FindTableIndex(ExceptionTableIndex *table, int count, char *returnaddr)
{
    ExceptionTableIndex *p;
    int i, m, n;

    for (i = 0, n = count - 1; i <= n;) {
        p = &table[m = (i + n) >> 1];
        if (returnaddr < p->function)
            n = m - 1;
        else if (returnaddr > p->function + (p->size & ~1))
            i = m + 1;
        else
            return p;
    }
    return 0;
}

static void FindExceptionRecord(char *returnaddr, ExceptionInfo *info)
{
    ExceptionTableIndex *p;
    unsigned int returnoffset, offset, skip, size, action;
    char *ptr;

    info->exception_record = 0;
    info->action_pointer = 0;

    if (!__FindExceptionTable(info, returnaddr))
        return;

    returnaddr = (char *)((int)returnaddr & ~1L);
    p = FindTableIndex((ExceptionTableIndex *)info->table_start,
                       (ExceptionTableIndex *)info->table_end - (ExceptionTableIndex *)info->table_start, returnaddr);
    if (p == 0)
        return;

    if (p->size & 1)
        info->exception_record = (char *)&p->unwind;
    else
        info->exception_record = p->unwind;
    info->code_start = p->function;

    returnoffset = returnaddr - p->function;
    ptr = __SkipUnwindInfo(info->exception_record);
    for (offset = 0;;) {
        ptr = __DecodeUnsignedNumber(ptr, &skip);
        if (skip == 0)
            return;
        ptr = __DecodeUnsignedNumber(ptr, &size);
        ptr = __DecodeUnsignedNumber(ptr, &action);
        offset += skip;
        if (returnoffset < offset)
            return;
        offset += size;
        if (returnoffset <= offset)
            break;
    }
    info->action_pointer = info->exception_record + action;
}

static exaction_type NextAction(ActionIterator *iter)
{
    ThrowContext *context = &iter->context;
    char *p;
    exaction_type action;

    for (;;) {
        if (!(p = iter->info.action_pointer) || (*(unsigned char *)p & EXACTION_ENDBIT)) {
            FindExceptionRecord(__PopStackFrame(context, &iter->info), &iter->info);
            if (iter->info.exception_record == 0)
                std::terminate();
            __SetupFrameInfo(context, &iter->info);
            if (iter->info.action_pointer == 0)
                continue;
        } else {
        switch (*(unsigned char *)p & EXACTION_MASK) {
        case EXACTION_DESTROYLOCAL: {
            int local;
            p = __DecodeSignedNumber(p + 1, &local);
            iter->info.action_pointer = p + 4;
            break;
        }
        case EXACTION_DESTROYLOCALCOND: {
            int cond, local;
            p = __DecodeSignedNumber(p + 1, &cond);
            p = __DecodeSignedNumber(p, &local);
            iter->info.action_pointer = p + 4;
            break;
        }
        case EXACTION_DESTROYLOCALPOINTER: {
            int pointer;
            p = __DecodeSignedNumber(p + 1, &pointer);
            iter->info.action_pointer = p + 4;
            break;
        }
        case EXACTION_DESTROYLOCALARRAY: {
            int localarray;
            unsigned int elements, element_size;
            p = __DecodeSignedNumber(p + 1, &localarray);
            p = __DecodeUnsignedNumber(p, &elements);
            p = __DecodeUnsignedNumber(p, &element_size);
            iter->info.action_pointer = p + 4;
            break;
        }
        case EXACTION_DESTROYBASE: {
            int objectptr, offset;
            p = __DecodeSignedNumber(p + 1, &objectptr);
            p = __DecodeSignedNumber(p, &offset);
            iter->info.action_pointer = p + 4;
            break;
        }
        case EXACTION_DESTROYMEMBER: {
            int objectptr, offset;
            p = __DecodeSignedNumber(p + 1, &objectptr);
            p = __DecodeSignedNumber(p, &offset);
            iter->info.action_pointer = p + 4;
            break;
        }
        case EXACTION_DESTROYMEMBERCOND: {
            int cond, objectptr, offset;
            p = __DecodeSignedNumber(p + 1, &cond);
            p = __DecodeSignedNumber(p, &objectptr);
            p = __DecodeSignedNumber(p, &offset);
            iter->info.action_pointer = p + 4;
            break;
        }
        case EXACTION_DESTROYMEMBERARRAY: {
            int objectptr, offset;
            unsigned int elements, element_size;
            p = __DecodeSignedNumber(p + 1, &objectptr);
            p = __DecodeSignedNumber(p, &offset);
            p = __DecodeUnsignedNumber(p, &elements);
            p = __DecodeUnsignedNumber(p, &element_size);
            iter->info.action_pointer = p + 4;
            break;
        }
        case EXACTION_DELETEPOINTER: {
            int objectptr;
            p = __DecodeSignedNumber(p + 1, &objectptr);
            iter->info.action_pointer = p + 4;
            break;
        }
        case EXACTION_DELETEPOINTERCOND: {
            int cond, objectptr;
            p = __DecodeSignedNumber(p + 1, &cond);
            p = __DecodeSignedNumber(p, &objectptr);
            iter->info.action_pointer = p + 4;
            break;
        }
        case EXACTION_CATCHBLOCK: {
            char *catch_type;
            unsigned int catch_pcoffset;
            int cinfo_ref;
            *(unsigned int *)&catch_type = GET_ADDRESS(p + 1);
            p = __DecodeUnsignedNumber(p + 5, &catch_pcoffset);
            iter->info.action_pointer = __DecodeSignedNumber(p, &cinfo_ref);
            break;
        }
        case EXACTION_ACTIVECATCHBLOCK: {
            int cinfo_ref;
            iter->info.action_pointer = __DecodeSignedNumber(p + 1, &cinfo_ref);
            break;
        }
        case EXACTION_SPECIFICATION: {
            unsigned int specs, pcoffset;
            int cinfo_ref;
            p = __DecodeUnsignedNumber(p + 1, &specs);
            p = __DecodeUnsignedNumber(p, &pcoffset);
            p = __DecodeSignedNumber(p, &cinfo_ref);
            iter->info.action_pointer = p + specs * 4;
            break;
        }
        default:
            std::terminate();
        }
        }

        while ((action = *(unsigned char *)iter->info.action_pointer & EXACTION_MASK) == EXACTION_BRANCH) {
            int target;
            __DecodeSignedNumber(iter->info.action_pointer + 1, &target);
            iter->info.action_pointer += target;
        }
        return action;
    }
}

static void UnwindStack(ThrowContext *context, ExceptionInfo *info, char *catcher)
{
    char *ap;
    exaction_type action;

    for (;;) {
        if ((ap = info->action_pointer) == 0) {
            FindExceptionRecord(__PopStackFrame(context, info), info);
            if (info->exception_record == 0)
                std::terminate();
            __SetupFrameInfo(context, info);
            if ((ap = info->action_pointer) == 0)
                continue;
        }

        action = *ap;
        switch (action & EXACTION_MASK) {
        case EXACTION_BRANCH: {
            int target;
            __DecodeSignedNumber(ap + 1, &target);
            info->action_pointer += target;
            break;
        }
        case EXACTION_DESTROYLOCAL: {
            int local;
            char *p = __DecodeSignedNumber(ap + 1, &local);
            DTORCALL_COMPLETE(GET_ADDRESS(p), context->FP + local);
            info->action_pointer = p + 4;
            break;
        }
        case EXACTION_DESTROYLOCALCOND: {
            int inreg = *(unsigned char *)ap & EXACTION_CONDINREG;
            int cond, local;
            void *dtor;
            char c;
            char *p, *next;
            p = __DecodeSignedNumber(ap + 1, &cond);
            p = __DecodeSignedNumber(p, &local);
            dtor = (void *)GET_ADDRESS(p);
            next = p + 4;
            if (inreg)
                c = (char)context->regs.GPR[cond];
            else
                c = *(char *)(context->FP + cond);
            if (c)
                DTORCALL_COMPLETE(dtor, context->FP + local);
            info->action_pointer = next;
            break;
        }
        case EXACTION_DESTROYLOCALPOINTER: {
            int inreg = *(unsigned char *)ap & EXACTION_PTRINREG;
            int pointer;
            void *dtor;
            void *object;
            char *p, *next;
            p = __DecodeSignedNumber(ap + 1, &pointer);
            dtor = (void *)GET_ADDRESS(p);
            next = p + 4;
            if (inreg)
                object = (void *)(int)context->regs.GPR[pointer];
            else
                object = *(void **)(context->FP + pointer);
            DTORCALL_COMPLETE(dtor, object);
            info->action_pointer = next;
            break;
        }
        case EXACTION_DESTROYLOCALARRAY: {
            int localarray;
            unsigned int elements, element_size;
            void *dtor;
            char *ptr;
            unsigned int n;
            char *p, *next;
            p = __DecodeSignedNumber(ap + 1, &localarray);
            p = __DecodeUnsignedNumber(p, &elements);
            p = __DecodeUnsignedNumber(p, &element_size);
            dtor = (void *)GET_ADDRESS(p);
            next = p + 4;
            ptr = context->FP + localarray;
            n = elements;
            for (ptr += n * element_size; n > 0; n--) {
                ptr -= element_size;
                DTORCALL_COMPLETE(dtor, ptr);
            }
            info->action_pointer = next;
            break;
        }
        case EXACTION_DESTROYBASE: {
            int inreg = *(unsigned char *)ap & EXACTION_PTRINREG;
            int objectptr, offset;
            void *dtor;
            char *object;
            char *p, *next;
            p = __DecodeSignedNumber(ap + 1, &objectptr);
            p = __DecodeSignedNumber(p, &offset);
            dtor = (void *)GET_ADDRESS(p);
            next = p + 4;
            if (inreg)
                object = (char *)(int)context->regs.GPR[objectptr];
            else
                object = *(char **)(context->FP + objectptr);
            DTORCALL_PARTIAL(dtor, object + offset);
            info->action_pointer = next;
            break;
        }
        case EXACTION_DESTROYMEMBER: {
            int inreg = *(unsigned char *)ap & EXACTION_PTRINREG;
            int objectptr, offset;
            void *dtor;
            char *object;
            char *p, *next;
            p = __DecodeSignedNumber(ap + 1, &objectptr);
            p = __DecodeSignedNumber(p, &offset);
            dtor = (void *)GET_ADDRESS(p);
            next = p + 4;
            if (inreg)
                object = (char *)(int)context->regs.GPR[objectptr];
            else
                object = *(char **)(context->FP + objectptr);
            DTORCALL_COMPLETE(dtor, object + offset);
            info->action_pointer = next;
            break;
        }
        case EXACTION_DESTROYMEMBERCOND: {
            int inreg = *(unsigned char *)ap & EXACTION_PTRINREG;
            int cond, objectptr, offset;
            void *dtor;
            char c;
            char *object;
            char *p, *next;
            p = __DecodeSignedNumber(ap + 1, &cond);
            p = __DecodeSignedNumber(p, &objectptr);
            p = __DecodeSignedNumber(p, &offset);
            dtor = (void *)GET_ADDRESS(p);
            next = p + 4;
            if (action & EXACTION_CONDINREG)
                c = (char)context->regs.GPR[cond];
            else
                c = *(char *)(context->FP + cond);
            if (c) {
                if (inreg)
                    object = (char *)(int)context->regs.GPR[objectptr];
                else
                    object = *(char **)(context->FP + objectptr);
                DTORCALL_COMPLETE(dtor, object + offset);
            }
            info->action_pointer = next;
            break;
        }
        case EXACTION_DESTROYMEMBERARRAY: {
            int inreg = *(unsigned char *)ap & EXACTION_PTRINREG;
            int objectptr, offset;
            unsigned int elements, element_size;
            void *dtor;
            char *ptr;
            unsigned int n;
            char *p, *next;
            p = __DecodeSignedNumber(ap + 1, &objectptr);
            p = __DecodeSignedNumber(p, &offset);
            p = __DecodeUnsignedNumber(p, &elements);
            p = __DecodeUnsignedNumber(p, &element_size);
            dtor = (void *)GET_ADDRESS(p);
            next = p + 4;
            if (inreg)
                ptr = (char *)(int)context->regs.GPR[objectptr] + offset;
            else
                ptr = *(char **)(context->FP + objectptr) + offset;
            n = elements;
            for (ptr += n * element_size; n > 0; n--) {
                ptr -= element_size;
                DTORCALL_COMPLETE(dtor, ptr);
            }
            info->action_pointer = next;
            break;
        }
        case EXACTION_DELETEPOINTER: {
            int inreg = *(unsigned char *)ap & EXACTION_PTRINREG;
            int objectptr;
            void *deletefunc;
            void *object;
            char *p, *next;
            p = __DecodeSignedNumber(ap + 1, &objectptr);
            deletefunc = (void *)GET_ADDRESS(p);
            next = p + 4;
            if (inreg)
                object = (void *)(int)context->regs.GPR[objectptr];
            else
                object = *(void **)(context->FP + objectptr);
            ((DeleteFunc)deletefunc)(object);
            info->action_pointer = next;
            break;
        }
        case EXACTION_DELETEPOINTERCOND: {
            int inreg = *(unsigned char *)ap & EXACTION_PTRINREG;
            int cond, objectptr;
            void *deletefunc;
            char c;
            void *object;
            char *p, *next;
            p = __DecodeSignedNumber(ap + 1, &cond);
            p = __DecodeSignedNumber(p, &objectptr);
            deletefunc = (void *)GET_ADDRESS(p);
            next = p + 4;
            if (action & EXACTION_CONDINREG)
                c = (char)context->regs.GPR[cond];
            else
                c = *(char *)(context->FP + cond);
            if (c) {
                if (inreg)
                    object = (void *)(int)context->regs.GPR[objectptr];
                else
                    object = *(void **)(context->FP + objectptr);
                ((DeleteFunc)deletefunc)(object);
            }
            info->action_pointer = next;
            break;
        }
        case EXACTION_CATCHBLOCK: {
            char *catch_type;
            unsigned int catch_pcoffset;
            int cinfo_ref;
            char *p;
            if (catcher == ap)
                return;
            *(unsigned int *)&catch_type = GET_ADDRESS(ap + 1);
            p = __DecodeUnsignedNumber(ap + 5, &catch_pcoffset);
            info->action_pointer = __DecodeSignedNumber(p, &cinfo_ref);
            break;
        }
        case EXACTION_ACTIVECATCHBLOCK: {
            int cinfo_ref;
            CatchInfo *catchinfo;
            char *p;
            p = __DecodeSignedNumber(ap + 1, &cinfo_ref);
            catchinfo = (CatchInfo *)(context->FP + cinfo_ref);
            if (catchinfo->dtor) {
                if (context->location == catchinfo->location)
                    context->dtor = catchinfo->dtor;
                else
                    DTORCALL_COMPLETE(catchinfo->dtor, catchinfo->location);
            }
            info->action_pointer = p;
            break;
        }
        case EXACTION_SPECIFICATION: {
            unsigned int specs, pcoffset;
            int cinfo_ref;
            char *p;
            if (catcher == ap)
                return;
            p = __DecodeUnsignedNumber(ap + 1, &specs);
            p = __DecodeUnsignedNumber(p, &pcoffset);
            p = __DecodeSignedNumber(p, &cinfo_ref);
            info->action_pointer = p + specs * 4;
            break;
        }
        default:
            std::terminate();
        }

        if (action & EXACTION_ENDBIT)
            info->action_pointer = 0;
    }
}

static inline exaction_type CurrentAction(const ActionIterator *iter)
{
    if (iter->info.action_pointer != 0)
        return *(unsigned char *)iter->info.action_pointer & EXACTION_MASK;
    return EXACTION_ENDOFLIST;
}

static CatchInfo *FindMostRecentException(ThrowContext *context, ExceptionInfo *info)
{
    ActionIterator iter;
    exaction_type action;
    CatchInfo *catchinfo;
    int cinfo_ref;

    iter.info = *info;
    iter.context = *context;

    for (action = CurrentAction(&iter);; action = NextAction(&iter)) {
        switch (action) {
        case EXACTION_ACTIVECATCHBLOCK:
            break;
        case EXACTION_ENDOFLIST:
        case EXACTION_DESTROYLOCAL:
        case EXACTION_DESTROYLOCALCOND:
        case EXACTION_DESTROYLOCALPOINTER:
        case EXACTION_DESTROYLOCALARRAY:
        case EXACTION_DESTROYBASE:
        case EXACTION_DESTROYMEMBER:
        case EXACTION_DESTROYMEMBERCOND:
        case EXACTION_DESTROYMEMBERARRAY:
        case EXACTION_DELETEPOINTER:
        case EXACTION_DELETEPOINTERCOND:
        case EXACTION_CATCHBLOCK:
        case EXACTION_SPECIFICATION:
            continue;
        case EXACTION_TERMINATE:
        default:
            std::terminate();
        }
        break;
    }

    __DecodeSignedNumber(iter.info.action_pointer + 1, &cinfo_ref);
    catchinfo = (CatchInfo *)(iter.context.FP + cinfo_ref);
    context->throwtype = (char *)catchinfo->typeinfo;
    context->location = catchinfo->location;
    context->dtor = 0;
    context->catchinfo = catchinfo;
    return catchinfo;
}

static inline void DecodeSpecification(char *p, ex_specification *spec)
{
    p = __DecodeUnsignedNumber(p + 1, &spec->specs);
    p = __DecodeUnsignedNumber(p, &spec->pcoffset);
    p = __DecodeSignedNumber(p, &spec->cinfo_ref);
    spec->spec = p;
}

static inline int IsInSpecification(char *extype, ex_specification *spec)
{
    long offset;
    char *p;
    unsigned int i;

    for (i = 0, p = spec->spec; i < spec->specs; i++, p += 4) {
        char *type;
        *(unsigned int *)&type = GET_ADDRESS(p);
        if (__throw_catch_compare(extype, type, &offset))
            return 1;
    }
    return 0;
}

extern "C" void __unexpected(CatchInfo *catchinfo)
{
    char *unexp = (char *)catchinfo->stacktop;
    ex_specification spec;

#pragma exception_magic
    try {
        std::unexpected();
    } catch (...) {
        DecodeSpecification(unexp, &spec);
        if (IsInSpecification((char *)((CatchInfo *)&__exception_magic)->typeinfo, &spec))
            throw;
        if (IsInSpecification("!std::bad_exception!!", &spec))
            throw std::bad_exception();
    }
    std::terminate();
}

/* the catch block's fields: the type string (0 for catch (...)) through memory and relocated -- by
 * nothing on this target, which leaves its test -- then the handler's offset and its CatchInfo's */
static inline char *DecodeCatchBlock(char *p, ex_catchblock *cb)
{
    char *q = p + 5;

    *(unsigned int *)&cb->catch_type = GET_ADDRESS(p + 1);
    cb->catch_type = cb->catch_type ? cb->catch_type : 0;
    p = __DecodeUnsignedNumber(q, &cb->catch_pcoffset);
    return __DecodeSignedNumber(p, &cb->cinfo_ref);
}

static char *FindExceptionHandler(ThrowContext *context, ExceptionInfo *info, long *result_offset)
{
    ActionIterator iter;
    exaction_type action;

    iter.info = *info;
    iter.context = *context;

    for (action = CurrentAction(&iter);; action = NextAction(&iter)) {
        switch (action) {
        case EXACTION_CATCHBLOCK: {
            ex_catchblock cb;
            DecodeCatchBlock(iter.info.action_pointer, &cb);
            if (!__throw_catch_compare(context->throwtype, cb.catch_type, result_offset))
                continue;
            break;
        }
        case EXACTION_SPECIFICATION: {
            ex_specification spec;
            DecodeSpecification(iter.info.action_pointer, &spec);
            if (!IsInSpecification(context->throwtype, &spec)) {
                char *unexp = iter.info.action_pointer;
                CatchInfo *catchinfo;

                UnwindStack(context, info, unexp);
                catchinfo = (CatchInfo *)(context->FP + spec.cinfo_ref);
                catchinfo->location = context->location;
                catchinfo->typeinfo = context->throwtype;
                catchinfo->dtor = context->dtor;
                catchinfo->stacktop = unexp;
                __TransferControl(context, info, info->code_start + spec.pcoffset);
            }
            continue;
        }
        case EXACTION_ENDOFLIST:
        case EXACTION_DESTROYLOCAL:
        case EXACTION_DESTROYLOCALCOND:
        case EXACTION_DESTROYLOCALPOINTER:
        case EXACTION_DESTROYLOCALARRAY:
        case EXACTION_DESTROYBASE:
        case EXACTION_DESTROYMEMBER:
        case EXACTION_DESTROYMEMBERCOND:
        case EXACTION_DESTROYMEMBERARRAY:
        case EXACTION_DELETEPOINTER:
        case EXACTION_DELETEPOINTERCOND:
        case EXACTION_ACTIVECATCHBLOCK:
            continue;
        case EXACTION_TERMINATE:
        default:
            std::terminate();
        }
        break;
    }
    return iter.info.action_pointer;
}

void __ThrowHandler(ThrowContext *context)
{
    ExceptionInfo info;
    char *catchblock;
    ex_catchblock cb;
    CatchInfo *catchinfo;
    long offset;
    long cinfo_ref, off;

    FindExceptionRecord(context->returnaddr, &info);
    if (info.exception_record == 0)
        std::terminate();
    __SetupFrameInfo(context, &info);

    if (context->throwtype == 0)
        context->catchinfo = FindMostRecentException(context, &info);
    else
        context->catchinfo = 0;

    catchblock = FindExceptionHandler(context, &info, &offset);
    DecodeCatchBlock(catchblock, &cb);
    UnwindStack(context, &info, catchblock);

    cinfo_ref = cb.cinfo_ref;
    off = offset;
    catchinfo = (CatchInfo *)(context->FP + cinfo_ref);
    catchinfo->location = context->location;
    catchinfo->typeinfo = context->throwtype;
    catchinfo->dtor = context->dtor;
    if (*context->throwtype == '*') {
        catchinfo->sublocation = &catchinfo->pointercopy;
        catchinfo->pointercopy = *(int *)context->location + off;
    } else {
        catchinfo->sublocation = (char *)context->location + off;
    }

    __TransferControl(context, &info, info.code_start + cb.catch_pcoffset);
}

extern "C" void __end__catch(CatchInfo *catchinfo)
{
    if (catchinfo->location && catchinfo->dtor)
        DTORCALL_COMPLETE(catchinfo->dtor, catchinfo->location);
}
