/* ExceptionMIPS.h -- the structures Metrowerks' MIPS exception handling shares between its two units
 * (ExceptionHandler.cpp, the table walk; ExceptionMIPS.cpp, the frames and registers). The layouts are
 * the image's: every offset below is one the code addresses. The names follow the PowerPC runtime's
 * (ExceptionPPC.cp, as the GameCube/Wii decompilations reconstruct it) where a field does the same job;
 * the file name and the frame fields' names are ours. Types are the PowerPC source's where the image
 * agrees: its `long`s are this compiler's 64-bit long (the decoded offsets, the catch offset). */

#ifndef EXCEPTIONMIPS_H
#define EXCEPTIONMIPS_H

/* the catch block's view of the exception (the PowerPC's CatchInfo; the pointer copy 32-bit here) */
typedef struct CatchInfo {
    void *location;     /* 0: the thrown object */
    void *typeinfo;     /* 4: its type string */
    void *dtor;         /* 8: its destructor */
    void *sublocation;  /* 12: the object as the handler's type sees it */
    int pointercopy;    /* 16: a thrown pointer, adjusted */
    void *stacktop;     /* 20: __unexpected's specification */
} CatchInfo;

/* the function an address belongs to and where its actions start (FindExceptionRecord) */
typedef struct ExceptionInfo {
    char *code_start;        /* 0: the function's first instruction */
    char *exception_record;  /* 4: its unwind information, then its ranges */
    char *action_pointer;    /* 8: the action list of the range holding the address */
    char *table_start;       /* 12: the exception table (__FindExceptionTable) */
    char *table_end;         /* 16 */
    struct {
        void *unused;
    } reserved;              /* 20: copied, never read -- a one-word struct: MW copies it through $f0
                              * by its address (`addiu v0,sp,N; swc1 $f0,0(v0)`), a float field by sp */
} ExceptionInfo;

/* the registers, and the frame they were found in (__SetupFrameInfo decodes the frame from the
 * function's unwind information, __PopStackFrame restores the saved registers out of it) */
typedef struct RegisterContext {
    __int128 GPR[32];           /* 32 */
    unsigned int framesize;     /* 544: the frame's size (FP + framesize is the caller's SP) */
    unsigned int saveoffset;    /* 548: the saved registers' offset in the frame */
    int nGPRs;                  /* 552: s-registers saved (s8 the ninth when there is no frame pointer) */
    int flag10;                 /* 556: the unwind byte's 0x10 */
    int hasFP;                  /* 560: s8 is the frame pointer */
    unsigned int nFPRs;         /* 564: $f20.. saved */
    int FPR[32];                /* 568 */
} RegisterContext;

/* __throw's frame: the throw's arguments, the frame being unwound, every register */
typedef struct ThrowContext {
    char *throwtype;        /* 0 */
    void *location;         /* 4 */
    void *dtor;             /* 8 */
    CatchInfo *catchinfo;   /* 12: a rethrow's exception */
    char *returnaddr;       /* 16 */
    char *SP;               /* 20 */
    char *FP;               /* 24 */
    char *throwSP;          /* 28 */
    RegisterContext regs;   /* 32 */
} ThrowContext;

typedef struct ActionIterator {
    ExceptionInfo info;
    ThrowContext context;
} ActionIterator;

char *__DecodeUnsignedNumber(char *p, unsigned int *result);
char *__DecodeSignedNumber(char *p, int *result);
char *__PopStackFrame(ThrowContext *context, ExceptionInfo *info);
void __SetupFrameInfo(ThrowContext *context, ExceptionInfo *info);
int __FindExceptionTable(ExceptionInfo *info, char *returnaddr);
char *__SkipUnwindInfo(char *p);
void __TransferControl(ThrowContext *context, ExceptionInfo *info, char *address);
void __ThrowHandler(ThrowContext *context);
extern "C" void __throw(char *throwtype, void *location, void *dtor);

#endif
