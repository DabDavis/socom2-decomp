# sc: compiler=ee-gcc2.96
# newlib's strchr for the EE (MAIN, the libc the game linked: README "The GCC front"): one
# assembler object, as the demo's strchr is -- its .text section symbol carries no gcc2_compiled.
# marker, where every compiled libc object has one; 128-bit lq/sq and MMI byte compares over
# aligned quadwords, a byte loop for the rest. Transcribed by tools/asmunit.py.
# Byte-identical to the demo's.

	.set	noreorder
	.set	noat
	.set	nomacro

	.text
	.align	3

	.globl	strchr
	.type	strchr,@function
	.ent	strchr
strchr:
	andi	$2,$4,0x7
	bne	$2,$0,.L00198EF8
	andi	$5,$5,0xff
	dsll	$3,$5,0x8
	lui	$6,0x101
	ori	$6,$6,0x101
	dsll	$6,$6,0x10
	ori	$6,$6,0x101
	dsll	$6,$6,0x10
	ori	$6,$6,0x101
	daddu	$10,$3,$5
	andi	$3,$4,0xf
	dsll	$2,$10,0x10
	lui	$8,0x8080
	ori	$8,$8,0x8080
	dsll	$8,$8,0x10
	ori	$8,$8,0x8080
	dsll	$8,$8,0x10
	ori	$8,$8,0x8080
	daddu	$2,$2,$10
	dsll32	$10,$2,0x0
	bne	$3,$0,.L00198E70
	daddu	$7,$2,$10
.L00198DE4:
	lq	$9,0($4)
	pcpyld	$10,$6,$6
	pnor	$3,$0,$9
	psubb	$2,$9,$10
	pcpyld	$6,$8,$8
	pand	$2,$2,$3
	pcpyld	$8,$7,$7
	pand	$2,$2,$6
	pcpyud	$3,$2,$7
	or	$3,$2,$3
	bnel	$3,$0,.L00198EFC
	lbu	$2,0($4)
	pxor	$2,$9,$8
	psubb	$3,$2,$10
	pnor	$2,$0,$2
	lui	$8,0x8080
	ori	$8,$8,0x8080
	dsll	$8,$8,0x10
	ori	$8,$8,0x8080
	dsll	$8,$8,0x10
	ori	$8,$8,0x8080
	pand	$3,$3,$2
	pand	$3,$3,$6
	lui	$6,0x101
	ori	$6,$6,0x101
	dsll	$6,$6,0x10
	ori	$6,$6,0x101
	dsll	$6,$6,0x10
	ori	$6,$6,0x101
	pcpyud	$2,$3,$5
	or	$3,$2,$3
	beql	$3,$0,.L00198DE4
	addiu	$4,$4,16
	beq	$0,$0,.L00198EFC
	lbu	$2,0($4)
.L00198E70:
	ld	$9,0($4)
	nor	$3,$0,$9
	dsubu	$2,$9,$6
	and	$2,$2,$3
	and	$2,$2,$8
	bnel	$2,$0,.L00198EFC
	lbu	$2,0($4)
	xor	$2,$9,$7
	dsubu	$3,$2,$6
	nor	$2,$0,$2
	and	$3,$3,$2
	and	$3,$3,$8
	bnel	$3,$0,.L00198EFC
	lbu	$2,0($4)
	daddu	$9,$6,$0
	addiu	$4,$4,8
.L00198EB0:
	ld	$6,0($4)
	dsubu	$2,$6,$9
	nor	$3,$0,$6
	and	$2,$2,$3
	and	$2,$2,$8
	bne	$2,$0,.L00198EF8
	xor	$2,$6,$7
	nor	$3,$0,$2
	dsubu	$2,$2,$9
	and	$2,$2,$3
	and	$2,$2,$8
	beql	$2,$0,.L00198EB0
	addiu	$4,$4,8
	beq	$0,$0,.L00198EFC
	lbu	$2,0($4)
.L00198EEC:
	beql	$2,$5,.L00198F08
	lbu	$3,0($4)
	addiu	$4,$4,1
.L00198EF8:
	lbu	$2,0($4)
.L00198EFC:
	bne	$2,$0,.L00198EEC
	sll	$0,$0,0x0
	lbu	$3,0($4)
.L00198F08:
	daddu	$2,$0,$0
	xor	$3,$3,$5
	jr	$31
	movz	$2,$4,$3
	.end	strchr
	.size	strchr,.-strchr
