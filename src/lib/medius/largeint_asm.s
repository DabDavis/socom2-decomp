# sc: compiler=ee-gcc2.96
# LargeInt's EE primitives: the RT library's big-integer core written in R5900 assembly (OVL2
# 0x628B48-0x629A44), between LargeInt.c's C (0x626B10-0x628B44: the tests, the bit functions,
# the prime search) and md5.c (0x629A48) -- SCEA's rt_crypt, which SOCOM 1 still compiled from C
# (its LargeInt object, 0x406C28, holds every one of these as a C function). Assembly, not GCC's:
# each function saves the argument and temporary registers it uses with sq (no GCC prologue saves
# a0-a3 or t0-t2), branches to the very next instruction (`b 1f; nop; 1:`), pads with stray nops,
# swaps a0/a1 with two moves that leave both equal (ShiftRight/ShiftLeft's `move a0,a1; move
# a1,a0`), counts leading zeros with the MMI's plzcw and adds through doublewords (daddu, then
# dsrl32/dsra32 for the carry). The functions sit unaligned one after another, the run 8-aligned
# at its start. Whether SCEA kept it in a .s of its own or as LargeInt.c's top-level asm, the
# object's bytes are the same; it is an assembler unit here, built by 2.96's GNU as.
#
# A number is 32 words, least significant first (largeint.c's LINT). Names: lintGetBits by the
# prime search's assert on its result ("nBits == lintGetBits(pPrime)"); SOCOM 1's IsGreater,
# IsSMaller, IsNotGreater, IsNotSmaller (its order), ShiftRight, ShiftLeft, lintAddIP and
# lintSubIP by their bodies; func_006290B8 and func_006291D8 are the add and subtract over the
# words a bit count covers (count / 32 + 1, at most 32), new in SOCOM II; func_00629274 (a copy
# of its second number, lintGetBits of both, a shift and subtract) and the two multiplies
# func_006296A0 and func_00629868 (lintGetBits of both factors, three zeroed numbers, multu word
# by word; the second takes a fourth argument) keep placeholders. The two `ori $2,$0,0x1` are the
# target's own encoding of a 1 (GNU as's `li` gives addiu).

	.set	noreorder
	.set	noat
	.set	nomacro

	.text
	.align	3

	.globl	lintGetBits
	.type	lintGetBits,@function
	.ent	lintGetBits
lintGetBits:
	addiu	$29,$29,-64
	sq	$10,48($29)
	sq	$9,32($29)
	sq	$8,16($29)
	sq	$3,0($29)
	addiu	$2,$0,1024
	addiu	$9,$0,32
	addiu	$10,$4,124
.L00628B68:
	lwu	$4,0($10)
	addiu	$10,$10,-4
	bne	$4,$0,.L00628B90
	sll	$0,$0,0x0
	addiu	$9,$9,-1
	addiu	$2,$2,-32
	bnel	$9,$0,.L00628B68
	sll	$0,$0,0x0
	beq	$0,$0,.L00628BAC
	sll	$0,$0,0x0
.L00628B90:
	lui	$8,0x8000
	and	$3,$4,$8
	bne	$3,$0,.L00628BAC
	sll	$0,$0,0x0
	plzcw	$8,$4
	addiu	$2,$2,-1
	subu	$2,$2,$8
.L00628BAC:
	lq	$3,0($29)
	lq	$8,16($29)
	lq	$9,32($29)
	lq	$10,48($29)
	addiu	$29,$29,64
	jr	$31
	sll	$0,$0,0x0
	.end	lintGetBits
	.size	lintGetBits,.-lintGetBits

	.globl	IsGreater
	.type	IsGreater,@function
	.ent	IsGreater
IsGreater:
	addiu	$29,$29,-48
	sq	$3,0($29)
	sq	$8,16($29)
	sq	$9,32($29)
	addiu	$3,$0,32
	addiu	$4,$4,124
	addiu	$5,$5,124
	beq	$0,$0,.L00628BEC
	sll	$0,$0,0x0
.L00628BEC:
	lwu	$8,0($4)
	lwu	$9,0($5)
	addiu	$3,$3,-1
	sltu	$2,$9,$8
	addiu	$4,$4,-4
	bgtz	$2,.L00628C24
	sll	$0,$0,0x0
	sltu	$2,$8,$9
	addiu	$5,$5,-4
	bgtz	$2,.L00628C20
	sll	$0,$0,0x0
	bgtz	$3,.L00628BEC
	sll	$0,$0,0x0
.L00628C20:
	or	$2,$0,$0
.L00628C24:
	lq	$9,32($29)
	lq	$8,16($29)
	lq	$3,0($29)
	addiu	$29,$29,48
	jr	$31
	sll	$0,$0,0x0
	.end	IsGreater
	.size	IsGreater,.-IsGreater

	.globl	IsSMaller
	.type	IsSMaller,@function
	.ent	IsSMaller
