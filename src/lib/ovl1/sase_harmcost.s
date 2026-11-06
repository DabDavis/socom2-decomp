# sc: compiler=ee-gcc2.96; flags=-G8
# sc: compiler=ee-gcc2.96
# Nellymoser's speech codec (OVL1 0x2485D0): the encoder's harmonic cost of a pitch candidate -- the
# spectral peaks (piPeakFreq, ascending, a sentinel past the last) walked once in steps of the
# candidate's harmonic spacing, each harmonic's peak magnitude weighted by a 256-entry window
# (*data_001D55A4, indexed by the peak's fractional position) and maximised with max.s, the sum of
# max x pfMag[bin] accumulated in the FPU's ACC (adda.s seeds it with 1e-16, madda.s adds, madd.s
# reads it back through a zero) beside the bins' energy, scaled by fScale: (sum / 2 - 3 energy / 8).
# Assembly, or a C function whose body is one asm: neither of the ee-gcc 2.9x/2.96 cc1s has an
# adda.s or madda.s template at all (only the 3.2 builds' cc1 does, and it does not choose them for
# this loop in C), and the division by (int)(fPitch * 128) has no zero-division trap, which every
# GCC C division here gets (`break 7`; -mno-check-zero-division takes 2.96 to `div1`) -- GCC round
# 43a, as sase_vdot.s (round 16a); the unit's name is ours. Arguments: a0 piPeakFreq, a1 (unused:
# overwritten by the first product), f12 fPitch, a2 pfMag, a3 iNumBins, f13 fScale; f0 the cost.

	.set	noreorder
	.set	noat
	.set	nomacro

	.extern	data_001D53F8,4
	.extern	data_001D55A4,4

	.text
	.align	4

	.globl	func_002485D0
	.type	func_002485D0,@function
	.ent	func_002485D0
func_002485D0:
	lui	$1,0x4300
	mtc1	$1,$f0
	lui	$3,0x1000
	lui	$1,0x4280
	mtc1	$1,$f1
	addiu	$12,$0,16384
	mul.s	$f0,$f12,$f0
	.set	macro
	lwc1	$f4,data_001D53F8
	.set	nomacro
	mul.s	$f12,$f12,$f1
	mtc1	$0,$f5
	cvt.w.s	$f1,$f0
	mfc1	$2,$f1
	cvt.w.s	$f0,$f12
	mfc1	$13,$f0
	div	$0,$3,$2
	lw	$8,0($4)
	mflo	$15
	mult	$5,$8,$13
	sll	$0,$0,0x0
	slt	$2,$5,$12
	beq	$2,$0,.L00248648
	daddu	$11,$15,$0
.L00248628:
	addiu	$4,$4,4
	sll	$0,$0,0x0
	lw	$8,0($4)
	mult	$5,$8,$13
	slti	$2,$5,16384
	sll	$0,$0,0x0
	bnel	$2,$0,.L00248628
	sll	$0,$0,0x0
.L00248648:
	addiu	$2,$11,2048
	sra	$9,$2,0xc
	slt	$3,$9,$7
	beq	$3,$0,.L002486EC
	sll	$0,$0,0x0
	.set	macro
	lw	$10,data_001D55A4
	.set	nomacro
	ori	$24,$0,0x8000
	lui	$1,0x3f80
	mtc1	$1,$f3
	adda.s	$f5,$f4
.L00248670:
	addu	$12,$12,$24
	slt	$3,$5,$12
	.set	macro
	lwc1	$f2,data_001D53F8
	.set	nomacro
	beq	$3,$0,.L002486C4
	sll	$2,$9,0x2
	sll	$0,$0,0x0
.L00248688:
	srl	$2,$5,0x5
	sll	$3,$8,0x2
	andi	$2,$2,0x3fc
	addu	$3,$3,$6
	addu	$2,$2,$10
	lwc1	$f1,0($3)
	lwc1	$f0,0($2)
	mul.s	$f1,$f1,$f0
	max.s	$f2,$f2,$f1
	addiu	$4,$4,4
	lw	$8,0($4)
	mult	$5,$8,$13
	slt	$2,$5,$12
	bne	$2,$0,.L00248688
	sll	$2,$9,0x2
.L002486C4:
	addu	$11,$11,$15
	addu	$2,$2,$6
	addiu	$3,$11,2048
	lwc1	$f0,0($2)
	sra	$9,$3,0xc
	slt	$2,$9,$7
	madda.s	$f2,$f0
	mul.s	$f0,$f0,$f0
	bne	$2,$0,.L00248670
	add.s	$f4,$f4,$f0
.L002486EC:
	madd.s	$f5,$f5,$f5
	lui	$1,0x3f00
	mtc1	$1,$f0
	lui	$1,0x3ec0
	mtc1	$1,$f1
	mul.s	$f0,$f5,$f0
	mul.s	$f1,$f4,$f1
	sub.s	$f0,$f0,$f1
	jr	$31
	mul.s	$f0,$f13,$f0
	.end	func_002485D0
	.size	func_002485D0,.-func_002485D0
