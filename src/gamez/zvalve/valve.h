/* The valves' support classes (valve_list.cpp, valve_main.cpp): CValve itself is zanim.h's, where
 * ZAnim and the states use it. SOCOM 1 has no debug information for these; the shapes are read off
 * the code and the header's name is ours. */
#ifndef VALVE_H
#define VALVE_H

#include "../zanim/zanim.h"

/* a valve command's data (SOCOM 1's struct name, from its mangled members) */
struct VALVE_CMD_DATA {
    static CValve *ParseValveName(const char *name, int &index);
};

#endif
