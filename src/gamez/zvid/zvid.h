/* zVid -- the video system's public state (SOCOM 1's _zvid_public, 0x80 bytes; zcam_main.cpp is
 * among the units that declare it). SOCOM II keeps SOCOM 1's head to the aspect, then moves the
 * render size 0xC on and adds the screen rectangle before the z range, which SOCOM 1's camera
 * constructor wrote as constants (CCamera's constructor and FOV setters read all of it). What moved
 * before the render size is not placed yet. The header's name is ours. */
#ifndef ZVID_H
#define ZVID_H

struct _zvid_public {
    unsigned char _0[0x28];   // SOCOM 1's wireFrameFlag .. procTime
    float aspect[2];   // 0x28
    unsigned char _30[0x44 - 0x30];   // SOCOM 1's displayBuf, renderBuf, with 0xC more
    unsigned int renderWidth;   // 0x44 (SOCOM 1: 0x38)
    unsigned int renderHeight;   // 0x48 (SOCOM 1: 0x3c)
    int screenLeft;   // 0x4c: SOCOM II's
    int screenTop;   // 0x50
    int screenRight;   // 0x54
    int screenBottom;   // 0x58
    float minZ;   // 0x5c (SOCOM 1: 0x40)
    float maxZ;   // 0x60 (SOCOM 1: 0x44)
    unsigned char _64[0x80 - 0x64];
};

extern _zvid_public zVid;

void zVid_Assert(bool ok, unsigned int color, char *file, int line);

#endif
