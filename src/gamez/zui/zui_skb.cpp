/* zui_skb.cpp -- the soft keyboard: SOCOM 1's unit (its symbol table's @unnamed@zui_skb_cpp@ names
 * CSkbKey, CSkbKeyProps and CSoftKeyboard), OVL1 from 0x38B180. Written so far:
 * CSoftKeyboard::FindKeyProps (0x38CC50, SOCOM 1's 0x3A6280 instruction for instruction, the key
 * map at 0x164 where SOCOM 1's is at 0x2C), and with it the tail's find<CSkbKey> (0x38E8F0,
 * include/map). The tree's other instance rows (insert_one 0x38EC10, insert_node_at 0x38EF50,
 * destroy 0x38E9A0 and its accessors, the constructor 0x38ED10) are byte-exact as a probe
 * (build/tree/zui_skb.cpp) and come with the functions that fill and tear down the map. */

#include <map>

namespace {

/* a key: a row byte, then a 16-bit code. SOCOM II's tree compares the byte unsigned, then the
 * halfword at 2 (SOCOM 1's had two bytes), and copies the byte with lb: a signed member compared
 * through unsigned casts, the copy member-wise */
class CSkbKey {
public:
    CSkbKey(const CSkbKey &k) : m_row(k.m_row), m_code(k.m_code) {}
    bool operator<(const CSkbKey &k) const
    {
        if ((unsigned char)m_row != (unsigned char)k.m_row)
            return (unsigned char)m_row < (unsigned char)k.m_row;
        return m_code < k.m_code;
    }

    signed char m_row;   // 0x0
    unsigned short m_code;   // 0x2
};

class CSkbKeyProps;

typedef std::map<CSkbKey, CSkbKeyProps *> CSkbKeyMap;

/* the keyboard, as much as its key lookup reads */
class CSoftKeyboard {
public:
    CSkbKeyProps *FindKeyProps(const CSkbKey &key);

    unsigned char _0[0x164];
    CSkbKeyMap m_keyProps;   // 0x164
};

/* a key's properties, or none (SOCOM 1's): the found branch first -- `i == end() ? 0 : ...` and
 * its if/return lay out a bne and a b, 8 bytes longer */
CSkbKeyProps *CSoftKeyboard::FindKeyProps(const CSkbKey &key)
{
    CSkbKeyMap::iterator i = m_keyProps.find(key);

    if (i != m_keyProps.end())
        return (*i).second;
    return 0;
}

}
