# sc: compiler=ee-gcc2.96
# newlib's memchr for the EE (MAIN, the libc the game linked: README "The GCC front"): one
# assembler object, as the demo's memchr is -- its .text section symbol carries no gcc2_compiled.
# marker, where every compiled libc object has one; 128-bit lq/sq and MMI byte compares over
# aligned quadwords, a byte loop for the rest. Transcribed by tools/asmunit.py.
# Byte-identical to the demo's.

	.set	noreorder
	.set	noat
	.set	nomacro

	.text
	.align	3

	.globl	memchr
	.type	memchr,@function
	.ent	memchr
memchr:
	sltiu	$2,$6,16
	bne	$2,$0,.L00195728
	andi	$5,$5,0xff
	andi	$2,$4,0xf
	bne	$2,$0,.L00195728
	daddu	$7,$4,$0
	dsll	$3,$5,0x8
	lui	$2,0x101
	ori	$2,$2,0x101
	dsll	$2,$2,0x10
	ori	$2,$2,0x101
	dsll	$2,$2,0x10
	ori	$2,$2,0x101
	daddu	$10,$3,$5
	lui	$3,0x8080
	ori	$3,$3,0x8080
	dsll	$3,$3,0x10
	ori	$3,$3,0x8080
	dsll	$3,$3,0x10
	ori	$3,$3,0x8080
	pcpyh	$8,$10
	pcpyld	$9,$8,$8
	daddu	$4,$2,$0
	pcpyld	$8,$3,$3
.L001956E8:
	lq	$2,0($7)
	pxor	$2,$2,$9
	pcpyld	$10,$4,$4
	pnor	$3,$0,$2
	psubb	$2,$2,$10
	pand	$2,$2,$3
	pand	$2,$2,$8
	pcpyud	$3,$2,$9
	or	$2,$2,$3
	bnel	$2,$0,.L00195728
	daddu	$4,$7,$0
	addiu	$6,$6,-16
	sltiu	$2,$6,16
	beq	$2,$0,.L001956E8
	addiu	$7,$7,16
	daddu	$4,$7,$0
.L00195728:
	lui	$2,0xffff
	addiu	$6,$6,-1
	ori	$2,$2,0xffff
	beq	$6,$2,.L00195758
	sll	$0,$0,0x0
	lui	$3,0xffff
	ori	$3,$3,0xffff
.L00195744:
	lbu	$2,0($4)
	beq	$2,$5,.L00195760
	addiu	$6,$6,-1
	bne	$6,$3,.L00195744
	addiu	$4,$4,1
.L00195758:
	jr	$31
	daddu	$2,$0,$0
.L00195760:
	jr	$31
	daddu	$2,$4,$0
	.end	memchr
	.size	memchr,.-memchr
