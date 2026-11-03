/* sc: compiler=ee-gcc2.9-991111-01-dtls13010; flags=-O2 -G0 */
/* libvu0 (MAIN 0x1BFC00-0x1C0944): Sony's SDK VU0 macro-mode library, the demo's object 0x152218
 * (a C object: gcc2_compiled.), its functions inline asm in C as Sony shipped libvu0.c -- the object
 * whole, 42 of 42: the leaf functions (round 2a), and round 16a's RotMatrixZ/X/Y (asm that calls
 * _sceVu0ecossin itself, ra kept in a2), RotMatrix (991111's void-context sibling call into
 * RotMatrixX), the C around calls -- CameraMatrix, NormalLightMatrix, LightColorMatrix,
 * ViewScreenMatrix (farz, the tenth argument, the ninth float, read from the caller's stack) --
 * CopyVectorXYZ and DropShadowMatrix (the light point read into three locals first: through lp after
 * the stores to m, which may alias it, every read is reloaded -- 0x1EC against 0x190; the elements
 * stored column by column, as the target's schedule has them: row by row, 72 words off).
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

void sceVu0RotMatrixZ(sceVu0FMATRIX m0, sceVu0FMATRIX m1, float rz)
{
	asm __volatile__("
	mtc1 $0,$f0
	c.lt.s %2,$f0
	li.s $f0,1.57079637050628662109e0
	bc1f _RotMatrixZ_01
	add.s %2,$f0,%2
	li $7,1
	j _RotMatrixZ_02
_RotMatrixZ_01:
	sub.s %2,$f0,%2
	move $7,$0
_RotMatrixZ_02:
	mfc1 $8,%2
	qmtc2 $8,vf6
	move $6,$31
	jal _sceVu0ecossin
	move $31,$6
	vmove.xyzw vf6xyzw,vf5xyzw
	vmove.xyzw vf7xyzw,vf5xyzw
	vmove.xyzw vf9xyzw,vf0xyzw
	vsub.xyz vf9xyz,vf9xyz,vf9xyz
	vmr32.xyzw vf8xyzw,vf9xyzw
	vsub.zw vf4zw,vf4zw,vf4zw
	vaddx.y vf6y,vf5y,vf4x
	vaddy.x vf6x,vf5x,vf4y
	vsubx.x vf7x,vf5x,vf4x
	vaddy.y vf7y,vf5y,vf4y
	li $7,4
_RotMatrixZ_loop:
	lqc2 vf4,0(%1)
	vmulax.xyzw ACCxyzw,vf6xyzw,vf4x
	vmadday.xyzw ACCxyzw,vf7xyzw,vf4y
	vmaddaz.xyzw ACCxyzw,vf8xyzw,vf4z
	vmaddw.xyzw vf5xyzw,vf9xyzw,vf4w
	sqc2 vf5,0(%0)
	addi $7,$7,-1
	addi %1,%1,16
	addi %0,%0,16
	bne $0,$7,_RotMatrixZ_loop
	": : "r" (m0), "r" (m1), "f" (rz) : "$6", "$7", "$8", "$f0");
}

void sceVu0RotMatrixX(sceVu0FMATRIX m0, sceVu0FMATRIX m1, float rx)
{
	asm __volatile__("
	mtc1 $0,$f0
	c.lt.s %2,$f0
	li.s $f0,1.57079637050628662109e0
	bc1f _RotMatrixX_01
	add.s %2,$f0,%2
	li $7,1
	j _RotMatrixX_02
_RotMatrixX_01:
	sub.s %2,$f0,%2
	move $7,$0
_RotMatrixX_02:
	mfc1 $8,%2
	qmtc2 $8,vf6
	move $6,$31
	jal _sceVu0ecossin
	move $31,$6
	vmove.xyzw vf6xyzw,vf5xyzw
	vmove.xyzw vf7xyzw,vf5xyzw
	vmove.xyzw vf8xyzw,vf5xyzw
	vmove.xyzw vf9xyzw,vf5xyzw
	vaddw.x vf6x,vf5x,vf0w
	vaddw.w vf9w,vf5w,vf0w
	vsub.zw vf4zw,vf4zw,vf4zw
	vaddx.z vf7z,vf5z,vf4x
	vaddy.y vf7y,vf5y,vf4y
	vsubx.y vf8y,vf5y,vf4x
	vaddy.z vf8z,vf5z,vf4y
	li $7,4
_RotMatrixX_loop:
	lqc2 vf4,0(%1)
	vmulax.xyzw ACCxyzw,vf6xyzw,vf4x
	vmadday.xyzw ACCxyzw,vf7xyzw,vf4y
	vmaddaz.xyzw ACCxyzw,vf8xyzw,vf4z
	vmaddw.xyzw vf5xyzw,vf9xyzw,vf4w
	sqc2 vf5,0(%0)
	addi $7,$7,-1
	addi %1,%1,16
	addi %0,%0,16
	bne $0,$7,_RotMatrixX_loop
	": : "r" (m0), "r" (m1), "f" (rx) : "$6", "$7", "$8", "$f0");
}

void sceVu0RotMatrixY(sceVu0FMATRIX m0, sceVu0FMATRIX m1, float ry)
{
	asm __volatile__("
	mtc1 $0,$f0
	c.lt.s %2,$f0
	li.s $f0,1.57079637050628662109e0
	bc1f _RotMatrixY_01
	add.s %2,$f0,%2
	li $7,1
	j _RotMatrixY_02
_RotMatrixY_01:
	sub.s %2,$f0,%2
	move $7,$0
_RotMatrixY_02:
	mfc1 $8,%2
	qmtc2 $8,vf6
	move $6,$31
	jal _sceVu0ecossin
	move $31,$6
	vmove.xyzw vf6xyzw,vf5xyzw
	vmove.xyzw vf7xyzw,vf5xyzw
	vmove.xyzw vf8xyzw,vf5xyzw
	vmove.xyzw vf9xyzw,vf5xyzw
	vaddw.y vf7y,vf5y,vf0w
	vaddw.w vf9w,vf5w,vf0w
	vsub.zw vf4zw,vf4zw,vf4zw
	vsubx.z vf6z,vf5z,vf4x
	vaddy.x vf6x,vf5x,vf4y
	vaddx.x vf8x,vf5x,vf4x
	vaddy.z vf8z,vf5z,vf4y
	li $7,4
_RotMatrixY_loop:
	lqc2 vf4,0(%1)
	vmulax.xyzw ACCxyzw,vf6xyzw,vf4x
	vmadday.xyzw ACCxyzw,vf7xyzw,vf4y
	vmaddaz.xyzw ACCxyzw,vf8xyzw,vf4z
	vmaddw.xyzw vf5xyzw,vf9xyzw,vf4w
	sqc2 vf5,0(%0)
	addi $7,$7,-1
	addi %1,%1,16
	addi %0,%0,16
	bne $0,$7,_RotMatrixY_loop
	": : "r" (m0), "r" (m1), "f" (ry) : "$6", "$7", "$8", "$f0");
}

void sceVu0RotMatrix(sceVu0FMATRIX m0, sceVu0FMATRIX m1, sceVu0FVECTOR rot)
{
	sceVu0RotMatrixZ(m0, m1, rot[2]);
	sceVu0RotMatrixY(m0, m0, rot[1]);
	sceVu0RotMatrixX(m0, m0, rot[0]);
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

void sceVu0CameraMatrix(sceVu0FMATRIX m, sceVu0FVECTOR p, sceVu0FVECTOR zd, sceVu0FVECTOR yd)
{
	sceVu0FMATRIX m0;
	sceVu0FVECTOR xd;

	sceVu0UnitMatrix(m0);
	sceVu0OuterProduct(xd, yd, zd);
	sceVu0Normalize(m0[0], xd);
	sceVu0Normalize(m0[2], zd);
	sceVu0OuterProduct(m0[1], m0[2], m0[0]);
	sceVu0TransMatrix(m0, m0, p);
	sceVu0InversMatrix(m, m0);
}

void sceVu0NormalLightMatrix(sceVu0FMATRIX m, sceVu0FVECTOR l0, sceVu0FVECTOR l1, sceVu0FVECTOR l2)
{
	float mul;
	sceVu0FVECTOR t;

	mul = -1.0;
	sceVu0ScaleVector(t, l0, mul);
	sceVu0Normalize(m[0], t);
	sceVu0ScaleVector(t, l1, mul);
	sceVu0Normalize(m[1], t);
	sceVu0ScaleVector(t, l2, mul);
	sceVu0Normalize(m[2], t);

	m[3][0] = m[3][1] = m[3][2] = 0.0f;
	m[3][3] = 1.0f;

	sceVu0TransposeMatrix(m, m);
}

void sceVu0LightColorMatrix(sceVu0FMATRIX m, sceVu0FVECTOR c0, sceVu0FVECTOR c1, sceVu0FVECTOR c2, sceVu0FVECTOR a)
{
	sceVu0CopyVector(m[0], c0);
	sceVu0CopyVector(m[1], c1);
	sceVu0CopyVector(m[2], c2);
	sceVu0CopyVector(m[3], a);
}

void sceVu0ViewScreenMatrix(sceVu0FMATRIX m, float scrz, float ax, float ay, float cx, float cy,
	float zmin, float zmax, float nearz, float farz)
{
	float az, cz;
	sceVu0FMATRIX mt;

	cz = (-zmax * nearz + zmin * farz) / (-nearz + farz);
	az = farz * nearz * (-zmin + zmax) / (-nearz + farz);

	sceVu0UnitMatrix(m);
	m[0][0] = scrz;
	m[1][1] = scrz;
	m[2][2] = 0.0f;
	m[3][3] = 0.0f;
	m[3][2] = 1.0f;
	m[2][3] = 1.0f;

	sceVu0UnitMatrix(mt);
	mt[0][0] = ax;
	mt[1][1] = ay;
	mt[2][2] = az;
	mt[3][0] = cx;
	mt[3][1] = cy;
	mt[3][2] = cz;

	sceVu0MulMatrix(m, mt, m);
}

void sceVu0DropShadowMatrix(sceVu0FMATRIX m, sceVu0FVECTOR lp, float a, float b, float c, int mode)
{
	if (mode) {
		float x = lp[0], y = lp[1], z = lp[2];
		float e = 1.0f - (a * x + b * y + c * z);

		m[0][0] = a * x + e;
		m[1][0] = b * x;
		m[2][0] = c * x;
		m[3][0] = -x;
		m[0][1] = a * y;
		m[1][1] = b * y + e;
		m[2][1] = c * y;
		m[3][1] = -y;
		m[0][2] = a * z;
		m[1][2] = b * z;
		m[2][2] = c * z + e;
		m[3][2] = -z;
		m[0][3] = a;
		m[1][3] = b;
		m[2][3] = c;
		m[3][3] = e - 1.0f;
	} else {
		float x = lp[0], y = lp[1], z = lp[2];
		float d = a * x + b * y + c * z;
		float e = -1.0f / d;

		m[0][0] = e * (a * x - d);
		m[1][0] = e * (b * x);
		m[2][0] = e * (c * x);
		m[3][0] = e * -x;
		m[0][1] = e * (a * y);
		m[1][1] = e * (b * y - d);
		m[2][1] = e * (c * y);
		m[3][1] = e * -y;
		m[0][2] = e * (a * z);
		m[1][2] = e * (b * z);
		m[2][2] = e * (c * z - d);
		m[3][2] = e * -z;
		m[0][3] = 0.0f;
		m[1][3] = 0.0f;
		m[2][3] = 0.0f;
		m[3][3] = e * -d;
	}
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

void sceVu0CopyVectorXYZ(sceVu0FVECTOR v0, sceVu0FVECTOR v1)
{
	v0[0] = v1[0];
	v0[1] = v1[1];
	v0[2] = v1[2];
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

