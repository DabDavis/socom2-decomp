/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libmpeg's decoder (MAIN 0x1B5160-0x1B9EE0): Sony's SDK, the demo's object 0x152F60 (C). This
 * unit holds, in the demo's order, the reference interpolators (_rix_000 ... _copyRefImage, inline
 * asm), _ipuSetMPEG1, BDEC's wait and _dmVector, the IPU command and wait primitives
 * (_sendIpuCommand ... _nextStartCode) and the header parsers (_sliceB ... _copyrightExtension); the
 * motion compensation before them (_motionComp0 ... _doMC) and most of the picture output after them
 * are not written yet (their rows stand as drafts or open); GCC round 19a wrote the picture layer's
 * _decPicture (its switch's frame case first: the target lays case 3's body right after the dispatch
 * tree) and _outputFrame (the field pair chosen into locals, one call), _isOutSizeOK and the object's
 * last function, receiveDataFromIPU; GCC round 20a the macroblock and slice layers (_waitBdecOut,
 * _mbAddressIncrement ... _motionVector) -- the MPEG Software Simulation Group's mpeg2decode text
 * (getpic.c's picture_data, start_of_slice and slice, macroblock.c's decode_macroblock with its
 * macroblock_modes in line and skipped_macroblock, motion.c), the VLCs the IPU's VDEC tables, the
 * blocks BDEC's, every byte-exact as that text reads but where the IPU changed it. _updateTempTackData
 * is left to its matching draft. GCC round 30a the picture output's _dispRefImage (as first written)
 * and _dispRefImageField (`d->_080 = pts` before `d->_088 = 1`: the long store may alias the long load,
 * so it stays below it) and _getPtsDtsFlags (each product of the PTS interpolation a DImode __muldi3;
 * `last + (half + odd)`; the flags ORed from the highest bit down -- fold pairs the six terms and each
 * pair's operands follow the source's order), over libmpeg.h's MpegRef with its time stamps, flags, centre offsets and display
 * size named. Names are SOCOM 1's (s1:exact).
 *
 * The IPU's idle wait and the command write are macros, expanded inside the readers
 * (_ipuVdec, _peepBit, _flushBuf, _nextBit) as well as being functions of their own: the
 * readers' copies keep no constant in a saved register, the functions' do. A command's index
 * into _isDirty shifts as the command's own type does: unsigned in _flushBuf and _nextBit (srl),
 * signed in _ipuVdec (sra). _nextHeader's cases are laid out in source order (sequence, GOP,
 * picture); _pictureDisplayExtension tests progressive_sequence first; _nextBit takes its result
 * before resetting nbits. IPU_TOP is read through a plain pointer (IPU_TOP_NV: GCC folds the
 * constant address into the `ld`, which GNU as expands through the destination register), after
 * IPU_BP, and nbits is stored in each arm (cross-jumped after reload into one store at the join:
 * _ipuVdec, open since round 10a, and _waitBdecOut, whose third store would weigh `d` too heavily
 * for global-alloc); _ipuVdec returns the command word itself (this cc1 truncates DI to HI with an
 * `andi 0xffff`: a mask written before it is a second one).
 *
 * OPEN: _extensionAndUserData (0x11C against 0x128: the target hoists both start
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

extern int DIntr(void);
extern int EIntr(void);

/* BDEC's output, the demo's _waitBdecOut: the wait until the fromIPU channel has drained (feeding
 * the toIPU channel through the no-data callback when it stops) or the IPU reports an error, then
 * the bit buffer re-read; an error resets the IPU between the stop and restart callbacks and stops
 * the fromIPU channel -- and SOCOM II's newer stop request (_10a0, func_001BBA70) stops both
 * channels and resets the IPU before anything is read (GCC round 20a) */
int _waitBdecOut(MpegDec *d)
{
	sceMpegCbData cb;
	sceMpegCbData cberr;
	int ret = 1;
	long top;
	u_int bp;
	int ie;

	_waitIpuIdle(d);
	while (*D3_QWC && !(*IPU_CTRL & 0x4000)) {
		if (*D4_QWC == 0 && !(*D4_CHCR & 0x100)) {
			cb.type = sceMpegCbNodata;
			_dispatchMpegCallback(d->mp, &cb);
		}
	}
	if (d->_10a0) {
		ie = DIntr();
		*D_ENABLEW = *D_ENABLER | 0x10000;
		*D4_CHCR = 0;
		*D3_CHCR = 0;
		*D_ENABLEW = *D_ENABLER & ~0x10000;
		if (ie)
			EIntr();
		*D3_QWC = 0;
		*D4_QWC = 0;
		*IPU_CTRL = 0x40000000;
		return 0;
	}
	bp = *IPU_BP;
	top = *IPU_TOP_NV;
	d->bits = top;
	if (top < 0)
		d->nbits = (bp & 0x1F) ? 32 - (bp & 0x1F) : 0;
	else
		d->nbits = 32;
	if (*IPU_CTRL & 0x4000) {
		_Error(d, "Error code detected(BDEC)");
		cberr.type = sceMpegCbStopDMA;
		_dispatchMpegCallback(d->mp, &cberr);
		*IPU_CTRL_NV = 0x40000000;
		cberr.type = sceMpegCbRestartDMA;
		_dispatchMpegCallback(d->mp, &cberr);
		ie = DIntr();
		*D_ENABLEW = *D_ENABLER | 0x10000;
		*D3_CHCR = 0;
		*D_ENABLEW = *D_ENABLER & ~0x10000;
		if (ie)
			EIntr();
		ret = 0;
		*D3_QWC = 0;
	}
	return ret;
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
	bp = *IPU_BP;
	top = *IPU_TOP_NV;
	d->bits = top;
	if (top < 0)
		d->nbits = -(bp & 0x1F) & 0x1F;
	else
		d->nbits = 32;
	d->Fault_Flag = ((int)r == 0);
	return r;
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
	d->quantizer_scale_code = _nextBit(d, 5);
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

extern void _Error1(MpegDec *d, char *fmt, int arg);

/* macroblock_address_increment: the IPU's table 0 (34 stuffing, 35 escape, 0 no code: an MPEG-2
 * stream's stuffing is read off the bits), mpeg2decode's Fault_Flag on an invalid code (GCC round
 * 20a) */
int _mbAddressIncrement(MpegDec *d)
{
	u_int code;
	int val, cont;

	val = 0;
	do {
		code = _ipuVdec(d, 0);
		switch (code) {
		case 35:	/* macroblock_escape */
			cont = 1;
			val += 33;
			break;
		case 0: {
			u_int bits = _peepBit(d, 11);

			if (d->mpeg2 && bits == 15) {
				_flushBuf(d, 11);
				cont = 1;
				break;
			}
			_Error1(d, "Invalid macroblock_address_increment code(0x%08x)", code);
			d->Fault_Flag = 1;
			return 1;
		}
		case 34:	/* macroblock_stuffing */
			cont = 1;
			break;
		default:
			val += code;
			cont = 0;
			break;
		}
	} while (cont);
	return val;
}

extern int _motionComp0(MpegDec *d, int MBA, int MBAinc, int macroblock_type, int motion_type,
			int PMV[2][2][2], int motion_vertical_field_select[2][2], int dmvector[2]);
extern void _doMC(MpegDec *d, int n);
static int _slice0(MpegDec *d, int MBAmax);
static int _sliceA0(MpegDec *d, int MBAmax, int *MBA, int *MBAinc, int PMV[2][2][2]);
static int _skipMB0(MpegDec *d, int PMV[2][2][2], int *motion_type,
		    int motion_vertical_field_select[2][2], int *macroblock_type);
static int _decMB0(MpegDec *d, int *macroblock_type, int *motion_type, int *dct_type,
		   int PMV[2][2][2], int motion_vertical_field_select[2][2], int dmvector[2]);

/* every macroblock of the picture: mpeg2decode's picture_data (getpic.c), a slice at a time while
 * _slice0 says go on (1, 3), then the last macroblock's compensation (GCC round 20a) */
int _pictureData0(MpegDec *d)
{
	int MBAmax;
	int ret;

	d->_810 = 0;
	/* number of macroblocks per picture */
	MBAmax = d->mb_width * d->mb_height;
	d->_814 = 0;

	if (d->picture_structure != 3)
		MBAmax >>= 1; /* field picture has half as mnay macroblocks as frame */

	do {
		ret = _slice0(d, MBAmax);
	} while (ret == 1 || ret == 3);

	_waitIpuIdle(d);
	if (!_waitBdecOut(d)) {
		if (d->_10a0) {
			d->_10a0 = 0;
			ret = 4;
		} else
			ret = 2;
	}
	while ((*D9_CHCR >> 8) & 1)	/* the scratchpad's channel: STR */
		;
	if (ret == 0)
		_doMC(d, !d->_810);
	if (ret == 1 || ret == 2)
		_Error(d, "= Skip to the next picture =");
	return ret == 0;
}

/* a slice's header: mpeg2decode's start_of_slice (getpic.c), its DC predictors the IPU's (GCC round
 * 20a): 2 out of range, 1 a fault, 0 go on */
static int _sliceA0(MpegDec *d, int MBAmax, int *MBA, int *MBAinc, int PMV[2][2][2])
{
	u_int code;
	int slice_vert_pos_ext;

	d->Fault_Flag = 0;

	_nextStartCode(d);
	code = _peepBit(d, 32);

	if (code < 0x101 || code > 0x1AF) {
		_Error1(d, "slice_start_code(0x%08x) out of range", code);
		return 2;
	}

	_flushBuf(d, 32);

	/* decode slice header (may change quantizer_scale) */
	slice_vert_pos_ext = _sliceB(d);

	/* decode macroblock address increment */
	*MBAinc = _mbAddressIncrement(d);

	if (d->Fault_Flag) {
		_Error(d, "_sliceA0(): error happens");
		return 1;
	}

	/* set current location */
	*MBA = ((slice_vert_pos_ext << 7) + (code & 255) - 1) * d->mb_width + *MBAinc - 1;
	*MBAinc = 1; /* first macroblock in slice: not skipped */

	/* reset all DC coefficient and motion vector predictors */
	d->dcr = 1;
	PMV[0][0][0] = PMV[0][0][1] = PMV[1][0][0] = PMV[1][0][1] = 0;
	PMV[0][1][0] = PMV[0][1][1] = PMV[1][1][0] = PMV[1][1][1] = 0;

	return 0;
}

/* a slice's macroblocks: mpeg2decode's slice (getpic.c), each macroblock decoded into the current
 * record while the IPU's previous one is compensated (GCC round 20a): 0 the picture's last, 1 and
 * 3 the next slice, 2 and 4 the next picture */
static int _slice0(MpegDec *d, int MBAmax)
{
	int PMV[2][2][2], motion_vertical_field_select[2][2];
	int dmvector[2];
	int MBA;
	int MBAinc, macroblock_type, motion_type, dct_type;
	int ret;

	MBA = 0; /* macroblock address */
	MBAinc = 0;

	if ((ret = _sliceA0(d, MBAmax, &MBA, &MBAinc, PMV)) != 0)
		return ret;

	d->Fault_Flag = 0;

	for (;;) {
		/* this is how we properly exit out of macroblock loop */
		if (MBA >= MBAmax)
			return 0; /* all macroblocks decoded */

		d->mb[d->_810].noCoef = 0;
		if (!_waitBdecOut(d)) {
			if (d->_10a0) {
				d->_10a0 = 0;
				return 4;
			}
			return 2;
		}

		if (MBAinc == 0) {
			if (!_peepBit(d, 23) || d->Fault_Flag) { /* next_start_code or fault */
				d->Fault_Flag = 0;
				return 3;
			}
			/* decode macroblock address increment */
			MBAinc = _mbAddressIncrement(d);
			if (d->Fault_Flag) {
				d->Fault_Flag = 0;
				return 1;
			}
		}

		if (MBA >= MBAmax) {
			/* MBAinc points beyond picture dimensions */
			_Error(d, "Too many macroblocks in picture");
			return 2;
		}

		if (MBAinc == 1) { /* not skipped */
			if (!_decMB0(d, &macroblock_type, &motion_type, &dct_type, PMV,
				     motion_vertical_field_select, dmvector)) {
				d->Fault_Flag = 0;
				return 1;
			}
		} else { /* MBAinc!=1: skipped macroblock */
			if (!_skipMB0(d, PMV, &motion_type, motion_vertical_field_select,
				      &macroblock_type)) {
				d->Fault_Flag = 0;
				return 2;
			}
		}

		if (!_motionComp0(d, MBA, MBAinc, macroblock_type, motion_type, PMV,
				  motion_vertical_field_select, dmvector)) {
			d->Fault_Flag = 0;
			return 2;
		}
		if (MBA)
			_doMC(d, d->_810 ^ 1);

		/* advance to next macroblock */
		MBA++;
		d->_810 ^= 1;
		MBAinc--;
	}
}

/* a skipped macroblock: mpeg2decode's skipped_macroblock (macroblock.c), its block clears and DC
 * predictors the IPU's (no coefficients, the DC reset bit) -- as written there (GCC round 20a) */
static int _skipMB0(MpegDec *d, int PMV[2][2][2], int *motion_type,
		    int motion_vertical_field_select[2][2], int *macroblock_type)
{
	int ret = 1;

	d->mb[d->_810].noCoef = 1;
	d->dcr = 1;

	/* reset motion vector predictors */
	if (d->picture_coding_type == 2)
		PMV[0][0][0] = PMV[0][0][1] = PMV[1][0][0] = PMV[1][0][1] = 0;

	/* derive motion_type */
	if (d->picture_structure == 3)
		*motion_type = 2;
	else {
		*motion_type = 1;
		/* predict from field of same parity */
		motion_vertical_field_select[0][0] = motion_vertical_field_select[0][1] =
			(d->picture_structure == 2);
	}

	if (d->picture_coding_type == 1) {
		_Error(d, "skiped macroblock in I picure is not allowed");
		ret = 0;
	}

	/* clear MACROBLOCK_INTRA */
	*macroblock_type &= ~1;
	return ret;
}

void receiveDataFromIPU(void *addr, int size);
void _motionVectors(MpegDec *d, int PMV[2][2][2], int dmvector[2],
		    int motion_vertical_field_select[2][2], int s, int motion_vector_count,
		    int mv_format, int h_r_size, int v_r_size, int dmv, int mvscale);
void _motionVector(MpegDec *d, int *PMV, int *dmvector, int h_r_size, int v_r_size, int dmv,
		   int mvscale, int full_pel_vector);

/* a coded macroblock: mpeg2decode's decode_macroblock (macroblock.c) with its macroblock_modes in
 * line, both writing through the caller's pointers, the blocks BDEC's into the current record
 * (GCC round 20a): the picture type into IPU_CTRL, the modes, the vectors, then BDEC */
static int _decMB0(MpegDec *d, int *macroblock_type, int *motion_type, int *dct_type,
		   int PMV[2][2][2], int motion_vertical_field_select[2][2], int dmvector[2])
{
	int motion_vector_count, mv_format, dmv, mvscale;

	*IPU_CTRL = (*IPU_CTRL & ~0x07000000) | (d->picture_coding_type << 24);

	/* get macroblock_type */
	*macroblock_type = _ipuVdec(d, 1);
	if (*macroblock_type == 0) {
		_Error(d, "Invalid macroblock_type code: 0");
		d->Fault_Flag = 1;
		return 0;
	}

	/* get frame/field motion type */
	if (*macroblock_type & (8 | 4)) {
		if (d->picture_structure == 3 && d->frame_pred_frame_dct)
			*motion_type = 2;
		else /* frame_motion_type or field_motion_type */
			*motion_type = _nextBit(d, 2);
	} else if ((*macroblock_type & 1) && d->concealment_motion_vectors) {
		/* concealment motion vectors */
		*motion_type = (d->picture_structure == 3) ? 2 : 1;
	}

	/* derive motion_vector_count, mv_format and dmv, (table 6-17, 6-18) */
	if (d->picture_structure == 3) {
		motion_vector_count = (*motion_type == 1) ? 2 : 1;
		mv_format = (*motion_type == 2) ? 1 : 0;
	} else {
		motion_vector_count = (*motion_type == 2) ? 2 : 1;
		mv_format = 0;
	}

	dmv = (*motion_type == 3); /* dual prime */

	/* field mv predictions in frame pictures have to be scaled */
	mvscale = ((mv_format == 0) && (d->picture_structure == 3));

	/* get dct_type (frame DCT / field DCT) */
	*dct_type = (d->picture_structure == 3)
		    && (!d->frame_pred_frame_dct)
		    && (*macroblock_type & (2 | 1))
		    ? _nextBit(d, 1)
		    : 0;

	if (*macroblock_type & 16)
		d->quantizer_scale_code = _nextBit(d, 5);

	/* decode forward motion vectors */
	if ((*macroblock_type & 8)
	    || ((*macroblock_type & 1) && d->concealment_motion_vectors)) {
		if (d->mpeg2)
			_motionVectors(d, PMV, dmvector, motion_vertical_field_select, 0,
				       motion_vector_count, mv_format, d->f_code[0][0] - 1,
				       d->f_code[0][1] - 1, dmv, mvscale);
		else
			_motionVector(d, PMV[0][0], dmvector, d->forward_f_code - 1,
				      d->forward_f_code - 1, 0, 0, d->full_pel_forward_vector);
	}

	if (d->Fault_Flag)
		return 0; /* trigger: go to next slice */

	/* decode backward motion vectors */
	if (*macroblock_type & 4) {
		if (d->mpeg2)
			_motionVectors(d, PMV, dmvector, motion_vertical_field_select, 1,
				       motion_vector_count, mv_format, d->f_code[1][0] - 1,
				       d->f_code[1][1] - 1, 0, mvscale);
		else
			_motionVector(d, PMV[0][1], dmvector, d->backward_f_code - 1,
				      d->backward_f_code - 1, 0, 0, d->full_pel_backward_vector);
	}

	if (d->Fault_Flag)
		return 0; /* trigger: go to next slice */

	if ((*macroblock_type & 1) && d->concealment_motion_vectors)
		_flushBuf(d, 1); /* remove marker_bit */

	if (*macroblock_type & (2 | 1)) {
		receiveDataFromIPU((void *)d->mb[d->_810].coef, 768);
		_waitIpuIdle(d);
		_sendIpuCommand(d, 0x20000000 | ((*macroblock_type & 1) << 27) | (d->dcr << 26)
				   | (*dct_type << 25) | (d->quantizer_scale_code << 16));
	} else
		d->mb[d->_810].noCoef = 1;

	d->dcr = 0;
	if (d->Fault_Flag)
		return 0; /* trigger: go to next slice */

	/* reset intra_dc predictors */
	if (!(*macroblock_type & 1))
		d->dcr = 1;

	/* reset motion vector predictors */
	if ((*macroblock_type & 1) && !d->concealment_motion_vectors) {
		/* intra mb without concealment motion vectors */
		PMV[0][0][0] = PMV[0][0][1] = PMV[1][0][0] = PMV[1][0][1] = 0;
		PMV[0][1][0] = PMV[0][1][1] = PMV[1][1][0] = PMV[1][1][1] = 0;
	}

	/* special "No_MC" macroblock_type case */
	if ((d->picture_coding_type == 2) && !(*macroblock_type & (8 | 1))) {
		/* non-intra mb without forward mv in a P picture */
		PMV[0][0][0] = PMV[0][0][1] = PMV[1][0][0] = PMV[1][0][1] = 0;

		/* derive motion_type */
		if (d->picture_structure == 3)
			*motion_type = 2;
		else {
			*motion_type = 1;
			/* predict from field of same parity */
			motion_vertical_field_select[0][0] = (d->picture_structure == 2);
		}
	}

	/* successfully decoded macroblock */
	return 1;
}

/* a motion vector component from its code and residual: the MPEG Software Simulation Group's
 * mpeg2decode text (motion.c's decode_motion_vector), as written there (GCC round 19a) */
void _decode_motion_vector(int *pred, int r_size, int motion_code, int motion_residual,
			   int full_pel_vector)
{
	int lim, vec;

	lim = 16 << r_size;
	vec = full_pel_vector ? (*pred >> 1) : (*pred);

	if (motion_code > 0) {
		vec += ((motion_code - 1) << r_size) + motion_residual + 1;
		if (vec >= lim)
			vec -= lim + lim;
	} else if (motion_code < 0) {
		vec -= ((-motion_code - 1) << r_size) + motion_residual + 1;
		if (vec < -lim)
			vec += lim + lim;
	}
	*pred = full_pel_vector ? (vec << 1) : vec;
}

/* one prediction's motion vectors (and dual-prime's differentials): mpeg2decode's motion_vectors
 * (motion.c), as written there, the IPU's VLC decoding standing for Get_motion_code and
 * Get_dmvector (GCC round 20a); MV_FIELD is 0 */
void _motionVectors(MpegDec *d, int PMV[2][2][2], int dmvector[2],
		    int motion_vertical_field_select[2][2], int s, int motion_vector_count,
		    int mv_format, int h_r_size, int v_r_size, int dmv, int mvscale)
{
	if (motion_vector_count == 1) {
		if (mv_format == 0 && !dmv) {
			motion_vertical_field_select[1][s] = motion_vertical_field_select[0][s] =
				_nextBit(d, 1);
		}

		_motionVector(d, PMV[0][s], dmvector, h_r_size, v_r_size, dmv, mvscale, 0);

		/* update other motion vector predictors */
		PMV[1][s][0] = PMV[0][s][0];
		PMV[1][s][1] = PMV[0][s][1];
	} else {
		motion_vertical_field_select[0][s] = _nextBit(d, 1);
		_motionVector(d, PMV[0][s], dmvector, h_r_size, v_r_size, dmv, mvscale, 0);

		motion_vertical_field_select[1][s] = _nextBit(d, 1);
		_motionVector(d, PMV[1][s], dmvector, h_r_size, v_r_size, dmv, mvscale, 0);
	}
}

/* one motion vector, horizontal then vertical: mpeg2decode's motion_vector (motion.c), as
 * written there; the motion code is the IPU's table 2 (GCC round 20a) */
void _motionVector(MpegDec *d, int *PMV, int *dmvector, int h_r_size, int v_r_size, int dmv,
		   int mvscale, int full_pel_vector)
{
	int motion_code, motion_residual;

	/* horizontal component */
	motion_code = _ipuVdec(d, 2);
	motion_residual = (h_r_size != 0 && motion_code != 0) ? _nextBit(d, h_r_size) : 0;

	_decode_motion_vector(&PMV[0], h_r_size, motion_code, motion_residual, full_pel_vector);

	if (dmv)
		dmvector[0] = _dmVector(d);

	/* vertical component */
	motion_code = _ipuVdec(d, 2);
	motion_residual = (v_r_size != 0 && motion_code != 0) ? _nextBit(d, v_r_size) : 0;

	if (mvscale)
		PMV[1] >>= 1; /* DIV 2 */

	_decode_motion_vector(&PMV[1], v_r_size, motion_code, motion_residual, full_pel_vector);

	if (mvscale)
		PMV[1] <<= 1;

	if (dmv)
		dmvector[1] = _dmVector(d);
}

/* dual-prime's derived vectors: mpeg2decode's Dual_Prime_Arithmetic (motion.c) over the decoder's
 * own picture_structure and top_field_first (GCC round 19a) */
void _dualPrimeVector(MpegDec *d, int DMV[][2], int *dmvector, int mvx, int mvy)
{
	if (d->picture_structure == 3) {
		if (d->top_field_first) {
			/* vector for prediction of top field from bottom field */
			DMV[0][0] = ((mvx + (mvx > 0)) >> 1) + dmvector[0];
			DMV[0][1] = ((mvy + (mvy > 0)) >> 1) + dmvector[1] - 1;

			/* vector for prediction of bottom field from top field */
			DMV[1][0] = ((3 * mvx + (mvx > 0)) >> 1) + dmvector[0];
			DMV[1][1] = ((3 * mvy + (mvy > 0)) >> 1) + dmvector[1] + 1;
		} else {
			/* vector for prediction of top field from bottom field */
			DMV[0][0] = ((3 * mvx + (mvx > 0)) >> 1) + dmvector[0];
			DMV[0][1] = ((3 * mvy + (mvy > 0)) >> 1) + dmvector[1] - 1;

			/* vector for prediction of bottom field from top field */
			DMV[1][0] = ((mvx + (mvx > 0)) >> 1) + dmvector[0];
			DMV[1][1] = ((mvy + (mvy > 0)) >> 1) + dmvector[1] + 1;
		}
	} else {
		/* vector for prediction from field of opposite 'parity' */
		DMV[0][0] = ((mvx + (mvx > 0)) >> 1) + dmvector[0];
		DMV[0][1] = ((mvy + (mvy > 0)) >> 1) + dmvector[1];

		/* correct for vertical field shift */
		if (d->picture_structure == 1)
			DMV[0][1]--;
		else
			DMV[0][1]++;
	}
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

/* GCC round 30a: a frame output -- its time stamps and flags into the caller's sceMpeg, the
 * aspect code its flags select (a 16-entry table, 0x1CE3E0), its display size and centre offsets;
 * the picture stored colour-converted or as RAW8 when the buffer holds it and the picture is valid */
extern void _csc_storeRefImage(MpegDec *d, MpegRef *ref);
extern void _cpr8(MpegDec *d, MpegRef *ref);
extern void _markOutput(MpegDec *d);
extern u_int data_001CE3E0[16];

/* a picture's time stamps and flags: with the default PTS gap set, a picture without a PTS is
 * given the last one plus half the gap times its field count (the odd half alternated by a parity
 * count); a PTM set by sceSetPtm overrides it once; the flags packed from the picture's */
void _getPtsDtsFlags(MpegDec *d, MpegRef *ref, long *pts, long *dts, u_long *flags)
{
	if (d->ptsGapSet && ref->pts < 0 && d->_080 >= 0) {
		int n = d->_088;
		int odd = (n & 1) * (d->ptsGap & 1) * (d->_090 & 1);

		*pts = d->_080 + ((int)((d->ptsGap * n) >> 1) + odd);
		if ((n & 1) * (d->ptsGap & 1))
			d->_090++;
	} else
		*pts = ref->pts;
	if (d->ptmSet == 2 && d->ptm >= 0) {
		*pts = d->ptm;
		d->ptmSet = 0;
		d->ptm = -1;
	}
	*dts = ref->dts;
	*flags = ((u_long)ref->_34 << 8) | ((u_long)ref->_38 << 7) | ((u_long)ref->_3c << 6) |
		((u_long)ref->_40 << 5) | ((u_long)ref->_30 << 3) | (u_long)ref->_2c;
}

void _dispRefImage(MpegDec *d, MpegRef *ref, int frameNo)
{
	_getPtsDtsFlags(d, ref, &d->mp->pts, &d->mp->dts, &d->mp->flags);
	d->_080 = d->mp->pts;
	d->_088 = data_001CE3E0[(int)(d->mp->flags >> 5) & 0xf];
	d->dispWidth = ref->dispWidth;
	d->dispHeight = ref->dispHeight;
	d->_0b4 = ref->centre[0];
	d->_0b8 = ref->centre[1];
	d->_0bc = ref->centre[2];
	d->_0c0 = ref->centre[3];
	d->_0c4 = ref->centre[4];
	d->_0c8 = ref->centre[5];
	if (_isOutSizeOK(d, ref) && ref->valid == 1) {
		if (d->csc)
			_csc_storeRefImage(d, ref);
		else
			_cpr8(d, ref);
		_markOutput(d);
	}
}

/* a field pair output: each field's time stamps and flags (the first field's into pts/dts/flags,
 * the second's into the 2nd set, both flagged 64 when the top field is first), the first field's
 * display size and centre offsets with the second's alternate ones; the top field stored at twice
 * its macroblock height */
void _dispRefImageField(MpegDec *d, MpegRef *top, MpegRef *bottom, int frameNo)
{
	MpegRef *first, *second;
	int flag = 0;

	if (d->picture_structure == 2) {
		first = top;
		second = bottom;
		flag = 64;
	} else {
		first = bottom;
		second = top;
	}
	_getPtsDtsFlags(d, first, &d->mp->pts, &d->mp->dts, &d->mp->flags);
	d->_080 = d->mp->pts;
	d->_088 = 1;
	_getPtsDtsFlags(d, second, &d->mp->pts2nd, &d->mp->dts2nd, &d->mp->flags2nd);
	d->_080 = d->mp->pts2nd;
	d->_088 = 1;
	d->mp->flags |= flag;
	d->mp->flags2nd |= flag;
	d->dispWidth = first->dispWidth;
	d->dispHeight = first->dispHeight;
	d->_0b4 = first->centre[0];
	d->_0b8 = second->centre[1];
	d->_0c0 = first->centre[3];
	d->_0c4 = second->centre[4];
	if (_isOutSizeOK(d, top) && top->valid == 1 && bottom->valid == 1) {
		top->mbh <<= 1;
		if (d->csc)
			_csc_storeRefImage(d, top);
		else
			_cpr8(d, top);
		top->mbh >>= 1;
		_markOutput(d);
	}
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
