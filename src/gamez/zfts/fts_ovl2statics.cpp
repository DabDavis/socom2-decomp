/* fts_ovl2statics.cpp -- the statics of OVL2's classes, defined in OVL1 (0x2CD670-0x2CE7FF, between
 * fts_dnas.cpp and game_main.cpp; the file name ours). ZSealEtc is loaded and dropped around the DNAS
 * authentication (fts_dnas.cpp loads DNAS into its slot), so what must outlive it -- the SEALs', the
 * anims', the pickups', the AI's lists and tables, the character dynamics -- lives here in FTSCore,
 * built by one initialiser: __sinit_fts_ovl2statics.cpp (0x400870, 0x134C bytes, the image's
 * largest). The unit has no functions of its own: its code is the initialiser's out-of-line copies
 * and their template tail -- the destructors it registers, in reverse of the statics' order, and
 * the constructors and vector pieces the character dynamics' constructor calls.
 *
 * Every static is SOCOM 1's (its seal_cfg.cpp, seal.cpp, seal_dyn.cpp, anim.cpp, seal_env.cpp,
 * char_type.cpp, pickup_main.cpp, seal_unit.cpp, seal_sensors.cpp, seal_aimoves.cpp -- the names
 * its symbols give) but the few SOCOM II added (the names ours), and each class is only what the
 * initialiser and the destructors need: the users are OVL2's. Defined in the initialiser's order. */

#include <list>
#include <map>
#include <new>
#include <string>
#include <vector>
#include "../zmath/zmath.h"

void *_malloc(unsigned int size, const char *file, int line);
void _free(void *p, const char *file, int line);

namespace zdb {
class CModel;

/* a collision query's result buffer (int_main.cpp's: built for n hits, 0x4C bytes here) */
class DiIntersect {
public:
    DiIntersect(int n);   /* 0x2DC6E0 */
    ~DiIntersect();       /* 0x2DC680 */

    unsigned char _0[0x4c];
};

/* the models by name (znode.h's) */
class CModelVector : public std::vector<CModel *> {
};
}

/* a string table (util_stable.h's: only its constructor and destructor, both util_stable.cpp's) */
class CSTable {
public:
    CSTable(unsigned int bytes, unsigned int strings);   /* 0x39FBD0 */
    ~CSTable();                                          /* 0x39FB30 */

    unsigned char _0[0x1c];
};

/* an animation's kind (zseal.h's: a 2-byte enum) */
enum AnimType { animTypeUndefined = -1, animTypeRef = 0, animTypeLast = 142 };

/* SOCOM II's vector<AnimType> runs on the 2-byte pod (include/vector's __vector_pod<unsigned short>,
 * seal_env.cpp's CAnimTypeVector stand-in): three zeroed words built, three this-tests and the data
 * deleted (0x2CDE70 and 0x2CDE00, the death and damage tables' element constructor and destructor) */
namespace std {
template <class Allocator>
class __vector_imp<AnimType, Allocator, true>
    : public __vector_pod<unsigned short, allocator<unsigned short> > {
};

template <class Allocator>
class vector<AnimType, Allocator> : public __vector_imp<AnimType, Allocator, true> {
};
}

/* the body parts' names to their indices (SOCOM 1's seal_cfg.cpp's bodyPartNames_map; read by
 * OVL2's 0x58B1A0, seal_cfg.cpp's) */
std::map<std::string, int> bodyPartNames_map;

