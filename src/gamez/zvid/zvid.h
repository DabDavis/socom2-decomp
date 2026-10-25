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
    /* SOCOM 1's displayBuf and renderBuf, the frame buffers' pages, then 0xC more: the final copy
     * draws into the first from the second (its FRAME and TEX0), CVideo::Clear clears the first in
     * the frame's pixel format (SOCOM 1's Clear cleared renderBuf; placed by the wep lane) */
    unsigned int displayBuf;   // 0x30
    unsigned int renderBuf;   // 0x34
    unsigned char _38[0x3c - 0x38];
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
    unsigned char _64[0x75 - 0x64];
    /* 2: the frame waits on the timer for a thirtieth of a second, else on the vertical blank
     * (0x3B0660; the name ours) */
    unsigned char syncMode;   // 0x75
    unsigned char _76[0x92 - 0x76];
    /* the final copy blends (its alpha word 96, else 0: the wep lane's; the name ours) */
    unsigned char finalBlend;   // 0x92
    unsigned char _93[0x94 - 0x93];
};   // 0x94 bytes (the game states set the byte at 0x91)

extern _zvid_public zVid;

void zVid_Assert(bool ok, unsigned int color, char *file, int line);

#endif
