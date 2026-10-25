/* zkit.h -- the SEAL's kit (CZKit) and the controllers and squads around the SEAL that this lane
 * writes: the network's controller (CRemoteCtrl), the squad (CSealUnit), the squirming hostage's
 * controller (CSealCtrlSquirm). SOCOM 1 files them all under gamez/zseal (seal_kit.cpp,
 * remote_control.cpp, seal_unit.cpp, seal_squirm.cpp); zseal.h is the SEAL lane's, so this lane's
 * classes live here (the header's name and directory are ours). Layouts are SOCOM 1's DWARF moved
 * to where SOCOM II's code puts each member; only what a written function touches is named. */
#ifndef ZKIT_H
#define ZKIT_H

#include "../zseal/zseal.h"

/* the network's controller: a SEAL another console drives (SOCOM 1's CRemoteCtrl : CSealCtrl;
 * SOCOM II's vtable 0x669590, its destructor and SetAiNetState in seal_ai.cpp's span) */
class CRemoteCtrl : public CSealCtrl {
public:
    /* (0x56F460, not written here: declared so no unit emits an implicit one) */
    virtual ~CRemoteCtrl();
    virtual bool PreTick(float dT);
    virtual bool Tick(float dT);
    virtual bool PostMortemPreTick(float dT);
    virtual void OnDeath();
    virtual CEntityCtrl *GetRemoteControl();
    virtual bool AllowCommand(CZSealBody *seal, FT_COMMAND cmd, void *data);
    virtual bool IsPlayer() const;
    virtual bool IsRemote() const;
    virtual bool PostTick(float dT);
    virtual void OnStunDamage(STUN_TYPE type, float amount, const CPnt3D *from);
    virtual void OnMissionStart();
    virtual void OnMissionEnd();
    virtual void SetAiNetState(unsigned int state);
    virtual void OnCommand(CZSealBody *seal, FT_COMMAND cmd, void *data, bool b);
    virtual bool GetJumpFlag() const;

    float m_fade_min;   // 0x250: the corpse's fade floor (0.001; the names ours)
    float m_fade_time;   // 0x254: the corpse's fade time (2.0)
};

#endif