/* the SEALs' line-of-sight queries (SOCOM 1's seal.cpp's dilr00-dilr49, sixteen hits each) */
namespace {
zdb::DiIntersect dilr00(16);
zdb::DiIntersect dilr01(16);
zdb::DiIntersect dilr02(16);
zdb::DiIntersect dilr03(16);
zdb::DiIntersect dilr04(16);
zdb::DiIntersect dilr05(16);
zdb::DiIntersect dilr06(16);
zdb::DiIntersect dilr07(16);
zdb::DiIntersect dilr08(16);
zdb::DiIntersect dilr09(16);
zdb::DiIntersect dilr10(16);
zdb::DiIntersect dilr11(16);
zdb::DiIntersect dilr12(16);
zdb::DiIntersect dilr13(16);
zdb::DiIntersect dilr14(16);
zdb::DiIntersect dilr15(16);
zdb::DiIntersect dilr16(16);
zdb::DiIntersect dilr17(16);
zdb::DiIntersect dilr18(16);
zdb::DiIntersect dilr19(16);
zdb::DiIntersect dilr20(16);
zdb::DiIntersect dilr21(16);
zdb::DiIntersect dilr22(16);
zdb::DiIntersect dilr23(16);
zdb::DiIntersect dilr24(16);
zdb::DiIntersect dilr25(16);
zdb::DiIntersect dilr26(16);
zdb::DiIntersect dilr27(16);
zdb::DiIntersect dilr28(16);
zdb::DiIntersect dilr29(16);
zdb::DiIntersect dilr30(16);
zdb::DiIntersect dilr31(16);
zdb::DiIntersect dilr32(16);
zdb::DiIntersect dilr33(16);
zdb::DiIntersect dilr34(16);
zdb::DiIntersect dilr35(16);
zdb::DiIntersect dilr36(16);
zdb::DiIntersect dilr37(16);
zdb::DiIntersect dilr38(16);
zdb::DiIntersect dilr39(16);
zdb::DiIntersect dilr40(16);
zdb::DiIntersect dilr41(16);
zdb::DiIntersect dilr42(16);
zdb::DiIntersect dilr43(16);
zdb::DiIntersect dilr44(16);
zdb::DiIntersect dilr45(16);
zdb::DiIntersect dilr46(16);
zdb::DiIntersect dilr47(16);
zdb::DiIntersect dilr48(16);
zdb::DiIntersect dilr49(16);
}

/* the character's dynamics (zseal.h's CharacterDynamics, SOCOM 1's DWARF layout, 0x1BC bytes),
 * with SOCOM II's defaults: the constructor sets them member by member before the members of class
 * type are built (SOCOM 1's inlined all of it; SOCOM II's camera wiggle and throw limits have
 * constructors of their own, called out of line -- the whole constructor passes inline_max_size, so
 * the initialiser inlines it one level), the camera list reserved for five */
class CharacterDynamics {
public:
    /* a camera's place behind the SEAL and the point it looks at (SOCOM 1's DWARF, 0x18 bytes) */
    class CCameraParams {
    public:
        CPnt3D m_cam_offset;   // 0x0
        CPnt3D m_cam_aimpoint;   // 0xc
    };

    /* the camera's wiggle (0x2CE1E0) */
    struct cam_wiggle {
        cam_wiggle() : amplitude(22.0f), duration(0.6f), rate(0.1f) {}

        float amplitude;   // 0x0
        float duration;   // 0x4
        float rate;   // 0x8
    };
    /* the throw's limits (0x2CE1A0) */
    struct throw_params {
        throw_params()
            : m_abort_threshold(0.15f), max_distance_stand(600.0f), max_distance_crouch(300.0f),
              toss_power_threshold(0.6f), toss_aim_threshold(0.3f)
        {
        }

        float m_abort_threshold;   // 0x0
        float max_distance_stand;   // 0x4
        float max_distance_crouch;   // 0x8
        float toss_power_threshold;   // 0xc
        float toss_aim_threshold;   // 0x10
    };