IsSMaller:
	addiu	$29,$29,-48
	sq	$3,0($29)
	sq	$8,16($29)
	sq	$9,32($29)
	addiu	$3,$0,32
	addiu	$4,$4,124
	addiu	$5,$5,124
	beq	$0,$0,.L00628C8C
	sll	$0,$0,0x0
.L00628C60:
	lwu	$8,0($4)
	lwu	$9,0($5)
	addiu	$3,$3,-1
	sltu	$2,$8,$9
	addiu	$4,$4,-4
	bgtz	$2,.L00628C98
	sll	$0,$0,0x0
	sltu	$2,$9,$8
	addiu	$5,$5,-4
	bgtz	$2,.L00628C94
	sll	$0,$0,0x0
.L00628C8C:
	bgtz	$3,.L00628C60
	sll	$0,$0,0x0
.L00628C94:
	or	$2,$0,$0
.L00628C98:
	lq	$9,32($29)
	lq	$8,16($29)
	lq	$3,0($29)
	addiu	$29,$29,48
	jr	$31
	sll	$0,$0,0x0
	.end	IsSMaller
	.size	IsSMaller,.-IsSMaller

	.globl	IsNotGreater
	.type	IsNotGreater,@function
	.ent	IsNotGreater
IsNotGreater:
	addiu	$29,$29,-48
	sq	$3,0($29)
	sq	$8,16($29)
	sq	$9,32($29)
	addiu	$3,$0,32
	addiu	$4,$4,124
	addiu	$5,$5,124
	beq	$0,$0,.L00628CD4
	sll	$0,$0,0x0
.L00628CD4:
	lwu	$8,0($4)
	lwu	$9,0($5)
	addiu	$3,$3,-1
	sltu	$2,$8,$9
	addiu	$4,$4,-4
	bgtz	$2,.L00628D18
	sll	$0,$0,0x0
	sltu	$2,$9,$8
	addiu	$5,$5,-4
	bgtz	$2,.L00628D14
	sll	$0,$0,0x0
	bgtz	$3,.L00628CD4
	sll	$0,$0,0x0
	ori	$2,$0,0x1
	beq	$0,$0,.L00628D18
	sll	$0,$0,0x0
.L00628D14:
	or	$2,$0,$0
.L00628D18:
	lq	$9,32($29)
	lq	$8,16($29)
	lq	$3,0($29)
	addiu	$29,$29,48
	jr	$31
	sll	$0,$0,0x0
	.end	IsNotGreater
	.size	IsNotGreater,.-IsNotGreater

	.globl	IsNotSmaller
	.type	IsNotSmaller,@function
	.ent	IsNotSmaller
IsNotSmaller:
	addiu	$29,$29,-48
	sq	$3,0($29)
	sq	$8,16($29)
	sq	$9,32($29)
	addiu	$3,$0,32
	addiu	$4,$4,124
	addiu	$5,$5,124
	beq	$0,$0,.L00628D54
	sll	$0,$0,0x0
.L00628D54:
	lwu	$8,0($4)
	lwu	$9,0($5)
	addiu	$3,$3,-1
	sltu	$2,$9,$8
	addiu	$4,$4,-4
	bgtz	$2,.L00628D98
	sll	$0,$0,0x0
	sltu	$2,$8,$9
	addiu	$5,$5,-4
	bgtz	$2,.L00628D94
	sll	$0,$0,0x0
	bgtz	$3,.L00628D54
	sll	$0,$0,0x0
	ori	$2,$0,0x1
	beq	$0,$0,.L00628D98
	sll	$0,$0,0x0
.L00628D94:
	or	$2,$0,$0
.L00628D98:
	lq	$9,32($29)
	lq	$8,16($29)
	lq	$3,0($29)
	addiu	$29,$29,48
	jr	$31
	sll	$0,$0,0x0
	.end	IsNotSmaller
	.size	IsNotSmaller,.-IsNotSmaller

	.globl	ShiftRight
	.type	ShiftRight,@function
	.ent	ShiftRight
ShiftRight:
	addiu	$29,$29,-144
	sq	$4,0($29)
	sq	$5,16($29)
	sq	$6,32($29)
	sq	$7,48($29)
	sq	$8,64($29)
	sq	$9,80($29)
	sq	$16,96($29)
	sq	$17,112($29)
	sq	$18,128($29)
	or	$17,$4,$0
	or	$4,$5,$0
	or	$5,$4,$0
	dsll32	$4,$4,0x1b
	dsrl	$5,$5,0x5
	dsrl32	$4,$4,0x1b
	dsll	$5,$5,0x2
	or	$8,$4,$0
	addiu	$9,$0,32
	dsubu	$9,$9,$8
	daddu	$18,$17,$5
	dsrl	$5,$5,0x2
	lwu	$6,0($18)
	addiu	$16,$0,32
	sll	$0,$0,0x0
	dsll	$4,$16,0x2
	sll	$0,$0,0x0
	addu	$17,$17,$4
