# sc: compiler=ee-gcc2.9-991111
# libkernl's TLB and debug exception handlers (MAIN 0x1AC500-0x1AC85C): Sony's SDK 2.7, the
# library's one assembler object besides the syscall stubs (src/lib/sdk/syscall.s). The demo links
# the same object at 0x163E00-0x164160 -- its .text section symbol without a gcc2_compiled. marker
# -- with the same three functions at the same offsets and the global label _xlaunch at +0x100,
# identical but for the relocated fields; its local labels name the save area (hstack) and the
# saved hi/lo/hi1/lo1/sa/epc words (EERH0 ... EEEPC), defined here in the unit's own .bss as there.
# _kTLBException saves the GPRs (128-bit, sq) and the multiply/shift state, then erets into
# _xlaunch, which calls the handler SetTLBHandler installed (_kTLBRefillHandler) on the handler
# stack; _kExitTLBHandler restores all of it, _kDebugException is the debug path through
# _kDebugHandler. Transcribed by tools/asmunit.py.

	.set	noreorder
	.set	noat
	.set	nomacro

	.text
	.align	4

	.globl	_kTLBException
	.type	_kTLBException,@function
	.ent	_kTLBException
_kTLBException:
	lui	$26,%hi(hstack)
	addiu	$26,$26,%lo(hstack)
	sq	$1,16($26)
	sq	$2,32($26)
	sq	$3,48($26)
	sq	$4,64($26)
	sq	$5,80($26)
	sq	$6,96($26)
	sq	$7,112($26)
	sq	$8,128($26)
	sq	$9,144($26)
	sq	$10,160($26)
	sq	$11,176($26)
	sq	$12,192($26)
	sq	$13,208($26)
	sq	$14,224($26)
	sq	$15,240($26)
	sq	$16,256($26)
	sq	$17,272($26)
	sq	$18,288($26)
	sq	$19,304($26)
	sq	$20,320($26)
	sq	$21,336($26)
	sq	$22,352($26)
	sq	$23,368($26)
	sq	$24,384($26)
	sq	$25,400($26)
	sq	$28,448($26)
	sq	$29,464($26)
	sq	$30,480($26)
	sq	$31,496($26)
	mfhi	$2
	lui	$1,%hi(EERH0)
	sd	$2,%lo(EERH0)($1)
	mfhi1	$2
	lui	$1,%hi(EERH1)
	sd	$2,%lo(EERH1)($1)
	mflo	$2
	lui	$1,%hi(EERL0)
	sd	$2,%lo(EERL0)($1)
	mflo1	$2
	lui	$1,%hi(EERL1)
	sd	$2,%lo(EERL1)($1)
	mfsa	$2
	lui	$1,%hi(EERSA)
	sd	$2,%lo(EERSA)($1)
	mfc0	$4,$12
	mfc0	$5,$13
	mfc0	$6,$14
	mfc0	$7,$8
	lui	$8,%hi(hstack)
	addiu	$8,$8,%lo(hstack)
	lui	$1,%hi(EEEPC)
	sw	$6,%lo(EEEPC)($1)
	lui	$1,%hi(_xlaunch)
	addiu	$1,$1,%lo(_xlaunch)
	mtc0	$1,$14
	sync.p
	mfc0	$1,$12
	addiu	$2,$0,-2
	and	$1,$1,$2
	mtc0	$1,$12
	sync.p
	eret
	.globl	_xlaunch
_xlaunch:
	lui	$1,%hi(_kTLBRefillHandler)
	lw	$1,%lo(_kTLBRefillHandler)($1)
	lui	$29,%hi(hstack)
	jalr	$1
	addiu	$29,$29,%lo(hstack)
	addiu	$3,$0,-84
	syscall
	.end	_kTLBException
	.size	_kTLBException,.-_kTLBException
	.space	36

	.globl	_kExitTLBHandler
	.type	_kExitTLBHandler,@function
	.ent	_kExitTLBHandler