    CharacterDynamics()
        : m_gravity(98.1f), m_DamageToNewtons(120.0f), m_jumpFactor(1.0f), m_landFallRate(0.0f),
          m_landHardFallRate(90.0f), m_groundTouchDistance(3.0f), m_maxSlopeCos(0.64278f),
          m_stepHeight(4.0f), m_blastBoostY(1.0f), m_standTurnFactor(1.0f), m_turn_maxRate(2.0f),
          m_accelXMin(2.0f), m_accelXMax(5.0f), m_accelZMin(2.0f), m_accelZMax(5.0f),
          m_initAimPitch(-0.15999433f), m_maxAimPitch(1.4835298f), m_minAimPitch(-1.4835298f),
          m_maxAimYaw(1.4835298f), m_proneMaxAimPitch(0.34906584f), m_proneMinAimPitch(-0.34906584f),
          m_proneMaxAimYaw(0.78539819f), m_proneZoomDeltaYaw(0.34906584f), m_maxLookPitch(1.5533431f),
          m_minLookPitch(-1.5533431f), m_maxLookYaw(1.5533431f), m_maxEyelookPitch(0.13962634f),
          m_minEyelookPitch(-0.24434610f), m_maxEyelookYaw(0.29670596f), m_maxBlinkRate(5.2359877f),
          m_maxBlinkAngle(-0.59341192f), m_maxZoomAimPitch(1.4835298f), m_minZoomAimPitch(-1.4835298f),
          m_maxZoomAimYaw(1.4835298f), m_proneMaxZoomAimPitch(0.34906584f),
          m_proneMinZoomAimPitch(-0.34906584f), m_proneMaxZoomAimYaw(0.78539819f), m_pitchRate(0.6f),
          m_turnThrottleA(0.75f), m_turnThrottleB(0.25f), m_pitchThrottleA(0.75f), m_pitchThrottleB(0.25f),
          m_minWaterFactor(0.6f), m_waterFactorSlope(0.066f), m_fbStiffness(0.9f), m_lrStiffness(0.9f),
          m_throtExp(1.0f), m_cameraRoll(0.0001f), m_zoomFactor(5500.0f), m_zoomRate(14000.0f),
          m_cameraSideHeight(-3.0f), m_cameraSideDist(10.0f), m_cameraSidePitch(0.0f),
          m_cameraFrontHeight(-4.0f), m_cameraFrontDist(0.0f), m_cameraFrontPitch(0.0f),
          m_cameraBackHeight(-0.25f), m_cameraBackDist(-5.0f), m_cameraBackPitch(-0.1f),
          m_cameraDownHeight(-10.0f), m_cameraDownDist(10.0f), m_cameraDownPitch(0.5f),
          m_cameraTetherStiffness(0.4f), m_cameraLookDwell(1.0f), m_cameraNetPosSmooth(0.8f),
          m_cameraReleaseRad(1.1f), m_cameraPeekDecayRate(6.0f), m_running_reload_speed(20.0f),
          m_running_chgweapon_speed(20.0f), m_lowHeight(13.0f), m_medHeight(21.5f), m_highHeight(26.5f),
          m_minStandHeight(10.0f), m_minJumpHeight(20.0f), m_loaded(0)
    {
        m_cam_params.reserve(5);
    }

