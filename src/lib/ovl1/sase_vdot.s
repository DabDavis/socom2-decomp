# sc: compiler=ee-gcc2.96
# Nellymoser's speech codec (OVL1 0x248F60): the encoder's dot product, hand-written in the R5900's
# FPU accumulator instructions -- mula.s for the first pair, madda.s through the loop (two pairs
# counted off before it), madd.s folding the last pair into $f0 in the return's slot. GCC never emits
# the accumulator's mula.s/madda.s from C: the routine is assembly, or a C function whose whole body
# is an asm (libvu0's shape: GNU as's hazard nops after the loads, the asm's last instruction in the
# return's slot) -- the same bytes either way (GCC round 16a; the unit's name is ours). Its float
# pointers in a0/a1 and the count in a2 (at least two) are the EABI's.

	.set	noreorder
	.set	noat
	.set	nomacro

	.text
	.align	4

	.globl	func_00248F60
	.type	func_00248F60,@function
	.ent	func_00248F60
func_00248F60:
	lwc1	$f0,0($4)
	addiu	$4,$4,4
	lwc1	$f1,0($5)
	addiu	$5,$5,4
	addiu	$6,$6,-2
	mula.s	$f0,$f1
.L00248F78:
	lwc1	$f0,0($4)
	addiu	$4,$4,4
	lwc1	$f1,0($5)
	addiu	$5,$5,4
	addiu	$6,$6,-1
	madda.s	$f0,$f1
	bgtz	$6,.L00248F78
	sll	$0,$0,0x0
	lwc1	$f0,0($4)
	sll	$0,$0,0x0
	lwc1	$f1,0($5)
	sll	$0,$0,0x0
	jr	$31
	madd.s	$f0,$f0,$f1
	.end	func_00248F60
	.size	func_00248F60,.-func_00248F60
