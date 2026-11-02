# sc: compiler=ee-gcc2.96
# newlib's strncpy for the EE (MAIN, the libc the game linked: README "The GCC front"): one
# assembler object, as the demo's strncpy is -- its .text section symbol carries no gcc2_compiled.
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

	.globl	strncpy
	.type	strncpy,@function
	.ent	strncpy
strncpy:
	daddu	$8,$4,$0
	or	$7,$5,$4
	addiu	$10,$0,16
	andi	$2,$7,0x7
	addiu	$9,$0,8
	bne	$2,$0,.L00199968
	andi	$2,$7,0xf
	movz	$9,$10,$2
	bne	$2,$0,.L001998D4
	sltu	$2,$6,$9
	bne	$2,$0,.L00199968
	sll	$0,$0,0x0
	lui	$7,0x101
	ori	$7,$7,0x101
	dsll	$7,$7,0x10
	ori	$7,$7,0x101
	dsll	$7,$7,0x10
	ori	$7,$7,0x101
	lq	$3,0($5)
	pcpyld	$9,$7,$7
	pnor	$3,$0,$3
	lui	$7,0x8080
	ori	$7,$7,0x8080
	dsll	$7,$7,0x10
	ori	$7,$7,0x8080
	dsll	$7,$7,0x10
	ori	$7,$7,0x8080
	psubb	$2,$3,$9
	pcpyld	$10,$7,$7
	pand	$2,$2,$3
	pand	$2,$2,$10
	pcpyud	$3,$2,$4
	or	$3,$2,$3
	bne	$3,$0,.L00199964
	daddu	$7,$8,$0
	lq	$3,0($5)
	addiu	$6,$6,-16
	addiu	$5,$5,16
	sltiu	$2,$6,16
	sq	$3,0($7)
	bne	$2,$0,.L00199964
	addiu	$7,$7,16
	lq	$2,0($5)
	pnor	$3,$0,$2
	psubb	$2,$2,$9
	pand	$2,$2,$3
	pand	$2,$2,$10
	pcpyud	$3,$2,$4
	or	$2,$2,$3
	beql	$2,$0,.L00199930
	lq	$3,0($5)
	beq	$0,$0,.L00199968
	daddu	$4,$7,$0
.L001998D4:
	bne	$2,$0,.L00199968
	sll	$0,$0,0x0
	ld	$3,0($5)
	lui	$9,0x101
	ori	$9,$9,0x101
	dsll	$9,$9,0x10
	ori	$9,$9,0x101
	dsll	$9,$9,0x10
	ori	$9,$9,0x101
	lui	$10,0x8080
	ori	$10,$10,0x8080
	dsll	$10,$10,0x10
	ori	$10,$10,0x8080
	dsll	$10,$10,0x10
	ori	$10,$10,0x8080
	dsubu	$2,$3,$9
	nor	$3,$0,$3
	and	$2,$2,$3
	and	$2,$2,$10
	bne	$2,$0,.L00199964
	daddu	$7,$8,$0
	ld	$3,0($5)
	sll	$0,$0,0x0
.L00199930:
	addiu	$6,$6,-8
	addiu	$5,$5,8
	sltiu	$2,$6,8
	sd	$3,0($7)
	bne	$2,$0,.L00199964
	addiu	$7,$7,8
	ld	$2,0($5)
	nor	$3,$0,$2
	dsubu	$2,$2,$9
	and	$2,$2,$3
	and	$2,$2,$10
	beql	$2,$0,.L00199930
	ld	$3,0($5)
.L00199964:
	daddu	$4,$7,$0
.L00199968:
	beq	$6,$0,.L001999B4
	daddu	$2,$6,$0
	lbu	$2,0($5)
	addiu	$6,$6,-1
	addiu	$5,$5,1
	sb	$2,0($4)
	sll	$2,$2,0x18
	bne	$2,$0,.L00199968
	addiu	$4,$4,1
	daddu	$2,$6,$0
	beq	$2,$0,.L001999B4
	addiu	$6,$6,-1
.L00199998:
	sb	$0,0($4)
	daddu	$2,$6,$0
	addiu	$4,$4,1
	addiu	$6,$6,-1
	sll	$0,$0,0x0
	bne	$2,$0,.L00199998
	sll	$0,$0,0x0
.L001999B4:
	jr	$31
	daddu	$2,$8,$0
	.end	strncpy
	.size	strncpy,.-strncpy
