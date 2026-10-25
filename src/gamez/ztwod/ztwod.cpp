/* ztwod.cpp -- C2D itself (OVL1 0x364D60-0x3651FF): its constructor and destructor, the statics
 * CGame::StartEngine sets up (C2D::Init, the screen's window; C2D::Open, the 2D camera), the
 * phase list's draw, on/off and the GS PRIM bits the flags give. The unit's name is ours (SOCOM 1's
 * DWARF has the header, gamez/ztwod/ztwod.h, but not this code); its bounds are inlining evidence:
 * C2DBitmap's constructor (0x364C50, just before) calls C2D's constructor and the PRIM helper out
 * of line, both short enough for -inline auto, so they are another unit's. Defined in reverse of
 * their image order (-inline deferred). */

#include "ztwod.h"

/* the camera only as big as it is (zcamera.h's, 0x570 bytes) */
namespace zdb {
class CCamera {
public:
    CCamera();

    unsigned char _0[0x570];
};
}

/* the screen as the video setup leaves it (OVL1 bss; no rows named yet): its size in pixels and
 * a float the 2D scale takes */
extern unsigned int data_004A44C4;
extern unsigned int data_004A44C8;
extern float data_004A44AC;

/* C2D's statics past SOCOM 1's (OVL1 bss): two scales the HUD multiplies by (MapCompass::SetPos;
 * the first the screen's width over 640), SOCOM 1's two at 1.0, the window's far corner and the
 * 2D camera (SOCOM 1 kept them in small data: gp-26224..-26200). Placeholders until named */
extern float data_0049E9F0;
extern float data_0049E9F4;
extern float data_0049E9F8;
extern float data_0049E9FC;
extern float data_0049EA08;
extern float data_0049EA0C;
extern zdb::CCamera *data_0049E9E0;

C2D::C2D()
{
    m_on = true;
    first = true;
    m_isAA = false;
    m_isTrans = false;
    m_hasTexture = true;
    m_isFrameAlpha = false;
    m_socom2_bit5 = false;
    m_active_and_handling_input = true;
    m_uivar = 0;
}

/* OPEN, 12 instructions short (0x68 against 0x98): include/list's. The target inlines the list
 * base's whole destructor -- C2Dlist's, list<C2D *>'s, __list_imp's and __list_deleter's this
 * tests, then the range erase -- so a pointer list's destructor is three high in SOCOM II, and a
 * class over it four (CHUD's inline ~CLensFlareManager is the same shape). include/list's is four:
 * probes (build/scratch/hud/p/c7.cpp against inc/list's variants) put the extra level in
 * ~__list_deleter's erase(iterator(end_.next_), iterator(&end_)) -- the inline iterator
 * constructor building the by-value arguments counts, the copy constructor does not; with raw node
 * pointers, or every iterator constructor out of line, the chain inlines whole under C2D. The
 * header is the entity lane's (~CEntityList, 0x2A00C0, was matched on its present height) */
C2D::~C2D()
{
}

/* the screen's window in GS coordinates, centred on 2048 (SOCOM 1's constants were a 640x448
 * screen's), and the scales */
void C2D::Init()
{
    float hw = 0.5f * data_004A44C4;
    float hh = 0.5f * data_004A44C8;

    m_topx = 2048.0f - hw;
    data_0049EA08 = 2048.0f + hw;
    m_topy = 2048.0f - hh;
    data_0049EA0C = 2048.0f + hh;
    data_0049E9F0 = data_004A44C4 / 640.0f;
    data_0049E9F4 = data_004A44AC;
    data_0049E9F8 = 1.0f;
    data_0049E9FC = 1.0f;
}

/* the 2D camera, made once */
void C2D::Open()
{
    if (!data_0049E9E0)
        data_0049E9E0 = new zdb::CCamera;
}

/* the children drawn while on */
void C2D::Draw(zdb::CCamera *cam)
{
    if (m_on) {
        iterator e = end();

        for (iterator i = begin(); i != e; ++i)
            (*i)->Draw(cam);
    }
}

void C2D::Off()
{
    m_on = false;
}

void C2D::On()
{
    m_on = true;
}

bool C2D::IsOn() const
{
    return m_on;
}

void C2D::Enable(bool on)
{
    m_on = on;
}

/* the GS PRIM word for a primitive type, with the bits the flags ask for: texture (TME), alpha
 * blending (ABE), antialiasing (AA1), Gouraud shading (IIP) -- SOCOM II's (the name is ours) */
void C2D::MakePrim(unsigned int &prim, unsigned int type)
{
    prim = type;
    if (m_hasTexture)
        prim |= 0x10;
    if (m_isTrans)
        prim |= 0x40;
    if (m_isAA)
        prim |= 0x80;
    if (m_socom2_bit5)
        prim |= 0x8;
}
