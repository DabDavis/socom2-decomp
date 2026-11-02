/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* rt_msg: the RT library's message fields (OVL2 0x638880-0x638CF0, SOCOM 1's object at
 * 0x410B88), SCEA's Medius/DME network stack, prebuilt with ee-gcc 2.96 at -O2 -G0 (README "The
 * GCC front"). Names are SOCOM 1's (s1:exact/gap); its rt_msg_util_assert is gone, and eight
 * network-order twins (no SOCOM 1 name) follow. Each accessor guards its pointers one test at a time and
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

RT_RESULT rt_msg_set_u2byte(unsigned char *msg, unsigned short value)
{
    if (msg == 0)
        return RT_INVALID_PARAM;
    msg[0] = (unsigned char)value;
    msg[1] = (unsigned char)(value >> 8);
    return RT_OK;
}

RT_RESULT rt_msg_get_u2byte(const unsigned char *msg, unsigned short *value)
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

RT_RESULT rt_msg_set_u4byte(unsigned char *msg, unsigned int value)
{
    if (msg == 0)
        return RT_INVALID_PARAM;
    *msg++ = (unsigned char)value;
    *msg++ = (unsigned char)(value >> 8);
    *msg++ = (unsigned char)(value >> 16);
    *msg = (unsigned char)(value >> 24);
    return RT_OK;
}

RT_RESULT rt_msg_get_u4byte(const unsigned char *msg, unsigned int *value)
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

/* The same fields in network order (most significant byte first), new since SOCOM 1's build. */

RT_RESULT func_00638B00(unsigned char *msg, short value)
{
    if (msg == 0)
        return RT_INVALID_PARAM;
    msg[0] = (unsigned char)(value >> 8);
    msg[1] = (unsigned char)value;
    return RT_OK;
}

RT_RESULT func_00638B28(const unsigned char *msg, short *value)
{
    if (msg == 0)
        return RT_INVALID_PARAM;
    if (value == 0)
        return RT_INVALID_PARAM;
    *value = msg[0] << 8;
    *value |= msg[1];
    return RT_OK;
}

RT_RESULT func_00638B60(unsigned char *msg, unsigned short value)
{
    if (msg == 0)
        return RT_INVALID_PARAM;
    msg[0] = (unsigned char)(value >> 8);
    msg[1] = (unsigned char)value;
    return RT_OK;
}

RT_RESULT func_00638B88(const unsigned char *msg, unsigned short *value)
{
    if (msg == 0)
        return RT_INVALID_PARAM;
    if (value == 0)
        return RT_INVALID_PARAM;
    *value = msg[0] << 8;
    *value |= msg[1];
    return RT_OK;
}

RT_RESULT func_00638BC0(unsigned char *msg, int value)
{
    if (msg == 0)
        return RT_INVALID_PARAM;
    *msg++ = (unsigned char)(value >> 24);
    *msg++ = (unsigned char)(value >> 16);
    *msg++ = (unsigned char)(value >> 8);
    *msg = (unsigned char)value;
    return RT_OK;
}

RT_RESULT func_00638BF8(const unsigned char *msg, int *value)
{
    if (msg == 0)
        return RT_INVALID_PARAM;
    if (value == 0)
        return RT_INVALID_PARAM;
    *value = *msg++ << 24;
    *value |= *msg++ << 16;
    *value |= *msg++ << 8;
    *value |= *msg;
    return RT_OK;
}

RT_RESULT func_00638C58(unsigned char *msg, unsigned int value)
{
    if (msg == 0)
        return RT_INVALID_PARAM;
    *msg++ = (unsigned char)(value >> 24);
    *msg++ = (unsigned char)(value >> 16);
    *msg++ = (unsigned char)(value >> 8);
    *msg = (unsigned char)value;
    return RT_OK;
}

RT_RESULT func_00638C90(const unsigned char *msg, unsigned int *value)
{
    if (msg == 0)
        return RT_INVALID_PARAM;
    if (value == 0)
        return RT_INVALID_PARAM;
    *value = *msg++ << 24;
    *value |= *msg++ << 16;
    *value |= *msg++ << 8;
    *value |= *msg;
    return RT_OK;
}
