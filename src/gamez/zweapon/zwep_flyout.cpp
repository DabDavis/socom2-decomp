/* zwep_flyout.cpp -- the last unit of OVL1's code, 0x3D4810-0x3D5947: a pointer vector of
 * projectiles (0x3D4810-0x3D4B7F, the CZWeapon static at 0x4B5238 that zwep_weapon.cpp's
 * initialiser builds), CProjectileNodeEx, and the flyout machine -- SOCOM 1's CFlyoutMachine and
 * CFlyout, which sat between zwep_global.cpp and zwep_projectile.cpp in the demo (0x316550-
 * 0x31718F) and moved here. SOCOM 1 gives the flyouts no unit name; the file name is ours. The two
 * classes' destructors and the machine's constructor stay drafts. Defined in reverse of their image
 * order (-inline deferred). */

#include <msl_utility>
#include <stdlib.h>
#include <vector>

#include "../znode/znode.h"
#include "../znode/node_world.h"
#include "../zsys/zsys.h"
#include "zweapon.h"

using namespace zdb;

/* the animations (zanim.h's CZAnimMain: only the lookup) */
class CZAnimMain {
public:
    CZAnim *GetAnim(const char *name, const char *set_name);
};
extern CZAnimMain ZAnim;

/* an animation stopped (0x2707C0, not named) */
extern "C" void func_002707C0(CZAnim *anim, int b);

/* an animation (zanim.h's CZAnim: only its activation with arguments) */
class CZAnim {
public:
    CZAnim *ActivateArg(int type, ...);
};

/* a tracer in flight: its node (an empty xref the tracer model is set on), its anim, where and how
 * fast (SOCOM 1's class, 0x3c bytes in both; only what the code here touches is named) */
class CFlyout {
public:
    CFlyout();
    ~CFlyout();

    CNode *m_instance;   // 0x0
    CZAnim *m_anim;   // 0x4
    CPnt3D m_pos;   // 0x8
    CPnt3D m_vel;   // 0x14: the end it flies to (the name SOCOM 1's slot, the use the target's)
    int m_20;   // 0x20: the speed (a float: AcquireFlyout stores it, Tick reads it)
    int m_24;   // 0x24: the distance flown (a float)
    float m_maxdist;   // 0x28: the weapon's range, or 300 (the name ours)
    unsigned char m_done;   // 0x2c (SOCOM 1's Tick releases a flyout whose byte is set)
    unsigned char _2d[0x30 - 0x2d];
    CPnt3D m_30;   // 0x30: zeroed when it is acquired (not named)
};   // 0x3c bytes

/* the tracers: a pool of flyouts, the free ones and the active ones by pointer (SOCOM 1's
 * CFlyoutMachine, 0x1c bytes; CZWeapon's static m_flyout_machine, 0x4B5260) */
class CFlyoutMachine {
public:
    CFlyoutMachine();
    ~CFlyoutMachine();
    void Init(unsigned int count);
    static void HookupReset();
    void ReleaseAllActive();
    bool ReleaseFlyout(CFlyout *flyout, int index);
    /* the done flyouts released, then every active one moved on by its speed: done once past its
     * range, or arrived when the step would overshoot its end (SOCOM 1's) */
    bool Tick(float dT);
    /* a free flyout sent from start to end at a speed, unless the two are within 70 (SOCOM 1's) */
    bool AcquireFlyout(CPnt3D &start, CPnt3D &end, CZAnim *anim, CZWeapon *weapon, float speed,
                       CMatrix *m, CBBox *box);
    /* an active flyout's index, or -1 (inline: ReleaseFlyout's search; the name ours) */
    int FindActive(CFlyout *flyout)
    {
        unsigned int n = m_active.size();

        for (unsigned int i = 0; i < n; i++) {
            if (m_active[i] == flyout)
                return i;
        }
        return -1;
    }
    /* the model the next tracer takes (new in SOCOM II: SetModelOrientation picks the standard,
     * enemy or ally one; the name ours) */
    static void SetTracerModel(CModel *model);

    CFlyout *m_flyouts;   // 0x0
    std::vector<CFlyout *> m_free;   // 0x4
    std::vector<CFlyout *> m_active;   // 0x10

    /* SOCOM 1's statics: the model in use and the three tracer models */
    static CModel *pFlyout_Model_Registered;
    static CModel *pFlyout_Model_Standard;
    static CModel *pFlyout_Model_Enemy;
    static CModel *pFlyout_Model_Ally;
    /* SOCOM II's: the enemy's and the ally's tracer animations, looked up only when the flag
     * says so (the names ours) */
    static CZAnim *pFlyout_Anim_Ally;
    static CZAnim *pFlyout_Anim_Enemy;
    static bool m_tracer_anims;
};   // 0x1c bytes

bool CFlyoutMachine::m_tracer_anims = true;
CModel *CFlyoutMachine::pFlyout_Model_Registered;
CModel *CFlyoutMachine::pFlyout_Model_Standard;
CModel *CFlyoutMachine::pFlyout_Model_Enemy;
CModel *CFlyoutMachine::pFlyout_Model_Ally;
CZAnim *CFlyoutMachine::pFlyout_Anim_Ally;
CZAnim *CFlyoutMachine::pFlyout_Anim_Enemy;

