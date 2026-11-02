# sc: compiler=ee-gcc2.96
# newlib's strcmp for the EE (MAIN, the libc the game linked: README "The GCC front"): one
# assembler object, as the demo's strcmp is -- its .text section symbol carries no gcc2_compiled.
# marker, where every compiled libc object has one; 128-bit lq/sq and MMI byte compares over
# aligned quadwords, a byte loop for the rest. Transcribed by tools/asmunit.py.
# Byte-identical to the demo's.

	.set	noreorder
	.set	noat
	.set	nomacro

	.text
	.align	3

	.globl	strcmp
	.type	strcmp,@function
	.ent	strcmp
strcmp:
	or	$8,$4,$5
	andi	$2,$8,0x7
	bnel	$2,$0,.L00199048
	lb	$2,0($4)
	andi	$9,$8,0xf
	lui	$7,0x101
	ori	$7,$7,0x101
	dsll	$7,$7,0x10
	ori	$7,$7,0x101
	dsll	$7,$7,0x10
	ori	$7,$7,0x101
	lui	$6,0x8080
	ori	$6,$6,0x8080
	dsll	$6,$6,0x10
	ori	$6,$6,0x8080
	dsll	$6,$6,0x10
	ori	$6,$6,0x8080
	bne	$9,$0,.L00198FDC
	ld	$2,0($5)
	lq	$3,0($4)
	pcpyld	$8,$7,$7
	lq	$2,0($5)
	pcpyld	$10,$6,$6
	psubw	$7,$2,$3
	pcpyud	$6,$7,$4
	or	$3,$6,$7
	bnel	$3,$0,.L00199048
	lb	$2,0($4)
	lq	$2,0($4)
	pnor	$3,$0,$2
.L00198F90:
	psubb	$2,$2,$8
	pand	$2,$2,$3
	pand	$2,$2,$10
	pcpyud	$3,$2,$4
	or	$6,$3,$2
	beq	$6,$0,.L00198FB4
	addiu	$4,$4,16
	jr	$31
	daddu	$2,$0,$0
.L00198FB4:
	addiu	$5,$5,16
	lq	$2,0($4)
	lq	$3,0($5)
	psubw	$7,$2,$3
	pcpyud	$6,$7,$4
	or	$9,$6,$7
	beql	$9,$0,.L00198F90
	pnor	$3,$0,$2
	beq	$0,$0,.L00199048
	lb	$2,0($4)
.L00198FDC:
	ld	$3,0($4)
	bnel	$3,$2,.L00199048
	lb	$2,0($4)
	ld	$2,0($4)
	nor	$8,$0,$2
.L00198FF0:
	dsubu	$2,$2,$7
	and	$2,$2,$8
	and	$2,$2,$6
	beq	$2,$0,.L0019900C
	addiu	$4,$4,8
	jr	$31
	daddu	$2,$0,$0
.L0019900C:
	addiu	$5,$5,8
	ld	$2,0($4)
	ld	$3,0($5)
	beql	$3,$2,.L00198FF0
	nor	$8,$0,$2
	beq	$0,$0,.L00199048
	lb	$2,0($4)
.L00199028:
	sll	$2,$3,0x18
	lb	$3,0($5)
	sra	$2,$2,0x18
	bnel	$2,$3,.L00199050
	lbu	$3,0($4)
	addiu	$4,$4,1
	addiu	$5,$5,1
	lb	$2,0($4)
.L00199048:
	bne	$2,$0,.L00199028
	lbu	$3,0($4)
.L00199050:
	lbu	$2,0($5)
	jr	$31
	subu	$2,$3,$2
	.end	strcmp
	.size	strcmp,.-strcmp
