/* The network game's info box (CMPInfoBox, SOCOM II's): the health bar, two bitmaps, a string and
 * who is talking on the radio, with the radio icons (SOCOM 1's CZStaticRadioBitmap and
 * CZDynamicRadioBitmap, renamed). Laid out from CHUD's constructor, which inlines all of their
 * constructors but the health bar's. The header's name and the members' names are ours. */
#ifndef HUD_MPINFOBOX_H
#define HUD_MPINFOBOX_H

#include "../../gamez/ztwod/ztwod.h"

class CRadioIcon_Static : public C2DBitmap {
public:
    CRadioIcon_Static()
    {
        m_pRadioOffTexture = 0;
        m_pRadioChannelTeamTexture = 0;
        m_pRadioChannelOffenseTexture = 0;
        m_pRadioChannelDefenseTexture = 0;
        m_pRadioChannelDeadTexture = 0;
    }
    virtual void Tick(float dT);

    zdb::CTexHandle *m_pRadioOffTexture;   // 0x80
    zdb::CTexHandle *m_pRadioChannelTeamTexture;   // 0x84
    zdb::CTexHandle *m_pRadioChannelOffenseTexture;   // 0x88
    zdb::CTexHandle *m_pRadioChannelDefenseTexture;   // 0x8c
    zdb::CTexHandle *m_pRadioChannelDeadTexture;   // 0x90
    zdb::CTexHandle *m_pRadioChannel10Texture;   // 0x94
};   // 0xa0 bytes

class CRadioIcon_Dynamic : public C2DBitmap {
public:
    CRadioIcon_Dynamic()
    {
        m_pRadioChannelInUseTexture = 0;
        m_pRadioChannelCanTalkTexture = 0;
    }
    virtual void Tick(float dT);

    zdb::CTexHandle *m_pRadioChannelInUseTexture;   // 0x80
    zdb::CTexHandle *m_pRadioChannelCanTalkTexture;   // 0x84
    unsigned char _88[0x90 - 0x88];
};   // 0x90 bytes

/* who is talking on the radio (its vtable 0x405BB0 overrides Tick) */
class CWhosTalking : public C2D {
public:
    virtual void Tick(float dT);

    unsigned char _18[0x20 - 0x18];
    CRadioIcon_Static m_static;   // 0x20
    CRadioIcon_Dynamic m_dynamic;   // 0xc0
    C2DBitmap m_bitmap;   // 0x150
    C2DString m_text1;   // 0x1d0
    C2DString m_text2;   // 0x268
    C2DBitmap m_bitmap2;   // 0x300
};   // 0x380 bytes

/* the health bar (its constructor and destructor are its own unit's: 0x2423D0, 0x242300) */
class CHealthBar : public C2D {
public:
    CHealthBar();
    virtual ~CHealthBar();
    /* the bar set from its owner's health (0x241CC0; the name ours) */
    void Update(class CZSealBody *owner, bool force);

    unsigned char _18[0x1d0 - 0x18];
};   // 0x1d0 bytes

/* the box (its vtable 0x405B40 overrides Off, On and Tick) */
class CMPInfoBox : public C2D {
public:
    virtual void Off();
    virtual void On();
    virtual void Tick(float dT);
    void UnInit();

    unsigned char _18[0x20 - 0x18];
    CHealthBar m_healthbar;   // 0x20
    C2DBitmap m_bitmap1;   // 0x1f0
    C2DBitmap m_bitmap2;   // 0x270
    C2DString m_text;   // 0x2f0
    CWhosTalking m_whostalking;   // 0x390
};   // 0x710 bytes

#endif
