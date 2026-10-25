/* seal_unit.cpp -- the squads, CSealUnit (SOCOM 1's unit, __sinit_seal_unit.cpp). OVL2
 * 0x5E6AA0-0x5EF9CF. So far only its tail's out-of-line copies: the breach state's destructor
 * (and with its table the inline virtuals and CAiState's destructor). Defined in reverse of their
 * image order (-inline deferred). */

#include "../../ai/ai_states.h"

/* (OVL2 0x5EA4D0; CAiState's destructor follows it in, 0x5EA450) */
CAiSBreach::~CAiSBreach()
{
}
