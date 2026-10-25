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

#include "../znode/znode.h"

class CZSealBody;

/* the kit's items (SOCOM 1's EQUIP_ITEM, a byte; the values are not needed yet) */
enum EQUIP_ITEM { EQUIP_NONE = 0, EQUIP_LAST = 255 };

/* an item's class as CZWeapon::GetIDType gives it (a byte; SOCOM II's sniper rifles are 101, 254
 * is no item -- the names ours) */
enum EQUIP_IDTYPE { IDTYPE_SNIPER_RIFLE = 101, IDTYPE_NONE = 254, IDTYPE_LAST = 255 };

/* a weapon's fire modes (SOCOM 1's DWARF, a byte; SOCOM II's item lookups take 255 for the item's
 * current mode -- that name ours) */
enum FIREMODE {
    SAFETY = 0, SINGLE = 1, BURST = 2, AUTOFIRE = 3, SPECIAL_MODE = 4, NUM_FIREMODES = 5,
    FIREMODE_CURRENT = 255
};

/* an item of the kit (SOCOM 1's CZFTSWeapon : CZWeapon; only the byte its kind is read from --
 * SOCOM 1's at 0x60) */
class CZFTSWeapon {
public:
    unsigned char _0[0x7c];
    EQUIP_ITEM m_equip;   // 0x7c
};

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
    bool HasItem(EQUIP_ITEM item) const;
    bool WeaponInfoChanged();
    bool ItemHasAmmo(int index) const;
    bool ItemHasAmmo(int index, FIREMODE mode) const;
    bool CurItemHasAmmo();
    bool ItemHasAmmo(EQUIP_ITEM item);
    bool IsFireDelayed();
    FIREMODE GetCurFireMode();
    int GetNumMags(int index);
    int unwrap(int index, FIREMODE mode) const;
    EQUIP_IDTYPE GetCurItemIDType();
    bool HasSniperRifle();
    bool HasItemWithAmmo(EQUIP_ITEM item);
    EQUIP_ITEM GetSecondaryEnum() const;
    EQUIP_ITEM GetPrimaryEnum() const;
    EQUIP_ITEM GetCurItemEnum() const;
    void *GetAmmoByIndex(int index);
    void *GetCurAmmo();
    void NVG_Enabled(bool on);
    bool NVG_Enabled() const;

    /* an index wrapped into [0, n), or -1 (SOCOM 1's unwrap__5CZKitFii, out of line there; SOCOM
     * II inlines it everywhere) */
    static int wrap(int i, int n)
    {
        if (i == n || n == 0)
            return -1;
        if (i >= n)
            do
                i -= n;
            while (i >= n);
        if (i < 0)
            do
                i += n;
            while (i < 0);
        return i;
    }
    /* the first item of that kind with ammunition, else the last of that kind, else -1 (inlined
     * in each caller; the name ours) */
    int FindItemWithAmmo(EQUIP_ITEM item) const
    {
        int found = -1;

        for (int i = 0; i < m_num_items; i++) {
            if (m_item[i] && item == m_item[i]->m_equip) {
                if (ItemHasAmmo(i))
                    return i;
                found = i;
            }
        }
        return found;
    }

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
    unsigned char _60[0xe4 - 0x60];
    CZFTSWeapon *m_item[30];   // 0xe4 (SOCOM 1's 0xd0)
    void *m_ammo[30];   // 0x15c (SOCOM 1's CZAmmo *, 0x148)
    int m_reloads[30][10];   // 0x1d4 (SOCOM 1's 0x1c0): each item's magazines, the rounds in each
    int m_currentmag[30];   // 0x684 (SOCOM 1's 0x670)
    int m_firemode[30];   // 0x6fc (SOCOM 1's 0x6e8): read as a FIREMODE, its low byte
    unsigned char _774[0x820 - 0x774];
    float m_fire_delay;   // 0x820 (SOCOM 1's 0x804)
    int m_selected_item;   // 0x824 (SOCOM 1's 0x808)
    int m_last_selected;   // 0x828
    int m_num_items;   // 0x82c (SOCOM 1's 0x810)
    CZSealBody *m_body;   // 0x830 (SOCOM 1's 0x814)
    unsigned char _834[0x8b8 - 0x834];
    float m_pickupRequestWait;   // 0x8b8 (SOCOM 1's 0x878)
};

#endif
