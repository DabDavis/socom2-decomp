/* ai_events.cpp (OVL2): the AI event lists. Only what has been matched so far: the deleter
 * constructor of the callback list's std::list<CAiEventCallback>, a template instance
 * (OVL2 0x50FFC0). The unit name is SOCOM 1's (__sinit_ai_events.cpp, exact at 0x668480);
 * its directory is not known. */

#include <list>

class CAiEventCallback;

template class std::__list_deleter<CAiEventCallback, std::allocator<CAiEventCallback> >;