_kExitTLBHandler:
	mfc0	$1,$12
	addiu	$26,$0,-28
	and	$1,$1,$26
	mtc0	$1,$12
	sync.p
	lui	$2,%hi(EEEPC)
	lw	$2,%lo(EEEPC)($2)
	mtc0	$2,$14
	sync.p
	lui	$2,%hi(EERH0)
	ld	$2,%lo(EERH0)($2)
	mthi	$2
	lui	$2,%hi(EERH1)
	ld	$2,%lo(EERH1)($2)
	mthi1	$2
	lui	$2,%hi(EERL0)
	ld	$2,%lo(EERL0)($2)
	mtlo	$2
	lui	$2,%hi(EERL1)
	ld	$2,%lo(EERL1)($2)
	mtlo1	$2
	lui	$2,%hi(EERSA)
	ld	$2,%lo(EERSA)($2)
	mtsa	$2
	sync.p
	lui	$26,%hi(hstack)
	addiu	$26,$26,%lo(hstack)
	lq	$1,16($26)
	lq	$2,32($26)
	lq	$3,48($26)
	lq	$4,64($26)
	lq	$5,80($26)
	lq	$6,96($26)
	lq	$7,112($26)
	lq	$8,128($26)
	lq	$9,144($26)
	lq	$10,160($26)
	lq	$11,176($26)
	lq	$12,192($26)
	lq	$13,208($26)
	lq	$14,224($26)
	lq	$15,240($26)
	lq	$16,256($26)
	lq	$17,272($26)
	lq	$18,288($26)
	lq	$19,304($26)
	lq	$20,320($26)
	lq	$21,336($26)
	lq	$22,352($26)
	lq	$23,368($26)
	lq	$24,384($26)
	lq	$25,400($26)
	lq	$28,448($26)
	lq	$29,464($26)
	lq	$30,480($26)
	lq	$31,496($26)
	mfc0	$26,$12
	ori	$26,$26,0x13
	mtc0	$26,$12
	sync.p
	eret
	.end	_kExitTLBHandler
	.size	_kExitTLBHandler,.-_kExitTLBHandler
	.space	12

	.globl	_kDebugException
	.type	_kDebugException,@function
	.ent	_kDebugException
_kDebugException:
	lui	$26,%hi(hstack)
	addiu	$26,$26,%lo(hstack)
	sq	$1,16($26)
	sq	$2,32($26)
	sq	$3,48($26)
	sq	$4,64($26)
	sq	$5,80($26)
	sq	$6,96($26)
	sq	$7,112($26)
	sq	$8,128($26)
	sq	$9,144($26)
	sq	$10,160($26)
	sq	$11,176($26)
	sq	$12,192($26)
	sq	$13,208($26)
	sq	$14,224($26)
	sq	$15,240($26)
	sq	$16,256($26)
	sq	$17,272($26)
	sq	$18,288($26)
	sq	$19,304($26)
	sq	$20,320($26)
	sq	$21,336($26)
	sq	$22,352($26)
	sq	$23,368($26)
	sq	$24,384($26)
	sq	$25,400($26)
	sq	$28,448($26)
	sq	$29,464($26)
	sq	$30,480($26)
	sq	$31,496($26)
	mfhi	$2
	lui	$1,%hi(EERH0)
	sd	$2,%lo(EERH0)($1)
	mfhi1	$2
	lui	$1,%hi(EERH1)
	sd	$2,%lo(EERH1)($1)
	mflo	$2
	lui	$1,%hi(EERL0)
	sd	$2,%lo(EERL0)($1)
	mflo1	$2
	lui	$1,%hi(EERL1)
	sd	$2,%lo(EERL1)($1)
	mfsa	$2
	lui	$1,%hi(EERSA)
	sd	$2,%lo(EERSA)($1)
	mfc0	$4,$12
	mfc0	$5,$13
	mfc0	$6,$14
	mfc0	$7,$8
	mfc0	$8,$23
	lui	$9,%hi(hstack)
	addiu	$9,$9,%lo(hstack)
	lui	$1,%hi(_kDebugException+252)
	addiu	$1,$1,%lo(_kDebugException+252)
	mtc0	$1,$14
	sync.p
	mfc0	$1,$12
	addiu	$2,$0,-2
	and	$1,$1,$2
	mtc0	$1,$12
	sync.p
	eret
	andi	$2,$5,0x7c
	lui	$1,0x1d
	addu	$1,$1,$2
	lw	$1,-12112($1)
	lui	$29,%hi(hstack)
	jalr	$1
	addiu	$29,$29,%lo(hstack)
	break	0x3ff,0x3ff
	.end	_kDebugException
	.size	_kDebugException,.-_kDebugException

	.bss
	.align	4
hstack:
	.space	512
EERH0:
	.space	8
EERH1:
	.space	8
EERL0:
	.space	8
EERL1:
	.space	8
EERSA:
	.space	8
EEEPC:
	.space	8