    float m_gravity;   // 0x0
    float m_DamageToNewtons;   // 0x4
    float m_jumpFactor;   // 0x8
    float m_landFallRate;   // 0xc
    float m_landHardFallRate;   // 0x10
    float m_groundTouchDistance;   // 0x14
    float m_maxSlopeCos;   // 0x18
    float m_stepHeight;   // 0x1c
    float m_blastBoostY;   // 0x20
    float m_fallDist[3];   // 0x24
    float m_landSpeed[3];   // 0x30
    float m_standTurnFactor;   // 0x3c
    float m_turn_maxRate;   // 0x40
    float m_accelXMin;   // 0x44
    float m_accelXMax;   // 0x48
    float m_accelZMin;   // 0x4c
    float m_accelZMax;   // 0x50
    float m_initAimPitch;   // 0x54
    float m_maxAimPitch;   // 0x58
    float m_minAimPitch;   // 0x5c
    float m_maxAimYaw;   // 0x60
    float m_proneMaxAimPitch;   // 0x64
    float m_proneMinAimPitch;   // 0x68
    float m_proneMaxAimYaw;   // 0x6c
    float m_proneZoomDeltaYaw;   // 0x70
    float m_maxLookPitch;   // 0x74
    float m_minLookPitch;   // 0x78
    float m_maxLookYaw;   // 0x7c
    float m_maxEyelookPitch;   // 0x80
    float m_minEyelookPitch;   // 0x84
    float m_maxEyelookYaw;   // 0x88
    float m_maxBlinkRate;   // 0x8c
    float m_maxBlinkAngle;   // 0x90
    float m_cosMaxAimPitch;   // 0x94
    float m_cosMinAimPitch;   // 0x98
    float m_cosMaxAimYaw;   // 0x9c
    float m_cosProneMaxAimPitch;   // 0xa0
    float m_cosProneMinAimPitch;   // 0xa4
    float m_cosProneMaxAimYaw;   // 0xa8
    float m_cosMaxLookPitch;   // 0xac
    float m_cosMinLookPitch;   // 0xb0
    float m_cosMaxLookYaw;   // 0xb4
    float m_cosMaxEyelookPitch;   // 0xb8
    float m_cosMinEyelookPitch;   // 0xbc
    float m_cosMaxEyelookYaw;   // 0xc0
    float m_maxZoomAimPitch;   // 0xc4
    float m_minZoomAimPitch;   // 0xc8
    float m_maxZoomAimYaw;   // 0xcc
    float m_proneMaxZoomAimPitch;   // 0xd0
    float m_proneMinZoomAimPitch;   // 0xd4
    float m_proneMaxZoomAimYaw;   // 0xd8
    float m_cosMaxZoomAimPitch;   // 0xdc
    float m_cosMinZoomAimPitch;   // 0xe0
    float m_cosMaxZoomAimYaw;   // 0xe4
    float m_cosProneMaxZoomAimPitch;   // 0xe8
    float m_cosProneMinZoomAimPitch;   // 0xec
    float m_cosProneMaxZoomAimYaw;   // 0xf0
    float m_pitchRate;   // 0xf4
    float m_turnThrottleA;   // 0xf8
    float m_turnThrottleB;   // 0xfc
    float m_pitchThrottleA;   // 0x100
    float m_pitchThrottleB;   // 0x104
    float m_minWaterFactor;   // 0x108
    float m_waterFactorSlope;   // 0x10c
    float m_fbStiffness;   // 0x110
    float m_lrStiffness;   // 0x114
    float m_throtExp;   // 0x118
    float m_throtScale;   // 0x11c
    float m_cameraRoll;   // 0x120
    float m_zoomFactor;   // 0x124
    float m_zoomRate;   // 0x128
    float m_cameraSideHeight;   // 0x12c
    float m_cameraSideDist;   // 0x130
    float m_cameraSidePitch;   // 0x134
    float m_cameraFrontHeight;   // 0x138
    float m_cameraFrontDist;   // 0x13c
    float m_cameraFrontPitch;   // 0x140
    float m_cameraBackHeight;   // 0x144
    float m_cameraBackDist;   // 0x148
    float m_cameraBackPitch;   // 0x14c
    float m_cameraDownHeight;   // 0x150
    float m_cameraDownDist;   // 0x154
    float m_cameraDownPitch;   // 0x158
    float m_cameraTetherStiffness;   // 0x15c
    float m_cameraLookDwell;   // 0x160
    float m_cameraNetPosSmooth;   // 0x164
    float m_cameraReleaseRad;   // 0x168
    float m_cameraPeekDecayRate;   // 0x16c
    float m_running_reload_speed;   // 0x170
    float m_running_chgweapon_speed;   // 0x174
    float m_lowHeight;   // 0x178
    float m_medHeight;   // 0x17c
    float m_highHeight;   // 0x180
    float m_minStandHeight;   // 0x184
    float m_minJumpHeight;   // 0x188
    cam_wiggle m_cam_wiggle;   // 0x18c
    throw_params m_throw_params;   // 0x198
    unsigned char m_loaded;   // 0x1ac
    std::vector<CCameraParams> m_cam_params;   // 0x1b0
};   // 0x1bc bytes

CharacterDynamics theCharacterDynamics;

/* the anim names' maps (SOCOM 1's anim.cpp had newNamesMap and animNamesMap; SOCOM II's anim.cpp,
 * OVL2's 0x5E30F0 and 0x5E3FF0, reads three -- the names ours) */
static std::map<std::string, int> s_animNamesMap0;
static std::map<std::string, int> s_animNamesMap1;
static std::map<std::string, int> s_animNamesMap2;

/* the look anims' rotations and positions per frame (SOCOM 1's seal_env.cpp: CZSealBody's
 * InitLookAnims and ApplyLookAnim read them) */
CQuat dQlookhead[12];
CQuat dQlookneck[12];
CQuat dQlookspinehi[12];
CQuat dQlookspinelo[12];
CPnt3D Rlookhead[12];
CPnt3D Rlookneck[12];
CPnt3D Rlookspinehi[12];
CPnt3D Rlookspinelo[12];

/* the multiplayer messages' strings (SOCOM 1's header static, one copy per unit there; SOCOM II's
 * one table, char_type.cpp's InitMultiplayerMessages fills it) */
CSTable MultiplayerMsgStrings(0, 1024);

