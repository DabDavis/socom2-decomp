/* zNetMediusRoom.cpp -- the lobby's rooms and games: the room and game lists, joining a room,
 * the games' details (SOCOM 1's zNetMedius.cpp's BriefingRoom and RoomGame code; the unit and its
 * name are ours: OVL1 0x2E1CE0-0x2E5FDF, after the lobby's own unit, its static initialiser
 * 0x401E70 building the room list 0x44F568, whose destructor 0x2E5F60 closes it). Only some of it
 * so far. Defined in reverse of their image order (-inline deferred). */

#include "zonline.h"

extern "C" {
extern int LobbyWorldID;   /* the room we are in, -1 none (SOCOM 1's) */

void func_002E2920(CZOnlineLobby *lobby, C2D *page);   /* the game list asked for */
void func_002E3AA0(CZOnlineLobby *lobby, C2D *page);   /* the room list asked for */
}

/* a room's games deleted and the list emptied (OVL1 0x2E47D0, among the unit's room-list
 * members): the current room's too, from ReleaseUIResources. Pinned: never inlined */
#pragma auto_inline off
void RoomGameVector::DeleteAll()
{
    std::transform(begin(), end(), begin(), DeleteItem<RoomGame>());
    clear();
}
#pragma auto_inline reset

/* the room list asked for again, in the active lobby (SOCOM 1's; OVL1 0x2E3A30). The page is
 * taken into a local zeroed before the first test (the target's zero in that test's delay slot) */
bool CZOnlineLobby::RefreshRoomList(C2D *page)
{
    C2D *p = 0;

    if (!m_isopen)
        return false;
    if (m_current_state != LOBBY_ACTIVE)
        return false;
    if (page)
        p = page;
    func_002E3AA0(this, p);
    return true;
}

/* back to the room we were in, when there is one (SOCOM 1's; OVL1 0x2E3730): `> -1` branches on
 * an slti as the target does, `>= 0` on bltz */
bool CZOnlineLobby::RejoinRoom()
{
    if (!m_isopen)
        return false;
    if (LobbyWorldID > -1)
        ChangeState(LOBBY_ACTIVE);
    return true;
}

/* the game list asked for again (SOCOM 1's; OVL1 0x2E28D0) */
bool CZOnlineLobby::RefreshGameList(C2D *page)
{
    C2D *p = 0;

    if (!m_isopen)
        return false;
    if (page)
        p = page;
    func_002E2920(this, p);
    return true;
}
