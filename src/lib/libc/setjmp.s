# sc: compiler=ee-gcc2.96
# newlib's machine/mips/setjmp.S (MAIN 0x198320-0x1983D8), the toolchain's libc: setjmp and
# longjmp, an assembler source in newlib itself (the R5900 build saves the callee-saved GPRs with
# sq, the whole 128 bits, and $f24-$f31 with swc1, into a 208-byte jmp_buf). Between _sbrk_r's
# and copysignf's objects, as the alphabetical link order puts setjmp.o. longjmp returns 1 for a
# 0 value. Named by body (the save and restore of every callee-saved register through a0).

	.set	noreorder
	.set	noat
	.set	nomacro

	.text
	.align	4

	.globl	setjmp
	.type	setjmp,@function
	.ent	setjmp
setjmp:
	sq	$16,0($4)
	sq	$17,16($4)
	sq	$18,32($4)
	sq	$19,48($4)
	sq	$20,64($4)
	sq	$21,80($4)
	sq	$22,96($4)
	sq	$23,112($4)
	sq	$30,128($4)
	swc1	$f24,144($4)
	swc1	$f25,148($4)
	swc1	$f26,152($4)
	swc1	$f27,156($4)
	swc1	$f28,160($4)
	swc1	$f29,164($4)
	swc1	$f30,168($4)
	swc1	$f31,172($4)
	sq	$29,176($4)
	sq	$31,192($4)
	daddu	$2,$0,$0
	jr	$31
	sll	$0,$0,0x0
	.end	setjmp
	.size	setjmp,.-setjmp

	.globl	longjmp
	.type	longjmp,@function
	.ent	longjmp
longjmp:
	lq	$16,0($4)
	lq	$17,16($4)
	lq	$18,32($4)
	lq	$19,48($4)
	lq	$20,64($4)
	lq	$21,80($4)
	lq	$22,96($4)
	lq	$23,112($4)
	lq	$30,128($4)
	lwc1	$f24,144($4)
	lwc1	$f25,148($4)
	lwc1	$f26,152($4)
	lwc1	$f27,156($4)
	lwc1	$f28,160($4)
	lwc1	$f29,164($4)
	lwc1	$f30,168($4)
	lwc1	$f31,172($4)
	lq	$29,176($4)
	lq	$31,192($4)
	bne	$5,$0,.L001983D0
	sll	$0,$0,0x0
	addiu	$5,$0,1
.L001983D0:
	daddu	$2,$5,$0
	jr	$31
	sll	$0,$0,0x0
	.end	longjmp
	.size	longjmp,.-longjmp
