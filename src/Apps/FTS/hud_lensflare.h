/* The HUD's lens flares: SOCOM II's manager and entries, where SOCOM 1's hud_lensflare.cpp had
 * CLensFlareSaturate. The header's name is ours. */
#ifndef HUD_LENSFLARE_H
#define HUD_LENSFLARE_H

#include <list>
#include <vector>

#include "../../gamez/ztwod/ztwod.h"

namespace zdb {
class CNode;
}
struct _zrdr;

/* one element of a flare (SOCOM II's; the name ours): a bitmap placed along the flare's line by its
 * position scale, sized and faded by how far it is through its range -- 0xA0 bytes, the bitmap a
 * MEMBER (its constructor stores no table of its own, and deleting one destroys the bitmap without a
 * virtual call: the implicit destructor of a class with no virtuals) */
class CFlareElement {
public:
    /* off, opaque to begin with, scaled from a thousandth to a hundredth (inlined at LoadFlare's new) */
    CFlareElement() : m_opacity(0.0f), m_opacityRange(1.0f), m_scale(0.001f), m_scaleRange(0.01f - m_scale) {}
    /* the bitmap hidden first (an inline written destructor: deleting one tests it once, the
     * delete's null test merged with the destructor's this-test, then the bitmap member's own) */
    ~CFlareElement() { m_bmp.Off(); }
    /* the element's settings from its record (texture, color, position_scale, size, opacity, scale) */
    bool Load(_zrdr *rdr);
    /* shown at a point of the flare's line, or hidden: `t` how far through its range */
    void Place(bool visible, const CPnt3D *pos, float t);

    C2DBitmap m_bmp;   // 0x0
    float m_posScale;   // 0x80: position_scale (the screen offset from the centre scaled by it)
    float m_halfWidth;   // 0x84: half the size
    float m_halfHeight;   // 0x88
    float m_opacity;   // 0x8c: opacity's second value ...
    float m_opacityRange;   // 0x90: ... and the first less it
    float m_scale;   // 0x94: scale's second value ...
    float m_scaleRange;   // 0x98: ... and the first less it
};   // 0xa0 bytes

/* a flare: a bitmap (its vtable 0x4052B0 has C2DBitmap's Draw) with a handle in the flare system
 * (-1 for none), placed on a node of the world when it tracks one (the manager's PreDITick and
 * ReInit; the names ours). Its flags are set through inline setters taking a bool (the target's
 * unfolded `andi rN,zero,1`) */
class CLensFlareEntry : public C2DBitmap {
public:
    CLensFlareEntry(_zrdr *rdr, zdb::CNode *node);
    virtual ~CLensFlareEntry();
    virtual void Tick(float dT);
    /* the flare's state reset (SOCOM II's, the name ours: the constructor's first call) */
    void Init();
    /* its record read: the elements, the node, the ranges (0x1FFF40, the constructor's last call) */
    void LoadFlare(_zrdr *rdr, zdb::CNode *node);

    void SetOn(bool on) { m_on = on; }
    void SetFlag1(bool on) { m_flag1 = on; }
    void SetVisible(bool on) { m_visible = on; }
    void SetFlag3(bool on) { m_flag3 = on; }
    void SetFlag4(bool on) { m_flag4 = on; }
    void SetFlag6(bool on) { m_flag6 = on; }
    void SetRangeOpacity(bool on) { m_rangeOpacity = on; }
    bool IsSeen() { return m_flag1; }
    bool IsVisible() { return m_visible; }

    unsigned char m_on : 1;   // 0x80 bit 0: PreDITick asks the flare system only about these
    unsigned char m_flag1 : 1;
    unsigned char m_visible : 1;   // bit 2: the flare system's answer
    unsigned char m_flag3 : 1;
    unsigned char m_flag4 : 1;   // bit 4: LoadFlare sets it from "on"
    unsigned char m_tracking : 1;   // bit 5: the position follows the node each frame
    unsigned char m_flag6 : 1;
    unsigned char m_angleOpacity : 1;   // bit 7: "angle_opacity"
    unsigned char m_rangeOpacity : 1;   // 0x81 bit 0: unless "opacity_range_off"
    float m_84;   // 0x84: 0.75 to begin with
    float m_88;   // 0x88: 1 / (1 - m_84)
    int m_handle;   // 0x8c
    float m_90;   // 0x90: what PreDITick has the flare system's query write
    float m_fadeEnd;   // 0x94: "fade_range"'s second value
    float m_fadeScale;   // 0x98: one over the range
    float m_9c;   // 0x9c
    float m_a0;   // 0xa0
    float m_a4;   // 0xa4: 0.5 to begin with
    std::vector<CFlareElement *> m_elements;   // 0xa8: element1, element2, ...
    zdb::CNode *m_node;   // 0xb4
    CPnt3D m_pos;   // 0xb8: the node's origin in the world
    float m_size;   // 0xc4: given to the flare system with the position
    float m_c8;   // 0xc8
    float m_cc;   // 0xcc
    float m_d0;   // 0xd0
    float m_d4;   // 0xd4
    float m_d8;   // 0xd8
    float m_dc;   // 0xdc
};   // 0xe0 bytes (LoadRdr's new)

/* the flares, a list as the base (built before the vtable pointer is stored, which follows it at
 * 0xc). The vtable is three words: Tick its only slot, no virtual destructor */
class CLensFlareManager : public std::list<CLensFlareEntry *> {
public:
    virtual void Tick(float dT);
    void PreDITick(float dT);
    void ReInit();
    void Reset();
    void LoadRdr(const char *name);
    /* a LightSourceDefinition record: when active is "on", a flare made on every node the
     * group_node name (with its wildcards) finds (0x1FF630, the name ours) */
    void LoadLightSource(_zrdr *def);
    void Init();
    void Clear();

    bool m_active;   // 0x10
};   // 0x14 bytes

#endif
