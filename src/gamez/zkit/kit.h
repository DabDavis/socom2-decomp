/* kit.h -- the SEAL's kit, CZKit: its items, ammunition, reticle and zoom (SOCOM 1's DWARF: 0x87c
 * bytes, gamez/zseal; seal_kit.cpp's). SOCOM 1's members moved to where SOCOM II's code puts them:
 * the reticle block 0x14 on (InitializeSniperKick, the screen offsets), the item counters and the
 * body 0x1c on (SetBody, GetNumItems, GetSelectedItemNum), the pickup wait 0x40 on
 * (PendingPickupHandled). Only what a written function touches is named.
 *
 * zseal.h (the SEAL lane's) still declares a one-word CZKit of its own for CZSealBody::m_kit, so
 * this header does not include zseal.h, and only seal_kit.cpp includes it; the weapon classes are
 * zweapon.h's. */
#ifndef KIT_H
#define KIT_H

#include "../znode/znode.h"
#include "../zweapon/zweapon.h"

class CZSealBody;


/* an item's class as CZWeapon::GetIDType gives it (a byte; SOCOM II's sniper rifles are 101, 254
 * is no item -- the names ours) */
enum EQUIP_IDTYPE {
    IDTYPE_PISTOL = 4, IDTYPE_SMG = 31, IDTYPE_ASSAULT_RIFLE = 51, IDTYPE_SHOTGUN = 81,
    IDTYPE_MACHINEGUN = 91, IDTYPE_SNIPER_RIFLE = 101, IDTYPE_GRENADE = 121, IDTYPE_HEAVYWEAPON = 141,
    IDTYPE_EXPLOSIVE = 151, IDTYPE_BINOCULARS = 190, IDTYPE_NVG = 191, IDTYPE_NONE = 254,
    IDTYPE_LAST = 255
};

/* a reticle (a signed byte: SetCurReticule sign-extends GetReticuleByIndex's result only to
 * compare it; the names ours) */
enum RETICULE_TYPE { RETICULE_NONE = -1, RETICULE_LAST = 127 };

/* the weapons' classes and enums (EQUIP_ITEM, FIREMODE, CZWeaponList, CZWeapon, CZFTSWeapon) are
 * zweapon.h's: the stand-ins this header carried had zweapon.h's offsets (m_maxfiremode 0x24, m_ID
 * 0x7c, m_icon 0xbc, the zoom vector's size 0x284) */

/* the body's controller as the kit reads it (CSealCtrl; only the float the sniper's kick starts
 * from -- zseal.h does not name it) */
struct CKitCtrl {
    /* the bit the MP bomb's placing sets (inlined with true: `daddiu ...,0x10`; SOCOM 1's at 0x168
     * -- the name ours) */
    void SetBombPlaced(bool b) { m_bomb_placed = b; }
    /* (inlined with false: the target keeps `andi v0,zero,1`, the mask unfolded) */
    void SetThrowing(bool b) { m_throwing = b; }

    unsigned char _0[0x130];
    float m_130;   // 0x130
    unsigned char _134[0x170 - 0x134];
    unsigned char m_170 : 4;   // 0x170
    unsigned char m_bomb_placed : 1;   // bit 4
    unsigned char m_bit5 : 1;
    unsigned char m_throwing : 1;   // bit 6 (SOCOM 1's m_throwing, the same bit)
};

/* the body as the kit reads it (CZSealBody; only its controller, CEntity's m_control, and the byte
 * the scope tests read -- SOCOM 1's at 0x160) */
