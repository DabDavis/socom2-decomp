# sc: compiler=ee-gcc2.96
# newlib's strncmp for the EE (MAIN, the libc the game linked: README "The GCC front"): one
# assembler object, as the demo's strncmp is -- its .text section symbol carries no gcc2_compiled.
# marker, where every compiled libc object has one; 128-bit lq/sq and MMI byte compares over
# aligned quadwords, a byte loop for the rest. Transcribed by tools/asmunit.py.
# Byte-identical to the demo's.

	.set	noreorder
	.set	noat
	.set	nomacro

	.text
	.align	3

	.globl	strncmp
	.type	strncmp,@function
	.ent	strncmp
strncmp:
	bne	$6,$0,.L00199658
	or	$3,$4,$5
.L00199650:
	jr	$31
	daddu	$2,$0,$0
.L00199658:
	andi	$2,$3,0x7
	bne	$2,$0,.L001997B4
	daddu	$2,$6,$0
	andi	$2,$3,0xf
	sltiu	$7,$6,16
	lui	$9,0x101
	ori	$9,$9,0x101
	dsll	$9,$9,0x10
	ori	$9,$9,0x101
	dsll	$9,$9,0x10
	ori	$9,$9,0x101
	or	$2,$2,$7
	bne	$2,$0,.L00199730
	daddu	$7,$4,$0
	lq	$3,0($4)
	pcpyld	$10,$9,$9
	lq	$2,0($5)
	lui	$8,0x8080
	ori	$8,$8,0x8080
	dsll	$8,$8,0x10
	ori	$8,$8,0x8080
	dsll	$8,$8,0x10
	ori	$8,$8,0x8080
	psubw	$3,$3,$2
	pcpyld	$9,$8,$8
	pcpyud	$2,$3,$4
	daddu	$8,$5,$0
	or	$3,$2,$3
	bne	$3,$0,.L001997B4
	daddu	$2,$6,$0
	addiu	$6,$6,-16
.L001996D4:
	beq	$6,$0,.L00199650
	sll	$0,$0,0x0
	lq	$2,0($7)
	pnor	$3,$0,$2
	psubb	$2,$2,$10
	pand	$2,$2,$3
	pand	$3,$2,$9
	pcpyud	$2,$3,$4
	or	$2,$2,$3
	bne	$2,$0,.L00199650
	addiu	$7,$7,16
	sltiu	$2,$6,16
	lq	$3,0($7)
	bne	$2,$0,.L001997A8
	addiu	$8,$8,16
	lq	$2,0($8)
	psubw	$3,$3,$2
	pcpyud	$2,$3,$4
	or	$2,$2,$3
	beql	$2,$0,.L001996D4
	addiu	$6,$6,-16
	beq	$0,$0,.L001997AC
	daddu	$4,$7,$0
.L00199730:
	sltiu	$2,$6,8
	bne	$2,$0,.L001997A8
	daddu	$8,$5,$0
	ld	$3,0($4)
	ld	$2,0($5)
	bne	$3,$2,.L001997B4
	daddu	$2,$6,$0
	addiu	$6,$6,-8
	lui	$10,0x8080
	ori	$10,$10,0x8080
	dsll	$10,$10,0x10
	ori	$10,$10,0x8080
	dsll	$10,$10,0x10
	ori	$10,$10,0x8080
.L00199768:
	beq	$6,$0,.L00199650
	sll	$0,$0,0x0
	ld	$2,0($7)
	nor	$3,$0,$2
	dsubu	$2,$2,$9
	and	$2,$2,$3
	and	$2,$2,$10
	bne	$2,$0,.L00199650
	addiu	$7,$7,8
	sltiu	$2,$6,8
	bne	$2,$0,.L001997A8
	addiu	$8,$8,8
	ld	$3,0($7)
	ld	$2,0($8)
	beql	$3,$2,.L00199768
	addiu	$6,$6,-8
.L001997A8:
	daddu	$4,$7,$0
.L001997AC:
	daddu	$5,$8,$0
	daddu	$2,$6,$0
.L001997B4:
	beq	$2,$0,.L001997F0
	addiu	$6,$6,-1
	beq	$0,$0,.L001997E4
	lb	$3,0($4)
	sll	$0,$0,0x0
.L001997C8:
	beq	$6,$0,.L00199650
	sll	$0,$0,0x0
	beq	$7,$0,.L00199650
	addiu	$4,$4,1
	addiu	$5,$5,1
	lb	$3,0($4)
	addiu	$6,$6,-1
.L001997E4:
	lb	$2,0($5)
	beq	$3,$2,.L001997C8
	lbu	$7,0($4)
.L001997F0:
	lbu	$2,0($5)
	lbu	$3,0($4)
	jr	$31
	subu	$2,$3,$2
	.end	strncmp
	.size	strncmp,.-strncmp
