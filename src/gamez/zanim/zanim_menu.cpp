/* zanim_menu.cpp -- the menu's ZAnim commands and the scene and database they work on (OVL1
 * 0x27F7D0-...). The unit name is SOCOM 1's: its anonymous-namespace handlers mangle as
 * @unnamed@zanim_menu_cpp@. Defined in reverse of their image order (-inline deferred). */

#include <string.h>

#include "zanim.h"

char database[64];
zdb::CNode *g_menuRootNode;

void SetDatabase(const char *name)
{
    strcpy(database, name);
}

char *GetDatabase()
{
    return database;
}

void RegisterMenuNode(zdb::CNode *node)
{
    g_menuRootNode = node;
}

void UnRegisterMenuNode()
{
    g_menuRootNode = 0;
}

zdb::CNode *GetMenuNode()
{
    return g_menuRootNode;
}
