/* kit.h -- the SEAL's kit, CZKit: its items, ammunition, reticle and zoom (SOCOM 1's DWARF: 0x87c
 * bytes, gamez/zseal; seal_kit.cpp's). SOCOM 1's members moved to where SOCOM II's code puts them:
 * the reticle block 0x14 on (InitializeSniperKick, the screen offsets), the item counters and the
 * body 0x1c on (SetBody, GetNumItems, GetSelectedItemNum), the pickup wait 0x40 on
 * (PendingPickupHandled). Only what a written function touches is named.
 *
 * zseal.h (the SEAL lane's) still declares a one-word CZKit of its own for CZSealBody::m_kit, so
 * this header stands alone -- it does not include zseal.h -- and only seal_kit.cpp includes it. */
#ifndef KIT_H
#define KIT_H

class CZSealBody;
class CZFTSWeapon;

/* the body's controller as the kit reads it (CSealCtrl; only the float the sniper's kick starts
 * from -- zseal.h does not name it) */
struct CKitCtrl {
    unsigned char _0[0x130];
    float m_130;   // 0x130
};

/* the body as the kit reads it (CZSealBody; only its controller, CEntity's m_control, and the byte
 * the scope tests read -- SOCOM 1's at 0x160) */
struct CKitBody {
    unsigned char _0[0xc0];
    CKitCtrl *m_control;   // 0xc0
    unsigned char _c4[0x200 - 0xc4];
    signed char m_200;   // 0x200: the current item's zoom class (binoculars 4, scopes 5 and up)
};

class CZKit {
public:
    float ScreenOffsetX();
    float ScreenOffsetY();
    void IncrScreenOffsetY(float dy);
    bool IsUsingBinocs();
    void InitializeSniperKick();
    bool IsUsingScope();
    void SetWeaponInfoChanged();
    void PendingPickupHandled();
    void SetBody(CZSealBody *body);
    int GetNumItems() const;
    int GetSelectedItemNum();
    CZFTSWeapon *GetCurItem() const;
    CZFTSWeapon *GetItemByIndex(int index) const;

    unsigned int m_first : 1;   // 0x0 (SOCOM 1's bits; SOCOM II inserted one before bit 14)
    unsigned int m_reticuletypeChanged : 1;
    unsigned int m_weaponinfoChanged : 1;
    unsigned int m_retblocked : 1;
    unsigned int m_nvg_enabled : 1;
    unsigned int m_binocs_enabled : 1;
    unsigned int m_lase_echo : 1;
    unsigned int m_lase_deploy_echo : 1;
    unsigned int m_lensfx_enable : 1;
    unsigned int m_waiting_to_fire : 1;
    unsigned int m_waiting_to_raise : 1;
    unsigned int m_changeToRifle : 1;
    unsigned int m_requested_item : 1;
    unsigned int m_bit13 : 1;   // new in SOCOM II
    unsigned int m_current_requested_ammo : 1;   // bit 14 (SOCOM 1's bit 13)
    unsigned int m_heartbeat_enabled : 1;
    unsigned int m_unused : 16;
    unsigned char _4[0x38 - 0x4];
    float m_firerifle_kick_baseline;   // 0x38 (SOCOM 1's 0x24)
    float m_firerifle_kick_movedist;   // 0x3c (SOCOM 1's 0x28)
    unsigned char _40[0x58 - 0x40];
    float m_screenoffset_x;   // 0x58 (SOCOM 1's 0x44)
    float m_screenoffset_y;   // 0x5c (SOCOM 1's 0x48)
    unsigned char _60[0x824 - 0x60];
    int m_selected_item;   // 0x824 (SOCOM 1's 0x808)
    int m_last_selected;   // 0x828
    int m_num_items;   // 0x82c (SOCOM 1's 0x810)
    CZSealBody *m_body;   // 0x830 (SOCOM 1's 0x814)
    unsigned char _834[0x8b8 - 0x834];
    float m_pickupRequestWait;   // 0x8b8 (SOCOM 1's 0x878)
};

#endif
