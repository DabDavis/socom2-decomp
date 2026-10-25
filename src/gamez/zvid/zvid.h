/* zVid -- the video system's public state (SOCOM 1's _zvid_public, 0x80 bytes; zcam_main.cpp is
 * among the units that declare it). SOCOM II keeps SOCOM 1's head to the aspect, then moves the
 * render size 0xC on and adds the screen rectangle before the z range, which SOCOM 1's camera
 * constructor wrote as constants (CCamera's constructor and FOV setters read all of it). What moved
 * before the render size is not placed yet. The header's name is ours. */
#ifndef ZVID_H
#define ZVID_H

struct _zvid_public {
    unsigned int wireFrameFlag;   // 0x0
    int showTextureBuffer;   // 0x4
    int showTextureBufferFormat;   // 0x8
    unsigned long frameNumber;   // 0x10 (64 bits; CNode::Rendered compares nodes' stamps with it)
    unsigned int frameRate;   // 0x18
    float frameTime;   // 0x1c
    float runTime;   // 0x20
    float procTime;   // 0x24
    float aspect[2];   // 0x28
    /* SOCOM 1's displayBuf, renderBuf, with 0xC more: only what CVideo::Clear reads is placed,
     * the frame buffer and its pixel format the FRAME register takes (SOCOM 1's Clear read
     * renderBuf; the names ours) */
    unsigned int frameBuf;   // 0x30
    unsigned char _34[0x3c - 0x34];
    unsigned int framePsm;   // 0x3c
    unsigned char _40[0x44 - 0x40];
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