.L00628E20:
	addiu	$17,$17,-4
	lwu	$4,0($17)
	bne	$4,$0,.L00628E44
	sll	$0,$0,0x0
	addiu	$16,$16,-1
	bnel	$16,$5,.L00628E20
	sll	$0,$0,0x0
	addiu	$17,$17,4
	addiu	$16,$16,1
.L00628E44:
	lq	$17,0($29)
	addiu	$18,$18,4
	addiu	$16,$16,-1
	beq	$0,$0,.L00628E80
	sll	$0,$0,0x0
.L00628E58:
	lwu	$4,0($18)
	or	$7,$6,$0
	or	$6,$4,$0
	dsrlv	$7,$7,$8
	dsllv	$4,$4,$9
	addiu	$16,$16,-1
	or	$4,$4,$7
	addiu	$18,$18,4
	sw	$4,0($17)
	addiu	$17,$17,4
.L00628E80:
	bne	$16,$5,.L00628E58
	sll	$0,$0,0x0
	dsrlv	$6,$6,$8
	sw	$6,0($17)
	or	$4,$0,$0
	beq	$0,$0,.L00628EAC
	sll	$0,$0,0x0
.L00628E9C:
	addiu	$17,$17,4
	addiu	$16,$16,-1
	sw	$4,0($17)
	sll	$0,$0,0x0
.L00628EAC:
	sll	$0,$0,0x0
	bne	$16,$0,.L00628E9C
	sll	$0,$0,0x0
	lq	$18,128($29)
	lq	$17,112($29)
	lq	$16,96($29)
	lq	$9,80($29)
	lq	$8,64($29)
	lq	$7,48($29)
	lq	$6,32($29)
	lq	$5,16($29)
	lq	$4,0($29)
	addiu	$29,$29,144
	jr	$31
	sll	$0,$0,0x0
	.end	ShiftRight
	.size	ShiftRight,.-ShiftRight

	.globl	ShiftLeft
	.type	ShiftLeft,@function
	.ent	ShiftLeft
ShiftLeft:
	addiu	$29,$29,-144
	sq	$4,0($29)
	sq	$5,16($29)
	sq	$6,32($29)
	sq	$7,48($29)
	sq	$8,64($29)
	sq	$9,80($29)
	sq	$16,96($29)
	sq	$17,112($29)
	sq	$18,128($29)
	or	$17,$4,$0
	or	$4,$5,$0
	or	$5,$4,$0
	dsll32	$4,$4,0x1b
	dsrl	$5,$5,0x5
	dsrl32	$4,$4,0x1b
	dsll	$5,$5,0x2
	or	$8,$4,$0
	addiu	$16,$0,32
	addiu	$9,$0,32
	dsll	$16,$16,0x2
	dsubu	$9,$9,$8
	daddu	$17,$17,$16
	dsrl	$16,$16,0x2
	dsubu	$18,$17,$5
	dsrl	$5,$5,0x2
.L00628F50:
	addiu	$18,$18,-4
	addiu	$16,$16,-1
	lwu	$6,0($18)
	bne	$6,$0,.L00628F74
	sll	$0,$0,0x0
	bnel	$16,$5,.L00628F50
	sll	$0,$0,0x0
	beq	$0,$0,.L00628FD0
	sll	$0,$0,0x0
.L00628F74:
	addiu	$1,$0,31
	beq	$16,$1,.L00628F8C
	sll	$0,$0,0x0
	addiu	$18,$18,4
	addiu	$16,$16,1
	lwu	$6,0($18)
.L00628F8C:
	dsll	$17,$5,0x2
	addu	$17,$17,$18
	addiu	$17,$17,4
	beq	$0,$0,.L00628FC8
	sll	$0,$0,0x0
.L00628FA0:
	addiu	$18,$18,-4
	addiu	$17,$17,-4
	lwu	$4,0($18)
	or	$7,$6,$0
	or	$6,$4,$0
	dsllv	$7,$7,$8
	dsrlv	$4,$4,$9
	addiu	$16,$16,-1
	or	$4,$4,$7
	sw	$4,0($17)
.L00628FC8:
	bne	$16,$5,.L00628FA0
	sll	$0,$0,0x0
.L00628FD0:
	addiu	$17,$17,-4
	dsllv	$6,$6,$8
	sw	$6,0($17)
	or	$4,$0,$0
	beq	$0,$0,.L00628FF8
	sll	$0,$0,0x0
.L00628FE8:
	addiu	$17,$17,-4
	addiu	$16,$16,-1
	sw	$4,0($17)
	sll	$0,$0,0x0
