# sc: compiler=ee-gcc2.96
# newlib's memset for the EE (MAIN, the libc the game linked: README "The GCC front"): one
# assembler object, as the demo's memset is -- its .text section symbol carries no gcc2_compiled.
# marker, where every compiled libc object has one; 128-bit lq/sq and MMI byte compares over
# aligned quadwords, a byte loop for the rest. Transcribed by tools/asmunit.py.
# Against the demo's: identical but for one short loop's tail -- the demo's object puts a nop
# before the loop's branch and the increment in its delay slot (sb; nop; bne; addiu), this one
# the increment before the branch and a nop in the slot (sb; addiu; bne; nop).

	.set	noreorder
	.set	noat
	.set	nomacro

	.text
	.align	3

	.globl	memset
	.type	memset,@function
	.ent	memset
memset:
	sltiu	$2,$6,8
	bne	$2,$0,.L00195A38
	daddu	$3,$4,$0
	andi	$2,$4,0xf
	bne	$2,$0,.L00195A38
	daddu	$7,$4,$0
	andi	$9,$5,0xff
	sltiu	$10,$6,32
	daddu	$8,$9,$0
	dsll	$3,$8,0x8
	or	$8,$3,$9
	pcpyh	$3,$8
	bne	$10,$0,.L00195A24
	sltiu	$2,$6,8
	pcpyld	$8,$3,$3
.L001959F4:
	sq	$8,0($7)
	addiu	$6,$6,-32
	addiu	$7,$7,16
	sltiu	$2,$6,32
	sq	$8,0($7)
	beq	$2,$0,.L001959F4
	addiu	$7,$7,16
	beq	$0,$0,.L00195A24
	sltiu	$2,$6,8
.L00195A18:
	addiu	$6,$6,-8
	addiu	$7,$7,8
	sltiu	$2,$6,8
.L00195A24:
	sll	$0,$0,0x0
	sll	$0,$0,0x0
	beql	$2,$0,.L00195A18
	sd	$3,0($7)
	daddu	$3,$7,$0
.L00195A38:
	lui	$2,0xffff
	addiu	$6,$6,-1
	ori	$2,$2,0xffff
	beq	$6,$2,.L00195A70
	sll	$0,$0,0x0
	lui	$2,0xffff
	ori	$2,$2,0xffff
.L00195A54:
	sb	$5,0($3)
	addiu	$6,$6,-1
	addiu	$3,$3,1
	sll	$0,$0,0x0
	sll	$0,$0,0x0
	bne	$6,$2,.L00195A54
	sll	$0,$0,0x0
.L00195A70:
	jr	$31
	daddu	$2,$4,$0
	.end	memset
	.size	memset,.-memset
