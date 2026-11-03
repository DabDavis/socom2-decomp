/* sc: compiler=mwcps2-3.0b52-030722; flags=-O4,p -sdatathreshold 0 -inline auto,deferred,bottomup,level=5 -enum min -Cpp_exceptions on */
/* ExceptionDecode.cpp -- Metrowerks' MIPS exception tables' number encoding (MAIN 0x182930-0x182A87):
 * every number in an unwind record or an action is one to four bytes, the first byte's low bits saying
 * how many (0: seven bits; 01: fourteen; 011: twenty-one; 111: twenty-nine), big-endian after it. SOCOM
 * 1's runtime names both (0x142C90, 0x142D30); the file name is ours.
 *
 * A unit of its own: ExceptionHandler.cpp and ExceptionMIPS.cpp call both out of line from every action
 * decode -- in either unit a deferred auto-inliner inlines them (UnwindStack grows from 0x844 to 0x1650),
 * and without them both units' functions come out at the image's sizes. The image emits the unit
 * reversed (deferred): __DecodeUnsignedNumber first. */

#include "ExceptionMIPS.h"

char *__DecodeSignedNumber(char *p, int *result)
{
    int b0 = p[0];
    unsigned int b1, b2;

    if (!(b0 & 1)) {
        *result = b0 >> 1;
        return p + 1;
    }
    b1 = (unsigned char)p[1];
    if (!(b0 & 2)) {
        *result = ((b0 >> 2) << 8) | b1;
        return p + 2;
    }
    b2 = (unsigned char)p[2];
    if (!(b0 & 4)) {
        *result = ((b0 >> 3) << 16) | (b1 << 8) | b2;
        return p + 3;
    }
    *result = ((b0 >> 3) << 24) | (b1 << 16) | (b2 << 8) | (unsigned char)p[3];
    return p + 4;
}

char *__DecodeUnsignedNumber(char *p, unsigned int *result)
{
    unsigned int b0 = (unsigned char)p[0];
    unsigned int b1, b2;

    if (!(b0 & 1)) {
        *result = b0 >> 1;
        return p + 1;
    }
    b1 = (unsigned char)p[1];
    if (!(b0 & 2)) {
        *result = ((b0 >> 2) << 8) | b1;
        return p + 2;
    }
    b2 = (unsigned char)p[2];
    if (!(b0 & 4)) {
        *result = ((b0 >> 3) << 16) | (b1 << 8) | b2;
        return p + 3;
    }
    *result = ((b0 >> 3) << 24) | (b1 << 16) | (b2 << 8) | (unsigned char)p[3];
    return p + 4;
}