.L00628FF8:
	sll	$0,$0,0x0
	bne	$16,$0,.L00628FE8
	sll	$0,$0,0x0
	lq	$18,128($29)
	lq	$17,112($29)
	lq	$16,96($29)
	lq	$9,80($29)
	lq	$8,64($29)
	lq	$7,48($29)
	lq	$6,32($29)
	lq	$5,16($29)
	lq	$4,0($29)
	addiu	$29,$29,144
	jr	$31
	sll	$0,$0,0x0
	.end	ShiftLeft
	.size	ShiftLeft,.-ShiftLeft

	.globl	lintAddIP
	.type	lintAddIP,@function
	.ent	lintAddIP
lintAddIP:
	addiu	$29,$29,-96
	sq	$4,0($29)
	sq	$5,16($29)
	sq	$6,32($29)
	sq	$20,48($29)
	sq	$21,64($29)
	sq	$22,80($29)
	addiu	$20,$0,32
	or	$21,$4,$0
	or	$22,$5,$0
	or	$4,$0,$0
	beq	$0,$0,.L0062908C
	sll	$0,$0,0x0
.L00629068:
	lwu	$5,0($21)
	lwu	$6,0($22)
	addiu	$20,$20,-1
	daddu	$4,$4,$5
	addiu	$21,$21,4
	daddu	$4,$4,$6
	addiu	$22,$22,4
	sw	$4,-4($21)
	dsrl32	$4,$4,0x0
.L0062908C:
	bne	$20,$0,.L00629068
	sll	$0,$0,0x0
	lq	$22,80($29)
	lq	$21,64($29)
	lq	$20,48($29)
	lq	$6,32($29)
	lq	$5,16($29)
	lq	$4,0($29)
	addiu	$29,$29,96
	jr	$31
	sll	$0,$0,0x0
	.end	lintAddIP
	.size	lintAddIP,.-lintAddIP

	.globl	func_006290B8
	.type	func_006290B8,@function
	.ent	func_006290B8
func_006290B8:
	addiu	$29,$29,-96
	sq	$4,0($29)
	sq	$5,16($29)
	sq	$6,32($29)
	sq	$20,48($29)
	sq	$21,64($29)
	sq	$22,80($29)
	or	$20,$6,$0
	or	$21,$4,$0
	dsrl	$20,$20,0x5
	or	$22,$5,$0
	addiu	$20,$20,1
	or	$4,$0,$0
	sltiu	$2,$20,32
	bne	$2,$0,.L00629128
	sll	$0,$0,0x0
	addiu	$20,$0,32
	beq	$0,$0,.L00629128
	sll	$0,$0,0x0
.L00629104:
	lwu	$5,0($21)
	lwu	$6,0($22)
	addiu	$20,$20,-1
	daddu	$4,$4,$5
	addiu	$21,$21,4
	daddu	$4,$4,$6
	addiu	$22,$22,4
	sw	$4,-4($21)
	dsrl32	$4,$4,0x0
.L00629128:
	bne	$20,$0,.L00629104
	sll	$0,$0,0x0
	lq	$22,80($29)
	lq	$21,64($29)
	lq	$20,48($29)
	lq	$6,32($29)
	lq	$5,16($29)
	lq	$4,0($29)
	addiu	$29,$29,96
	jr	$31
	sll	$0,$0,0x0
	.end	func_006290B8
	.size	func_006290B8,.-func_006290B8

	.globl	lintSubIP
	.type	lintSubIP,@function
	.ent	lintSubIP
lintSubIP:
	addiu	$29,$29,-96
	sq	$4,0($29)
	sq	$5,16($29)
	sq	$6,32($29)
	sq	$20,48($29)
	sq	$21,64($29)
	sq	$22,80($29)
	addiu	$20,$0,32
	or	$21,$4,$0
	or	$22,$5,$0
	or	$4,$0,$0
	beq	$0,$0,.L006291AC
	sll	$0,$0,0x0
.L00629188:
	lwu	$5,0($21)
	lwu	$6,0($22)
	addiu	$20,$20,-1
	daddu	$4,$4,$5
	addiu	$21,$21,4
	dsubu	$4,$4,$6
	addiu	$22,$22,4
	sw	$4,-4($21)
	dsra32	$4,$4,0x0
.L006291AC:
	bne	$20,$0,.L00629188
	sll	$0,$0,0x0
	lq	$22,80($29)
	lq	$21,64($29)
	lq	$20,48($29)
	lq	$6,32($29)
	lq	$5,16($29)
	lq	$4,0($29)
	addiu	$29,$29,96
	jr	$31
	sll	$0,$0,0x0
	.end	lintSubIP
	.size	lintSubIP,.-lintSubIP

	.globl	func_006291D8
	.type	func_006291D8,@function
	.ent	func_006291D8
