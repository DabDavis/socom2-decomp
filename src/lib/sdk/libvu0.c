/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libvu0 (MAIN 0x1BFC00-0x1C0944): Sony's SDK VU0 macro-mode library, the demo's object 0x152218
 * (a C object: gcc2_compiled.), its functions inline asm in C as Sony shipped libvu0.c. The leaf
 * functions here; sceVu0RotMatrix, sceVu0CopyVectorXYZ and sceVu0LightColorMatrix stand as drafts,
 * CameraMatrix, NormalLightMatrix, ViewScreenMatrix and DropShadowMatrix (C around calls) are open.
 *
 * The toolchain is the one build whose GNU as both fills the return's delay slot with the asm's last
 * instruction where the target has it (sceVu0CopyVector's `jr ra; sq`, sceVu0MulMatrix's loop
 * branch) -- 991111's 2.9-ee-991111b leaves them empty (3s-5s on 14 of these) -- and encodes
 * `vsqrt Q,vf5x` with a zero fsf field, as the target holds it (0x4A0503BD) -- 2.96's 2.10 as sets it
 * (0x4A2503BD). ee-gcc 2.9-ee-991111-01, the dtls13010 Linux build (its as reports 2.9-ee-991111),
 * does both: all 32 byte-exact (README "The GCC front"). The asm bodies are the target's, in GNU as's
 * reorder mode: the delay slots are the assembler's to fill, as they were. */

typedef float sceVu0FVECTOR[4] __attribute__((aligned (16)));
typedef float sceVu0FMATRIX[4][4] __attribute__((aligned (16)));
typedef int sceVu0IVECTOR[4] __attribute__((aligned (16)));

