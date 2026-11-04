/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libmpeg's decoder (MAIN 0x1B5160-0x1B9EE0): Sony's SDK, the demo's object 0x152F60 (C). This
 * unit holds, in the demo's order, the reference interpolators (_rix_000 ... _copyRefImage, inline
 * asm), _ipuSetMPEG1 and _dmVector, the IPU command and wait primitives (_sendIpuCommand ...
 * _nextStartCode) and the header parsers (_sliceB ... _copyrightExtension); the motion
 * compensation before them (_motionComp0 ... _doMC), the macroblock layer between (_waitBdecOut,
 * 0x1B6ED0, ... _motionVector) and most of the picture output after them are not written yet (their
 * rows stand as drafts or open); GCC round 19a wrote the picture layer's _decPicture (its switch's
 * frame case first: the target lays case 3's body right after the dispatch tree) and _outputFrame
 * (the field pair chosen into locals, one call), _isOutSizeOK and the object's last function,
 * receiveDataFromIPU. _updateTempTackData is left to its matching draft. Names are SOCOM 1's
 * (s1:exact).
 *
 * The IPU's idle wait and the command write are macros, expanded inside the readers
 * (_ipuVdec, _peepBit, _flushBuf, _nextBit) as well as being functions of their own: the
 * readers' copies keep no constant in a saved register, the functions' do. A command's index
 * into _isDirty shifts as the command's own type does: unsigned in _flushBuf and _nextBit (srl),
 * signed in _ipuVdec (sra). _nextHeader's cases are laid out in source order (sequence, GOP,
 * picture); _pictureDisplayExtension tests progressive_sequence first; _nextBit takes its result
 * before resetting nbits.
 *
 * OPEN: _ipuVdec (0x180 against 0x178: the target reads IPU_TOP as `lui; ld 0x2030(reg)` and
 * IPU_BP as `lui; ori; lw`, and returns from the command word's register -- two words longer
 * here, in the tail); _extensionAndUserData (0x11C against 0x128: the target hoists both start
 * codes, 0x1B5 and 0x1B2, into saved registers and tests them at the loop's foot; the `while
 * (a == X || a == Y)` written here keeps only one). */

#include "libmpeg.h"

extern int _isDirty[];
extern int _dispatchMpegCbNodata(sceMpeg *mp);
extern int _dispatchMpegCallback(sceMpeg *mp, sceMpegCbData *cbdata);
extern void _Error(MpegDec *d, char *msg);
extern void _sequenceHeader(MpegDec *d);
extern void _updateTempTackData(MpegDec *d, int tr);
extern void _unknown_extension(MpegDec *d);
extern void _sequenceExtension(MpegDec *d);
extern void _sequenceDisplayExtension(MpegDec *d);
extern void _sequenceScalableExtension(MpegDec *d);
extern void _pictureSpatialScalableExtension(MpegDec *d);
extern void _pictureTemporalScalableExtension(MpegDec *d);

void _waitIpuIdle(MpegDec *d);
short _ipuVdec(MpegDec *d, int tbl);
long _waitIpuIdle64(MpegDec *d);
u_int _peepBit(MpegDec *d, int n);
void _flushBuf(MpegDec *d, int n);
u_int _nextBit(MpegDec *d, int n);
void _nextStartCode(MpegDec *d);
void _extrainfo(MpegDec *d);
void _pictureHeader(MpegDec *d);
void _extensionAndUserData(MpegDec *d);
void _pictureCodingExtension(MpegDec *d);
void _groupOfPicturesHeader(MpegDec *d);
void _quantMatrixExtension(MpegDec *d);
void _pictureDisplayExtension(MpegDec *d);
void _copyrightExtension(MpegDec *d);

#define WAIT_IDLE(d) {								\
	int _i = 0;								\
	while ((*IPU_CTRL & 0x80004000) == 0x80000000) {			\
		if (_i++ > 5000) {						\
			_dispatchMpegCbNodata((d)->mp);				\
			_i = 0;							\
		}								\
	}									\
}

#define SEND_CMD(d, cmd) {							\
	*IPU_CMD_W = (cmd);							\
	(d)->dirty = _isDirty[(cmd) >> 28];					\
}

/* The reference fetch's interpolators, eight _rix_ and eight _ri0_ (the demo's names; _doMC
 * tail-calls them, and the table at 0x1CE378 holds them): MMI over a block of quadword rows, the
 * source realigned by qfsrv. Each is one asm in noreorder mode, its words the target's: written
 * for reorder mode (each filled slot handed back to GNU as) 13 of the 16 compile byte-exact under
 * this toolchain's as and 2.96's, none under 991111's (10-51 words off: its as fills no slot, as
 * in libvu0) -- but the four _ri0_XY0 clear their outer-loop flag in the slot of the branch that
 * tests it (`bnez t8; move t8,zero`), which reorder mode cannot write. The argument block, as the
 * bodies read it: */
typedef struct {
	void *dst;	/* 0x00: halfword pixels out, two rows' stride a row */
	int shift;	/* 0x04: the byte offset into the source quadwords (mtsab) */
	int rows;	/* 0x08 */
	int again;	/* 0x0C: a second pass (the other field) */
	int stride;	/* 0x10 */
	void *src0;	/* 0x14 */
	void *src1;	/* 0x18: the next quadword of each row */
} MpegRi;

void _rix_000(MpegRi *ri)
{
	asm __volatile__("
	.set noreorder
	lw $5,20($4)
	lw $6,24($4)
	lw $7,8($4)
	lw $14,0($4)
	lw $13,4($4)
	lw $12,16($4)
	sll $11,$12,0x1
	addiu $15,$0,-1
	mtsab $13,0
1:
	lq $8,0($5)
	addi $7,$7,-1
	lq $9,0($6)
	addu $5,$5,$12
	qfsrv $10,$9,$8
	pextlb $8,$0,$10
	pextub $9,$0,$10
	sq $8,0($14)
	addu $6,$6,$12
	sq $9,16($14)
	bgtz $7,1b
	addu $14,$14,$11
	addiu $5,$5,128
	addiu $6,$6,128
	lw $7,12($4)
	and $10,$15,$7
	bne $10,$0,1b
	daddu $15,$0,$0
	.set reorder
	");
}

void _ri0_000(MpegRi *ri)
{
	asm __volatile__("
	.set noreorder
	lw $5,20($4)
	lw $6,24($4)
	lw $14,0($4)
	lw $13,4($4)
	lw $12,16($4)
	sll $11,$12,0x1
	mtsab $13,0
	addiu $24,$0,-1
1:
	lw $7,8($4)
	addiu $15,$0,-1
2:
	ld $8,0($5)
	ld $9,0($6)
	pcpyld $8,$9,$8
	qfsrv $9,$8,$8
	pextlb $8,$0,$9
	sq $8,0($14)
	addi $7,$7,-1
	addu $5,$5,$12
	addu $14,$14,$11
	bgtz $7,2b
	addu $6,$6,$12
	addiu $5,$5,320
	addiu $6,$6,320
	lw $7,12($4)
	and $10,$15,$7
	bne $10,$0,2b
	daddu $15,$0,$0
	lw $5,20($4)
	lw $6,24($4)
	lw $14,0($4)
	addiu $5,$5,64
	addiu $6,$6,64
	addiu $14,$14,128
	bne $24,$0,1b
	daddu $24,$0,$0
	.set reorder
	");
}

void _rix_001(MpegRi *ri)
{
	asm __volatile__("
	.set noreorder
	pnor $25,$0,$0
	psrlh $25,$25,0xf
	lw $5,20($4)
	lw $6,24($4)
	lw $7,8($4)
	lw $14,0($4)
	lw $13,4($4)
	lw $24,16($4)
	lq $8,0($5)
	sll $12,$24,0x1
	lq $9,0($6)
	mtsab $13,0
	qfsrv $10,$9,$8
	pextlb $8,$0,$10
	addiu $11,$0,-1
	beq $7,$0,2f
	pextub $9,$0,$10
1:
	addu $5,$5,$24
	addu $6,$6,$24
	lq $10,0($5)
	lq $15,0($6)
	qfsrv $2,$15,$10
	pextlb $10,$0,$2
	addi $7,$7,-1
	pextub $15,$0,$2
	paddh $2,$8,$10
	paddh $3,$9,$15
	por $8,$10,$0
	por $9,$15,$0
	paddh $2,$2,$25
	paddh $3,$3,$25
	psrlh $2,$2,0x1
	psrlh $3,$3,0x1
	sq $2,0($14)
	sq $3,16($14)
	bgtz $7,1b
	addu $14,$14,$12
2:
	addiu $5,$5,128
	addiu $6,$6,128
	lw $7,12($4)
	and $10,$11,$7
	bne $10,$0,1b
	daddu $11,$0,$0
	.set reorder
	");
}

void _ri0_001(MpegRi *ri)
{
	asm __volatile__("
	.set noreorder
	pnor $25,$0,$0
	psrlh $25,$25,0xf
	lw $5,20($4)
	lw $6,24($4)
	lw $14,0($4)
	lw $13,4($4)
	lw $12,16($4)
	addiu $11,$0,1
	sll $24,$12,0x1
	mtsab $13,0
1:
	lw $7,8($4)
	ld $8,0($5)
	ld $9,0($6)
	pcpyld $8,$9,$8
	qfsrv $8,$8,$8
	ori $11,$11,0x8000
	beq $7,$0,3f
	pextlb $15,$0,$8
2:
	addu $5,$5,$12
	addu $6,$6,$12
	ld $8,0($5)
	ld $9,0($6)
	pcpyld $8,$9,$8
	qfsrv $8,$8,$8
	pextlb $10,$0,$8
	addi $7,$7,-1
	paddh $9,$10,$15
	por $15,$10,$0
	paddh $10,$9,$25
	psrlh $10,$10,0x1
	sq $10,0($14)
	bgtz $7,2b
	addu $14,$14,$24
3:
	psrah $10,$11,0xf
	addiu $5,$5,320
	lw $7,12($4)
	addiu $6,$6,320
	and $10,$10,$7
	bne $10,$0,2b
	andi $11,$11,0x7fff
	lw $5,20($4)
	lw $6,24($4)
	lw $14,0($4)
	addiu $5,$5,64
	addiu $6,$6,64
	addiu $14,$14,128
	andi $10,$11,0x1
	bne $10,$0,1b
	andi $11,$11,0xfffe
	.set reorder
	");
}

void _rix_010(MpegRi *ri)
{
	asm __volatile__("
	.set noreorder
	pnor $25,$0,$0
	psrlh $25,$25,0xf
	lw $5,20($4)
	lw $6,24($4)
	lw $7,8($4)
	lw $14,0($4)
	lw $13,4($4)
	addiu $24,$0,1
	lw $9,16($4)
	sll $8,$9,0x1
	addiu $11,$0,-1
1:
	lq $10,0($5)
	lq $15,0($6)
	mtsab $13,0
	qfsrv $2,$15,$10
	qfsrv $3,$10,$15
	pextlb $10,$0,$2
	addi $7,$7,-1
	pextub $15,$0,$2
	mtsab $24,0
	qfsrv $3,$3,$2
	pextlb $2,$0,$3
	pextub $3,$0,$3
	paddh $10,$10,$2
	paddh $15,$15,$3
	paddh $2,$10,$25
	paddh $3,$15,$25
	psrlh $2,$2,0x1
	psrlh $3,$3,0x1
	sq $2,0($14)
	sq $3,16($14)
	addu $5,$5,$9
	addu $6,$6,$9
	bgtz $7,1b
	addu $14,$14,$8
	addiu $5,$5,128
	addiu $6,$6,128
	lw $7,12($4)
	and $12,$11,$7
	bne $12,$0,1b
	daddu $11,$0,$0
	.set reorder
	");
}

void _ri0_010(MpegRi *ri)
{
	asm __volatile__("
	.set noreorder
	pnor $25,$0,$0
	psrlh $25,$25,0xf
	lw $5,20($4)
	lw $6,24($4)
	lw $14,0($4)
	lw $13,4($4)
	addiu $24,$0,1
	addiu $12,$0,-1
	lw $3,16($4)
	sll $2,$3,0x1
1:
	lw $7,8($4)
	addiu $11,$0,-1
2:
	ld $8,0($5)
	ld $9,0($6)
	pcpyld $8,$9,$8
	mtsab $13,0
	qfsrv $8,$8,$8
	pextlb $9,$0,$8
	addi $7,$7,-1
	addu $5,$5,$3
	addu $6,$6,$3
	mtsab $24,0
	qfsrv $10,$0,$8
	pextlb $8,$0,$10
	paddh $10,$9,$8
	paddh $10,$10,$25
	psrlh $10,$10,0x1
	sq $10,0($14)
	bgtz $7,2b
	addu $14,$14,$2
	addiu $5,$5,320
	addiu $6,$6,320
	lw $7,12($4)
	and $10,$11,$7
	bne $10,$0,2b
	daddu $11,$0,$0
	lw $5,20($4)
	lw $6,24($4)
	lw $14,0($4)
	addiu $5,$5,64
	addiu $6,$6,64
	addiu $14,$14,128
	bne $12,$0,1b
	daddu $12,$0,$0
	.set reorder
	");
}

void _rix_011(MpegRi *ri)
{
	asm __volatile__("
	.set noreorder
	pnor $25,$0,$0
	psrlh $25,$25,0xf
	psllh $25,$25,0x1
	lw $5,20($4)
	lw $6,24($4)
	lw $7,8($4)
	lw $14,0($4)
	lw $13,4($4)
	lw $12,16($4)
	addiu $24,$0,1
	lq $8,0($5)
	lq $9,0($6)
	mtsab $13,0
	qfsrv $10,$9,$8
	qfsrv $15,$8,$9
	pextlb $8,$0,$10
	pextub $9,$0,$10
	addiu $11,$0,-1
	mtsab $24,0
	qfsrv $15,$15,$10
	pextlb $10,$0,$15
	pextub $15,$0,$15
	paddh $8,$8,$10
	beq $7,$0,2f
	paddh $9,$9,$15
1:
	addu $5,$5,$12
	addu $6,$6,$12
	lq $10,0($5)
	lq $15,0($6)
	mtsab $13,0
	qfsrv $2,$15,$10
	qfsrv $3,$10,$15
	pextlb $10,$0,$2
	addi $7,$7,-1
	pextub $15,$0,$2
	mtsab $24,0
	qfsrv $3,$3,$2
	pextlb $2,$0,$3
	pextub $3,$0,$3
	paddh $10,$10,$2
	paddh $15,$15,$3
	paddh $2,$8,$10
	paddh $3,$9,$15
	por $8,$10,$0
	por $9,$15,$0
	paddh $2,$2,$25
	paddh $3,$3,$25
	psrlh $2,$2,0x2
	psrlh $3,$3,0x2
	sq $2,0($14)
	sll $10,$12,0x1
	sq $3,16($14)
	bgtz $7,1b
	addu $14,$14,$10
2:
	addiu $5,$5,128
	addiu $6,$6,128
	lw $7,12($4)
	and $10,$11,$7
	bne $10,$0,1b
	daddu $11,$0,$0
	.set reorder
	");
}

void _ri0_011(MpegRi *ri)
{
	asm __volatile__("
	.set noreorder
	pnor $25,$0,$0
	psrlh $25,$25,0xf
	psllh $25,$25,0x1
	lw $5,20($4)
	lw $6,24($4)
	lw $14,0($4)
	lw $13,4($4)
	lw $12,16($4)
	addiu $24,$0,1
	addiu $11,$0,1
1:
	lw $7,8($4)
	ld $8,0($5)
	ld $9,0($6)
	pcpyld $8,$9,$8
	mtsab $13,0
	qfsrv $8,$8,$8
	pextlb $9,$0,$8
	addu $5,$5,$12
	ori $11,$11,0x8000
	mtsab $24,0
	qfsrv $10,$0,$8
	pextlb $8,$0,$10
	beq $7,$0,3f
	paddh $15,$9,$8
2:
	addu $6,$6,$12
	ld $8,0($5)
	ld $9,0($6)
	pcpyld $8,$9,$8
	mtsab $13,0
	qfsrv $8,$8,$8
	pextlb $9,$0,$8
	addi $7,$7,-1
	addu $5,$5,$12
	mtsab $24,0
	qfsrv $10,$0,$8
	pextlb $8,$0,$10
	paddh $10,$9,$8
	paddh $9,$10,$15
	por $15,$10,$0
	paddh $10,$9,$25
	sll $8,$12,0x1
	psrlh $10,$10,0x2
	sq $10,0($14)
	bgtz $7,2b
	addu $14,$14,$8
3:
	psrah $10,$11,0xf
	addiu $5,$5,320
	lw $7,12($4)
	addiu $6,$6,320
	and $10,$10,$7
	bne $10,$0,2b
	andi $11,$11,0x7fff
	lw $5,20($4)
	lw $6,24($4)
	lw $14,0($4)
	addiu $5,$5,64
	addiu $6,$6,64
	addiu $14,$14,128
	andi $10,$11,0x1
	bne $10,$0,1b
	andi $11,$11,0xfffe
	.set reorder
	");
}

void _rix_100(MpegRi *ri)
{
	asm __volatile__("
	.set noreorder
	lw $5,20($4)
	lw $6,24($4)
	lw $7,8($4)
	lw $14,0($4)
	lw $13,4($4)
	lw $9,16($4)
	sll $8,$9,0x1
	addiu $11,$0,-1
	mtsab $13,0
1:
	lq $10,0($5)
	lq $15,0($6)
	qfsrv $2,$15,$10
	pextlb $10,$0,$2
	pextub $15,$0,$2
	lq $2,0($14)
	lq $3,16($14)
	paddh $2,$2,$10
	paddh $3,$3,$15
	pnor $25,$0,$0
	psrlh $25,$25,0xf
	paddh $10,$2,$25
	psrlh $2,$10,0x1
	paddh $10,$3,$25
	psrlh $3,$10,0x1
	sq $2,0($14)
	sq $3,16($14)
	addi $7,$7,-1
	addu $5,$5,$9
	addu $14,$14,$8
	bgtz $7,1b
	addu $6,$6,$9
	addiu $5,$5,128
	addiu $6,$6,128
	lw $7,12($4)
	and $12,$11,$7
	bne $12,$0,1b
	daddu $11,$0,$0
	.set reorder
	");
}

void _ri0_100(MpegRi *ri)
{
	asm __volatile__("
	.set noreorder
	lw $5,20($4)
	lw $6,24($4)
	lw $14,0($4)
	lw $13,4($4)
	addiu $12,$0,-1
	lw $3,16($4)
	sll $2,$3,0x1
	mtsab $13,0
1:
	lw $7,8($4)
	addiu $11,$0,-1
2:
	ld $8,0($5)
	ld $9,0($6)
	pcpyld $8,$9,$8
	qfsrv $8,$8,$8
	pextlb $9,$0,$8
	addi $7,$7,-1
	addu $5,$5,$3
	addu $6,$6,$3
	lq $8,0($14)
	paddh $10,$9,$8
	pcgth $9,$10,$0
	psrlh $9,$9,0xf
	paddh $10,$10,$9
	psrlh $10,$10,0x1
	sq $10,0($14)
	bgtz $7,2b
	addu $14,$14,$2
	addiu $5,$5,320
	addiu $6,$6,320
	lw $7,12($4)
	and $10,$11,$7
	bne $10,$0,2b
	daddu $11,$0,$0
	lw $5,20($4)
	lw $6,24($4)
	lw $14,0($4)
	addiu $5,$5,64
	addiu $6,$6,64
	addiu $14,$14,128
	bne $12,$0,1b
	daddu $12,$0,$0
	.set reorder
	");
}

void _rix_101(MpegRi *ri)
{
	asm __volatile__("
	.set noreorder
	pnor $25,$0,$0
	psrlh $25,$25,0xf
	lw $5,20($4)
	lw $6,24($4)
	lw $7,8($4)
	lw $14,0($4)
	lw $13,4($4)
	lw $12,16($4)
	lq $8,0($5)
	lq $9,0($6)
	mtsab $13,0
	qfsrv $10,$9,$8
	sll $24,$12,0x1
	pextlb $8,$0,$10
	addiu $11,$0,-1
	beq $7,$0,2f
	pextub $9,$0,$10
1:
	addu $5,$5,$12
	addu $6,$6,$12
	lq $10,0($5)
	lq $15,0($6)
	qfsrv $2,$15,$10
	pextlb $10,$0,$2
	addi $7,$7,-1
	pextub $15,$0,$2
	paddh $2,$8,$10
	paddh $3,$9,$15
	por $8,$10,$0
	por $9,$15,$0
	paddh $2,$2,$25
	paddh $3,$3,$25
	psrlh $2,$2,0x1
	psrlh $3,$3,0x1
	lq $10,0($14)
	lq $15,16($14)
	paddh $2,$2,$10
	paddh $3,$3,$15
	paddh $10,$2,$25
	psrlh $2,$10,0x1
	pcgth $10,$3,$0
	psrlh $10,$10,0xf
	paddh $10,$3,$10
	psrlh $3,$10,0x1
	sq $2,0($14)
	sq $3,16($14)
	bgtz $7,1b
	addu $14,$14,$24
2:
	addiu $5,$5,128
	addiu $6,$6,128
	lw $7,12($4)
	and $10,$11,$7
	bne $10,$0,1b
	daddu $11,$0,$0
	.set reorder
	");
}

void _ri0_101(MpegRi *ri)
{
	asm __volatile__("
	.set noreorder
	pnor $25,$0,$0
	psrlh $25,$25,0xf
	lw $5,20($4)
	lw $6,24($4)
	lw $14,0($4)
	lw $13,4($4)
	lw $12,16($4)
	addiu $11,$0,1
	sll $24,$12,0x1
	mtsab $13,0
1:
	lw $7,8($4)
	ld $8,0($5)
	ld $9,0($6)
	pcpyld $8,$9,$8
	qfsrv $8,$8,$8
	ori $11,$11,0x8000
	beq $7,$0,3f
	pextlb $15,$0,$8
2:
	addu $5,$5,$12
	addu $6,$6,$12
	ld $8,0($5)
	ld $9,0($6)
	pcpyld $8,$9,$8
	qfsrv $8,$8,$8
	pextlb $10,$0,$8
	addi $7,$7,-1
	paddh $9,$10,$15
	por $15,$10,$0
	paddh $10,$9,$25
	psrlh $10,$10,0x1
	lq $8,0($14)
	paddh $10,$10,$8
	pcgth $9,$10,$0
	psrlh $9,$9,0xf
	paddh $10,$10,$9
	psrlh $10,$10,0x1
	sq $10,0($14)
	bgtz $7,2b
	addu $14,$14,$24
3:
	psrah $10,$11,0xf
	addiu $5,$5,320
	lw $7,12($4)
	addiu $6,$6,320
	and $10,$10,$7
	bne $10,$0,2b
	andi $11,$11,0x7fff
	lw $5,20($4)
	lw $6,24($4)
	lw $14,0($4)
	addiu $5,$5,64
	addiu $6,$6,64
	addiu $14,$14,128
	andi $10,$11,0x1
	bne $10,$0,1b
	andi $11,$11,0xfffe
	.set reorder
	");
}

void _rix_110(MpegRi *ri)
{
	asm __volatile__("
	.set noreorder
	pnor $25,$0,$0
	psrlh $25,$25,0xf
	lw $5,20($4)
	lw $6,24($4)
	lw $7,8($4)
	lw $14,0($4)
	lw $13,4($4)
	addiu $24,$0,1
	lw $9,16($4)
	sll $8,$9,0x1
	addiu $11,$0,-1
1:
	lq $10,0($5)
	lq $15,0($6)
	mtsab $13,0
	qfsrv $2,$15,$10
	qfsrv $3,$10,$15
	pextlb $10,$0,$2
	addi $7,$7,-1
	pextub $15,$0,$2
	mtsab $24,0
	qfsrv $3,$3,$2
	pextlb $2,$0,$3
	pextub $3,$0,$3
	paddh $10,$10,$2
	paddh $15,$15,$3
	paddh $2,$10,$25
	paddh $3,$15,$25
	psrlh $2,$2,0x1
	psrlh $3,$3,0x1
	lq $10,0($14)
	lq $15,16($14)
	paddh $2,$2,$10
	paddh $3,$3,$15
	paddh $10,$2,$25
	psrlh $2,$10,0x1
	pcgth $10,$3,$0
	psrlh $10,$10,0xf
	paddh $10,$3,$10
	psrlh $3,$10,0x1
	sq $2,0($14)
	sq $3,16($14)
	addu $5,$5,$9
	addu $6,$6,$9
	bgtz $7,1b
	addu $14,$14,$8
	addiu $5,$5,128
	addiu $6,$6,128
	lw $7,12($4)
	and $12,$11,$7
	bne $12,$0,1b
	daddu $11,$0,$0
	.set reorder
	");
}

void _ri0_110(MpegRi *ri)
{
	asm __volatile__("
	.set noreorder
	pnor $25,$0,$0
	psrlh $25,$25,0xf
	lw $5,20($4)
	lw $6,24($4)
	lw $14,0($4)
	lw $13,4($4)
	addiu $24,$0,1
	addiu $12,$0,-1
	lw $3,16($4)
	sll $2,$3,0x1
1:
	lw $7,8($4)
	addiu $11,$0,-1
2:
	ld $8,0($5)
	ld $9,0($6)
	pcpyld $8,$9,$8
	mtsab $13,0
	qfsrv $8,$8,$8
	pextlb $9,$0,$8
	addi $7,$7,-1
	addu $5,$5,$3
	addu $6,$6,$3
	mtsab $24,0
	qfsrv $10,$0,$8
	pextlb $8,$0,$10
	paddh $10,$9,$8
	paddh $10,$10,$25
	psrlh $10,$10,0x1
	lq $8,0($14)
	paddh $10,$10,$8
	pcgth $9,$10,$0
	psrlh $9,$9,0xf
	paddh $10,$10,$9
	psrlh $10,$10,0x1
	sq $10,0($14)
	bgtz $7,2b
	addu $14,$14,$2
	addiu $5,$5,320
	addiu $6,$6,320
	lw $7,12($4)
	and $10,$11,$7
	bne $10,$0,2b
	daddu $11,$0,$0
	lw $5,20($4)
	lw $6,24($4)
	lw $14,0($4)
	addiu $5,$5,64
	addiu $6,$6,64
	addiu $14,$14,128
	bne $12,$0,1b
	daddu $12,$0,$0
	.set reorder
	");
}

void _rix_111(MpegRi *ri)
{
	asm __volatile__("
	.set noreorder
	pnor $25,$0,$0
	psrlh $25,$25,0xf
	psllh $25,$25,0x1
	lw $5,20($4)
	lw $6,24($4)
	lw $7,8($4)
	lw $14,0($4)
	lw $13,4($4)
	lw $24,16($4)
	addiu $12,$0,1
	lq $8,0($5)
	lq $9,0($6)
	mtsab $13,0
	qfsrv $10,$9,$8
	qfsrv $15,$8,$9
	pextlb $8,$0,$10
	pextub $9,$0,$10
	addiu $11,$0,-1
	mtsab $12,0
	qfsrv $15,$15,$10
	pextlb $10,$0,$15
	pextub $15,$0,$15
	paddh $8,$8,$10
	beq $7,$0,2f
	paddh $9,$9,$15
1:
	addu $5,$5,$24
	addu $6,$6,$24
	lq $10,0($5)
	lq $15,0($6)
	mtsab $13,0
	qfsrv $2,$15,$10
	qfsrv $3,$10,$15
	pextlb $10,$0,$2
	addi $7,$7,-1
	pextub $15,$0,$2
	mtsab $12,0
	qfsrv $3,$3,$2
	pextlb $2,$0,$3
	pextub $3,$0,$3
	paddh $10,$10,$2
	paddh $15,$15,$3
	paddh $2,$8,$10
	paddh $3,$9,$15
	por $8,$10,$0
	por $9,$15,$0
	paddh $2,$2,$25
	paddh $3,$3,$25
	psrlh $2,$2,0x2
	psrlh $3,$3,0x2
	lq $10,0($14)
	lq $15,16($14)
	paddh $2,$2,$10
	paddh $3,$3,$15
	pnor $25,$0,$0
	psrlh $25,$25,0xf
	paddh $10,$2,$25
	psrlh $2,$10,0x1
	paddh $10,$3,$25
	psrlh $3,$10,0x1
	sq $2,0($14)
	sll $10,$24,0x1
	sq $3,16($14)
	bgtz $7,1b
	addu $14,$14,$10
2:
	addiu $5,$5,128
	addiu $6,$6,128
	lw $7,12($4)
	and $10,$11,$7
	bne $10,$0,1b
	daddu $11,$0,$0
	.set reorder
	");
}

void _ri0_111(MpegRi *ri)
{
	asm __volatile__("
	.set noreorder
	pnor $25,$0,$0
	psrlh $25,$25,0xf
	psllh $25,$25,0x1
	lw $5,20($4)
	lw $6,24($4)
	lw $14,0($4)
	lw $13,4($4)
	lw $12,16($4)
	addiu $24,$0,1
	addiu $11,$0,1
1:
	lw $7,8($4)
	ld $8,0($5)
	ld $9,0($6)
	pcpyld $8,$9,$8
	mtsab $13,0
	qfsrv $8,$8,$8
	pextlb $9,$0,$8
	addu $5,$5,$12
	ori $11,$11,0x8000
	mtsab $24,0
	qfsrv $10,$0,$8
	pextlb $8,$0,$10
	beq $7,$0,3f
	paddh $15,$9,$8
2:
	addu $6,$6,$12
	ld $8,0($5)
	ld $9,0($6)
	pcpyld $8,$9,$8
	mtsab $13,0
	qfsrv $8,$8,$8
	pextlb $9,$0,$8
	addi $7,$7,-1
	addu $5,$5,$12
	mtsab $24,0
	qfsrv $10,$0,$8
	pextlb $8,$0,$10
	paddh $10,$9,$8
	paddh $9,$10,$15
	por $15,$10,$0
	paddh $10,$9,$25
	psrlh $10,$10,0x2
	lq $8,0($14)
	paddh $10,$10,$8
	pcgth $9,$10,$0
	psrlh $9,$9,0xf
	paddh $10,$10,$9
	sll $8,$12,0x1
	psrlh $10,$10,0x1
	sq $10,0($14)
	bgtz $7,2b
	addu $14,$14,$8
3:
	psrah $10,$11,0xf
	addiu $5,$5,320
	lw $7,12($4)
	addiu $6,$6,320
	and $10,$10,$7
	bne $10,$0,2b
	andi $11,$11,0x7fff
	lw $5,20($4)
	lw $6,24($4)
	lw $14,0($4)
	addiu $5,$5,64
	addiu $6,$6,64
	addiu $14,$14,128
	andi $10,$11,0x1
	bne $10,$0,1b
	andi $11,$11,0xfffe
	.set reorder
	");
}

void _copyAddRefImage(u_long128 *dst, u_long128 *src0, u_long128 *src1)
{
	asm __volatile__("
	.set noreorder
	addiu $12,$0,24
	lui $10,%hi(_maxval)
	addiu $10,$10,%lo(_maxval)
	lq $11,0($10)
1:
	lq $8,0($5)
	addi $12,$12,-1
	lq $13,0($6)
	addiu $4,$4,16
	lq $9,16($5)
	paddh $8,$8,$13
	lq $2,16($6)
	pminh $8,$8,$11
	paddh $9,$9,$2
	pmaxh $8,$8,$0
	pminh $9,$9,$11
	addiu $5,$5,32
	pmaxh $9,$9,$0
	addiu $6,$6,32
	ppacb $10,$9,$8
	bne $12,$0,1b
	sq $10,-16($4)
	.set reorder
	");
}

/* _copyRefImage's asm carries the clamp constant _maxval (the demo's local label, which
 * _copyAddRefImage loads too) on a 16-byte boundary, as lq needs: `.align 4` pads the loop's end
 * with nops by the object's own layout. OPEN until the four functions before the interpolators
 * (_motionComp0, _getAllRefs, _getRef0, _doMC) are written: the target's object puts _rix_000 at
 * 8 mod 16 and _maxval after two nops; with them missing this one's loop ends on the boundary (no
 * nop: 0x58 against 0x60). Put 8 bytes before _rix_000 and it is byte-exact as written. */
void _copyRefImage(u_long128 *dst, u_long128 *src)
{
	asm __volatile__("
	.set noreorder
	addiu $12,$0,24
	lui $10,%hi(_maxval)
	addiu $10,$10,%lo(_maxval)
	lq $11,0($10)
1:
	lq $8,0($5)
	addi $12,$12,-1
	pminh $8,$8,$11
	lq $9,16($5)
	pmaxh $8,$8,$0
	pminh $9,$9,$11
	addiu $5,$5,32
	pmaxh $9,$9,$0
	addiu $4,$4,16
	ppacb $10,$9,$8
	bne $12,$0,1b
	sq $10,-16($4)
	.set reorder
	.align 4
_maxval:
	.word 0x00ff00ff, 0x00ff00ff, 0x00ff00ff, 0x00ff00ff
	");
}
void _ipuSetMPEG1(int mpeg1)
{
	*IPU_CTRL = (*IPU_CTRL & ~0x800000) | (mpeg1 << 23);
}

int _dmVector(MpegDec *d)
{
	return _ipuVdec(d, 3);
}

void _sendIpuCommand(MpegDec *d, u_int cmd)
{
	SEND_CMD(d, cmd);
}

void _waitIpuIdle(MpegDec *d)
{
	WAIT_IDLE(d);
}

long _waitIpuIdle64(MpegDec *d)
{
	long v;
	int i = 0;

	while ((v = *IPU_CMD) < 0 && !(*IPU_CTRL & 0x4000)) {
		if (i++ > 5000) {
			_dispatchMpegCbNodata(d->mp);
			i = 0;
		}
	}
	return v;
}

short _ipuVdec(MpegDec *d, int tbl)
{
	long r, top;
	u_int bp;
	int i = 0;

	WAIT_IDLE(d);
	SEND_CMD(d, (tbl << 26) | 0x30000000);
	while ((r = *IPU_CMD) < 0) {
		if (i++ > 5000) {
			_dispatchMpegCbNodata(d->mp);
			i = 0;
		}
	}
	top = *IPU_TOP;
	bp = *IPU_BP;
	d->bits = top;
	d->nbits = (top < 0) ? -(bp & 0x1F) & 0x1F : 32;
	d->ipuEnd = ((int)r == 0);
	return r & 0xFFFF;
}

u_int _peepBit(MpegDec *d, int n)
{
	if (d->dirty || d->nbits < n) {
		WAIT_IDLE(d);
		SEND_CMD(d, 0x40000000);
		d->bits = _waitIpuIdle64(d);
		d->nbits = 32;
	}
	return d->bits >> (32 - n);
}

void _flushBuf(MpegDec *d, int n)
{
	WAIT_IDLE(d);
	SEND_CMD(d, (u_int)n | 0x40000000);
	d->bits = _waitIpuIdle64(d);
	d->nbits = 32;
}

u_int _nextBit(MpegDec *d, int n)
{
	u_int v;

	WAIT_IDLE(d);
	if (d->dirty || d->nbits < n) {
		SEND_CMD(d, 0x40000000);
		d->bits = _waitIpuIdle64(d);
	}
	v = d->bits >> (32 - n);
	d->nbits = 32;
	SEND_CMD(d, (u_int)n | 0x40000000);
	d->bits = _waitIpuIdle64(d);
	return v;
}

void _nextStartCode(MpegDec *d)
{
	int n;

	_waitIpuIdle(d);
	n = -(*IPU_BP & 7) & 7;
	if (n)
		_flushBuf(d, n);
	while (_peepBit(d, 24) != 1)
		_flushBuf(d, 8);
}

int _sliceB(MpegDec *d)
{
	d->_1b4 = _nextBit(d, 5);
	if (_nextBit(d, 1)) {
		_nextBit(d, 1);
		_flushBuf(d, 7);
		_extrainfo(d);
	}
	return 0;
}

int _nextHeader(MpegDec *d)
{
	sceMpegCbDataTimeStamp cb;

	for (;;) {
		_nextStartCode(d);
		switch (_nextBit(d, 32)) {
		case 0x1B3:
			_sequenceHeader(d);
			break;
		case 0x1B8:
			_groupOfPicturesHeader(d);
			break;
		case 0x100:
			_pictureHeader(d);
			cb.type = sceMpegCbTimeStamp;
			cb.pts = -1;
			cb.dts = -1;
			_dispatchMpegCallback(d->mp, (sceMpegCbData *)&cb);
			d->pts = cb.pts;
			d->dts = cb.dts;
			return d->picture_coding_type;
		case 0x1B7:
			return 0;
		}
	}
}

void _pictureHeader(MpegDec *d)
{
	int tr = _nextBit(d, 10);

	d->picture_coding_type = _nextBit(d, 3);
	_nextBit(d, 16);
	if (d->picture_coding_type == 2 || d->picture_coding_type == 3) {
		d->full_pel_forward_vector = _nextBit(d, 1);
		d->forward_f_code = _nextBit(d, 3);
	}
	if (d->picture_coding_type == 3) {
		d->full_pel_backward_vector = _nextBit(d, 1);
		d->backward_f_code = _nextBit(d, 3);
	}
	_extrainfo(d);
	_extensionAndUserData(d);
	_updateTempTackData(d, tr);
}

void _extensionAndUserData(MpegDec *d)
{
	void (*ext[11])(MpegDec *) = {
		_unknown_extension, _sequenceExtension, _sequenceDisplayExtension,
		_quantMatrixExtension, _copyrightExtension, _sequenceScalableExtension,
		_unknown_extension, _pictureDisplayExtension, _pictureCodingExtension,
		_pictureSpatialScalableExtension, _pictureTemporalScalableExtension
	};
	u_int code, id;

	_nextStartCode(d);
	while ((code = _peepBit(d, 32)) == 0x1B5 || code == 0x1B2) {
		if (code == 0x1B5) {
			_flushBuf(d, 32);
			id = _nextBit(d, 4);
			if (id > 10)
				id = 0;
			ext[id](d);
			_nextStartCode(d);
		} else {
			_flushBuf(d, 32);
			_nextStartCode(d);
		}
	}
}

void _pictureCodingExtension(MpegDec *d)
{
	d->f_code[0][0] = _nextBit(d, 4);
	d->f_code[0][1] = _nextBit(d, 4);
	d->f_code[1][0] = _nextBit(d, 4);
	d->f_code[1][1] = _nextBit(d, 4);
	*IPU_CTRL = (*IPU_CTRL & ~0x30000) | (_nextBit(d, 2) << 16);
	d->picture_structure = _nextBit(d, 2);
	if (!d->firstStructure)
		d->firstStructure = d->picture_structure;
	d->top_field_first = _nextBit(d, 1);
	d->frame_pred_frame_dct = _nextBit(d, 1);
	d->concealment_motion_vectors = _nextBit(d, 1);
	*IPU_CTRL = (*IPU_CTRL & ~0x400000) | (_nextBit(d, 1) << 22);
	*IPU_CTRL = (*IPU_CTRL & ~0x200000) | (_nextBit(d, 1) << 21);
	*IPU_CTRL = (*IPU_CTRL & ~0x100000) | (_nextBit(d, 1) << 20);
	d->repeat_first_field = _nextBit(d, 1);
	_nextBit(d, 1);
	d->progressive_frame = _nextBit(d, 1);
	if (_nextBit(d, 1)) {
		_nextBit(d, 1);
		_nextBit(d, 3);
		_nextBit(d, 1);
		_nextBit(d, 7);
		_nextBit(d, 8);
	}
}

void _extrainfo(MpegDec *d)
{
	while (_nextBit(d, 1))
		_flushBuf(d, 8);
}

void _groupOfPicturesHeader(MpegDec *d)
{
	d->brokenLink = 0;
	d->_854 = 1;
	d->_84c = d->_850 + 1;
	_nextBit(d, 1);
	_nextBit(d, 5);
	_nextBit(d, 6);
	_nextBit(d, 1);
	_nextBit(d, 6);
	_nextBit(d, 6);
	d->closed_gop = _nextBit(d, 1);
	d->broken_link = _nextBit(d, 1);
	_extensionAndUserData(d);
}

void _quantMatrixExtension(MpegDec *d)
{
	if ((d->load_intra_quantiser_matrix = _nextBit(d, 1))) {
		_waitIpuIdle(d);
		_sendIpuCommand(d, 0x50000000);
		_waitIpuIdle(d);
	}
	if ((d->load_non_intra_quantiser_matrix = _nextBit(d, 1))) {
		_waitIpuIdle(d);
		_sendIpuCommand(d, 0x58000000);
		_waitIpuIdle(d);
	}
	if (_nextBit(d, 1))
		_Error(d, "load_chroma_intra_quantizer_matrix == 1");
	if (_nextBit(d, 1))
		_Error(d, "load_chroma_non_intra_quantizer_matrix == 1");
}

void _pictureDisplayExtension(MpegDec *d)
{
	int i, n;

	if (d->progressive_sequence) {
		if (d->repeat_first_field)
			n = d->top_field_first ? 3 : 2;
		else
			n = 1;
	} else {
		if (d->picture_structure != 3)
			n = 1;
		else
			n = d->repeat_first_field ? 3 : 2;
	}
	for (i = 0; i < n; i++) {
		d->frame_centre_horizontal_offset[i] = _nextBit(d, 16);
		_nextBit(d, 1);
		d->frame_centre_vertical_offset[i] = _nextBit(d, 16);
		_nextBit(d, 1);
	}
}

void _copyrightExtension(MpegDec *d)
{
	_nextBit(d, 1);
	_nextBit(d, 8);
	_nextBit(d, 1);
	_nextBit(d, 7);
	_nextBit(d, 1);
	_nextBit(d, 20);
	_nextBit(d, 1);
	_nextBit(d, 22);
	_nextBit(d, 1);
	_nextBit(d, 22);
}

/* The picture layer (GCC round 19a): a picture decoded into its structure's reference (the
 * frame's or a field's third slot), marked valid on success; a frame output, or a field pair. */
extern int _pictureData0(MpegDec *d);
extern void _dispRefImage(MpegDec *d, MpegRef *ref, int frameNo);
extern void _dispRefImageField(MpegDec *d, MpegRef *top, MpegRef *bottom, int frameNo);

int _decPicture(MpegDec *d)
{
	MpegRef *ref;
	int ret;

	if (d->picture_structure == 3 && d->secondField) {
		_Error(d, "odd number of field pictures");
		d->secondField = 0;
	}
	switch (d->picture_structure) {
	case 3:
		ref = d->ref[0][2];
		break;
	case 1:
		ref = d->ref[1][2];
		break;
	case 2:
		ref = d->ref[2][2];
		break;
	default:
		ref = d->ref[0][2];
		_Error(d, "unknown picture sutructure");
		break;
	}
	ret = _pictureData0(d);
	if (ret)
		ref->valid = 1;
	return ret;
}

void _outputFrame(MpegDec *d, int frameNo, int output)
{
	MpegRef *top, *bottom;

	if (output) {
		if (d->picture_structure == 3) {
			if (d->picture_coding_type == 3)
				top = d->ref[0][3];
			else
				top = d->ref[0][0];
			_dispRefImage(d, top, frameNo - 1);
		} else {
			if (d->picture_coding_type == 3) {
				top = d->ref[1][3];
				bottom = d->ref[2][3];
			} else {
				top = d->ref[1][0];
				bottom = d->ref[2][0];
			}
			_dispRefImageField(d, top, bottom, frameNo - 1);
		}
	}
	if (d->ptmSet == 1)
		d->ptmSet = 2;
}

extern int sprintf(char *buf, const char *fmt, ...);
extern int DIntr(void);
extern int EIntr(void);

/* the caller's image buffer against the picture: its width and height when set, else its size */
int _isOutSizeOK(MpegDec *d, MpegRef *ref)
{
	char buf[256];
	int ok;

	if (d->imageY)
		ok = d->imageX >= ref->width && d->imageY >= ref->height;
	else
		ok = d->imageSize >= ref->mbw * ref->mbh;
	if (!ok) {
		sprintf(buf, "Too small buffer size for %dx%d picture\n", ref->width, ref->height);
		_Error(d, buf);
	}
	return ok;
}

/* the IPU's output DMA (channel 3) into the scratchpad */
void receiveDataFromIPU(void *addr, int size)
{
	int ie = DIntr();

	*D3_MADR = ((u_int)addr & 0x0FFFFFFF) | 0x80000000;
	*D3_QWC = size >> 4;
	*D3_CHCR = 0x100;
	if (ie)
		EIntr();
}
