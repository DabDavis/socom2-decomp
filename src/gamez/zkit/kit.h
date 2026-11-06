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
#include "../znode/node_world.h"
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
    /* 0x170: the CBody base (0x90 bytes: the parts FindPart searches, passed by its first byte) */
    unsigned char m_cbody[4];
    short m_state;   // 0x174: CBody's, the SEAL's state (zseal.h's: 9 stateCarrying)
    unsigned char _176[0x200 - 0x176];
    signed char m_200;   // 0x200: the current item's zoom class (binoculars 4, scopes 5 and up)
    unsigned char _201[0x300 - 0x201];
    /* 0x388: attachments by kind, 8 bytes each -- the first word a record (zseal.h's CZBodyPart)
     * whose +0xC keeps the node hung on it (0x5C6980) and +0x1C its parent part (0x5C6A40: the
     * launcher's node moved last among its parent's children; the names ours) */
    struct Attach {
        struct Record {
            unsigned char _0[0xc];
            void *m_node;   // 0xc
            unsigned char _10[0x1c - 0x10];
            Record *m_parent;   // 0x1c
        } *m_rec;
        unsigned int m_4;
    };
    void *m_rhand;   // 0x300: the right hand's body part (zseal.h's m_rhand)
    unsigned char _304[0x350 - 0x304];
    /* 0x350, 0x354: the rifle's and the pistol's parts (zseal.h's m_rifle, m_pistol) */
    Attach::Record *m_rifle;
    Attach::Record *m_pistol;
    unsigned char _358[0x388 - 0x358];
    Attach m_attach[5];

    /* an attachment's part (inline; the name ours: ReParentToRifle's launcher part read through it
     * is numbered with the temporaries, after the three walks -- as a plain field read into a named
     * local it is numbered with the named locals and the saved registers shift) */
    Attach::Record *AttachPart(int kind) { return m_attach[kind].m_rec; }
    unsigned char _3b0[0x3c0 - 0x3b0];
    /* 0x3c0: the hit under the reticule (zseal.h's m_last_reticule: 0x5BE6F0 copies it whole and
     * reads its node) */
    zdb::IntersectStruct m_last_reticule;
    unsigned char _3e0[0xeb8 - 0x3e0];
    /* 0xeb8: the entity the SEAL works -- a turret (zseal.h's m_eb8) */
    struct CKitMover *m_eb8;
    unsigned char _ebc[0xf79 - 0xebc];
    unsigned char m_f79;   // 0xf79: the item kind the body is asked for (ChangeItemModel's test; the name ours)
    unsigned char _f7a[0x105e - 0xf7a];
    unsigned char _105e : 5;   // 0x105e: zseal.h's flag word at 0x105c, its bits 16-23
    unsigned char m_airborne : 1;   // bit 21: off the ground
    unsigned char _105e_6 : 2;
    unsigned char _105f[0x1350 - 0x105f];
    CPnt3D m_1350;   // 0x1350: the velocity kept in the air (zseal.h's m_1350)
};

/* an entity's motion as the reticle reads it (CEntity's: its model-frame velocity at 0x2C, its
 * rotation's at 0x44 and the float at 0x60 -- zentity.h's m_unk60 -- read off the worked turret or
 * the SEAL itself) */
