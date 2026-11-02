# sc: compiler=ee-gcc2.96
# newlib's memcmp for the EE (MAIN, the libc the game linked: README "The GCC front"): one
# assembler object, as the demo's memcmp is -- its .text section symbol carries no gcc2_compiled.
# marker, where every compiled libc object has one; 128-bit lq/sq and MMI byte compares over
# aligned quadwords, a byte loop for the rest. Transcribed by tools/asmunit.py.
# Byte-identical to the demo's.

	.set	noreorder
	.set	noat
	.set	nomacro

	.text
	.align	3

	.globl	memcmp
	.type	memcmp,@function
	.ent	memcmp
memcmp:
	sltiu	$2,$6,16
	bne	$2,$0,.L001957B4
	or	$2,$4,$5
	andi	$2,$2,0xf
	bne	$2,$0,.L001957B4
	sll	$0,$0,0x0
.L00195780:
	lq	$3,0($4)
	sltiu	$7,$6,32
	lq	$2,0($5)
	addiu	$4,$4,16
	pxor	$8,$2,$3
	addiu	$2,$5,16
	pcpyud	$10,$8,$7
	or	$9,$10,$8
	movz	$5,$2,$9
	bnel	$9,$0,.L001957B4
	addiu	$4,$4,-16
	beq	$7,$0,.L00195780
	addiu	$6,$6,-16
.L001957B4:
	lui	$2,0xffff
	addiu	$6,$6,-1
	ori	$2,$2,0xffff
	beq	$6,$2,.L001957F4
	sll	$0,$0,0x0
	lui	$7,0xffff
	ori	$7,$7,0xffff
.L001957D0:
	lbu	$3,0($4)
	lbu	$2,0($5)
	beq	$3,$2,.L001957E8
	addiu	$4,$4,1
	jr	$31
	subu	$2,$3,$2
.L001957E8:
	addiu	$6,$6,-1
	bne	$6,$7,.L001957D0
	addiu	$5,$5,1
.L001957F4:
	jr	$31
	daddu	$2,$0,$0
	.end	memcmp
	.size	memcmp,.-memcmp
