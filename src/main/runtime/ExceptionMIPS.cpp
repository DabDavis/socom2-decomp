/* sc: compiler=mwcps2-3.0b52-030722; flags=-O4,p -sdatathreshold 0 -inline auto,deferred,bottomup,level=5 -enum min -Cpp_exceptions on */
/* ExceptionMIPS.cpp -- Metrowerks' C++ exception handling, the MIPS half (MAIN 0x184120-0x1845F7): the
 * frame a return address leads to, the registers saved in it, and the two assembly entries -- __throw,
 * which saves every register into a ThrowContext on its own stack and calls __ThrowHandler, and
 * __TransferControl, which reloads them and jumps into the catch handler. SOCOM 1's runtime names
 * every function (0x1443F0-0x144890, the same six in the same order; no DWARF: the runtime was
 * prebuilt); the file name is ours.
 *
 * A unit of its own: ExceptionHandler.cpp's FindExceptionRecord calls __FindExceptionTable (two
 * stores) out of line, which a deferred auto-inliner does only across units. MW emits an `asm`
 * function at once, in source order, and defers the C ones to the unit's end, reversed: this order
 * gives the image's (__TransferControl, __throw, then __SkipUnwindInfo .. __PopStackFrame). The asm
 * rows were 0x70 and 0x80 in the seeded map, zero padding counted in (SOCOM 1's symbols: 0x64, 0x7C).
 *
 * The frame (MW's MIPS prologue): the saved s-registers from FP + saveoffset up, 16 bytes each (`sq`),
 * after the saved $f20.. rounded up to 16; then s8 when it is the frame pointer, then ra.
 *
 * The build (round 7): __PopStackFrame is the runtime's one row that separates 3.0b38 from 3.0b50 and
 * 3.0b52 -- byte-exact under b50 and b52, 0x2A8 under b38 against the target's 0x2B8. */

#include "ExceptionMIPS.h"

extern char __exception_table_start__[];
extern char __exception_table_end__[];

char *__PopStackFrame(ThrowContext *context, ExceptionInfo *info)
{
    unsigned long saveoffset;
    int hasfprs;
    unsigned long framesize;
    unsigned long nGPRs;
    char *returnaddr;
    __int128 *gpr;
    int i;
    unsigned long nFPRs;
    int *fpr;

    /* the declarations' order is the registers' (the last declared colours first: fpr v1, nFPRs a1,
     * i a2, gpr a3, nGPRs t0, framesize t1, hasfprs t2); the loads' is the schedule's -- framesize
     * first, or saveoffset lands in the branch's delay slot (lperm.py over the 24 orders: one) */
    framesize = context->regs.framesize;
    saveoffset = context->regs.saveoffset;
    nGPRs = context->regs.nGPRs;
    hasfprs = *info->exception_record & 0x40;

    if (hasfprs) {
        nFPRs = context->regs.nFPRs;
        fpr = (int *)(context->FP + saveoffset + ((nFPRs * 4 + 15) & ~15));
        /* through an integer: the target zero-extends fpr and adds in 64 bits (`dsrl32`, `daddu`),
         * where the pointer sum below adds the truncated offset */
        gpr = (__int128 *)((unsigned long)fpr + nGPRs * 16);
    } else {
        gpr = (__int128 *)(context->FP + saveoffset + nGPRs * 16);
    }

    if (context->regs.hasFP)
        gpr++;
    returnaddr = *(char **)gpr;

    if (context->regs.hasFP) {
        gpr--;
        context->regs.GPR[30] = *gpr;
    } else if (nGPRs == 9) {
        gpr--;
        nGPRs--;
        context->regs.GPR[30] = *gpr;
    }

    /* typed pointers: the target scales the count after truncating it (`sll 4`, `sll 2`), and the
     * unrolled copies step the source every second element -- byte pointers fold every step */
    gpr -= nGPRs;
    for (i = 0; i < nGPRs; i++) {
        context->regs.GPR[16 + i] = *gpr;
        gpr++;
    }

    context->SP = context->FP + framesize;

    if (hasfprs) {
        fpr -= nFPRs;
        for (i = 0; i < nFPRs; i++) {
            context->regs.FPR[20 + i] = *fpr;
            fpr++;
        }
    }

    return returnaddr;
}