func_006291D8:
	addiu	$29,$29,-96
	sq	$4,0($29)
	sq	$5,16($29)
	sq	$6,32($29)
	sq	$20,48($29)
	sq	$21,64($29)
	sq	$22,80($29)
	or	$20,$6,$0
	or	$21,$4,$0
	dsrl	$20,$20,0x5
	or	$22,$5,$0
	addiu	$20,$20,1
	or	$4,$0,$0
	sltiu	$2,$20,32
	bne	$2,$0,.L00629248
	sll	$0,$0,0x0
	addiu	$20,$0,32
	beq	$0,$0,.L00629248
	sll	$0,$0,0x0
.L00629224:
	lwu	$5,0($21)
	lwu	$6,0($22)
	addiu	$20,$20,-1
	daddu	$4,$4,$5
	addiu	$21,$21,4
	dsubu	$4,$4,$6
	addiu	$22,$22,4
	sw	$4,-4($21)
	dsra32	$4,$4,0x0
.L00629248:
	bne	$20,$0,.L00629224
	sll	$0,$0,0x0
	lq	$22,80($29)
	lq	$21,64($29)
	lq	$20,48($29)
	lq	$6,32($29)
	lq	$5,16($29)
	lq	$4,0($29)
	addiu	$29,$29,96
	jr	$31
	sll	$0,$0,0x0
	.end	func_006291D8
	.size	func_006291D8,.-func_006291D8

	.globl	func_00629274
	.type	func_00629274,@function
	.ent	func_00629274
func_00629274:
	addiu	$29,$29,-320
	sq	$23,176($29)
	sq	$22,160($29)
	sq	$21,144($29)
	sq	$20,128($29)
	sq	$19,112($29)
	sq	$18,96($29)
	sq	$17,80($29)
	sq	$16,64($29)
	sq	$31,48($29)
	or	$21,$4,$0
	or	$22,$5,$0
	addiu	$23,$29,192
	sw	$21,12($29)
	addiu	$3,$0,32
	beq	$0,$0,.L006292D4
	sll	$0,$0,0x0
.L006292B8:
	lq	$4,0($22)
	lq	$5,16($22)
	addiu	$3,$3,-8
	addiu	$22,$22,32
	sq	$4,0($23)
	sq	$5,16($23)
	addiu	$23,$23,32
.L006292D4:
	bne	$3,$0,.L006292B8
	sll	$0,$0,0x0
	addiu	$22,$22,-128
	addiu	$23,$23,-128
	or	$4,$21,$0
	jal	lintGetBits
	sll	$0,$0,0x0
	sw	$2,0($29)
	or	$4,$22,$0
	jal	lintGetBits
	sll	$0,$0,0x0
	sw	$2,4($29)
	lwu	$3,0($29)
	sw	$2,8($29)
	sltu	$3,$2,$3
	beq	$3,$0,.L006293E8
	sll	$0,$0,0x0
	lwu	$3,0($29)
	or	$17,$23,$0
	subu	$2,$3,$2
	or	$16,$3,$0
	or	$4,$2,$0
	addiu	$16,$16,31
	or	$5,$4,$0
	dsll32	$4,$4,0x1b
	dsrl	$5,$5,0x5
	dsrl32	$4,$4,0x1b
	dsll	$5,$5,0x2
	or	$8,$4,$0
	dsrl	$16,$16,0x5
	addiu	$9,$0,32
	dsll	$16,$16,0x2
	dsubu	$9,$9,$8
	daddu	$17,$17,$16
	dsrl	$16,$16,0x2
	dsubu	$18,$17,$5
	dsrl	$5,$5,0x2
	addiu	$18,$18,-4
	addiu	$16,$16,-1
	lwu	$6,0($18)
	beq	$0,$0,.L006293A4
	sll	$0,$0,0x0
.L0062937C:
	addiu	$18,$18,-4
	addiu	$17,$17,-4
	lwu	$4,0($18)
	or	$7,$6,$0
	or	$6,$4,$0
	dsllv	$7,$7,$8
	dsrlv	$4,$4,$9
	addiu	$16,$16,-1
	or	$4,$4,$7
	sw	$4,0($17)
.L006293A4:
	bne	$16,$5,.L0062937C
	sll	$0,$0,0x0
	addiu	$17,$17,-4
	dsllv	$6,$6,$8
	sw	$6,0($17)
	or	$4,$0,$0
	beq	$0,$0,.L006293D4
	sll	$0,$0,0x0
.L006293C4:
	addiu	$17,$17,-4
	addiu	$16,$16,-1
	sw	$4,0($17)
	sll	$0,$0,0x0
.L006293D4:
	sll	$0,$0,0x0
	bne	$16,$0,.L006293C4
	sll	$0,$0,0x0
	lwu	$4,0($29)
	sw	$4,4($29)
