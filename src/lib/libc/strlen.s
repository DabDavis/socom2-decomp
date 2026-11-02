# sc: compiler=ee-gcc2.96
# newlib's strlen for the EE (MAIN, the libc the game linked: README "The GCC front"): one
# assembler object, as the demo's strlen is -- its .text section symbol carries no gcc2_compiled.
# marker, where every compiled libc object has one; 128-bit lq/sq and MMI byte compares over
# aligned quadwords, a byte loop for the rest. Transcribed by tools/asmunit.py.
# Byte-identical to the demo's.

	.set	noreorder
	.set	noat
	.set	nomacro

	.text
	.align	3

	.globl	strlen
	.type	strlen,@function
	.ent	strlen
strlen:
	andi	$2,$4,0x7
	bne	$2,$0,.L0019939C
	daddu	$7,$4,$0
	andi	$3,$4,0xf
	lui	$2,0x101
	ori	$2,$2,0x101
	dsll	$2,$2,0x10
	ori	$2,$2,0x101
	dsll	$2,$2,0x10
	ori	$2,$2,0x101
	bne	$3,$0,.L0019932C
	daddu	$5,$4,$0
	lq	$3,0($5)
	pcpyld	$8,$2,$2
	lui	$4,0x8080
	ori	$4,$4,0x8080
	dsll	$4,$4,0x10
	ori	$4,$4,0x8080
	dsll	$4,$4,0x10
	ori	$4,$4,0x8080
	psubb	$2,$3,$8
	pnor	$3,$0,$3
	pcpyld	$9,$4,$4
	pand	$2,$2,$3
	pand	$2,$2,$9
	pcpyud	$3,$2,$8
	or	$6,$3,$2
	bnel	$6,$0,.L0019939C
	daddu	$4,$5,$0
	addiu	$5,$5,16
.L00199300:
	lq	$2,0($5)
	pnor	$3,$0,$2
	psubb	$2,$2,$8
	pand	$2,$2,$3
	pand	$4,$2,$9
	pcpyud	$3,$4,$6
	or	$3,$3,$4
	beql	$3,$0,.L00199300
	addiu	$5,$5,16
	beq	$0,$0,.L0019939C
	daddu	$4,$5,$0
.L0019932C:
	ld	$3,0($5)
	lui	$4,0x8080
	ori	$4,$4,0x8080
	dsll	$4,$4,0x10
	ori	$4,$4,0x8080
	dsll	$4,$4,0x10
	ori	$4,$4,0x8080
	dsubu	$2,$3,$2
	nor	$3,$0,$3
	and	$2,$2,$3
	and	$2,$2,$4
	bnel	$2,$0,.L0019939C
	daddu	$4,$5,$0
	lui	$6,0x101
	ori	$6,$6,0x101
	dsll	$6,$6,0x10
	ori	$6,$6,0x101
	dsll	$6,$6,0x10
	ori	$6,$6,0x101
	addiu	$5,$5,8
.L0019937C:
	ld	$2,0($5)
	nor	$3,$0,$2
	dsubu	$2,$2,$6
	and	$2,$2,$3
	and	$2,$2,$4
	beql	$2,$0,.L0019937C
	addiu	$5,$5,8
	daddu	$4,$5,$0
.L0019939C:
	lb	$2,0($4)
	sll	$0,$0,0x0
	sll	$0,$0,0x0
	sll	$0,$0,0x0
	sll	$0,$0,0x0
	bnel	$2,$0,.L0019939C
	addiu	$4,$4,1
	jr	$31
	subu	$2,$4,$7
	.end	strlen
	.size	strlen,.-strlen
