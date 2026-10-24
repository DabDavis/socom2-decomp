/* hud_letterbox.cpp -- the cinematic letterbox (OVL1 0x1FEC60-0x1FF053): SOCOM 1's unit (its DWARF
 * names it, Z:\dev\Apps\FTS\hud_letterbox.cpp), the same five functions in the same order. SOCOM
 * II draws a bar only while it is on, and the top bar sliding out spans the render width.
 * Defined in reverse of their image order (-inline deferred). */

#include "hud_letterbox.h"
#include "../../gamez/zvid/zvid.h"

/* two black untextured bars, nearly opaque (127 of 128), off, listed as children */
CLetterBox::CLetterBox()
{
    for (int i = 0; i < 2; i++) {
        m_bitmap[i].SetHasTexture(false);
        m_bitmap[i].Load(0.0f, 0.0f, 1.0f, 1.0f, 0);
        m_bitmap[i].SetColor(0.0f, 0.0f, 0.0f);
        m_bitmap[i].SetTrans(127.0f);
        m_bitmap[i].SetIsTrans(true);
        m_bitmap[i].Off();
        push_back(&m_bitmap[i]);
    }
    m_curpos = 0.0f;
}

void CLetterBox::Draw(zdb::CCamera *cam)
{
    if (m_bitmap[0].IsOn())
        m_bitmap[0].Draw(cam);
    if (m_bitmap[1].IsOn())
        m_bitmap[1].Draw(cam);
}

/* the bars slid 128 lines a second: in to 64 lines deep, or out to nothing (and off) */
void CLetterBox::Tick(float dT)
{
    switch (m_slidedir) {
    case SLIDE_IN:
        if (m_curpos == 64.0f) {
            m_slidedir = SLIDE_NONE;
            break;
        }
        m_curpos += 128.0f * dT;
        if (!(m_curpos < 64.0f)) {
            m_curpos = 64.0f;
            m_slidedir = SLIDE_NONE;
        }
        m_bitmap[0].Load(0.0f, 0.0f, 640.0f, m_curpos, 0);
        m_bitmap[1].Load(0.0f, 448.0f, 640.0f, 448.0f - m_curpos, 0);
        break;
    case SLIDE_OUT:
        if (m_curpos == 0.0f) {
            m_slidedir = SLIDE_NONE;
            break;
        }
        m_curpos -= 128.0f * dT;
        if (m_curpos <= 0.0f) {
            m_curpos = 0.0f;
            m_slidedir = SLIDE_NONE;
            m_bitmap[0].Off();
            m_bitmap[1].Off();
        }
        m_bitmap[0].Load(0.0f, 0.0f, zVid.renderWidth, m_curpos, 0);
        m_bitmap[1].Load(0.0f, 448.0f, 640.0f, 448.0f - m_curpos, 0);
        break;
    default:
        return;   /* a block of its own (an empty default or break leaves none) */
    }
}

void CLetterBox::AddLetterbox()
{
    m_slidedir = SLIDE_IN;
    m_bitmap[0].On();
    m_bitmap[1].On();
}

void CLetterBox::RemoveLetterbox()
{
    m_slidedir = SLIDE_OUT;
}
