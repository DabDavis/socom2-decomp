# sc: compiler=ee-gcc2.96
# crt0.s -- Sony's start-up (MAIN 0x180008-0x1801DF): _start, the ELF's entry point (the registers
# zeroed, .bss cleared, the main thread's stack and heap set up, the arguments passed, main called,
# then Exit), _exit (Exit(0)) and _root (ExitThread). SOCOM 1's DWARF has its unit
# (C:\usr\local\sce_2.5.1\EE\lib\crt0.s, 0x140008-0x1401E0, the same three functions).
# THE ORIGINAL WAS ASSEMBLED BY METROWERKS' ASSEMBLER (asm_r5900_elf: the unit carries MW's DWARF
# in SOCOM 1, where GCC's objects carry gcc2_compiled.), which scdiff has no path for; GNU as here
# gives the same bytes. Transcribed by tools/asmunit.py.

	.set	noreorder
	.set	noat
	.set	nomacro

	.text
	.align	3

	.globl	_start
	.type	_start,@function
	.ent	_start
_start:
	padduw	$1,$0,$0
	padduw	$2,$0,$0
	padduw	$3,$0,$0
	padduw	$4,$0,$0
	padduw	$5,$0,$0
	padduw	$6,$0,$0
	padduw	$7,$0,$0
	padduw	$8,$0,$0
	padduw	$9,$0,$0
	padduw	$10,$0,$0
	padduw	$11,$0,$0
	padduw	$12,$0,$0
	padduw	$13,$0,$0
	padduw	$14,$0,$0
	padduw	$15,$0,$0
	padduw	$16,$0,$0
	padduw	$17,$0,$0
	padduw	$18,$0,$0
	padduw	$19,$0,$0
	padduw	$20,$0,$0
	padduw	$21,$0,$0
	padduw	$22,$0,$0
	padduw	$23,$0,$0
	padduw	$24,$0,$0
	padduw	$25,$0,$0
	padduw	$28,$0,$0
	padduw	$29,$0,$0
	padduw	$30,$0,$0
	padduw	$31,$0,$0
	mthi	$0
	mthi1	$0
	mtlo	$0
	mtlo1	$0
	mtsah	$0,0
	mtc1	$0,$f0
	mtc1	$0,$f1
	mtc1	$0,$f2
	mtc1	$0,$f3
	mtc1	$0,$f4
	mtc1	$0,$f5
	mtc1	$0,$f6
	mtc1	$0,$f7
	mtc1	$0,$f8
	mtc1	$0,$f9
	mtc1	$0,$f10
	mtc1	$0,$f11
	mtc1	$0,$f12
	mtc1	$0,$f13
	mtc1	$0,$f14
	mtc1	$0,$f15
	mtc1	$0,$f16
	mtc1	$0,$f17
	mtc1	$0,$f18
	mtc1	$0,$f19
	mtc1	$0,$f20
	mtc1	$0,$f21
	mtc1	$0,$f22
	mtc1	$0,$f23
	mtc1	$0,$f24
	mtc1	$0,$f25
	mtc1	$0,$f26
	mtc1	$0,$f27
	mtc1	$0,$f28
	mtc1	$0,$f29
	mtc1	$0,$f30
	mtc1	$0,$f31
	adda.s	$f0,$f1
	sync.p
	.word	0x44c0f800		# ctc1 $0,c1_fcsr
	lui	$2,%hi(_fbss)
	lui	$3,0x68
	addiu	$2,$2,%lo(_fbss)
	addiu	$3,$3,28544
.L0018012C:
	sq	$0,0($2)
	sll	$0,$0,0x0
	sltu	$1,$2,$3
	sll	$0,$0,0x0
	sll	$0,$0,0x0
	bne	$1,$0,.L0018012C
	addiu	$2,$2,16
	lui	$4,%hi(_gp)
	lui	$5,0x0
	lui	$6,0x8
	lui	$7,%hi(_args)
	lui	$8,%hi(_root)
	addiu	$4,$4,%lo(_gp)
	addiu	$5,$5,-1
	addiu	$6,$6,0
	addiu	$7,$7,%lo(_args)
	addiu	$8,$8,%lo(_root)
	or	$28,$4,$0
	addiu	$3,$0,60
	syscall
	or	$29,$2,$0
	lui	$4,0x68
	lui	$5,0x0
	addiu	$4,$4,28544
	addiu	$5,$5,-1
	addiu	$3,$0,61
	syscall
	jal	_InitSys
	sll	$0,$0,0x0
	jal	FlushCache
	or	$4,$0,$0
	ei
	lui	$2,%hi(_args)
	addiu	$2,$2,%lo(_args)
	lw	$4,0($2)
	jal	func_001C4CC0
	addiu	$5,$2,4
	j	Exit
	or	$4,$2,$0
	.end	_start
	.size	_start,.-_start

	.globl	_exit
	.type	_exit,@function
	.ent	_exit
_exit:
	j	Exit
	or	$4,$0,$0
	.end	_exit
	.size	_exit,.-_exit

	.globl	_root
	.type	_root,@function
	.ent	_root
_root:
	addiu	$3,$0,35
	syscall
	sll	$0,$0,0x0
	sll	$0,$0,0x0
	.end	_root
	.size	_root,.-_root

	.bss
	.align	4
_args:
	.space	324