void __SetupFrameInfo(ThrowContext *context, ExceptionInfo *info)
{
    char *p = info->exception_record;
    int hasfprs = *p & 0x40;

    context->regs.hasFP = *p & 0x20;
    context->regs.flag10 = *p & 0x10;
    context->regs.nGPRs = *p & 0x0F;
    p = __DecodeUnsignedNumber(p + 1, &context->regs.framesize);
    p = __DecodeUnsignedNumber(p, &context->regs.saveoffset);

    if (context->regs.hasFP)
        context->FP = (char *)context->regs.GPR[30];
    else
        context->FP = context->SP;

    if (hasfprs)
        __DecodeUnsignedNumber(p, &context->regs.nFPRs);
    else
        context->regs.nFPRs = 0;
}

int __FindExceptionTable(ExceptionInfo *info, char *returnaddr)
{
    info->table_start = __exception_table_start__;
    info->table_end = __exception_table_end__;
    return 1;
}

char *__SkipUnwindInfo(char *p)
{
    unsigned int skip;
    int hasfprs = *p & 0x40;

    p = __DecodeUnsignedNumber(p + 1, &skip);
    p = __DecodeUnsignedNumber(p, &skip);
    if (hasfprs)
        p = __DecodeUnsignedNumber(p, &skip);
    return p;
}

/* into the handler: the callee-saved registers back, the stack below the throw's (the exception
 * object lives in the thrower's frames) by the handler's register-save offset, then the jump */
asm void __TransferControl(ThrowContext *context, ExceptionInfo *info, char *address)
{
    .set noreorder
    lq s0, 288(a0)
    lq s1, 304(a0)
    lq s2, 320(a0)
    lq s3, 336(a0)
    lq s4, 352(a0)
    lq s5, 368(a0)
    lq s6, 384(a0)
    lq s7, 400(a0)
    lq s8, 512(a0)
    lwc1 $f20, 648(a0)
    lwc1 $f21, 652(a0)
    lwc1 $f22, 656(a0)
    lwc1 $f23, 660(a0)
    lwc1 $f24, 664(a0)
    lwc1 $f25, 668(a0)
    lwc1 $f26, 672(a0)
    lwc1 $f27, 676(a0)
    lwc1 $f28, 680(a0)
    lwc1 $f29, 684(a0)
    lwc1 $f30, 688(a0)
    lwc1 $f31, 692(a0)
    lw sp, 28(a0)
    lw v0, 548(a0)
    jr a2
    sub sp, sp, v0
}

/* the throw: a ThrowContext on this frame -- the arguments, the return address, the caller's SP twice
 * (the frame being unwound, and where the catch handler's stack will start), the callee-saved
 * registers -- handed to __ThrowHandler, which never returns */
extern "C" asm void __throw(char *throwtype, void *location, void *dtor)
{
    .set noreorder
    paddub v0, sp, zero
    addiu sp, sp, -704
    sq s0, 288(sp)
    sq s1, 304(sp)
    sq s2, 320(sp)
    sq s3, 336(sp)
    sq s4, 352(sp)
    sq s5, 368(sp)
    sq s6, 384(sp)
    sq s7, 400(sp)
    sq s8, 512(sp)
    swc1 $f20, 648(sp)
    swc1 $f21, 652(sp)
    swc1 $f22, 656(sp)
    swc1 $f23, 660(sp)
    swc1 $f24, 664(sp)
    swc1 $f25, 668(sp)
    swc1 $f26, 672(sp)
    swc1 $f27, 676(sp)
    swc1 $f28, 680(sp)
    swc1 $f29, 684(sp)
    swc1 $f30, 688(sp)
    swc1 $f31, 692(sp)
    sw v0, 20(sp)
    sw v0, 28(sp)
    sw ra, 16(sp)
    sw a0, 0(sp)
    sw a1, 4(sp)
    sw a2, 8(sp)
    jal __ThrowHandler
    paddub a0, sp, zero
}
