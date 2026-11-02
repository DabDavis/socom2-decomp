# sc: compiler=ee-gcc2.96
# newlib's strcat for the EE (MAIN, the libc the game linked: README "The GCC front"): one
# assembler object, as the demo's strcat is -- its .text section symbol carries no gcc2_compiled.
# marker, where every compiled libc object has one; 128-bit lq/sq and MMI byte compares over
# aligned quadwords, a byte loop for the rest. Transcribed by tools/asmunit.py.
# Byte-identical to the demo's.

	.set	noreorder
	.set	noat
	.set	nomacro

	.text
	.align	3

	.globl	strcat
	.type	strcat,@function
	.ent	strcat
strcat:
	addiu	$29,$29,-32
	sq	$16,0($29)
	daddu	$16,$4,$0
	andi	$2,$16,0x7
	bne	$2,$0,.L00198D4C
	sq	$31,16($29)
	andi	$2,$16,0xf
	lui	$3,0x101
	ori	$3,$3,0x101
	dsll	$3,$3,0x10
	ori	$3,$3,0x101
	dsll	$3,$3,0x10
	ori	$3,$3,0x101
	lui	$4,0x8080
	ori	$4,$4,0x8080
	dsll	$4,$4,0x10
	ori	$4,$4,0x8080
	dsll	$4,$4,0x10
	ori	$4,$4,0x8080
	bne	$2,$0,.L00198D08
	ld	$6,0($16)
	lq	$2,0($16)
	pcpyld	$7,$3,$3
	pcpyld	$8,$4,$4
	psubb	$3,$2,$7
	pnor	$2,$0,$2
	pand	$3,$3,$2
	pand	$3,$3,$8
	pcpyud	$2,$3,$3
	or	$3,$2,$3
	bne	$3,$0,.L00198D4C
	daddu	$4,$16,$0
	addiu	$6,$4,16
.L00198CDC:
	lq	$2,0($6)
	pnor	$3,$0,$2
	psubb	$2,$2,$7
	pand	$2,$2,$3
	pand	$2,$2,$8
	pcpyud	$3,$2,$2
	or	$2,$2,$3
	beql	$2,$0,.L00198CDC
	addiu	$6,$6,16
	beq	$0,$0,.L00198D4C
	daddu	$4,$6,$0
.L00198D08:
	daddu	$7,$3,$0
	daddu	$8,$4,$0
	dsubu	$3,$6,$3
	nor	$2,$0,$6
	and	$3,$3,$2
	and	$3,$3,$4
	bne	$3,$0,.L00198D4C
	daddu	$4,$16,$0
	addiu	$6,$16,8
.L00198D2C:
	ld	$2,0($6)
	nor	$3,$0,$2
	dsubu	$2,$2,$7
	and	$2,$2,$3
	and	$2,$2,$8
	beql	$2,$0,.L00198D2C
	addiu	$6,$6,8
	daddu	$4,$6,$0
.L00198D4C:
	lb	$2,0($4)
	sll	$0,$0,0x0
	sll	$0,$0,0x0
	sll	$0,$0,0x0
	sll	$0,$0,0x0
	bnel	$2,$0,.L00198D4C
	addiu	$4,$4,1
	jal	strcpy
	sll	$0,$0,0x0
	daddu	$2,$16,$0
	lq	$31,16($29)
	lq	$16,0($29)
	jr	$31
	addiu	$29,$29,32
	.end	strcat
	.size	strcat,.-strcat