struct CKitBody {
    unsigned char _0[0x38];
    CPnt3D m_velW;   // 0x38: CEntity's world velocity (the AI's accuracy asks whether it moves)
    unsigned char _44[0xc0 - 0x44];
    CKitCtrl *m_control;   // 0xc0
    unsigned char _c4[0xe1 - 0xc4];
    unsigned char _e1 : 4;   // 0xe1
    unsigned char m_alive : 1;   // bit 4: CEntity's alive bit
    unsigned char _e1_5 : 3;
    unsigned char _e2[0x170 - 0xe2];
    unsigned char m_cbody[0x90];   // 0x170: the CBody base (0x90 bytes: the parts FindPart searches)
    signed char m_200;   // 0x200: the current item's zoom class (binoculars 4, scopes 5 and up)
    unsigned char _201[0x388 - 0x201];
    /* 0x388: attachments by kind, 8 bytes each -- the first word a record whose +0xC keeps the node
     * hung on it (0x5C6980; the names ours) */
    struct Attach {
        struct Record {
            unsigned char _0[0xc];
            void *m_node;   // 0xc
        } *m_rec;
        unsigned int m_4;
    } m_attach[4];
};

/* a weapon as the kit reads it (CZWeapon; only the wait between shots, SOCOM 1's m_firewait --
 * 0x48 there; zweapon.h does not name it) */
struct CKitWeapon {
    unsigned char _0[0x50];
    float m_firewait;   // 0x50
};

/* the AI's weapon hints (SOCOM 1's CAiWepHint: statics in seal_kit.cpp, after the saved fire modes;
 * no DWARF) */
class CAiWepHint {
public:
    /* a weapon's accuracy at a distance, doubled when asked, held within [0, 1] (OVL2 0x5C9340;
     * SOCOM 1's took the bool too, unclamped) and the middle of its AI range (OVL2 0x5C93C0;
     * SOCOM 1's) */
    static float GetWeaponAccuracy(CZWeapon *w, float d, bool b);
    static float GetWeaponEffectiveRange(CZWeapon *w);
};

