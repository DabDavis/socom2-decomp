/* An item on the HUD's map (CZMapItem): only its vtable, every virtual inline -- their out-of-line
 * copies come out where the first map item's vtable does (hud_claymoremapitem.cpp's are linked,
 * 0x208B80-0x208FC0). SOCOM 1's DWARF has the class empty (4 bytes, the vtable pointer). The
 * header's name is ours. */
#ifndef HUD_MAPITEM_H
#define HUD_MAPITEM_H

namespace zdb {
class CCamera;
}
class CZNewHudMap;

class CZMapItem {
public:
    virtual ~CZMapItem() {}
    virtual bool Tick(CZNewHudMap *map, float dT) { return false; }
    virtual void Draw(zdb::CCamera *camera) {}
    virtual void PostDraw(zdb::CCamera *camera) {}
    virtual void Clip(CZNewHudMap *map) {}
};   // 4 bytes

#endif
