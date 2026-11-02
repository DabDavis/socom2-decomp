# sc: compiler=ee-gcc2.96
# newlib's memcpy for the EE (MAIN, the libc the game linked: README "The GCC front"): one
# assembler object, as the demo's memcpy is -- its .text section symbol carries no gcc2_compiled.
# marker, where every compiled libc object has one; 128-bit lq/sq and MMI byte compares over
# aligned quadwords, a byte loop for the rest. Transcribed by tools/asmunit.py.
# Against the demo's: identical but for one short loop's tail -- the demo's object puts a nop
# before the loop's branch and the increment in its delay slot (sb; nop; bne; addiu), this one
# the increment before the branch and a nop in the slot (sb; addiu; bne; nop).

	.set	noreorder
	.set	noat
	.set	nomacro

	.text
	.align	4

	.globl	memcpy
	.type	memcpy,@function
	.ent	memcpy
memcpy:
	daddu	$8,$4,$0
	sltiu	$2,$6,32
	bne	$2,$0,.L0019587C
	daddu	$3,$8,$0
	or	$2,$5,$8
	andi	$2,$2,0xf
	bnel	$2,$0,.L00195880
	addiu	$6,$6,-1
	daddu	$7,$8,$0
.L00195824:
	lq	$3,0($5)
	addiu	$6,$6,-32
	addiu	$5,$5,16
	sltiu	$4,$6,32
	sq	$3,0($7)
	addiu	$7,$7,16
	lq	$2,0($5)
	addiu	$5,$5,16
	sq	$2,0($7)
	beq	$4,$0,.L00195824
	addiu	$7,$7,16
	sltiu	$2,$6,8
	bne	$2,$0,.L0019587C
	daddu	$3,$7,$0
.L0019585C:
	ld	$3,0($5)
	addiu	$6,$6,-8
	addiu	$5,$5,8
	sltiu	$2,$6,8
	sd	$3,0($7)
	beq	$2,$0,.L0019585C
	addiu	$7,$7,8
	daddu	$3,$7,$0
.L0019587C:
	addiu	$6,$6,-1
.L00195880:
	addiu	$2,$0,-1
	beq	$6,$2,.L001958A8
	daddu	$4,$2,$0
.L0019588C:
	lbu	$2,0($5)
	addiu	$6,$6,-1
	addiu	$5,$5,1
	sb	$2,0($3)
	addiu	$3,$3,1
	bne	$6,$4,.L0019588C
	sll	$0,$0,0x0
.L001958A8:
	jr	$31
	daddu	$2,$8,$0
	.end	memcpy
	.size	memcpy,.-memcpy
