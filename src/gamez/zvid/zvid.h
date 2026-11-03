/* zVid -- the video system's public state (SOCOM 1's _zvid_public, 0x80 bytes; zcam_main.cpp is
 * among the units that declare it). SOCOM II keeps SOCOM 1's head to the aspect, then moves the
 * render size 0xC on and adds the screen rectangle before the z range, which SOCOM 1's camera
 * constructor wrote as constants (CCamera's constructor and FOV setters read all of it). What moved
 * before the render size is not placed yet. The header's name is ours. */
#ifndef ZVID_H
#define ZVID_H

/* the display mode zVid_Init takes (SOCOM 1's DWARF: a byte enum) */
enum _zvid_mode {
    ZVID_MODE_NTSC = 0,
    ZVID_MODE_PAL = 1,
    ZVID_MODE_DTV480P = 2,
    ZVID_MODE_NUM = 3
};

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
    /* the second render buffer's page (zVid_Init: 280, after renderBuf's 140; the name ours) */
    unsigned int renderBuf2;   // 0x38
    unsigned int framePsm;   // 0x3c
    /* the frame buffers' pixel format (zVid_Init: 0; SOCOM 1's private zvid + 0x254: the set-up's
     * sceGsSetDefDBuff and the image loader's sceGsSetDefLoadImage take it) */
    unsigned int psm;   // 0x40
    unsigned int renderWidth;   // 0x44 (SOCOM 1: 0x38)
    unsigned int renderHeight;   // 0x48 (SOCOM 1: 0x3c)
    int screenLeft;   // 0x4c: SOCOM II's
    int screenTop;   // 0x50
    int screenRight;   // 0x54
    int screenBottom;   // 0x58
    float minZ;   // 0x5c (SOCOM 1: 0x40)
    float maxZ;   // 0x60 (SOCOM 1: 0x44)
    /* the z range as integers, which zVid_Init converts into minZ/maxZ (10, 0xFFFF: SOCOM II's) */
    int iMinZ;   // 0x64
    int iMaxZ;   // 0x68
    unsigned int textureBaseAddr;   // 0x6c (SOCOM 1: 0x48)
    unsigned int textureEndAddr;   // 0x70 (SOCOM 1: 0x4c)
    unsigned char videoMode;   // 0x74 (SOCOM 1's _zvid_mode videoMode, 0x50: zVid_Init's)
    /* SOCOM 1's vidMode (0x51), which zvid_SetVideoMode stores: 2 (DTV480P) waits on the timer for a
     * thirtieth of a second, else on the vertical blank (0x3B0660; the name ours) */
    unsigned char syncMode;   // 0x75
    unsigned char _76[0x78 - 0x76];
    float hblnkRate;   // 0x78 (SOCOM 1: 0x54)
    /* 1: the frame copied whole each swap; 2: the copy's other packet (0x3B07B0; the name ours --
     * SOCOM 1's doMpeg224 sat here, 0x58 + 0x24) */
    unsigned char swapMode;   // 0x7c
    unsigned char pcrtcDo;   // 0x7d (SOCOM 1: 0x59)
    unsigned char _7e[0x80 - 0x7e];
    int pcrtcOffset[2];   // 0x80 (SOCOM 1's IPNT2D, 0x5c)
    /* SOCOM II's: zvid_SetVideoMode sets both, 18 the second for DTV480P (the names ours) */
    int modeOffset[2];   // 0x88
    unsigned char flag90;   // 0x90 (zVid_Init: 0)
    unsigned char flag91;   // 0x91 (zVid_Init: 1; the game states set it)
    /* the final copy blends (its alpha word 96, else 0: the wep lane's; the name ours) */
    unsigned char finalBlend;   // 0x92
    unsigned char _93[0x94 - 0x93];
    /* SOCOM 1's ztest_on/ztest_off (0x64/0x68) and dmaFrameRestore (0x70): the DMA references
     * zVid_Init points at 0x3E1340/0x3E1300 */
    void *ztest_on;   // 0x94
    void *ztest_off;   // 0x98
};   // 0xb0 bytes in the image (dmaFrameRestore at 0xa0; lodLevel follows at 0x4A4530)

extern _zvid_public zVid;

void zVid_Assert(bool ok, unsigned int color, char *file, int line);

#endif