class CZKit {
public:
    void ResetScreenOffsetXY();
    void Binocs_Enabled(bool on);
    void SetCurReticule(bool force);
    bool CurReticuleTypeChanged();
    float GetCurZoom();
    RETICULE_TYPE GetReticuleByIndex(int index);
    void *GetCurIcon();
    void *GetIcon(int index);
    void *GetFiremodeGraphic(int index);
    const char *GetItemName(int index);
    RETICULE_TYPE GetCurReticule();
    RETICULE_TYPE GetCurItemReticule();
    bool IsZoomed();
    bool SetItemAmmo(unsigned int index, void *ammo);
    bool SetItemAmmo(unsigned int index, void *ammo, int rounds, int count);
    bool SetItem(unsigned int index, EQUIP_ITEM item);
    void FillWeaponEnum(int index, CZFTSWeapon **out);
    /* a grenade thrown from a point at a velocity, an explosive placed instead (OVL2 0x5BD030;
     * SOCOM 1's) */
    CZProjectile *FireGrenade(const CPnt3D *pos, const CPnt3D *vel);
    /* whether an item is in its launcher's fire mode, by the launcher's kind (OVL2 0x5BD6E0,
     * 0x5BD7F0, 0x5BD8F0: SOCOM 1's WeaponIsGrenadeLauncherMode and, on an index, its
     * CurWeaponIsM203Mode; the middle one new -- its name ours) */
    bool WeaponIsGrenadeLauncherMode(int index);
    bool WeaponIsLauncherMode(int index);
    bool WeaponIsM203Mode(int index);
    bool SetItem(unsigned int index, CZWeapon *w, void *ammo, int rounds);
    void SetItemByIndex(int index, CZWeapon *w);
    bool ItemHasFiremodeGraphic(int index);
    bool CurItemHasFiremodeGraphic();
    bool IsUsingItem(int index);
    bool IsUsingItem(EQUIP_ITEM item);
    bool SetCurFireMode(FIREMODE mode);
    float ScreenOffsetX();
    float ScreenOffsetY();
    void IncrScreenOffsetY(float dy);
    bool IsUsingBinocs();
    void InitializeSniperKick();
    /* the rounds a pull of the trigger fires, and the wait after (OVL2 0x5C0940 -- new in SOCOM II,
     * SOCOM 1's WillFireWeapon did it inline: the name ours -- and 0x5C09F0, SOCOM 1's) */
    int GetShotCount();
    float GetFireDelay();
    /* the wait before the next shot set, -1 for the item in hand's own: its second wait when asked,
     * else its first -- the weapons' shared record told (OVL2 0x5C32B0, new in SOCOM II: CSealCtrl's
     * PreTick and 0x5C5340 call it; the name ours) */
    void SetFireDelay(float delay, bool alt);
    /* the hand's fire mode slot (0 or 1, by bit 18) set to a mode, the mode kept, and -- the body
     * alive -- the slot's item kind noted (OVL2 0x5C48B0, new in SOCOM II: CZSealBody::ReCreate
     * calls it; the name ours) */
    void SetHandFireMode(int mode);
    /* the item in hand's node hung under a part of the body -- hidden, made the part's child, kept in
     * the body's attachment of a kind (OVL2 0x5C6980, new in SOCOM II: 0x5C50B0 hangs it on
     * "aimnodes"; the name ours) */
    void AttachItemNode(unsigned char kind, const char *part);
    /* the item pointer of an index, as GetItemPtr but the bound compared signed (inline: 0x5C6980
     * expands it -- its slt where GetItemPtr's own copy has sltu; the name ours) */
    void *ItemPtr(int index) const
    {
        return index + 1 < (int)m_itemptr.size() ? m_itemptr[index + 1] : 0;
    }
    /* the hand's slot (inline; the names ours): a signed byte enum assigned in an if/else -- the
     * target's `li 1; movz` and sign extension; `bit ? 1 : 0` (any type) folds to an sltu, and the
     * enum's ternary branches */
    enum HAND_SLOT { HAND_NONE = -1, HAND_FIRST = 0, HAND_SECOND = 1 };
    HAND_SLOT HandSlot() const
    {
        HAND_SLOT h;

        if (m_second_hand)
            h = HAND_SECOND;
        else
            h = HAND_FIRST;
        return h;
    }
    /* the sniper's kick begun: the baseline taken, its distance drawn (OVL2 0x5B91C0, new in SOCOM
     * II: the name ours) */
    void StartSniperKick();
    void HandlePlaceMPBomb(const CPnt3D *pos);
    bool IsUsingScope();
    void SetWeaponInfoChanged();
    void PendingPickupHandled();
    void SetBody(CZSealBody *body);
    int GetNumItems() const;
    int GetSelectedItemNum();
    CZFTSWeapon *GetCurItem() const;
    CZFTSWeapon *GetItemByIndex(int index) const;
    bool HasItem(EQUIP_ITEM item) const;
    /* the first item of a kind with ammunition, else the last of the kind, or -1 (OVL2 0x5C8870;
     * SOCOM 1's took the first of the kind) */
    int GetItemIndex(EQUIP_ITEM item) const;
    /* the first item loaded with a kind of ammunition, a count of rounds in all (OVL2 0x5C8750;
     * SOCOM 1's) */
    int GetAmmoIndex(EQUIP_AMMO ammo, int count);
    bool WeaponInfoChanged();
    /* the hot-swap flag, set and read by CWeaponSel::CheckHotSwap (OVL2 0x5BC2A0, 0x5BC2B0: new in
     * SOCOM II, the names ours) */
    void SetHotSwap(bool b);
    /* an item of a kind dropped with its ammunition (OVL2 0x5B9E10, new in SOCOM II: the name ours) */
    void RemoveItem(EQUIP_ITEM item);
    bool GetHotSwap();
    /* const, where SOCOM 1's were not (ItemHasAmmo__5CZKitFi): SOCOM II's const GetCurItem
     * (SOCOM 1's const too) tail-calls GetItemByIndex, which calls unwrap, which calls this */
    bool ItemHasAmmo(int index) const;
    bool ItemHasAmmo(int index, FIREMODE mode) const;
    bool CurItemHasAmmo();
    bool ItemHasAmmo(EQUIP_ITEM item);
    bool IsFireDelayed();
    FIREMODE GetCurFireMode();
    int GetNumMags(int index);
    int GetCurNumMags();
    int GetAmmoTotal(int index);
    int unwrap(int index, FIREMODE mode) const;
    EQUIP_IDTYPE GetCurItemIDType();
    bool HasSniperRifle();
    bool HasItemWithAmmo(EQUIP_ITEM item);
    EQUIP_ITEM GetSecondaryEnum() const;
    EQUIP_ITEM GetPrimaryEnum() const;
    EQUIP_ITEM GetCurItemEnum() const;
    void *GetAmmoByIndex(int index);
    /* the first item of a fire mode's ammunition kind with rounds, else the last (OVL2 0x5C6530, new
     * in SOCOM II: the HUD's ammunition display asks it; the name ours) */
    int GetAmmoItemIndex(FIREMODE mode);
    /* the first primary weapon, pistol and class-145 item, or -1 (OVL2 0x5C7230, 0x5C71A0, 0x5C7110:
     * new in SOCOM II, the names ours) */
    int GetPrimaryIndex();
    int GetSecondaryIndex();
    int GetClass145Index();
    /* the binoculars added unless held (OVL2 0x5C7450; SOCOM 1's) */
    void SetupBinocular();
    /* the explosive's damage taken from the first explosive's ammunition (OVL2 0x5C77B0, new in
     * SOCOM II: PostSetupItems calls it; the name ours) */
    void SetupExplosiveDamage();
    void *GetCurAmmo();
    void NVG_Enabled(bool on);
    bool NVG_Enabled() const;
    /* the per-item pointers at 0x834: read, cleared, set (grown with nulls as needed) -- OVL2
     * 0x5B8FF0, 0x5B8FC0, 0x5B8F20 (the names ours) */
    void *GetItemPtr(int index);
    void ClearItemPtr(int index);
    void SetItemPtr(int index, void *p);
    /* an item let go with its ammunition: the slots closed up (0x5B9A00), the count made the last
     * item's place plus one, the selection reset when it fell off (OVL2 0x5B9BB0; SOCOM 1's
     * RemoveItemAndAmmo(int), which SOCOM II's closing up and WepWaitOn join) */
    void RemoveItemAndAmmo(int index);