.L006293E8:
	lwu	$4,0($29)
	lwu	$5,4($29)
	lwu	$6,8($29)
	sltu	$3,$4,$6
	bne	$3,$0,.L00629670
	sll	$0,$0,0x0
	sltu	$3,$4,$5
	beq	$3,$0,.L006294D0
	sll	$0,$0,0x0
	lwu	$4,0($29)
	lwu	$5,4($29)
	sw	$4,4($29)
	subu	$5,$5,$4
	lwu	$16,4($29)
	addiu	$17,$29,192
	addiu	$16,$16,31
	or	$4,$5,$0
	dsrl	$16,$16,0x5
	dsll32	$4,$4,0x1b
	sltiu	$2,$16,32
	dsrl	$5,$5,0x5
	addu	$16,$16,$2
	dsrl32	$4,$4,0x1b
	dsll	$5,$5,0x2
	or	$8,$4,$0
	addiu	$9,$0,32
	dsubu	$9,$9,$8
	daddu	$18,$17,$5
	dsrl	$5,$5,0x2
	lwu	$6,0($18)
	addiu	$18,$18,4
	addiu	$16,$16,-1
	beq	$0,$0,.L00629498
	sll	$0,$0,0x0
.L00629470:
	lwu	$4,0($18)
	or	$7,$6,$0
	or	$6,$4,$0
	dsrlv	$7,$7,$8
	dsllv	$4,$4,$9
	addiu	$16,$16,-1
	or	$4,$4,$7
	addiu	$18,$18,4
	sw	$4,0($17)
	addiu	$17,$17,4
.L00629498:
	bne	$16,$5,.L00629470
	sll	$0,$0,0x0
	dsrlv	$6,$6,$8
	sw	$6,0($17)
	or	$4,$0,$0
	beq	$0,$0,.L006294C4
	sll	$0,$0,0x0
.L006294B4:
	addiu	$17,$17,4
	addiu	$16,$16,-1
	sw	$4,0($17)
	sll	$0,$0,0x0
.L006294C4:
	sll	$0,$0,0x0
	bne	$16,$0,.L006294B4
	sll	$0,$0,0x0
.L006294D0:
	lwu	$3,0($29)
	lwu	$4,12($29)
	addiu	$3,$3,-1
	addiu	$5,$29,192
	dsrl	$3,$3,0x5
	dsll	$2,$3,0x2
	addiu	$3,$3,1
	addu	$4,$4,$2
	addu	$5,$5,$2
	beq	$0,$0,.L00629528
	sll	$0,$0,0x0
.L006294FC:
	lwu	$8,0($4)
	lwu	$9,0($5)
	addiu	$3,$3,-1
	sltu	$2,$9,$8
	addiu	$4,$4,-4
	bgtz	$2,.L00629530
	sll	$0,$0,0x0
	sltu	$2,$8,$9
	addiu	$5,$5,-4
	bgtz	$2,.L00629594
	sll	$0,$0,0x0
.L00629528:
	bgtz	$3,.L006294FC
	sll	$0,$0,0x0
.L00629530:
	lwu	$20,0($29)
	lwu	$21,12($29)
	addiu	$20,$20,31
	addiu	$22,$29,192
	dsrl	$20,$20,0x5
	or	$4,$0,$0
	beq	$0,$0,.L00629574
	sll	$0,$0,0x0
.L00629550:
	lwu	$5,0($21)
	lwu	$6,0($22)
	addiu	$20,$20,-1
	daddu	$4,$4,$5
	addiu	$21,$21,4
	dsubu	$4,$4,$6
	addiu	$22,$22,4
	sw	$4,-4($21)
	dsra32	$4,$4,0x0
.L00629574:
	bne	$20,$0,.L00629550
	sll	$0,$0,0x0
	lwu	$4,12($29)
	jal	lintGetBits
	sll	$0,$0,0x0
	sw	$2,0($29)
	beq	$0,$0,.L006293E8
	sll	$0,$0,0x0
.L00629594:
	lwu	$5,4($29)
	lwu	$6,8($29)
	sltu	$3,$6,$5
	beq	$3,$0,.L00629670
	sll	$0,$0,0x0
	addiu	$6,$5,-1
	addiu	$5,$0,1
	sw	$6,4($29)
	lwu	$16,4($29)
	addiu	$17,$29,192
	addiu	$16,$16,31
	or	$4,$5,$0
	dsrl	$16,$16,0x5
	dsll32	$4,$4,0x1b
	sltiu	$2,$16,32
	dsrl	$5,$5,0x5
	addu	$16,$16,$2
	dsrl32	$4,$4,0x1b
	dsll	$5,$5,0x2
	or	$8,$4,$0
	addiu	$9,$0,32
	dsubu	$9,$9,$8
	daddu	$18,$17,$5
	dsrl	$5,$5,0x2
	lwu	$6,0($18)
	addiu	$18,$18,4
	addiu	$16,$16,-1
	beq	$0,$0,.L00629630
	sll	$0,$0,0x0