/* SOCOM II's: a vector of blocks freed with the game's _free when it goes, its destructor out of
 * line in OVL2 (0x5D9540, between seal_tasks.cpp's CTGroup::Tick and End: the blocks freed from the
 * back, the storage traded away, the vector's own teardown -- registered by its OVL2 address, so
 * not inline: an inline one would have been emitted here). No code addresses it but the
 * initialiser -- the class and the static's names ours. OPEN ON THAT ROW'S NAME ALONE: the
 * initialiser is byte-exact with every relocation resolved but this one, which needs OVL2's row
 * 0x5D9540 named __dt__13CMallocBlocksFv (the kit lane's seal_tasks.cpp range; measured with the row
 * renamed for the run only, build/scratch/states/temprename.py: MATCH, 592 relocations resolved) */
class CMallocBlocks : public std::vector<void *> {
public:
    ~CMallocBlocks();   /* OVL2 0x5D9540 */
};

static CMallocBlocks s_taskBlocks;

/* the character types and their gear (SOCOM 1's char_type.cpp's statics: the types by name, the
 * gear made, the anim sets' names, the gears' models -- each a class of its own over its container
 * there, SOCOM II's gear list a vector) */
class CCharacterType;
class CCharacterGear;

class CCharTypeVec : public std::vector<CCharacterType *> {
};

class CCharacterGearList : public std::vector<CCharacterGear *> {
};

class CCharacterType {
public:
    static CCharTypeVec m_char_list;
    static CCharacterGearList m_gear_list;
    static std::list<const char *> m_anim_set_names;
};

CCharTypeVec CCharacterType::m_char_list;
CCharacterGearList CCharacterType::m_gear_list;
std::list<const char *> CCharacterType::m_anim_set_names;

/* the gears' models, shared by name: a model vector room for 256 (SOCOM 1's: the static is the
 * cache class itself, its destructor the one registered) */
class CCharacterModelCache : public zdb::CModelVector {
public:
    CCharacterModelCache() { reserve(256); }

    static CCharacterModelCache m_ModelCache;
};

CCharacterModelCache CCharacterModelCache::m_ModelCache;

/* the SEAL body's added aims (SOCOM 1's seal.cpp's CZSealBody::m_aim_anims, a vector<ANIM_ADD *>);
 * no code but the initialiser addresses it in SOCOM II */
struct ANIM_ADD;

class CZDecalDef {
public:
    /* empty and user-declared (zwep_weapon.cpp's) */
    CZDecalDef() {}

    float m_decalminsize;   // 0x0
    float m_decalmaxsize;   // 0x4
    void *m_texture;   // 0x8
};

class CZSealBody {
public:
    static std::vector<ANIM_ADD *> m_aim_anims;
    static std::vector<CZDecalDef> m_blooddriptexhandles;
};

std::vector<ANIM_ADD *> CZSealBody::m_aim_anims;

/* the anim sets: the death and damage anims by location and state, every set made (SOCOM 1's
 * anim.cpp's statics) and the anims' auxiliary data */
class AnimSet {
public:
    class List : public std::vector<AnimSet *> {
    };

    static std::vector<AnimType> deathanim[6][3];
    static std::vector<AnimType> pistoldeathanim[6][3];
    static std::vector<AnimType> teammatedeathanim[6][3];
    static std::vector<AnimType> teammatepistoldeathanim[6][3];
    static std::vector<AnimType> damageanim[6][3];
    static std::vector<AnimType> pistoldamageanim[6][3];
    static List list;
};

std::vector<AnimType> AnimSet::deathanim[6][3];
std::vector<AnimType> AnimSet::pistoldeathanim[6][3];
std::vector<AnimType> AnimSet::teammatedeathanim[6][3];
std::vector<AnimType> AnimSet::teammatepistoldeathanim[6][3];
std::vector<AnimType> AnimSet::damageanim[6][3];
std::vector<AnimType> AnimSet::pistoldamageanim[6][3];
AnimSet::List AnimSet::list;

class CAnimAuxData {
public:
    class CList : public std::vector<CAnimAuxData *> {
    };

    static CList list;
};

CAnimAuxData::CList CAnimAuxData::list;

/* the pickups, live and pooled (SOCOM 1's pickup_main.cpp: CPickupList) */
class CPickup;

class CPickupList : public std::list<CPickup *> {
};