void sceVu0ApplyMatrix(sceVu0FVECTOR v0, sceVu0FMATRIX m0, sceVu0FVECTOR v1)
{
	asm __volatile__("
	lqc2 vf4,0(%1)
	lqc2 vf5,16(%1)
	lqc2 vf6,32(%1)
	lqc2 vf7,48(%1)
	lqc2 vf8,0(%2)
	vmulax.xyzw ACCxyzw,vf4xyzw,vf8x
	vmadday.xyzw ACCxyzw,vf5xyzw,vf8y
	vmaddaz.xyzw ACCxyzw,vf6xyzw,vf8z
	vmaddw.xyzw vf9xyzw,vf7xyzw,vf8w
	sqc2 vf9,0(%0)
	":  : "r" (v0), "r" (m0), "r" (v1));
}

void sceVu0MulMatrix(sceVu0FMATRIX m0, sceVu0FMATRIX m1, sceVu0FMATRIX m2)
{
	asm __volatile__("
	lqc2 vf4,0(%1)
	lqc2 vf5,16(%1)
	lqc2 vf6,32(%1)
	lqc2 vf7,48(%1)
	addiu $7,$0,4
	_sceVu0MulMatrix_0:
	lqc2 vf8,0(%2)
	vmulax.xyzw ACCxyzw,vf4xyzw,vf8x
	vmadday.xyzw ACCxyzw,vf5xyzw,vf8y
	vmaddaz.xyzw ACCxyzw,vf6xyzw,vf8z
	vmaddw.xyzw vf9xyzw,vf7xyzw,vf8w
	sqc2 vf9,0(%0)
	addi $7,$7,-1
	addi %2,%2,16
	addi %0,%0,16
	bne $0,$7,_sceVu0MulMatrix_0
	":  : "r" (m0), "r" (m1), "r" (m2): "$7");
}

void sceVu0OuterProduct(sceVu0FVECTOR v0, sceVu0FVECTOR v1, sceVu0FVECTOR v2)
{
	asm __volatile__("
	lqc2 vf4,0(%1)
	lqc2 vf5,0(%2)
	vopmula.xyz ACCxyz,vf4xyz,vf5xyz
	vopmsub.xyz vf6xyz,vf5xyz,vf4xyz
	vsub.w vf6w,vf6w,vf6w
	sqc2 vf6,0(%0)
	":  : "r" (v0), "r" (v1), "r" (v2));
}

float sceVu0InnerProduct(sceVu0FVECTOR v0, sceVu0FVECTOR v1)
{
	register float ret;
	asm __volatile__("
	lqc2 vf4,0(%1)
	lqc2 vf5,0(%2)
	vmul.xyz vf5xyz,vf4xyz,vf5xyz
	vaddy.x vf5x,vf5x,vf5y
	vaddz.x vf5x,vf5x,vf5z
	qmfc2 $2,vf5
	mtc1 $2,%0
	": "=f" (ret) : "r" (v0), "r" (v1): "$2");
	return ret;
}

void sceVu0Normalize(sceVu0FVECTOR v0, sceVu0FVECTOR v1)
{
	asm __volatile__("
	lqc2 vf4,0(%1)
	vmul.xyz vf5xyz,vf4xyz,vf4xyz
	vaddy.x vf5x,vf5x,vf5y
	vaddz.x vf5x,vf5x,vf5z
	vsqrt Q,vf5x
	vwaitq
	vaddq.x vf5x,vf0x,Q
	vnop
	vnop
	vdiv Q,vf0w,vf5x
	vsub.xyzw vf6xyzw,vf0xyzw,vf0xyzw
	vwaitq
	vmulq.xyz vf6xyz,vf4xyz,Q
	sqc2 vf6,0(%0)
	":  : "r" (v0), "r" (v1));
}

void sceVu0TransposeMatrix(sceVu0FMATRIX m0, sceVu0FMATRIX m1)
{
	asm __volatile__("
	lq $8,0(%1)
	lq $9,16(%1)
	lq $10,32(%1)
	lq $11,48(%1)
	pextlw $12,$9,$8
	pextuw $13,$9,$8
	pextlw $14,$11,$10
	pextuw $15,$11,$10
	pcpyld $8,$14,$12
	pcpyud $9,$12,$14
	pcpyld $10,$15,$13
	pcpyud $11,$13,$15
	sq $8,0(%0)
	sq $9,16(%0)
	sq $10,32(%0)
	sq $11,48(%0)
	":  : "r" (m0), "r" (m1): "$8", "$9", "$10", "$11", "$12", "$13", "$14", "$15");
}

void sceVu0InversMatrix(sceVu0FMATRIX m0, sceVu0FMATRIX m1)
{
	asm __volatile__("
	lq $8,0(%1)
	lq $9,16(%1)
	lq $10,32(%1)
	lqc2 vf4,48(%1)
	vmove.xyzw vf5xyzw,vf4xyzw
	vsub.xyz vf4xyz,vf4xyz,vf4xyz
	vmove.xyzw vf9xyzw,vf4xyzw
	qmfc2 $11,vf4
	pextlw $12,$9,$8
	pextuw $13,$9,$8
	pextlw $14,$11,$10
	pextuw $15,$11,$10
	pcpyld $8,$14,$12
	pcpyud $9,$12,$14
	pcpyld $10,$15,$13
	qmtc2 $8,vf6
	qmtc2 $9,vf7
	qmtc2 $10,vf8
	vmulax.xyz ACCxyz,vf6xyz,vf5x
	vmadday.xyz ACCxyz,vf7xyz,vf5y
	vmaddz.xyz vf4xyz,vf8xyz,vf5z
	vsub.xyz vf4xyz,vf9xyz,vf4xyz
	sq $8,0(%0)
	sq $9,16(%0)
	sq $10,32(%0)
	sqc2 vf4,48(%0)
	":  : "r" (m0), "r" (m1): "$8", "$9", "$10", "$11", "$12", "$13", "$14", "$15");
}

void sceVu0DivVector(sceVu0FVECTOR v0, sceVu0FVECTOR v1, float q)
{
	asm __volatile__("
	lqc2 vf4,0(%1)
	mfc1 $8,%2
	qmtc2 $8,vf5
	vdiv Q,vf0w,vf5x
	vwaitq
	vmulq.xyzw vf4xyzw,vf4xyzw,Q
	sqc2 vf4,0(%0)
	":  : "r" (v0), "r" (v1), "f" (q): "$8");
}

void sceVu0DivVectorXYZ(sceVu0FVECTOR v0, sceVu0FVECTOR v1, float q)
{
	asm __volatile__("
	lqc2 vf4,0(%1)
	mfc1 $8,%2
	qmtc2 $8,vf5
	vdiv Q,vf0w,vf5x
	vwaitq
	vmulq.xyz vf4xyz,vf4xyz,Q
	sqc2 vf4,0(%0)
	":  : "r" (v0), "r" (v1), "f" (q): "$8");
}

void sceVu0InterVector(sceVu0FVECTOR v0, sceVu0FVECTOR v1, sceVu0FVECTOR v2, float r)
{
	asm __volatile__("
	lqc2 vf4,0(%1)
	lqc2 vf5,0(%2)
	mfc1 $8,%3
	qmtc2 $8,vf6
	vaddw.x vf7x,vf0x,vf0w
	vsub.x vf8x,vf7x,vf6x
	vmulax.xyzw ACCxyzw,vf4xyzw,vf6x
	vmaddx.xyzw vf9xyzw,vf5xyzw,vf8x
	sqc2 vf9,0(%0)
	":  : "r" (v0), "r" (v1), "r" (v2), "f" (r): "$8");
}

void sceVu0AddVector(sceVu0FVECTOR v0, sceVu0FVECTOR v1, sceVu0FVECTOR v2)
{
	asm __volatile__("
	lqc2 vf4,0(%1)
	lqc2 vf5,0(%2)
	vadd.xyzw vf6xyzw,vf4xyzw,vf5xyzw
	sqc2 vf6,0(%0)
	":  : "r" (v0), "r" (v1), "r" (v2));
}

void sceVu0SubVector(sceVu0FVECTOR v0, sceVu0FVECTOR v1, sceVu0FVECTOR v2)
{
	asm __volatile__("
	lqc2 vf4,0(%1)
	lqc2 vf5,0(%2)
	vsub.xyzw vf6xyzw,vf4xyzw,vf5xyzw
	sqc2 vf6,0(%0)
	":  : "r" (v0), "r" (v1), "r" (v2));
}

void sceVu0MulVector(sceVu0FVECTOR v0, sceVu0FVECTOR v1, sceVu0FVECTOR v2)
{
	asm __volatile__("
	lqc2 vf4,0(%1)
	lqc2 vf5,0(%2)
	vmul.xyzw vf6xyzw,vf4xyzw,vf5xyzw
	sqc2 vf6,0(%0)
	":  : "r" (v0), "r" (v1), "r" (v2));
}

void sceVu0ScaleVector(sceVu0FVECTOR v0, sceVu0FVECTOR v1, float s)
{
	asm __volatile__("
	lqc2 vf4,0(%1)
	mfc1 $8,%2
	qmtc2 $8,vf5
	vmulx.xyzw vf6xyzw,vf4xyzw,vf5x
	sqc2 vf6,0(%0)
	":  : "r" (v0), "r" (v1), "f" (s): "$8");
}

void sceVu0TransMatrix(sceVu0FMATRIX m0, sceVu0FMATRIX m1, sceVu0FVECTOR tv)
{
	asm __volatile__("
	lqc2 vf4,0(%2)
	lqc2 vf5,48(%1)
	lq $7,0(%1)
	lq $8,16(%1)
	lq $9,32(%1)
	vadd.xyz vf5xyz,vf5xyz,vf4xyz
	sq $7,0(%0)
	sq $8,16(%0)
	sq $9,32(%0)
	sqc2 vf5,48(%0)
	":  : "r" (m0), "r" (m1), "r" (tv): "$7", "$8", "$9");
}

void sceVu0CopyVector(sceVu0FVECTOR v0, sceVu0FVECTOR v1)
{
	asm __volatile__("
	lq $6,0(%1)
	sq $6,0(%0)
	":  : "r" (v0), "r" (v1): "$6");
}

void sceVu0CopyMatrix(sceVu0FMATRIX m0, sceVu0FMATRIX m1)
{
	asm __volatile__("
	lq $6,0(%1)
	lq $7,16(%1)
	lq $8,32(%1)
	lq $9,48(%1)
	sq $6,0(%0)
	sq $7,16(%0)
	sq $8,32(%0)
	sq $9,48(%0)
	":  : "r" (m0), "r" (m1): "$6", "$7", "$8", "$9");
}

void sceVu0FTOI4Vector(sceVu0IVECTOR v0, sceVu0FVECTOR v1)
{
	asm __volatile__("
	lqc2 vf4,0(%1)
	vftoi4.xyzw vf5xyzw,vf4xyzw
	sqc2 vf5,0(%0)
	":  : "r" (v0), "r" (v1));
}

void sceVu0FTOI0Vector(sceVu0IVECTOR v0, sceVu0FVECTOR v1)
{
	asm __volatile__("
	lqc2 vf4,0(%1)
	vftoi0.xyzw vf5xyzw,vf4xyzw
	sqc2 vf5,0(%0)
	":  : "r" (v0), "r" (v1));
}

void sceVu0ITOF4Vector(sceVu0FVECTOR v0, sceVu0IVECTOR v1)
{
	asm __volatile__("
	lqc2 vf4,0(%1)
	vitof4.xyzw vf5xyzw,vf4xyzw
	sqc2 vf5,0(%0)
	":  : "r" (v0), "r" (v1));
}

void sceVu0ITOF0Vector(sceVu0FVECTOR v0, sceVu0IVECTOR v1)
{
	asm __volatile__("
	lqc2 vf4,0(%1)
	vitof0.xyzw vf5xyzw,vf4xyzw
	sqc2 vf5,0(%0)
	":  : "r" (v0), "r" (v1));
}

void sceVu0UnitMatrix(sceVu0FMATRIX m0)
{
	asm __volatile__("
	vsub.xyzw vf4xyzw,vf0xyzw,vf0xyzw
	vadd.w vf4w,vf4w,vf0w
	vmr32.xyzw vf5xyzw,vf4xyzw
	vmr32.xyzw vf6xyzw,vf5xyzw
	vmr32.xyzw vf7xyzw,vf6xyzw
	sqc2 vf4,48(%0)
	sqc2 vf5,32(%0)
	sqc2 vf6,16(%0)
	sqc2 vf7,0(%0)
	":  : "r" (m0));
}

void _sceVu0ecossin(float t)
{
	asm __volatile__("
	lui $8,0x1d
	addiu $8,$8,-6688
	lqc2 vf5,0($8)
	vmr32.w vf6w,vf6w
	vaddx.x vf4x,vf0x,vf6x
	vmul.x vf6x,vf6x,vf6x
	vmulx.yzw vf4yzw,vf4yzw,vf0x
	vmulw.xyzw vf8xyzw,vf5xyzw,vf6w
	vsub.xyzw vf5xyzw,vf0xyzw,vf0xyzw
	vmulx.xyzw vf8xyzw,vf8xyzw,vf6x
	vmulx.xyz vf8xyz,vf8xyz,vf6x
	vaddw.x vf4x,vf4x,vf8w
	vmulx.xy vf8xy,vf8xy,vf6x
	vaddz.x vf4x,vf4x,vf8z
	vmulx.x vf8x,vf8x,vf6x
	vaddy.x vf4x,vf4x,vf8y
	vaddx.x vf4x,vf4x,vf8x
	vaddx.xy vf4xy,vf5xy,vf4x
	vmul.x vf7x,vf4x,vf4x
	vsubx.w vf7w,vf0w,vf7x
	vsqrt Q,vf7w
	vwaitq
	vaddq.x vf7x,vf0x,Q
	bne $7,$0,__sceVu0ecossin_0
	vaddx.x vf4x,vf5x,vf7x
	beq $0,$0,__sceVu0ecossin_1
	__sceVu0ecossin_0:
	vsubx.x vf4x,vf5x,vf7x
	__sceVu0ecossin_1:
	":  : "f" (t): "$7", "$8");
}

void sceVu0ClampVector(sceVu0FVECTOR v0, sceVu0FVECTOR v1, float min, float max)
{
	asm __volatile__("
	mfc1 $8,%2
	mfc1 $9,%3
	lqc2 vf6,0(%1)
	qmtc2 $8,vf4
	qmtc2 $9,vf5
	vmaxx.xyzw vf6xyzw,vf6xyzw,vf4x
	vminix.xyzw vf6xyzw,vf6xyzw,vf5x
	sqc2 vf6,0(%0)
	":  : "r" (v0), "r" (v1), "f" (min), "f" (max): "$8", "$9");
}

void sceVu0RotTransPersN(sceVu0IVECTOR *v0, sceVu0FMATRIX m0, sceVu0FVECTOR *v1, int n, int mode)
{
	asm __volatile__("
	lqc2 vf4,0(%1)
	lqc2 vf5,16(%1)
	lqc2 vf6,32(%1)
	lqc2 vf7,48(%1)
	_sceVu0RotTransPersN_0:
	lqc2 vf8,0(%2)
	vmulax.xyzw ACCxyzw,vf4xyzw,vf8x
	vmadday.xyzw ACCxyzw,vf5xyzw,vf8y
	vmaddaz.xyzw ACCxyzw,vf6xyzw,vf8z
	vmaddw.xyzw vf9xyzw,vf7xyzw,vf8w
	vdiv Q,vf0w,vf9w
	vwaitq
	vmulq.xyz vf9xyz,vf9xyz,Q
	vftoi4.xyzw vf10xyzw,vf9xyzw
	beq %4,$0,_sceVu0RotTransPersN_1
	vftoi0.zw vf10zw,vf9zw
	_sceVu0RotTransPersN_1:
	sqc2 vf10,0(%0)
	addi %3,%3,-1
	addi %2,%2,16
	addi %0,%0,16
	bne $0,%3,_sceVu0RotTransPersN_0
	":  : "r" (v0), "r" (m0), "r" (v1), "r" (n), "r" (mode));
}

void sceVu0RotTransPers(sceVu0IVECTOR v0, sceVu0FMATRIX m0, sceVu0FVECTOR v1, int mode)
{
	asm __volatile__("
	lqc2 vf4,0(%1)
	lqc2 vf5,16(%1)
	lqc2 vf6,32(%1)
	lqc2 vf7,48(%1)
	lqc2 vf8,0(%2)
	vmulax.xyzw ACCxyzw,vf4xyzw,vf8x
	vmadday.xyzw ACCxyzw,vf5xyzw,vf8y
	vmaddaz.xyzw ACCxyzw,vf6xyzw,vf8z
	vmaddw.xyzw vf9xyzw,vf7xyzw,vf8w
	vdiv Q,vf0w,vf9w
	vwaitq
	vmulq.xyz vf9xyz,vf9xyz,Q
	vftoi4.xyzw vf10xyzw,vf9xyzw
	beq %3,$0,_sceVu0RotTransPers_0
	vftoi0.zw vf10zw,vf9zw
	_sceVu0RotTransPers_0:
	sqc2 vf10,0(%0)
	":  : "r" (v0), "r" (m0), "r" (v1), "r" (mode));
}

void sceVu0InterVectorXYZ(sceVu0FVECTOR v0, sceVu0FVECTOR v1, sceVu0FVECTOR v2, float r)
{
	asm __volatile__("
	lqc2 vf4,0(%1)
	lqc2 vf5,0(%2)
	mfc1 $8,%3
	qmtc2 $8,vf6
	vmove.w vf9w,vf4w
	vaddw.x vf7x,vf0x,vf0w
	vsub.x vf8x,vf7x,vf6x
	vmulax.xyz ACCxyz,vf4xyz,vf6x
	vmaddx.xyz vf9xyz,vf5xyz,vf8x
	sqc2 vf9,0(%0)
	":  : "r" (v0), "r" (v1), "r" (v2), "f" (r): "$8");
}

void sceVu0ScaleVectorXYZ(sceVu0FVECTOR v0, sceVu0FVECTOR v1, float s)
{
	asm __volatile__("
	lqc2 vf4,0(%1)
	mfc1 $8,%2
	qmtc2 $8,vf5
	vmulx.xyz vf4xyz,vf4xyz,vf5x
	sqc2 vf4,0(%0)
	":  : "r" (v0), "r" (v1), "f" (s): "$8");
}

int sceVu0ClipScreen(sceVu0FVECTOR v0)
{
	register int ret;
	asm __volatile__("
	vsub.xyzw vf4xyzw,vf0xyzw,vf0xyzw
	lui %0,0x4580
	dsll %0,%0,0x10
	ori %0,%0,0x4580
	dsll %0,%0,0x10
	lqc2 vf7,0(%1)
	qmtc2 %0,vf6
	ctc2 $0,$vi16
	vsub.xyw vf5xyw,vf7xyw,vf4xyw
	vsub.xy vf5xy,vf6xy,vf7xy
	vnop
	vnop
	vnop
	vnop
	vnop
	cfc2 %0,$vi16
	andi %0,%0,0xc0
	": "=r" (ret) : "r" (v0));
	return ret;
}

int sceVu0ClipScreen3(sceVu0FVECTOR v0, sceVu0FVECTOR v1, sceVu0FVECTOR v2)
{
	register int ret;
	asm __volatile__("
	vsub.xyzw vf4xyzw,vf0xyzw,vf0xyzw
	lui %0,0x4580
	dsll %0,%0,0x10
	ori %0,%0,0x4580
	dsll %0,%0,0x10
	lqc2 vf6,0(%1)
	lqc2 vf8,0(%2)
	lqc2 vf9,0(%3)
	qmtc2 %0,vf7
	ctc2 $0,$vi16
	vsub.xyw vf5xyw,vf6xyw,vf4xyw
	vsub.xy vf5xy,vf7xy,vf6xy
	vsub.xyw vf5xyw,vf8xyw,vf4xyw
	vsub.xy vf5xy,vf7xy,vf8xy
	vsub.xyw vf5xyw,vf9xyw,vf4xyw
	vsub.xy vf5xy,vf7xy,vf9xy
	vnop
	vnop
	vnop
	vnop
	vnop
	cfc2 %0,$vi16
	andi %0,%0,0xc0
	": "=r" (ret) : "r" (v0), "r" (v1), "r" (v2));
	return ret;
}

int sceVu0ClipAll(sceVu0FVECTOR minv, sceVu0FVECTOR maxv, sceVu0FMATRIX ms, sceVu0FVECTOR *vm, int n)
{
	register int ret;
	asm __volatile__("
	lqc2 vf8,0(%4)
	lqc2 vf4,0(%3)
	lqc2 vf5,16(%3)
	lqc2 vf6,32(%3)
	lqc2 vf7,48(%3)
	lqc2 vf9,0(%1)
	lqc2 vf10,0(%2)
	lqc2 vf11,0(%1)
	lqc2 vf12,0(%2)
	_sceVu0ClipAll_0:
	vmulax.xyzw ACCxyzw,vf4xyzw,vf8x
	vmadday.xyzw ACCxyzw,vf5xyzw,vf8y
	vmaddaz.xyzw ACCxyzw,vf6xyzw,vf8z
	vmaddw.xyzw vf8xyzw,vf7xyzw,vf8w
	vmulw.xyz vf11xyz,vf9xyz,vf8w
	vmulw.xyz vf12xyz,vf10xyz,vf8w
	vnop
	vnop
	ctc2 $0,$vi16
	vsub.xyw vf11xyw,vf8xyw,vf11xyw
	vsub.xyw vf12xyw,vf12xyw,vf8xyw
	vmove.w vf11w,vf9w
	vmove.w vf12w,vf10w
	vnop
	addi %4,%4,16
	lqc2 vf8,0(%4)
	addi %5,%5,-1
	cfc2 %0,$vi16
	andi %0,%0,0xc0
	beq %0,$0,_sceVu0ClipAll_1
	bne $0,%5,_sceVu0ClipAll_0
	addi %0,$0,1
	_sceVu0ClipAll_1:
	": "=r" (ret) : "r" (minv), "r" (maxv), "r" (ms), "r" (vm), "r" (n));
	return ret;
}

void sceVpu0Reset(void)
{
	asm __volatile__("
	lui $2,0x1000
	lui $4,0x1000
	ori $2,$2,0x3830
	ori $4,$4,0x3820
	sw $0,0($2)
	lui $3,0x1000
	sw $0,0($4)
	ori $3,$3,0x3810
	addiu $2,$0,1
	sw $2,0($3)
	cfc2 $8,$vi28
	ori $8,$8,0x2
	ctc2 $8,$vi28
	sync.p
	lui $4,0x1d
	lui $5,0x1000
	addiu $4,$4,-6672
	ori $5,$5,0x4000
	lq $2,0($4)
	sq $2,0($5)
	lq $3,16($4)
	sq $3,0($5)
	":  : : "$2", "$3", "$4", "$5", "$8");
}

