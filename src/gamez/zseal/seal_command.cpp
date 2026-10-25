/* seal_command.cpp -- the orders: the AI's and the pad's controllers taking them, the SEAL issuing
 * them, which are allowed (SOCOM 1's unit, __sinit_seal_command.cpp, in SOCOM 1's order). OVL2
 * 0x58FBE0 (CSealCtrlAi::OnCommand) to seal_control.cpp (0x5920A0). Defined in reverse of their
 * image order (-inline deferred). */

#include "zseal.h"

extern "C" {
/* CZKit::GetItemIndex and ItemHasAmmo(int) (SOCOM 1's GetItemIndex__5CZKitCF10EQUIP_ITEM and
 * ItemHasAmmo__5CZKitFi, called here where SOCOM 1's CanDeploy calls them; OVL2 0x5C8870,
 * 0x5BDA10 -- the kit lane's) */
int func_005C8870(const CZKit *kit, EQUIP_ITEM item);
bool func_005BDA10(CZKit *kit, int index);
/* a switch that lets a charge be set anywhere (OVL2 bss 0x66B030; the name ours) */
extern unsigned char data_0066B030;
}

/* whether an item can be deployed at an action: held, with ammunition, and a breaching charge only
 * at a breach unless the switch is on (OVL2 0x591130; SOCOM 1's with the switch) */
bool CanDeploy(CZKit *kit, EQUIP_ITEM item, CNodeAction *action)
{
    NODEACTION_TYPE type = action ? action->m_type : NODEACTION_NONE;
    bool ok = false;

    if (kit->HasItem(item) && func_005BDA10(kit, func_005C8870(kit, item))) {
        if (item != 152 || data_0066B030 || type == NODEACTION_BREACH)
            ok = true;
    }
    return ok;
}

/* an order given: order 17 only if the controller allows it; flagged once given (OVL2 0x590AC0;
 * SOCOM 1's, whose test was order 11) */
void CZSealBody::IssueCommand(CZSealBody *seal, FT_COMMAND cmd, void *data, bool b)
{
    CSealCtrl *ctrl = (CSealCtrl *)m_control;

    switch (cmd) {
    case (FT_COMMAND)17:
        if (ctrl->AllowCommand(seal, cmd, data)) {
            ctrl->OnCommand(seal, cmd, data, b);
            m_order_flag = true;
        }
        break;
    default:
        ctrl->OnCommand(seal, cmd, data, b);
        m_order_flag = true;
        break;
    }
}
