/* The HUD's compass: a strip textured with the compass, its anchors and arrows (MapCompass). SOCOM
 * 1's DWARF layout moved for SOCOM II: 38 anchors where SOCOM 1 had 33 (the anchor tables 0x50
 * longer), the anchor-list head a plain C2D whose 16-aligned neighbours keep it at 0x480, and an
 * anchor 0x2A0 bytes (its string 0x10 longer). The header's name is ours. */
#ifndef HUD_MAPCOMPASS_H
#define HUD_MAPCOMPASS_H

#include "../../gamez/ztwod/ztwod.h"

class CZSealBody;

class CHudCompassAnchor {
public:
    C2DBitmapPoly m_icon;   // 0x0
    C2DString m_string;   // 0x1f0
    float m_width;   // 0x288
    float m_height;   // 0x28c
    zdb::CTexHandle *m_htex;   // 0x290
    zdb::CTexture *m_texture;   // 0x294
};   // 0x2a0 bytes

class MapCompass {
public:
    MapCompass();
    ~MapCompass();
    void Tick(CZSealBody *seal);
    void Draw(zdb::CCamera *camera);
    void SetTrans(float trans);
    void SetTexture(zdb::CTexture *tex);
    void SetPos(float topx, float topy, float botx, float boty);
    void Off();
    void On();
    void SetScale(float scale);
    void InitHUD_Compass(void *world);

    float m_scale;   // 0x0
    zdb::CTexture *m_newcompasstex;   // 0x4
    zdb::CTexHandle *m_anchor_img[38];   // 0x8
    CPnt3D m_anchor_clr[38];   // 0xa0
    float m_tex_width;   // 0x268
    float m_tex_height;   // 0x26c
    float m_compasstopx;   // 0x270
    float m_compasstopy;   // 0x274
    float m_compassbotx;   // 0x278
    float m_compassboty;   // 0x27c
    float m_compasscenterx;   // 0x280
    float m_compasscentery;   // 0x284
    C2DBitmapPoly m_poly;   // 0x290
    C2D m_anchor_head;   // 0x480
    C2DBitmap m_fov;   // 0x4a0
    zdb::CTexture *m_fovtex;   // 0x520
    CHudCompassAnchor m_compass_anchors[38];   // 0x530
    CHudCompassAnchor m_compass_arrows[38];   // 0x68f0
};   // 0xccb0 bytes

#endif
