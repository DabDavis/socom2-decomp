/* The cinematic letterbox: two black bars sliding in from the top and bottom of the screen
 * (CLetterBox). SOCOM 1's DWARF layout, the offsets unchanged in SOCOM II (the bitmaps are
 * 16-aligned, so C2D's lost 4 bytes leave them where they were). The header's name is ours. */
#ifndef HUD_LETTERBOX_H
#define HUD_LETTERBOX_H

#include "../../gamez/ztwod/ztwod.h"

enum SLIDE_DIRECTION {
    SLIDE_NONE = 0,
    SLIDE_IN = 1,
    SLIDE_OUT = 2,
};

class CLetterBox : public C2D {
public:
    CLetterBox();
    virtual void Draw(zdb::CCamera *cam);
    virtual void Tick(float dT);
    void RemoveLetterbox();
    void AddLetterbox();

    C2DBitmap m_bitmap[2];   // 0x20
    SLIDE_DIRECTION m_slidedir;   // 0x120
    float m_curpos;   // 0x124
};   // 0x130 bytes

#endif
