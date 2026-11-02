/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* rt_msg: the RT library's little-endian message fields (OVL2 0x638880-0x638A68), SCEA's
 * Medius/DME network stack, prebuilt with ee-gcc 2.96 at -O2 -G0 (README "The GCC front").
 * Names are SOCOM 1's (s1:exact/gap). Each accessor guards its pointers one test at a time and
 * returns RT_RESULT 2 (invalid parameter): the guards, not a result variable, are what put
 * the result straight in v0 (2.96). */

typedef int RT_RESULT;

#define RT_OK               0
#define RT_INVALID_PARAM    2

RT_RESULT rt_msg_set_byte(unsigned char *msg, char value)
{
    if (msg == 0)
        return RT_INVALID_PARAM;
    *msg = value;
    return RT_OK;
}

RT_RESULT rt_msg_get_byte(const unsigned char *msg, char *value)
{
    if (msg == 0)
        return RT_INVALID_PARAM;
    if (value == 0)
        return RT_INVALID_PARAM;
    *value = *msg;
    return RT_OK;
}

RT_RESULT rt_msg_set_ubyte(unsigned char *msg, unsigned char value)
{
    if (msg == 0)
        return RT_INVALID_PARAM;
    *msg = value;
    return RT_OK;
}

RT_RESULT rt_msg_get_ubyte(const unsigned char *msg, unsigned char *value)
{
    if (msg == 0)
        return RT_INVALID_PARAM;
    if (value == 0)
        return RT_INVALID_PARAM;
    *value = *msg;
    return RT_OK;
}

RT_RESULT rt_msg_set_2byte(unsigned char *msg, short value)
{
    if (msg == 0)
        return RT_INVALID_PARAM;
    msg[0] = (unsigned char)value;
    msg[1] = (unsigned char)(value >> 8);
    return RT_OK;
}

RT_RESULT rt_msg_get_2byte(const unsigned char *msg, short *value)
{
    if (msg == 0)
        return RT_INVALID_PARAM;
    if (value == 0)
        return RT_INVALID_PARAM;
    *value = msg[0];
    *value |= msg[1] << 8;
    return RT_OK;
}

RT_RESULT rt_msg_set_4byte(unsigned char *msg, int value)
{
    if (msg == 0)
        return RT_INVALID_PARAM;
    *msg++ = (unsigned char)value;
    *msg++ = (unsigned char)(value >> 8);
    *msg++ = (unsigned char)(value >> 16);
    *msg = (unsigned char)(value >> 24);
    return RT_OK;
}

RT_RESULT rt_msg_get_4byte(const unsigned char *msg, int *value)
{
    if (msg == 0)
        return RT_INVALID_PARAM;
    if (value == 0)
        return RT_INVALID_PARAM;
    *value = *msg++;
    *value |= *msg++ << 8;
    *value |= *msg++ << 16;
    *value |= *msg << 24;
    return RT_OK;
}
