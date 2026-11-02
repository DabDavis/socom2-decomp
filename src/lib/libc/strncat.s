# sc: compiler=ee-gcc2.96
# newlib's strncat for the EE (MAIN, the libc the game linked: README "The GCC front"): one
# assembler object, as the demo's strncat is -- its .text section symbol carries no gcc2_compiled.
# marker, where every compiled libc object has one; 128-bit lq/sq and MMI byte compares over
# aligned quadwords, a byte loop for the rest. Transcribed by tools/asmunit.py.
# Byte-identical to the demo's.

	.set	noreorder
	.set	noat
	.set	nomacro

	.text
	.align	3

	.globl	strncat
	.type	strncat,@function
	.ent	strncat
strncat:
	daddu	$8,$4,$0
	andi	$2,$8,0x7
	bnel	$2,$0,.L001995B4
	lb	$2,0($4)
	andi	$3,$8,0xf
	bnel	$3,$0,.L00199544
	ld	$2,0($8)
	lui	$7,0x101
	ori	$7,$7,0x101
	dsll	$7,$7,0x10
	ori	$7,$7,0x101
	dsll	$7,$7,0x10
	ori	$7,$7,0x101
	lq	$2,0($8)
	pcpyld	$9,$7,$7
	lui	$4,0x8080
	ori	$4,$4,0x8080
	dsll	$4,$4,0x10
	ori	$4,$4,0x8080
	dsll	$4,$4,0x10
	ori	$4,$4,0x8080
	psubb	$3,$2,$9
	pnor	$2,$0,$2
	pcpyld	$10,$4,$4
	pand	$3,$3,$2
	pand	$3,$3,$10
	pcpyud	$2,$3,$4
	or	$3,$3,$2
	bne	$3,$0,.L001995AC
	daddu	$7,$8,$0
	addiu	$7,$7,16
	sll	$0,$0,0x0
.L00199518:
	lq	$2,0($7)
	pnor	$3,$0,$2
	psubb	$2,$2,$9
	pand	$2,$2,$3
	pand	$2,$2,$10
	pcpyud	$3,$2,$4
	or	$2,$2,$3
	beq	$2,$0,.L00199518
	addiu	$7,$7,16
	beq	$0,$0,.L001995AC
	addiu	$7,$7,-16
.L00199544:
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
	dsubu	$3,$2,$9
	nor	$2,$0,$2
	and	$3,$3,$2
	and	$3,$3,$4
	bne	$3,$0,.L001995AC
	daddu	$7,$8,$0
	addiu	$7,$7,8
.L00199590:
	ld	$2,0($7)
	nor	$3,$0,$2
	dsubu	$2,$2,$9
	and	$2,$2,$3
	and	$2,$2,$4
	beql	$2,$0,.L00199590
	addiu	$7,$7,8
.L001995AC:
	daddu	$4,$7,$0
	lb	$2,0($4)
.L001995B4:
	beq	$2,$0,.L001995DC
	addiu	$6,$6,-1
	addiu	$4,$4,1
.L001995C0:
	lb	$2,0($4)
	sll	$0,$0,0x0
	sll	$0,$0,0x0
	sll	$0,$0,0x0
	sll	$0,$0,0x0
	bnel	$2,$0,.L001995C0
	addiu	$4,$4,1
.L001995DC:
	lui	$2,0xffff
	ori	$2,$2,0xffff
	beq	$6,$2,.L0019963C
	sll	$0,$0,0x0
	lbu	$2,0($5)
	addiu	$5,$5,1
	sb	$2,0($4)
	sll	$2,$2,0x18
	beq	$2,$0,.L0019963C
	addiu	$4,$4,1
	lui	$3,0xffff
	ori	$3,$3,0xffff
	sll	$0,$0,0x0
.L00199610:
	beql	$6,$0,.L00199618
	sb	$0,0($4)
.L00199618:
	addiu	$6,$6,-1
	beq	$6,$3,.L0019963C
	sll	$0,$0,0x0
	lbu	$2,0($5)
	addiu	$5,$5,1
	sb	$2,0($4)
	sll	$2,$2,0x18
	bne	$2,$0,.L00199610
	addiu	$4,$4,1
.L0019963C:
	jr	$31
	daddu	$2,$8,$0
	.end	strncat
	.size	strncat,.-strncat
