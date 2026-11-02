# sc: compiler=ee-gcc2.96
# newlib's strcpy for the EE (MAIN, the libc the game linked: README "The GCC front"): one
# assembler object, as the demo's strcpy is -- its .text section symbol carries no gcc2_compiled.
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

	.globl	strcpy
	.type	strcpy,@function
	.ent	strcpy
strcpy:
	daddu	$7,$4,$0
	or	$8,$5,$7
	andi	$2,$8,0x7
	bne	$2,$0,.L00199150
	daddu	$3,$7,$0
	andi	$2,$8,0xf
	lui	$9,0x101
	ori	$9,$9,0x101
	dsll	$9,$9,0x10
	ori	$9,$9,0x101
	dsll	$9,$9,0x10
	ori	$9,$9,0x101
	lui	$4,0x8080
	ori	$4,$4,0x8080
	dsll	$4,$4,0x10
	ori	$4,$4,0x8080
	dsll	$4,$4,0x10
	ori	$4,$4,0x8080
	bnel	$2,$0,.L00199110
	ld	$10,0($5)
	pcpyld	$10,$9,$9
	lq	$9,0($5)
	pcpyld	$8,$4,$4
	psubb	$2,$9,$10
	pnor	$3,$0,$9
	pand	$2,$2,$3
	pand	$2,$2,$8
	pcpyud	$4,$2,$9
	or	$3,$2,$4
	bne	$3,$0,.L0019914C
	daddu	$6,$7,$0
.L001990DC:
	sq	$9,0($6)
	addiu	$5,$5,16
	lq	$9,0($5)
	psubb	$2,$9,$10
	pnor	$3,$0,$9
	pand	$2,$2,$3
	pand	$2,$2,$8
	pcpyud	$4,$2,$9
	or	$3,$2,$4
	beq	$3,$0,.L001990DC
	addiu	$6,$6,16
	beq	$0,$0,.L00199150
	daddu	$3,$6,$0
.L00199110:
	dsubu	$2,$10,$9
	nor	$3,$0,$10
	and	$2,$2,$3
	and	$2,$2,$4
	bne	$2,$0,.L0019914C
	daddu	$6,$7,$0
.L00199128:
	sd	$10,0($6)
	addiu	$5,$5,8
	ld	$10,0($5)
	nor	$2,$0,$10
	dsubu	$3,$10,$9
	and	$3,$3,$2
	and	$3,$3,$4
	beq	$3,$0,.L00199128
	addiu	$6,$6,8
.L0019914C:
	daddu	$3,$6,$0
.L00199150:
	lbu	$2,0($5)
	addiu	$5,$5,1
	sb	$2,0($3)
	sll	$2,$2,0x18
	addiu	$3,$3,1
	bne	$2,$0,.L00199150
	sll	$0,$0,0x0
	jr	$31
	daddu	$2,$7,$0
	.end	strcpy
	.size	strcpy,.-strcpy