    /* an index wrapped into [0, n), or -1 (SOCOM 1's unwrap__5CZKitFii, out of line there; SOCOM
     * II inlines it everywhere) */
    static int wrap(int index, int n)
    {
        /* the index copied only past the first test: a parameter wrapped in place is copied at the
         * inline's entry, ahead of the test (GetAmmoIndex, where the index lives on) */
        if (index == n || n == 0)
            return -1;
        int i = index;

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
    /* an item's magazines emptied, its ammunition let go, the count made the last item's place
     * plus one (inline: RemoveItemAndAmmo expands them -- each wraps its index again, as the
     * target's three wraps do; the names ours) */
    void ClearReloads(int index)
    {
        int i = wrap(index, m_num_items);

        for (int k = 0; k < 10; k++)
            m_reloads[i][k] = 0;
    }
    void ClearAmmo(int index)
    {
        int i = wrap(index, m_num_items);

        ClearReloads(i);
        m_ammo[i] = 0;
    }
    /* the count raised past an index (inline: SetItemByIndex expands it; the name ours) */
    void GrowTo(int index)
    {
        if (index >= m_num_items)
            m_num_items = index + 1;
    }
    void ResetZoom(CKitCtrl *ctrl);
    void UpdateNumItems()
    {
        int last = -1;

        for (int i = 0; i < 30; i++)
            if (m_item[i])
                last = i;
        m_num_items = last + 1;
    }
    /* the first item of that kind with ammunition, else the last of that kind, else -1 (inlined
     * in each caller; the name ours) */
    int FindItemWithAmmo(EQUIP_ITEM item) const
    {
        int found = -1;

        for (int i = 0; i < m_num_items; i++) {
            if (m_item[i] && item == m_item[i]->m_ID) {
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
    unsigned int m_bit16 : 1;   // new in SOCOM II (the names ours)
    unsigned int m_alt_firedelay : 1;   // bit 17: the fire delay the item's second wait (0x5C32B0)
    unsigned int m_second_hand : 1;   // bit 18: the hand's fire mode slot 1, not 0 (0x5C48B0)
    unsigned int m_unused : 13;
    bool m_hotswap;   // 0x4: new in SOCOM II, CWeaponSel::CheckHotSwap's (the name ours)
    unsigned char _5[0x28 - 0x5];
    float m_sniper_posx;   // 0x28 (SOCOM 1's 0x14)
    float m_sniper_posy;   // 0x2c (SOCOM 1's 0x18)
    unsigned char _30[0x38 - 0x30];
    float m_firerifle_kick_baseline;   // 0x38 (SOCOM 1's 0x24)
    float m_firerifle_kick_movedist;   // 0x3c (SOCOM 1's 0x28)
    float m_firerifle_kick_accum;   // 0x40 (SOCOM 1's 0x2c)
    int m_firerifle_kick_state;   // 0x44 (SOCOM 1's 0x30)
    unsigned char _48[0x54 - 0x48];
    RETICULE_TYPE m_curitemreticule;   // 0x54 (SOCOM 1's int at 0x40)
    unsigned char _55[0x58 - 0x55];
    float m_screenoffset_x;   // 0x58 (SOCOM 1's 0x44)
    float m_screenoffset_y;   // 0x5c (SOCOM 1's 0x48)
    unsigned char _60[0x6c - 0x60];
    unsigned int m_itemstate[30];   // 0x6c (SOCOM 1's 0x58): bit 3, the item in use
    CZFTSWeapon *m_item[30];   // 0xe4 (SOCOM 1's 0xd0)
    void *m_ammo[30];   // 0x15c (SOCOM 1's CZAmmo *, 0x148)
    int m_reloads[30][10];   // 0x1d4 (SOCOM 1's 0x1c0): each item's magazines, the rounds in each
    int m_currentmag[30];   // 0x684 (SOCOM 1's 0x670)
    int m_firemode[30];   // 0x6fc (SOCOM 1's 0x6e8): read as a FIREMODE, its low byte
    void *m_associatedgear[30];   // 0x774 (SOCOM 1's CCharacterGear *, 0x760): the item's gear
    int m_hand_firemode;   // 0x7ec: the mode SetHandFireMode was last given (new; the names ours)
    EQUIP_ITEM m_hand_item;   // 0x7f0: the hand slot's item kind
    unsigned char m_iteminuse[30];   // 0x7f1 (SOCOM 1's 0x7d8: 0x5B9A00 closes it up with the items)
    int m_cur_zoom;   // 0x810 (SOCOM 1's 0x7f8)
    unsigned char _814[0x820 - 0x814];
    float m_fire_delay;   // 0x820 (SOCOM 1's 0x804)
    int m_selected_item;   // 0x824 (SOCOM 1's 0x808)
    int m_last_selected;   // 0x828
    int m_num_items;   // 0x82c (SOCOM 1's 0x810)
    CZSealBody *m_body;   // 0x830 (SOCOM 1's 0x814)
    /* 0x834 (new in SOCOM II): a pointer per item, kept one place on (slot 0 for item -1) -- the
     * environment's pickup code (OVL2 0x59F0C0, 0x59F730) sets, reads and clears it through the
     * three accessors below 0x5B9020 (the names ours) */
    std::vector<void *> m_itemptr;
    unsigned char _840[0x8a0 - 0x840];
    float m_explosiveDamage;   // 0x8a0: the first explosive's ammunition's damage (new; the name ours)
    unsigned char _8a4[0x8b8 - 0x8a4];
    float m_pickupRequestWait;   // 0x8b8 (SOCOM 1's 0x878)
};

#endif