/* an empty xref named for the flyouts, its box (1,1,1) to (1,1,1) through a local box, still and
 * cleared (SOCOM 1's: the box's two corners copied as PNT3Ds) */
CFlyout::CFlyout()
{
    m_instance = CNode::CreateXRef();
    m_instance->SetName("FlyoutXRef");
    CBBox box;

    box.m_min = CPnt3D(1.0f, 1.0f, 1.0f);
    box.m_max = CPnt3D(1.0f, 1.0f, 1.0f);
    m_instance->m_bbox = box;
    m_pos = CPnt3D::zero;
    m_vel = CPnt3D::zero;
    m_20 = 0;
    m_24 = 0;
    m_done = 0;
}

/* the pool built, both vectors sized to it, every flyout free (SOCOM 1's) */
void CFlyoutMachine::Init(unsigned int count)
{
    m_flyouts = new CFlyout[count];
    m_free.reserve(count);
    m_active.reserve(count);
    CFlyout *flyout = 0;

    for (unsigned int i = 0; i < count; i++) {
        flyout = &m_flyouts[i];
        m_free.push_back(flyout);
    }
}

/* the tracer models looked up again, and the two animations when they are wanted */
void CFlyoutMachine::HookupReset()
{
    pFlyout_Model_Standard = CWorld::GetAssetList()->GetModel("tracer");
    pFlyout_Model_Enemy = CWorld::GetAssetList()->GetModel("tracer_enemy");
    pFlyout_Model_Ally = CWorld::GetAssetList()->GetModel("tracer_ally");
    pFlyout_Model_Registered = pFlyout_Model_Standard;
    if (m_tracer_anims) {
        pFlyout_Anim_Enemy = ZAnim.GetAnim("tracer_enemy_anim", 0);
        pFlyout_Anim_Ally = ZAnim.GetAnim("tracer_ally_anim", 0);
        return;
    }
    pFlyout_Anim_Enemy = pFlyout_Anim_Ally = 0;
}

/* (one counter for both loops, declared after the count; the zero point copied once into a local
 * that both working points start from) */
bool CFlyoutMachine::Tick(float dT)
{
    int n = m_active.size();
    int i;

    if (n == 0)
        return false;
    for (i = 0; i < n; i++) {
        CFlyout *flyout = m_active[i];

        if (flyout->m_done && ReleaseFlyout(flyout, i)) {
            i--;
            n--;
        }
    }
    n = m_active.size();
    for (i = 0; i < n; i++) {
        CFlyout *flyout = m_active[i];
        CPnt3D d = flyout->m_vel - flyout->m_pos;
        CPnt3D zero = CPnt3D::zero;
        CPnt3D step = zero;
        CPnt3D pos = zero;

        *(float *)&flyout->m_24 += dT * *(float *)&flyout->m_20;
        if (*(float *)&flyout->m_24 > flyout->m_maxdist) {
            flyout->m_done = 1;
        } else {
            d.Normalize(&step);
            step.Scale(1.0f + *(float *)&flyout->m_24, &step);
            if (step.x * step.x + step.y * step.y + step.z * step.z > d.x * d.x + d.y * d.y + d.z * d.z) {
                flyout->m_instance->SetPosition(flyout->m_vel.x, flyout->m_vel.y, flyout->m_vel.z);
                flyout->m_pos = flyout->m_vel;
                flyout->m_done = 1;
            } else {
                pos = flyout->m_pos + step;
                flyout->m_instance->SetPosition(pos.x, pos.y, pos.z);
                flyout->m_pos = pos;
            }
        }
    }
    return true;
}

/* SOCOM 1's, with SOCOM II's tracer animations: the free flyout on top taken, its node given the
 * registered model (or none, the enemy's or the ally's tracer animation run on it instead), placed
 * a random tenth of 1..990 along the line, its end, speed and range set (the animation's handle then
 * cleared with the rest, as SOCOM 1's statement order has it), the box and matrix given, made
 * active and added to the world */
