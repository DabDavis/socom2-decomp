/* valve_main.cpp -- a valve's name and its ZAnim comparison command (OVL1 0x352960-0x35450F).
 * The file name is SOCOM 1's (its asserts name valve_main.cpp from CValve::FreeName on).
 * Defined in reverse of their image order (-inline deferred). */

#include <stdlib.h>
#include <string.h>

#include "valve.h"

extern "C" long func_001914B8(const char *s);   /* newlib's atol (MAIN) */

/* a ZAnim argument instead of a valve: "INPUT_VALVE<n>" names argument n, anything else a valve,
 * created if new (SOCOM 1's; its debug walk of the argument table after the create is gone) */
CValve *VALVE_CMD_DATA::ParseValveName(const char *name, int &index)
{
    if (name && !strncmp(name, "INPUT_VALVE", 11)) {
        index = 0;
        if ((int)strlen(name) > 11)
            index = func_001914B8(name + 11);
        return 0;
    }
    return CValve::Create(name, (VALVE_TYPE)2);
}

/* the name taken as it is, owned by the caller's pool */
void CValve::AssignName(char *name)
{
    FreeName();
    m_name = name;
    m_name_pooled = 1;
}

void CValve::SetName(const char *name)
{
    FreeName();
    m_name = name ? _strdup(name, 0, 0) : 0;
    m_name_pooled = 0;
}

void CValve::FreeName()
{
    if (m_name_pooled)
        m_name = 0;
    else {
        if (m_name)
            _free(m_name, 0, 0);
        m_name = 0;
    }
    m_name_pooled = 0;
}
