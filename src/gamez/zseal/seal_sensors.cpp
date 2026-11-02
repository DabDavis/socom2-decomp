/* seal_sensors.cpp -- what an AI SEAL senses: its view cone, the visibility tests, the targets it
 * sees fall (SOCOM 1's unit, __sinit_seal_sensors.cpp: UpdateViewCone, SetViewFOV, ComputeVisibility,
 * RegisterCanSee ... NotifyTargetDown). OVL2 0x5C9400-0x5CDBCF, after seal_kit.cpp's CAiWepHint and
 * before seal_tasks.cpp. So far NotifyTargetDown. Defined in reverse of their image order (-inline
 * deferred). */

#include "zseal.h"

/* a target seen to fall: the AI calls it, "tango down", at full chance (OVL2 0x5C9B10; SOCOM 1's
 * played its sound 18 the same way, through the same slot +0x24) */
void CSealCtrlAi::NotifyTargetDown(CZSealBody *seal)
{
    PlayAiSound(CHRSND_TANGO_DOWN, 1.0f, -1);
}