bool CFlyoutMachine::AcquireFlyout(CPnt3D &start, CPnt3D &end, CZAnim *anim, CZWeapon *weapon,
                                   float speed, CMatrix *m, CBBox *box)
{
    if (m_free.size() == 0)
        return false;
    CPnt3D d = end - start;

    if (d.Length() < 70.0f)
        return false;
    CFlyout *flyout = m_free.back();

    if (m_tracer_anims) {
        flyout->m_instance->SetModel(0);
        CZAnim *a = pFlyout_Model_Registered == pFlyout_Model_Enemy ? pFlyout_Anim_Enemy : pFlyout_Anim_Ally;

        if (a)
            flyout->m_anim = a->ActivateArg(1, 3, flyout->m_instance, 0);
    } else
        flyout->m_instance->SetModel(pFlyout_Model_Registered);
    float r = 1.0f + rand() % 990;

    r /= 10.0f;
    CPnt3D dir = end - start;

    dir.Normalize();
    dir.Scale(r, &dir);
    dir.Add(&start, &dir);
    flyout->m_pos = dir;
    flyout->m_anim = 0;
    flyout->m_vel = end;
    *(float *)&flyout->m_20 = speed;
    flyout->m_done = 0;
    flyout->m_24 = 0;
    flyout->m_maxdist = weapon ? weapon->GetMaxRange() : 300.0f;
    flyout->m_30 = CPnt3D::zero;
    if (box)
        flyout->m_instance->m_bbox = *box;
    if (m)
        flyout->m_instance->SetMatrix(m);
    flyout->m_instance->SetPosition(flyout->m_pos.x, flyout->m_pos.y, flyout->m_pos.z);
    m_free.pop_back();
    m_active.push_back(flyout);
    CWorld::m_world->AddChild(flyout->m_instance);
    flyout->m_instance->SetActive(true);
    return true;
}

/* every active flyout freed, then every free one's node's children let go of (SOCOM 1's, the
 * children SOCOM II's) */
void CFlyoutMachine::ReleaseAllActive()
{
    CFlyout *flyout = 0;

    while (m_active.size()) {
        flyout = m_active.back();
        m_active.pop_back();
        m_free.push_back(flyout);
        flyout->m_instance->RemoveFromParent();
        flyout->m_instance->SetActive(false);
        flyout->m_instance->SetModel(0);
    }
    /* (the size read once; the children freed as the base std::vector, whose four pieces the
     * target calls) */
    unsigned int n = m_free.size();

    for (unsigned int i = 0; i < n; i++) {
        flyout = m_free[i];
        if (flyout && flyout->m_instance)
            FreeVector<std::vector<CNode *> >(flyout->m_instance->GetChildren());
    }
}

/* an active flyout freed: found when no index is given, its anim stopped, swapped to the end and
 * popped, its node taken out of the world (SOCOM 1's) */
bool CFlyoutMachine::ReleaseFlyout(CFlyout *flyout, int index)
{
    unsigned int n = m_active.size();

    if (n == 0)
        return false;
    if (index == -1)
        index = FindActive(flyout);
    if (flyout->m_anim) {
        func_002707C0(flyout->m_anim, 0);
        flyout->m_anim = 0;
    }
    flyout->m_20 = 0;
    if (index != m_active.size() - 1)
        std::swap(m_active[index], m_active[m_active.size() - 1]);
    m_active.pop_back();
    m_free.push_back(flyout);
    flyout->m_instance->RemoveFromParent();
    flyout->m_instance->SetActive(false);
    flyout->m_instance->SetModel(0);
    return true;
}

void CFlyoutMachine::SetTracerModel(CModel *model)
{
    pFlyout_Model_Registered = model;
}

/* ---- CPlacedExplosiveList (0x3D4810-0x3D4B7F) ---- */

/* MSL's single-element erase of the pod storage (0x28F220: body.cpp's tail emits it; include/vector
 * has no single erase -- called through its placeholder) */
extern "C" unsigned int *func_0028F220(void *v, unsigned int *position);

/* one element erased, the iterator after it given back (inline: the iterator handed over by value,
 * copied bitwise, and the raw pointer the erase returns made the iterator) */
static inline CPlacedExplosiveList::iterator Erase(CPlacedExplosiveList *list,
                                                   CPlacedExplosiveList::iterator pos)
{
    return CPlacedExplosiveList::iterator((PlacedExplosive **)func_0028F220(list, (unsigned int *)pos.m_p));
}

/* the storage cleared, then emptied element by element (Clear inlined) */
CPlacedExplosiveList::CPlacedExplosiveList()
{
    Clear();
}

CPlacedExplosiveList::~CPlacedExplosiveList()
{
    Clear();
}

/* every charge erased, one at a time */
void CPlacedExplosiveList::Clear()
{
    iterator it;

    it = begin();
    while (it != end())
        it = Erase(this, it);
}

void CPlacedExplosiveList::Add(PlacedExplosive *explosive)
{
    push_back(explosive);
}

/* every entry of the charge erased */
void CPlacedExplosiveList::Remove(PlacedExplosive *explosive)
{
    iterator it;

    it = begin();
    while (it != end()) {
        if (explosive == *it)
            it = Erase(this, it);
        else
            ++it;
    }
}

int CPlacedExplosiveList::Count()
{
    return size();
}

PlacedExplosive *CPlacedExplosiveList::Get(int i)
{
    return (*this)[i];
}

/* ---- CProjectileNodeEx (0x3D4B80-0x3D4BDF) ---- */

CProjectileNodeEx::CProjectileNodeEx()
{
}

void CProjectileNodeEx::SetProjectile(CZProjectile *projectile)
{
    m_projectile = projectile;
}

/* a weapon's hit on the node: the projectile's own */
int CProjectileNodeEx::OnWeaponHit(CNode *node, IntersectStruct *hit, CZProjectile *projectile)
{
    m_projectile->OnWeaponHit(hit, projectile);
    return 0;
}
