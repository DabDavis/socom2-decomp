/* zentity.h -- the world's entities: CEntity, its controller CEntityCtrl, the target list and the
 * entity lists (SOCOM 1's DWARF files CEntity under gamez/zentity/zentity.h). The code is OVL1
 * 0x29E470-0x2A27DF in three units: ent_main.cpp and ent_target.cpp (SOCOM 1's __sinit names,
 * the statics they build) and the ticks and CEntityCtrl after them (ent_ctrl.cpp, the name ours).
 *
 * The layout is SOCOM 1's DWARF moved where SOCOM II's code puts it: a position of its own at
 * 0x1C (copied out of the node), the node at 0x28 and everything after it 0xC further, a word at
 * 0x60, a new pointer at 0xC4, one more flag bit, CSaferStr grown to 12 bytes -- 0x170 bytes in
 * all (CZSealBody's CBody base sits at 0x170). The virtual functions are SOCOM II's table,
 * __vt__7CEntity at 0x4061C0: SOCOM 1's with a slot after the destructor's, GetTargetPosW grown
 * to three and five more at the end. */
#ifndef ZENTITY_H
#define ZENTITY_H

#include <list>
#include <vector>

#include "../zmath/zmath.h"
#include "../znode/znode.h"

class CEntity;
class CEntityCtrl;
class CEntityPath;

/* a cell of an AI map (SOCOM 1's DWARF) */
class CAiMapLoc {
public:
    unsigned int m_mapid : 6;
    unsigned int x : 13;
    unsigned int y : 13;
};

/* a string, 12 bytes in SOCOM II (MSL 3.0's basic_string; SOCOM 1's was a pointer) */
class CSaferStr {
public:
    CSaferStr();
    ~CSaferStr();

    unsigned int m_rep[3];
};

/* what an entity knows of another (SOCOM 1's DWARF) */
class CTarget {
public:
    CEntity *m_ent;   // 0x0
    CPnt3D m_vec;   // 0x4
    float m_distSq;   // 0x10
    float m_dist;   // 0x14
    float m_visibility;   // 0x18
    float m_aware;   // 0x1c
    int m_diHandle;   // 0x20
    unsigned int m_d_computed : 1;   // 0x24
    unsigned int m_known : 1;
    unsigned int m_visible : 1;
    unsigned int m_hostile : 1;
    unsigned int m_targeted : 1;
    unsigned int m_dirty_di : 1;
    unsigned int m_unused : 26;
};

class CTargetList : public std::vector<CTarget> {
};

/* every entity, or a group of them (SOCOM 1's DWARF: a list of pointers and nothing more) */
class CEntityList : public std::list<CEntity *> {
public:
    CEntity *GetEntity(const char *name) const;
    CEntity *GetEntityById(unsigned int id) const;
    CEntity *GetEntityByIndex(unsigned int index) const;
};

/* what drives an entity: a player's pad, the AI, the network (SOCOM 1's DWARF; SOCOM II has four
 * throttles, its constructor clears 16 bytes) */
class CEntityCtrl {
public:
    CEntityCtrl(int);
    virtual ~CEntityCtrl();
    virtual bool PreTick(float dT);
    virtual bool Tick(float dT);
    virtual bool PostMortemPreTick(float dT);
    virtual void OnDeath();
    virtual CEntityCtrl *GetRemoteControl() { return 0; }
    virtual void SetThrottle(unsigned int axis, float value) { m_throttle[axis] = value; }

    CEntity *m_entity;   // 0x4
    float m_throttle[4];   // 0x8
};

class CEntity {
public:
    enum TYPE { ENT_TYPE_UNKNOWN = 0, ENT_TYPE_RECYCLE = 1, ENT_TYPE_SEAL = 2, ENT_TYPE_TURRET = 3 };

