# sc: compiler=ee-gcc2.96
# newlib's memmove for the EE (MAIN, the libc the game linked: README "The GCC front"): one
# assembler object, as the demo's memmove is -- its .text section symbol carries no gcc2_compiled.
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

	.globl	memmove
	.type	memmove,@function
	.ent	memmove
memmove:
	daddu	$8,$4,$0
	sltu	$2,$5,$8
	beq	$2,$0,.L00195908
	daddu	$3,$8,$0
	addu	$7,$5,$6
	sltu	$2,$8,$7
	beq	$2,$0,.L00195908
	addiu	$2,$0,-1
	addu	$3,$8,$6
	addiu	$6,$6,-1
	beq	$6,$2,.L001959AC
	daddu	$5,$7,$0
	daddu	$4,$2,$0
.L001958E4:
	addiu	$5,$5,-1
	addiu	$3,$3,-1
	lbu	$2,0($5)
	addiu	$6,$6,-1
	sb	$2,0($3)
	bne	$6,$4,.L001958E4
	sll	$0,$0,0x0
	jr	$31
	daddu	$2,$8,$0
.L00195908:
	sltiu	$2,$6,32
	bnel	$2,$0,.L00195984
	addiu	$6,$6,-1
	or	$2,$5,$3
	andi	$2,$2,0xf
	bnel	$2,$0,.L00195984
	addiu	$6,$6,-1
	daddu	$7,$3,$0
.L00195928:
	lq	$3,0($5)
	addiu	$6,$6,-32
	addiu	$5,$5,16
	sltiu	$4,$6,32
	sq	$3,0($7)
	addiu	$7,$7,16
	lq	$2,0($5)
	addiu	$5,$5,16
	sq	$2,0($7)
	beq	$4,$0,.L00195928
	addiu	$7,$7,16
	sltiu	$2,$6,8
	bne	$2,$0,.L00195980
	daddu	$3,$7,$0
.L00195960:
	ld	$3,0($5)
	addiu	$6,$6,-8
	addiu	$5,$5,8
	sltiu	$2,$6,8
	sd	$3,0($7)
	beq	$2,$0,.L00195960
	addiu	$7,$7,8
	daddu	$3,$7,$0
.L00195980:
	addiu	$6,$6,-1
.L00195984:
	addiu	$2,$0,-1
	beq	$6,$2,.L001959AC
	daddu	$4,$2,$0
.L00195990:
	lbu	$2,0($5)
	addiu	$6,$6,-1
	addiu	$5,$5,1
	sb	$2,0($3)
	addiu	$3,$3,1
	bne	$6,$4,.L00195990
	sll	$0,$0,0x0
.L001959AC:
	jr	$31
	daddu	$2,$8,$0
	.end	memmove
	.size	memmove,.-memmove