.L00629608:
	lwu	$4,0($18)
	or	$7,$6,$0
	or	$6,$4,$0
	dsrlv	$7,$7,$8
	dsllv	$4,$4,$9
	addiu	$16,$16,-1
	or	$4,$4,$7
	addiu	$18,$18,4
	sw	$4,0($17)
	addiu	$17,$17,4
.L00629630:
	bne	$16,$5,.L00629608
	sll	$0,$0,0x0
	dsrlv	$6,$6,$8
	sw	$6,0($17)
	or	$4,$0,$0
	beq	$0,$0,.L0062965C
	sll	$0,$0,0x0
.L0062964C:
	addiu	$17,$17,4
	addiu	$16,$16,-1
	sw	$4,0($17)
	sll	$0,$0,0x0
.L0062965C:
	sll	$0,$0,0x0
	bnel	$16,$0,.L0062964C
	sll	$0,$0,0x0
	beq	$0,$0,.L006293E8
	sll	$0,$0,0x0
.L00629670:
	lq	$31,48($29)
	lq	$16,64($29)
	lq	$17,80($29)
	lq	$18,96($29)
	lq	$19,112($29)
	lq	$20,128($29)
	lq	$21,144($29)
	lq	$22,160($29)
	lq	$23,176($29)
	addiu	$29,$29,320
	jr	$31
	sll	$0,$0,0x0
	.end	func_00629274
	.size	func_00629274,.-func_00629274

	.globl	func_006296A0
	.type	func_006296A0,@function
	.ent	func_006296A0
func_006296A0:
	addiu	$29,$29,-576
	sq	$31,176($29)
	sw	$4,24($29)
	sw	$5,0($29)
	sw	$6,4($29)
	or	$4,$5,$0
	jal	lintGetBits
	sll	$0,$0,0x0
	or	$25,$2,$0
	addiu	$3,$2,31
	srl	$2,$3,0x5
	sw	$2,8($29)
	lwu	$4,4($29)
	jal	lintGetBits
	sll	$0,$0,0x0
	addu	$25,$25,$2
	addiu	$3,$2,31
	srl	$2,$3,0x5
	sw	$2,12($29)
	dsrl	$25,$25,0x5
	addiu	$25,$25,1
	sltiu	$2,$25,32
	bne	$2,$0,.L00629704
	sll	$0,$0,0x0
	addiu	$25,$0,32
.L00629704:
	addiu	$4,$29,448
	or	$5,$0,$0
	jal	lintSetSmallValue
	sll	$0,$0,0x0
	addiu	$4,$29,192
	or	$5,$0,$0
	jal	lintSetSmallValue
	sll	$0,$0,0x0
	addiu	$4,$29,320
	or	$5,$0,$0
	jal	lintSetSmallValue
	sll	$0,$0,0x0
	addiu	$3,$29,192
	sw	$3,16($29)
	addiu	$3,$29,324
	sw	$3,20($29)
	lwu	$8,0($29)
	lwu	$14,8($29)
	lwu	$10,16($29)
	lwu	$11,20($29)
	beq	$0,$0,.L00629818
.L00629758:
	lwu	$9,4($29)
	lwu	$4,0($8)
	lwu	$15,12($29)
	beq	$0,$0,.L00629794
	sll	$0,$0,0x0
.L0062976C:
	lwu	$5,0($9)
	multu	$4,$5
	addiu	$9,$9,4
	addiu	$15,$15,-1
	mflo	$2
	mfhi	$3
	sw	$2,0($10)
	sw	$3,0($11)
	addiu	$10,$10,4
	addiu	$11,$11,4
.L00629794:
	bne	$15,$0,.L0062976C
	sll	$0,$0,0x0
	or	$9,$25,$0
	addiu	$11,$29,448
	addiu	$12,$29,192
	addiu	$13,$29,320
	or	$4,$0,$0
	beq	$0,$0,.L006297E8
	sll	$0,$0,0x0
.L006297B8:
	lwu	$5,0($11)
	lwu	$6,0($12)
	lwu	$7,0($13)
	addiu	$9,$9,-1
	daddu	$4,$4,$5
	addiu	$11,$11,4
	daddu	$4,$4,$6
	addiu	$12,$12,4
	daddu	$4,$4,$7
	addiu	$13,$13,4
	sw	$4,-4($11)
	dsrl32	$4,$4,0x0
.L006297E8:
	bne	$9,$0,.L006297B8
	sll	$0,$0,0x0
	lwu	$10,16($29)
	lwu	$11,20($29)
	sw	$0,0($10)
	sw	$0,0($11)
	addiu	$10,$10,4
	addiu	$11,$11,4
	addiu	$14,$14,-1
	sw	$10,16($29)
	sw	$11,20($29)
	addiu	$8,$8,4
.L00629818:
	bne	$14,$0,.L00629758
	sll	$0,$0,0x0
	addiu	$8,$0,32
	addiu	$9,$29,448
	lwu	$10,24($29)
	beq	$0,$0,.L00629850
	sll	$0,$0,0x0