class CPickup {
public:
    static CPickupList m_list;
    static CPickupList m_pool;
};

CPickupList CPickup::m_list;
CPickupList CPickup::m_pool;

/* SOCOM II's: a word of flags built all set (0x44D4D0: CZSealBody::PostPreTick tests its bit 0) --
 * the class and the static's names ours */
class CFlagWord {
public:
    CFlagWord() : m_bits(-1) {}

    int m_bits;
};

static CFlagWord s_sealBodyFlags;

std::vector<CZDecalDef> CZSealBody::m_blooddriptexhandles;

/* the squads (SOCOM 1's seal_unit.cpp: CSealUnit::ulist) */
class CSealUnit {
public:
    class ulist : public std::list<CSealUnit *> {
    };

    static ulist m_list;
};

CSealUnit::ulist CSealUnit::m_list;

/* the sensors' query pool (SOCOM 1's seal_sensors.cpp's _diPool, a CDiPool -- the kit lane's
 * seal_dipool.h has its users' view: its dirlist is this unit's CDiList, the used list, and its
 * dirlist::DeleteAll this unit's CDirList::DeleteAll, OVL2's copy 0x5CC6C0 of OVL1's 0x2CD9F0 -- but
 * ~CDiPool, which only this unit emits, shows the free list's class apart: a user destructor calling
 * DeleteAll, a word after the list the initialiser stores through the list's own address, and
 * list<dir *> its direct base, its destructor 0x2CD690 called with 0): six queries of
 * thirty-two hits each, malloc'd and built in place, the free ones and the directions handed out
 * kept in lists -- SOCOM II's has three counts and three more words. The destructor (0x2CD8A0) tears
 * the queries down and frees them, empties the free list, and the members go: the directions list
 * deleting what it holds (0x2CD9F0, called), then the free list */
class CDiPool {
public:
    /* a direction handed out (SOCOM 1's CDiPool::dir) */
    class dir {
    public:
        unsigned char _0[0x10];
    };

    /* the queries free (the class ours) */
    class CDiList : public std::list<zdb::DiIntersect *> {
    };

    /* the directions handed out, deleted with the list, and a count (the initialiser zeroes the word
     * after the list through the list's own address, kept across the list's constructor: a member of
     * the list's class, as SOCOM 1's -- the class and its members' names ours) */
    class CDirList : public std::list<dir *> {
    public:
        CDirList() : m_n(0) {}
        ~CDirList() { DeleteAll(); }
        void DeleteAll()
        {
            iterator e = end();
            iterator i;

            for (i = begin(); i != e; ++i)
                delete *i;
            erase(begin(), end());
        }

        int m_n;   // 0xc
    };   // 0x10 bytes

    CDiPool() : m_count(6), m_8(2), m_c(1), m_2c(0), m_30(0), m_34(0), m_38(0)
    {
        m_buf = (zdb::DiIntersect *)_malloc(6 * sizeof(zdb::DiIntersect), 0, 0);
        for (int i = 0; i < 6; i++)
            new (&m_buf[i]) zdb::DiIntersect(32);
    }
    ~CDiPool()
    {
        if (m_buf) {
            for (int i = 0; i < m_count; i++)
                m_buf[i].~DiIntersect();
            _free(m_buf, 0, 0);
            m_buf = 0;
        }
        m_free.erase(m_free.begin(), m_free.end());
    }

    zdb::DiIntersect *m_buf;   // 0x0
    int m_count;   // 0x4
    int m_8;   // 0x8
    int m_c;   // 0xc
    CDiList m_free;   // 0x10
    CDirList m_dirs;   // 0x1c
    unsigned char m_2c;   // 0x2c
    int m_30;   // 0x30
    int m_34;   // 0x34
    int m_38;   // 0x38
};   // 0x3c bytes

CDiPool _diPool;

/* the dodges and the scripted groups (SOCOM 1's seal_aimoves.cpp: CAiDodge::CList; SOCOM II's
 * CAiGroup the same, the class's name ours) */
class CAiDodge {
public:
    class CList : public std::list<CAiDodge *> {
    };

    static CList m_list;
};

CAiDodge::CList CAiDodge::m_list;

class CAiGroup {
public:
    class CList : public std::list<CAiGroup *> {
    };

    static CList m_list;
};

CAiGroup::CList CAiGroup::m_list;
