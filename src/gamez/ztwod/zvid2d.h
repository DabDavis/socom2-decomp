/* zVid as the 2D code reads it: SOCOM II's 0x94 bytes (OVL1 0x4A4480, data.txt's row), the fields
 * named as gamez/zvid/zvid.h names them where it has them. zvid.h (the integrator's) stops at
 * SOCOM 1's 0x80 bytes and pads past maxZ, so the C2D units read this declaration instead; the two
 * agree where they overlap and are never included together. The header's name is ours.
 *   - zVid_Init (0x3B1200) stores the z range as ints at 0x64 and 0x68 (10 and 0xFFFF) and their
 *     float copies at minZ and maxZ: the 2D depth is maxZ, the float for C2DLine's ends and
 *     C2DBitmap's corners, the int for C2DPoly's XYZ2 words and C2DString's packets;
 *   - the byte at 0x75 is the video mode zVid_Init takes (UICheckVideoMode reads it; the states
 *     test it against 2, C2DPoly against 1, when it squeezes y about screenTop by aspect[1]). */
#ifndef ZVID2D_H
#define ZVID2D_H

struct _zvid_2d {
    unsigned int wireFrameFlag;   // 0x0
    int showTextureBuffer;   // 0x4
    int showTextureBufferFormat;   // 0x8
    unsigned long frameNumber;   // 0x10
    unsigned int frameRate;   // 0x18
    float frameTime;   // 0x1c
    float runTime;   // 0x20
    float procTime;   // 0x24
    float aspect[2];   // 0x28: [1] is C2D::m_fYPixel
    unsigned char _30[0x44 - 0x30];
    unsigned int renderWidth;   // 0x44
    unsigned int renderHeight;   // 0x48
    int screenLeft;   // 0x4c
    int screenTop;   // 0x50
    int screenRight;   // 0x54
    int screenBottom;   // 0x58
    float minZ;   // 0x5c
    float maxZ;   // 0x60: the 2D depth
    int iMinZ;   // 0x64: SOCOM II's (the names ours)
    int iMaxZ;   // 0x68
    unsigned char _6c[0x75 - 0x6c];
    unsigned char videoMode;   // 0x75
    unsigned char _76[0x94 - 0x76];
};   // the row's 0x94 bytes (sizeof rounds to 8)

extern _zvid_2d zVid;

#endif
