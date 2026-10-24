/* Camera and projector matrices (OVL1 0x307700-0x307B40), built on libvu0. A separate
 * translation unit from CQuat's as far as anything shows; the file name is a placeholder.
 * Defined in reverse of their image order (-inline deferred). */

#include "zmath.h"


extern "C" float fabsf(float);

extern "C" void func_003079C0(const CPnt3D *eye, const CPnt3D *at, const CPnt3D *dir, sceVu0FMATRIX out,
                              float a, float b)
{
    sceVu0FVECTOR xc;
    sceVu0FVECTOR p;
    sceVu0FVECTOR yd;
    sceVu0FVECTOR zd;
    sceVu0FMATRIX cam;
    sceVu0FMATRIX proj;
    sceVu0FVECTOR x;
    sceVu0FVECTOR v1;
    sceVu0FVECTOR v2;

    sceVu0UnitMatrix(cam);
    sceVu0UnitMatrix(proj);
    v1[0] = dir->x; v1[1] = dir->y; v1[2] = dir->z; v1[3] = 1.0f;
    v2[0] = at->x; v2[1] = at->y; v2[2] = at->z; v2[3] = 1.0f;
    sceVu0OuterProduct(x, v1, v2);
    sceVu0Normalize(x, x);
    xc[0] = x[0]; xc[1] = x[1]; xc[2] = x[2];
    VecSet(zd, at->x, at->y, at->z, 1.0f);
    VecSet(p, eye->x, eye->y, eye->z, 1.0f);
    sceVu0OuterProduct(yd, xc, zd);
    sceVu0CameraMatrix(cam, p, zd, yd);
    sceVu0UnitMatrix(proj);
    proj[0][0] = proj[1][1] = (a - 1.0f) / (b * a);
    proj[3][0] = proj[3][1] = 0.5f;
    sceVu0MulMatrix(out, proj, cam);
}

/* The same projector, its up axis the one dir leans on least -- the source's two tests
 * run straight-line (the last one updates no minimum, and fabsf is no pure function to
 * MW, so that was never a dead call), with `up` otherwise uninitialised. */
extern "C" void func_00307810(const CPnt3D *eye, const CPnt3D *dir, sceVu0FMATRIX out, float a, float b)
{
    float min;
    sceVu0FVECTOR up;
    int axis;
    sceVu0FVECTOR d;
    sceVu0FVECTOR p;
    sceVu0FVECTOR yd;
    sceVu0FVECTOR zd;
    sceVu0FMATRIX cam;
    sceVu0FMATRIX proj;

    sceVu0UnitMatrix(cam);
    sceVu0UnitMatrix(proj);
    VecSet(d, dir->x, dir->y, dir->z, 0.0f);
    min = fabsf(d[0]);
    axis = 0;
    if (fabsf(d[1]) < min) {
        axis = 1;
        min = fabsf(d[1]);
    }
    if (fabsf(d[2]) < min)
        axis = 2;
    up[axis] = 1.0f;
    sceVu0OuterProduct(yd, d, up);
    sceVu0Normalize(yd, yd);
    VecSet(zd, dir->x, dir->y, dir->z, 1.0f);
    VecSet(p, eye->x, eye->y, eye->z, 1.0f);
    sceVu0CameraMatrix(cam, p, zd, yd);
    sceVu0UnitMatrix(proj);
    proj[0][0] = proj[1][1] = (a - 1.0f) / (b * a);
    proj[3][0] = proj[3][1] = 0.5f;
    sceVu0MulMatrix(out, proj, cam);
}

/* A camera looking along zd from p: the up axis is x or z, whichever zd leans on less.
 * Two unit matrices are set up and never used, and `up` keeps whatever the stack held in
 * its other components -- both as the original has them. */
extern "C" void func_00307700(sceVu0FVECTOR zd, sceVu0FVECTOR p, sceVu0FMATRIX out)
{
    sceVu0FVECTOR up;
    sceVu0FVECTOR yd;
    sceVu0FMATRIX m0;
    sceVu0FMATRIX m1;
    float z;

    sceVu0UnitMatrix(m0);
    sceVu0UnitMatrix(m1);
    z = zd[2];
    if (fabsf(zd[0]) < fabsf(z))
        up[0] = 1.0f;
    else
        up[2] = 1.0f;
    up[3] = 1.0f;
    sceVu0OuterProduct(yd, zd, up);
    sceVu0Normalize(yd, yd);
    if (yd[1] < 0.0f) {
        yd[0] = -yd[0];
        yd[1] = -yd[1];
        yd[2] = -yd[2];
    }
    sceVu0CameraMatrix(out, p, zd, yd);
}
