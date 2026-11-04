/* Sony libvu0 -- the prototypes the game calls, in our own words. The library itself
 * is linked prebuilt (GCC) into MAIN at 0x1BFC30 and is not decompiled here. */
#ifndef _SC_LIBVU0_H
#define _SC_LIBVU0_H

typedef float sceVu0FVECTOR[4] __attribute__((aligned(16)));
typedef float sceVu0FMATRIX[4][4] __attribute__((aligned(16)));

#ifdef __cplusplus
extern "C" {
#endif
void sceVu0MulMatrix(sceVu0FMATRIX m0, sceVu0FMATRIX m1, sceVu0FMATRIX m2);
void sceVu0CopyMatrix(sceVu0FMATRIX m0, sceVu0FMATRIX m1);
void sceVu0OuterProduct(sceVu0FVECTOR v0, sceVu0FVECTOR v1, sceVu0FVECTOR v2);
float sceVu0InnerProduct(sceVu0FVECTOR v0, sceVu0FVECTOR v1);
void sceVu0TransposeMatrix(sceVu0FMATRIX m0, sceVu0FMATRIX m1);
void sceVu0InversMatrix(sceVu0FMATRIX m0, sceVu0FMATRIX m1);
void sceVu0AddVector(sceVu0FVECTOR v0, sceVu0FVECTOR v1, sceVu0FVECTOR v2);
void sceVu0ScaleVector(sceVu0FVECTOR v0, sceVu0FVECTOR v1, float s);
void sceVu0UnitMatrix(sceVu0FMATRIX m);
void sceVu0RotMatrixZ(sceVu0FMATRIX m0, sceVu0FMATRIX m1, float rz);
void sceVu0Normalize(sceVu0FVECTOR v0, sceVu0FVECTOR v1);
void sceVu0TransMatrix(sceVu0FMATRIX m0, sceVu0FMATRIX m1, sceVu0FVECTOR tv);
void sceVu0CameraMatrix(sceVu0FMATRIX m, sceVu0FVECTOR p, sceVu0FVECTOR zd, sceVu0FVECTOR yd);
void sceVu0ApplyMatrix(sceVu0FVECTOR v0, sceVu0FMATRIX m0, sceVu0FVECTOR v1);
void sceVu0CopyVector(sceVu0FVECTOR v0, sceVu0FVECTOR v1);
/* v0.xyz = v1 * r + v2 * (1 - r) and v0.w = v1.w, a quadword written (MAIN 0x1C0768,
 * src/lib/sdk/libvu0.c) */
void sceVu0InterVectorXYZ(sceVu0FVECTOR v0, sceVu0FVECTOR v1, sceVu0FVECTOR v2, float r);
void sceVu0ViewScreenMatrix(sceVu0FMATRIX m, float scrz, float ax, float ay, float cx, float cy,
                            float zmin, float zmax, float nearz, float farz);
#ifdef __cplusplus
}
#endif
#endif
