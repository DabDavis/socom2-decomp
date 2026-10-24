/* zVid -- the video system's public state (SOCOM 1's _zvid_public, 0x80 bytes; zcam_main.cpp is
 * among the units that declare it). SOCOM II's render size sits 0xC further on than SOCOM 1's
 * (CCamera's FOV setters read the width at 0x44 and the height at 0x48); what moved before it is
 * not placed yet. The header's name is ours. */
#ifndef ZVID_H
#define ZVID_H

struct _zvid_public {
    unsigned char _0[0x44];   // SOCOM 1's wireFrameFlag .. maxZ (0x0-0x47), with 0xC more
    unsigned int renderWidth;   // 0x44 (SOCOM 1: 0x38)
    unsigned int renderHeight;   // 0x48 (SOCOM 1: 0x3c)
    unsigned char _4c[0x80 - 0x4c];
};

extern _zvid_public zVid;

#endif