    CEntity(TYPE type, zdb::CNode *node);
    virtual ~CEntity();                                         /* +0x08 */
    /* +0x0C: inline, its out-of-line copy an earlier unit's (0x295F10, camera.cpp's); SOCOM 1's
     * slot was GetPosW (CTurret's override keeps that name). The name is ours */
    virtual zdb::CNode *GetPosNode() { return m_node; }
    /* +0x10: the position copied out of the node (the name is ours) */
    virtual void UpdatePosW();
    virtual void PreTick(float dT);
    virtual void Tick(float dT);
    virtual void PostPreTick(float dT);                         /* +0x1C */
    virtual void PrePostTick(float dT);
    virtual void PostTick(float dT);
    virtual void ComputeNextPosition(float dT);
    virtual void UpdateNextPosition();                          /* +0x2C */
    /* the point to aim at, and two more (CZSealBody's three overrides read three body parts:
     * SOCOM 1's spinehi, head and neck where its DWARF puts them, 0x7C on) */
    virtual CPnt3D *GetTargetPosW(CPnt3D *out);
    virtual CPnt3D *GetHeadPosW(CPnt3D *out);
    virtual CPnt3D *GetNeckPosW(CPnt3D *out);
    virtual void TeleportTo(const CPnt3D *pos, const CQuat *quat);   /* +0x3C */
    virtual void TeleportTo(const CMatrix *m);
    virtual void ResetToInitialState();
    virtual void UpdateAiMapLoc();                              /* +0x48 */
    virtual void OnMissionStart();
    virtual void OnMissionEnd();
    virtual void OnRecycleEntity(CEntity *e);
    virtual void OnDeath();                                     /* +0x58 */
    /* +0x5C-0x6C, new in SOCOM II: four that say no and one that updates the path's position (the
     * names are ours) */
    virtual int Virtual5C();
    virtual int Virtual60();
    virtual int Virtual64();
    virtual int Virtual68();
    virtual void UpdatePath();

    void SetDisplayName(const char *name);
    void JoinTeam(unsigned int team);
    void SetTeamMask(unsigned int mask);
    zdb::CNode *GetNode();
    void SetController(CEntityCtrl *ctrl);
    void SetIncludeInTargetList(bool on);
    void SetUpdateTargetList(bool on);
    CEntityPath *CreatePath(int type, const char *name);

    /* the entities, const: their walks use the const list's begin() and end() (CEntityItr::
     * Advance, the ticks). Inline, its out-of-line copy camera.cpp's (0x299140; SOCOM 1's
     * 0x1EA1C0 likewise) */
    static const CEntityList &GetEntityList() { return m_list; }
    static int GetEntityIndex(const CEntity *e);
    static bool PreTickAll(float dT, void *data);
    static bool TickAll(float dT, void *data);
    static bool PrePostTickAll(float dT, void *data);
    static bool PostTickAll(float dT, void *data);
    static void RemoveAll();
    static void DoRecycleEntity(CEntity *e);
    static CEntity *CreateRecycler();
    static bool RecycleEntity(CEntity *e);
    static void CreateEntity(CEntity *e, bool include);
    static void UpdateTargetList(bool b);
    static bool EntityIsValid(CEntity *e);
    static void CloseRecycler();
    static void OpenRecycler(unsigned int index);

    unsigned int m_id;   // 0x4
    unsigned int m_index;   // 0x8
    unsigned int m_vehicle_index;   // 0xc
    TYPE m_type;   // 0x10
    char *m_display_name;   // 0x14
    unsigned int m_UnitsSeenBy;   // 0x18
    CPnt3D m_posW;   // 0x1c: the node's position (the name is ours)
    zdb::CNode *m_node;   // 0x28
    CPnt3D m_velM;   // 0x2c
    CPnt3D m_velW;   // 0x38
    CPnt3D m_velR;   // 0x44
    CQuat m_quat;   // 0x50
    unsigned int m_unk60;   // 0x60
    CPnt3D m_next_velw;   // 0x64
    CQuat m_next_quat;   // 0x70
    CMatrix m_next_matrix;   // 0x80
    CEntityCtrl *m_control;   // 0xc0
    CEntityPath *m_path;   // 0xc4 (the name is ours)
    unsigned int m_team;   // 0xc8
    float m_max_target_range_sq;   // 0xcc
    CTargetList m_targets;   // 0xd0
    unsigned int m_aware_counter;   // 0xdc
    unsigned int m_blink_eyes : 1;   // 0xe0
    unsigned int m_drip_blood : 1;
    unsigned int m_animate_footsteps : 1;
    unsigned int m_interpolate_animations : 1;
    unsigned int m_lean_into_turns : 1;
    unsigned int m_do_weapon_recoil : 1;
    unsigned int m_check_player_collision : 1;
    unsigned int m_get_new_altitude : 1;
    unsigned int m_bit8 : 1;
    unsigned int m_noshoot : 1;
    unsigned int m_update_targetlist : 1;
    unsigned int m_include_in_targetlist : 1;
    unsigned int m_isAlive : 1;
    unsigned int m_bit13 : 1;
    CAiMapLoc m_mapLoc;   // 0xe4
    CAiMapLoc m_mapLocValid;   // 0xe8
    CSaferStr m_SOPList[10];   // 0xec

    /* the ids no entity has (SOCOM 1's; read from data, so defined out of the class) */
    static const unsigned int invalidId;
    static const unsigned int deletedId;

    static CEntityList m_list;
    static CEntityList m_recycle;
    static CEntityList m_target_inc;
    static CEntityList m_target_add;
    static CEntityList m_target_rem;
};

#endif