struct CKitMover {
    unsigned char _0[0x2c];
    CPnt3D m_velM;   // 0x2c
    CPnt3D m_velW;   // 0x38
    CPnt3D m_velR;   // 0x44
    unsigned char _50[0x60 - 0x50];
    float m_60;   // 0x60
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

/* the weapons' stash, an ammunition pickup's contents (zwep_weapon.cpp's CZItemStash, three
 * parallel lists out of line in OVL1: only the append the kit calls, 0x3CFEF0) */
class CZItemStash {
public:
    void Add(CZWeapon *weapon, CZAmmo *ammo, int rounds);
};

class CZKit {
public:
    /* (OVL2 0x5C8190; SOCOM 1's, by its place after ~CZKit and its caller, CZSealBody's constructor) */
    CZKit();
    /* (OVL2 0x5C8110; SOCOM 1's, the item pointers' vector freed -- the kit's round 52) */
    ~CZKit();
    void ResetScreenOffsetXY();
    /* the reticle's offsets zeroed (OVL2 0x5B9020, new in SOCOM II beside ResetScreenOffsetXY:
     * DecrZoomState calls it where SOCOM 1's called InitializeSniperKick -- the name ours) */
    void ResetRetOffsetXY();
    void Binocs_Enabled(bool on);
    /* the lens effects applied: on with a colour, the light maps tinted by it at once and the
     * camera's fog the base fog by it; off, the tints back to white over a quarter second and the
     * fog to the base (SOCOM 1's Apply_LensEffects; OVL2 0x5C1800 -- CameraZoom and the HUD's map
     * call it) and the bit set through a bool (inline: both arms keep the target's unfolded masks;
     * the name ours) */
    void Apply_LensEffects(bool on, CPnt3D *color);
    void SetLensFxEnable(bool b) { m_lensfx_enable = b; }
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
    /* the reticule node's settle countdown, m_8a4 (OVL2 0x5BE990, new in SOCOM II: CSealCtrl's
     * OnActionButton and PerformProximityActions and the HUD reticule's 0x215C10 let the explosive
     * (151) be placed only once it reads 0; the name ours) */
    float GetReticuleSettle();
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
    /* the item in hand in its launcher's mode (OVL2 0x5BD7E0, a tail call on m_selected_item as
     * SOCOM 1's CurWeaponIsM203Mode was its test's; the name ours) */
    bool CurWeaponIsLauncherMode();
    bool WeaponIsM203Mode(int index);
    /* the other two on the item in hand (OVL2 0x5BDA00, 0x5BD6D0: tail calls on m_selected_item --
     * SOCOM 1's CurWeaponIsM203Mode, and its grenade launcher twin, the name ours) */
    bool CurWeaponIsM203Mode();
    bool CurWeaponIsGrenadeLauncherMode();
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
    /* the scope's frame: the heartbeat's rumble paced by the adrenaline, the rifle's kick and its
     * return -- the baseline settling by a rate -- and the sight's drift within limits the
     * adrenaline scales (OVL2 0x5B9280; SOCOM 1's SniperTick, the settle rate and the two switches
     * new: CZSealBody::Tick calls it while a scope or the binoculars are up) */
    void SniperTick(float dT, CTargetMods &mods, float settle);
    /* the rounds a pull of the trigger fires, and the wait after (OVL2 0x5C0940 -- new in SOCOM II,
     * SOCOM 1's WillFireWeapon did it inline: the name ours -- and 0x5C09F0, SOCOM 1's) */
    int GetShotCount();
    float GetFireDelay();
    /* the wait before the next shot set, -1 for the item in hand's own: its second wait when asked,
     * else its first -- the weapons' shared record told (OVL2 0x5C32B0, new in SOCOM II: CSealCtrl's
     * PreTick and 0x5C5340 call it; the name ours) */
    void SetFireDelay(float delay, bool alt);
    /* the launcher's next round chambered and the reload run (OVL2 0x5C3000, new in SOCOM II; the
     * name ours) */
    void ChamberLauncherRound();
    /* the item in hand reloaded, false when the body cannot (SOCOM 1's; OVL2 0x5C2A90) */
    bool Reload();
    /* the hand's fire mode slot (0 or 1, by bit 18) set to a mode, the mode kept, and -- the body
     * alive -- the slot's item kind noted (OVL2 0x5C48B0, new in SOCOM II: CZSealBody::ReCreate
     * calls it; the name ours) */
    void SetHandFireMode(int mode);
    /* the item in hand's node hung under a part of the body -- hidden, made the part's child, kept in
     * the body's attachment of a kind (OVL2 0x5C6980, new in SOCOM II: 0x5C50B0 hangs it on
     * "aimnodes"; the name ours) */
    void AttachItemNode(int kind, const char *part);
    /* the item an index's ammunition is fired from: the item itself, or -- when it is a launcher's
     * grenades -- the first class-145 item (for that class's ammunition) or the first primary
     * weapon (OVL2 0x5C67D0, new in SOCOM II: the launcher-mode tests, GetReticuleByIndex and four
     * more call it; the name ours) */
    CZFTSWeapon *GetFiringItem(int index);
    /* the mode after the selected item's: from its launcher ammunition's item when the mode is one
     * (4 and up, the mode the item kind), through 0x5C3EE0 with the item it fires from (OVL2
     * 0x5C4480, new in SOCOM II: 0x5C4600 passes the selected item, which it reads again itself --
     * the index unused; the name ours) */
    int NextFireMode(int index);
    /* the selected item's fire mode stepped (OVL2 0x5C4600; SOCOM 1's) */
    void CycleFireMode();
    /* an explosive of a kind taken in hand and placed: the item before it kept to go back to, a
     * direct-fire weapon's selection undone, else -- the body able, the wait run out, the placing
     * allowed (0x5BD530) -- placed (0x5BE9A0), or the item before it taken back (OVL2 0x5C0020;
     * SOCOM 1's HandlePlacingExlosive, sic, which placed it inline) */
    void HandlePlacingExlosive(EQUIP_ITEM item, float a, float b);
    /* the laser target the SEAL's aim point is near: the first of the AI maps' laser names whose cell
     * is within range of him and whose anim of that name is ready (SOCOM 1's; OVL2 0x5BFE60) */
    bool FetchLaseTarget();
    /* the laser designator held on its target while the trigger is down: timed, its lock sound
     * playing for a player, the target's anim set off when the time is up; dropped when the aim
     * leaves the target's cell or the trigger comes up (SOCOM 1's; OVL2 0x5BFAC0) */
    void TickLaserDesignator(float dT);
    /* the lase dropped: no target, no anim, its timer cleared (inline: TickLaserDesignator's five
     * drops; the name ours) */
    void ClearLase()
    {
        m_lase_mapname = 0;
        m_lase_anim = 0;
        m_lase_timer = 0.0f;
        m_lase_echo = false;
        CZWeapon::ClearTimer(&m_lase_timer);
    }
    /* an AI SEAL's lob: the aim raised step by step until a round at the muzzle's speed comes down
     * at the target's height (OVL2 0x5BF8A0, new in SOCOM II; the name ours) */
    void LobAim(CZSealBody *body, const CPnt3D *from, const CPnt3D *to, CPnt3D *dir, float speed);
    /* the SEAL's remote charges in range set off, the detonator taken up (SOCOM 1's; OVL2 0x5C0130) */
    void DetonateRemoteExplosives();
    /* the item of a kind taken in hand (OVL2 0x5C8A20: SOCOM 1's, unwritten) and the item at an
     * index (OVL2 0x5C4B10: SOCOM 1's, unwritten) */
    bool SetCurItemEnum(EQUIP_ITEM item);
    void SetCurItemByIndex(int index);
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
    /* the MP bomb picked up (SOCOM 1's; OVL2 0x5BBF10) */
    void PickupMPBomb();
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
    /* rounds in the current magazine (SOCOM 1's; OVL2 0x5BD530). An unsigned char, not a bool (the
     * return type is not mangled): its launcher arm's int result is narrowed with the target's
     * `andi 0xff` at the join; every caller branches on v0 as on a bool */
    unsigned char CurMagHasAmmo();
    bool ItemHasAmmo(EQUIP_ITEM item);
    /* whether the weapon selector may pick an item (SOCOM 1's; OVL2 0x5BDC30) */
    bool isSelectableItem(int index);
    bool IsFireDelayed();
    /* the reticle's offsets returned to the centre and its size eased toward a goal the weapon's
     * target mods and the SEAL's motion set -- a player's only (SOCOM 1's TickReticule; OVL2
     * 0x5C2670: SOCOM II's explosives, grenades, binoculars and item class 201 hold the bounds at
     * zero, and the dilation reads the worked turret's motion) */
    void TickReticule(float dT);
    /* the ground under the right hand: the world's altitude hits below the hand's place, the highest
     * one at or under it, else the lowest -- volumetric materials skipped, liquid ones unless `liquid`
     * (OVL2 0x5C2430, new in SOCOM II: the explosives' tick 0x5C1970 asks it; the name ours) */
    zdb::IntersectStruct *GetHandFloorHit(bool liquid);
    /* a shot's kick: the rumbles, the reticle knocked up (by the wiggle on the burst's reported shot,
     * plainly past it), the knock bounded by the screen's aspect -- its vertical scale kept in
     * recoil_constscl --, the reticle dilated by the shot and the burst, and the body's recoil
     * counter raised (SOCOM 1's Recoil; OVL2 0x5C3360: Fire 0x5C5340 calls it) */
    void Recoil(CZFTSWeapon *w);
    FIREMODE GetCurFireMode();
    int GetNumMags(int index);
    int GetCurNumMags();
    int GetAmmoTotal(int index);
    /* the rounds an item's magazine holds (SOCOM 1's; OVL2 0x5C3CF0) */
    int GetMaxAmmoCount(int index);
    /* the rounds in an item's current magazine (SOCOM 1's; OVL2 0x5C38A0) */
    int GetAmmoCount(int index);
    /* the selected item's magazine capacity and rounds (OVL2 0x5C3CE0, 0x5C3890: SOCOM 1's
     * GetCurMaxAmmoCount and GetCurAmmoCount, which did the work inline -- SOCOM II's tail-call the
     * indexed pair with the selected item; CAmmoDisplay::Update calls both) */
    int GetCurMaxAmmoCount();
    int GetCurAmmoCount();
    int unwrap(int index, FIREMODE mode) const;
    EQUIP_IDTYPE GetCurItemIDType();
    /* the current item's class as the body keeps its fire points (a byte: CZSealBody's 0x5DFAD0
     * indexes its eight-byte slots at 0x388 with it, masked -- OVL2 0x5B8760; the names ours) */
    enum FIRE_CLASS { FIRE_CLASS_NONE = 0, FIRE_CLASS_RIFLE = 1, FIRE_CLASS_PISTOL = 2,
                      FIRE_CLASS_THROWN = 3, FIRE_CLASS_145 = 4, FIRE_CLASS_LAST = 255 };
    FIRE_CLASS GetCurItemFireClass();
    /* the body's item model changed: at once, to the rifle's or the pistol's by the first item's fire
     * class, the gear of the matching attachment and the fifth set; else, by kind, an item asked
     * for or the node hung on "aimnodes" (OVL2 0x5C50B0: SOCOM 1's ChangeItemModel(int, int, int,
     * bool) -- the callers narrow their byte arguments, the switch takes the int unmasked) */
    void ChangeItemModel(int item, int kind, int x, bool now);
    /* the frame's kit work: the first frame's set-up, or an asked-for item handed to the body (OVL2
     * 0x5C1660: SOCOM 1's PreTick, CZSealBody::WeaponsTick calls it last) */
    void PreTick(float dT);
    /* an item's fire class (inline: GetFireClassIndex's -- GetCurItemFireClass's tests, the class
     * zeroed after the call, as the target schedules it; the name ours) and the first item of one, or -1 (OVL2 0x5B8630, new in SOCOM II: the name ours) */
    static FIRE_CLASS FireClassOf(CZFTSWeapon *w)
    {
        EQUIP_ITEM t = CZWeapon::GetIDType(w->m_ID);
        FIRE_CLASS c = FIRE_CLASS_NONE;

        if (CZWeapon::IsPrimaryType(t))
            c = FIRE_CLASS_RIFLE;
        else if (CZWeapon::IsSecondaryType(t))
            c = FIRE_CLASS_PISTOL;
        else if (CZWeapon::IsLauncherType(t))
            c = FIRE_CLASS_145;
        else if (t == IDTYPE_GRENADE || t == IDTYPE_EXPLOSIVE)
            c = FIRE_CLASS_THROWN;
        return c;
    }
    int GetFireClassIndex(FIRE_CLASS c);
    /* whether a slot's item uses ammunition: held, and none of the ten kinds without (inline:
     * GetPickupSlot's; the name ours) and the slot a picked-up item takes (OVL2 0x5BAE60, new in
     * SOCOM II: the name ours) */
    bool UsesAmmo(int i)
    {
        CZWeapon *w = m_item[i];

        if (!w)
            return false;
        switch (w->m_ID) {
        case 190:
        case 193:
        case 11:
        case 146:
        case 141:
        case 145:
        case 194:
        case 195:
        case 254:
        case 255:
            return false;
        }
        return true;
    }
    int GetPickupSlot(int skip);
    /* the first launcher held, or -1 (inline: HasPickupSlot's; the name ours) and whether a picked-up
     * item has a slot (OVL2 0x5BAC30, new in SOCOM II: the name ours) */
    int GetLauncherIndex()
    {
        for (int i = 0; i < m_num_items; i++)
            if (m_item[i] && m_item[i]->IsLauncherWeapon())
                return i;
        return -1;
    }
    bool HasPickupSlot();
    /* the fire point's "projectile" child shown, for items 186 and 146 (OVL2 0x5B85A0; the name ours) */
    void ShowProjectile();
    /* the sniper rifle's scope or thermal scope node shown, by item 195 (OVL2 0x5B82E0, new in SOCOM
     * II; the name ours) */
    void UpdateScopeModel();
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
    /* the three weapons' models hung back on the body's rifle, pistol and launcher parts (OVL2
     * 0x5C6A40, 0x5C6DA0: SOCOM 1's ReParentToRifle / ReParentToPistol, which hung items 0's and
     * 1's; SOCOM II's ChangeToRifleMode / ChangeToPistolMode call them first, as SOCOM 1's) */
    void ReParentToRifle();
    void ReParentToPistol();
    /* the binoculars added unless held (OVL2 0x5C7450; SOCOM 1's) */
    void SetupBinocular();
    /* an item's model instanced and its node kept in m_itemptr (OVL2 0x5C72C0; SOCOM 1's) */
    bool InitItemModel(const char *model, const char *name);
    /* the explosive's damage taken from the first explosive's ammunition (OVL2 0x5C77B0, new in
     * SOCOM II: PostSetupItems calls it; the name ours) */
    void SetupExplosiveDamage();
    /* with item 194 held, each rifle's magazines doubled (SOCOM 1's; OVL2 0x5C75F0) */
    void Setup2XAmmo();
    /* the sounds found, the flashlight's light map made and the aim, kick, heartbeat and lase state
     * cleared (SOCOM 1's; OVL2 0x5C7E00) */
    void Init();
    /* the items' nodes deleted, each that has no parent, the pointers cleared and the list emptied,
     * the light map deleted (SOCOM 1's; OVL2 0x5C7CE0) */
    void UnInit();
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
    /* the items closed up over the empty slots (OVL2 0x5B9A00, new in SOCOM II: the name ours), and
     * whether a slot from an index on holds one (inline, its test materialised: the names ours) */
    void CompactItems();
    bool SlotEmpty(int i) { return !m_item[i]; }
    bool AnyItemFrom(int i)
    {
        for (int j = i; j < m_num_items; j++)
            if (!SlotEmpty(j))
                return true;
        return false;
    }
    /* an item's ammunition let go (OVL2 0x5B9F00; SOCOM 1's) and whether an item has room for more
     * rounds (OVL2 0x5BA5B0; SOCOM 1's, the count unread) */
    void RemoveAmmo(int index);
    /* the kit's ammunition put into a stash -- every item's (-1) or one index's (OVL2 0x5BA040;
     * SOCOM 1's FillAmmoPickupInstance: the SEAL's DropAmmo calls it) */
    void FillAmmoPickupInstance(CZItemStash *stash, int index);
    bool CanAddAmmo(int index, int count);
    /* rounds added to an item's magazines, the rest returned (OVL2 0x5BA3D0; SOCOM 1's) */
    int AddAmmo(int index, int rounds);

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
    unsigned char _5[0x8 - 0x5];
    int m_8;   // 0x8: zeroed by Init (new in SOCOM II; the name ours)
    unsigned char _c[0x18 - 0xc];
    float m_18;   // 0x18: zeroed by the constructor (new in SOCOM II; the names ours)
    float m_1c;   // 0x1c
    float m_retoffsetx;   // 0x20 (SOCOM 1's 0xc: its TickReticule's)
    float m_retoffsety;   // 0x24 (SOCOM 1's 0x10)
    float m_sniper_posx;   // 0x28 (SOCOM 1's 0x14)
    float m_sniper_posy;   // 0x2c (SOCOM 1's 0x18)
    float m_sniper_prev_posx;   // 0x30 (SOCOM 1's 0x1c: both builds' Init zero it twice)
    float m_sniper_prev_posy;   // 0x34 (SOCOM 1's 0x20)
    float m_firerifle_kick_baseline;   // 0x38 (SOCOM 1's 0x24)
    float m_firerifle_kick_movedist;   // 0x3c (SOCOM 1's 0x28)
    float m_firerifle_kick_accum;   // 0x40 (SOCOM 1's 0x2c)
    int m_firerifle_kick_state;   // 0x44 (SOCOM 1's 0x30)
    unsigned int m_heartbeat_state;   // 0x48 (SOCOM 1's 0x34)
    float m_heartbeat_accum;   // 0x4c (SOCOM 1's 0x38)
    float m_chill_parabola;   // 0x50 (SOCOM 1's 0x3c)
    RETICULE_TYPE m_curitemreticule;   // 0x54 (SOCOM 1's int at 0x40)
    unsigned char _55[0x58 - 0x55];
    float m_screenoffset_x;   // 0x58 (SOCOM 1's 0x44)
    float m_screenoffset_y;   // 0x5c (SOCOM 1's 0x48)
    float m_lase_timer;   // 0x60 (SOCOM 1's 0x4c)
    class CZAnim *m_lase_anim;   // 0x64 (SOCOM 1's 0x50)
    struct CKitMapName *m_lase_mapname;   // 0x68 (SOCOM 1's CAiMap::name *, 0x54)
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
    int m_restore_zoom;   // 0x814 (SOCOM 1's 0x7fc)
    int m_burst_count;   // 0x818 (SOCOM 1's 0x800: DetonateRemoteExplosives counts up)
    float m_81c;   // 0x81c: a wait the explosive's placing needs run out (SOCOM 1's tested its
                   // m_fire_delay there; the name ours)
    float m_fire_delay;   // 0x820 (SOCOM 1's 0x804)
    int m_selected_item;   // 0x824 (SOCOM 1's 0x808)
    int m_last_selected;   // 0x828
    int m_num_items;   // 0x82c (SOCOM 1's 0x810)
    CZSealBody *m_body;   // 0x830 (SOCOM 1's 0x814)
    /* 0x834 (new in SOCOM II): a pointer per item, kept one place on (slot 0 for item -1) -- the
     * environment's pickup code (OVL2 0x59F0C0, 0x59F730) sets, reads and clears it through the
     * three accessors below 0x5B9020 (the names ours) */
    std::vector<void *> m_itemptr;
    void *m_detonator_model;   // 0x840 (SOCOM 1's CNode *, 0x824)
    void *m_flashlight_model;   // 0x844 (SOCOM 1's CNode *, 0x828)
    void *m_detonator_icon;   // 0x848 (SOCOM 1's CTexture *, 0x82c)
    float m_retsize;   // 0x84c (SOCOM 1's 0x830): the reticle's size
    float m_prev_retsize;   // 0x850 (SOCOM 1's 0x834)
    float m_prev_factor;   // 0x854 (SOCOM 1's 0x838)
    float m_prev_axis_a;   // 0x858 (SOCOM 1's 0x83c)
    float m_prev_axis_b;   // 0x85c (SOCOM 1's 0x840)
    float m_retblockedsize;   // 0x860 (SOCOM 1's 0x844)
    float m_retblockedX;   // 0x864 (SOCOM 1's 0x848)
    float m_retblockedY;   // 0x868 (SOCOM 1's 0x84c)
    float m_retsizegoal;   // 0x86c (SOCOM 1's 0x850)
    float m_retmax;   // 0x870 (SOCOM 1's 0x854)
    float m_retmin;   // 0x874 (SOCOM 1's 0x858)
    int m_tracercount;   // 0x878 (SOCOM 1's 0x85c)
    float m_firetime;   // 0x87c (SOCOM 1's 0x860)
    unsigned char _880[0x884 - 0x880];
    zdb::CLightMap *m_lightMap;   // 0x884 (SOCOM 1's 0x864)
    CSnd *m_rldsnd;   // 0x888 (SOCOM 1's 0x868)
    CSnd *m_clicksnd;   // 0x88c (SOCOM 1's 0x86c)
    CSnd *m_lasesnd_start;   // 0x890 (SOCOM 1's 0x870)
    CSnd *m_lasesnd_fin;   // 0x894 (SOCOM 1's 0x874)
    /* the laser designator's lock sound playing and a second instance it stops (new in SOCOM II:
     * TickLaserDesignator; the names ours) */
    class CSndInstance *m_lase_inst;   // 0x898
    class CSndInstance *m_lase_inst2;   // 0x89c
    float m_explosiveDamage;   // 0x8a0: the first explosive's ammunition's damage (new; the name ours)
    /* 0x8a4-0x8b4, 0x8bc: new in SOCOM II, set by Init -- 0.2, zero, CPnt3D::zero, -1 (the names
     * ours) */
    float m_8a4;   // 0x8a4: 0x5BE6F0's countdown -- 0.2 while the aimed-at node moves
    zdb::CNode *m_8a8;   // 0x8a8: the node under the reticule last frame (0x5BE6F0)
    CPnt3D m_8ac;   // 0x8ac: its place in the world then
    float m_pickupRequestWait;   // 0x8b8 (SOCOM 1's 0x878)
    float m_8bc;   // 0x8bc
};

#endif