.L00629834:
	lq	$4,0($9)
	lq	$5,16($9)
	addiu	$8,$8,-8
	addiu	$9,$9,32
	sq	$4,0($10)
	sq	$5,16($10)
	addiu	$10,$10,32
.L00629850:
	bne	$8,$0,.L00629834
	sll	$0,$0,0x0
	lq	$31,176($29)
	addiu	$29,$29,576
	jr	$31
	sll	$0,$0,0x0
	.end	func_006296A0
	.size	func_006296A0,.-func_006296A0

	.globl	func_00629868
	.type	func_00629868,@function
	.ent	func_00629868
func_00629868:
	addiu	$29,$29,-576
	sq	$31,176($29)
	sw	$4,24($29)
	sw	$5,0($29)
	sw	$6,4($29)
	sw	$7,28($29)
	or	$4,$5,$0
	jal	lintGetBits
	sll	$0,$0,0x0
	or	$25,$2,$0
	addiu	$3,$2,31
	srl	$2,$3,0x5
	sw	$2,8($29)
	lwu	$4,4($29)
	jal	lintGetBits
	sll	$0,$0,0x0
	addu	$25,$25,$2
	addiu	$3,$2,31
	srl	$2,$3,0x5
	sw	$2,12($29)
	dsrl	$25,$25,0x5
	addiu	$25,$25,1
	sltiu	$2,$25,32
	bne	$2,$0,.L006298D0
	sll	$0,$0,0x0
	addiu	$25,$0,32
.L006298D0:
	addiu	$4,$29,448
	or	$5,$0,$0
	jal	lintSetSmallValue
	sll	$0,$0,0x0
	addiu	$4,$29,192
	or	$5,$0,$0
	jal	lintSetSmallValue
	sll	$0,$0,0x0
	addiu	$4,$29,320
	or	$5,$0,$0
	jal	lintSetSmallValue
	sll	$0,$0,0x0
	addiu	$3,$29,192
	sw	$3,16($29)
	addiu	$3,$29,324
	sw	$3,20($29)
	lwu	$8,0($29)
	lwu	$14,8($29)
	lwu	$10,16($29)
	lwu	$11,20($29)
	beq	$0,$0,.L006299E4
.L00629924:
	lwu	$9,4($29)
	lwu	$4,0($8)
	lwu	$15,12($29)
	beq	$0,$0,.L00629960
	sll	$0,$0,0x0
.L00629938:
	lwu	$5,0($9)
	multu	$4,$5
	addiu	$9,$9,4
	addiu	$15,$15,-1
	mflo	$2
	mfhi	$3
	sw	$2,0($10)
	sw	$3,0($11)
	addiu	$10,$10,4
	addiu	$11,$11,4
.L00629960:
	bne	$15,$0,.L00629938
	sll	$0,$0,0x0
	or	$9,$25,$0
	addiu	$11,$29,448
	addiu	$12,$29,192
	addiu	$13,$29,320
	or	$4,$0,$0
	beq	$0,$0,.L006299B4
	sll	$0,$0,0x0
.L00629984:
	lwu	$5,0($11)
	lwu	$6,0($12)
	lwu	$7,0($13)
	addiu	$9,$9,-1
	daddu	$4,$4,$5
	addiu	$11,$11,4
	daddu	$4,$4,$6
	addiu	$12,$12,4
	daddu	$4,$4,$7
	addiu	$13,$13,4
	sw	$4,-4($11)
	dsrl32	$4,$4,0x0
.L006299B4:
	bne	$9,$0,.L00629984
	sll	$0,$0,0x0
	lwu	$10,16($29)
	lwu	$11,20($29)
	sw	$0,0($10)
	sw	$0,0($11)
	addiu	$10,$10,4
	addiu	$11,$11,4
	addiu	$14,$14,-1
	sw	$10,16($29)
	sw	$11,20($29)
	addiu	$8,$8,4
.L006299E4:
	bne	$14,$0,.L00629924
	sll	$0,$0,0x0
	addiu	$4,$29,448
	lwu	$5,28($29)
	jal	func_00629274
	sll	$0,$0,0x0
	addiu	$8,$0,32
	addiu	$9,$29,448
	lwu	$10,24($29)
	beq	$0,$0,.L00629A2C
	sll	$0,$0,0x0
.L00629A10:
	lq	$4,0($9)
	lq	$5,16($9)
	addiu	$8,$8,-8
	addiu	$9,$9,32
	sq	$4,0($10)
	sq	$5,16($10)
	addiu	$10,$10,32
.L00629A2C:
	bne	$8,$0,.L00629A10
	sll	$0,$0,0x0
	lq	$31,176($29)
	addiu	$29,$29,576
	jr	$31
	sll	$0,$0,0x0
	.end	func_00629868
	.size	func_00629868,.-func_00629868
