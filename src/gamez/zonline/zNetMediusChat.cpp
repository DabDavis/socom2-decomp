/* zNetMediusChat.cpp -- the lobby's chat and its message plumbing: the chat queues, the
 * censor, the messages' byte-order converters (SOCOM 1's zNetMedius.cpp's chat code; the unit and
 * its name are ours: OVL1 from about 0x2ED460, after the ladder unit, to 0x2F628F, its static
 * initialiser 0x4020E0 building the two queues 0x44FA20 and 0x44FA50, whose destructor 0x2F5E50
 * and deque instances close it). Only some of it so far. Defined in reverse of their image order
 * (-inline deferred). */

#include "zonline.h"

/* a pair of 24-byte slots, a word at 4 marking one in use (only what is read here) */
struct ChatSlot {
    int m_0;
    int m_used;   // 0x4
    unsigned char _8[0x18 - 0x8];
};

extern "C" {
void func_002F3EF0(void *part);   /* a type-129 response's tail converted */

/* the first free slot of two looked for, and nothing done with it: the search survives, its
 * result does not (a void leaf: v1 is its only temporary; OVL1 0x2F3FF0) */
void func_002F3FF0(ChatSlot *slots)
{
    for (int i = 0; i < 2; i++) {
        if (!slots[i].m_used)
            break;
    }
}

/* a response put into the host's order: only the type-129 ones carry anything to convert (OVL1
 * 0x2F4030; the request side, 0x2F4080, is empty) */
void func_002F4030(void *msg, int type)
{
    switch (type) {
    case 32:
        break;
    case 33:
        break;
    case 129:
        func_002F3EF0((char *)msg + 140);
        break;
    }
}
}
