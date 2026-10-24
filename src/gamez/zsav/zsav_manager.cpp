/* zsav_manager.cpp -- CSaveManager (OVL1 0x33E4D0): the save/load driver over a CIO file,
 * keeping a list of the modules that save. The layout is SOCOM 1's DWARF (SCUS_972.05), whose
 * constructor is byte-identical; SOCOM 1 kept it with Save, RegisterModule and the destructor.
 * The file name is an inference: the press beta's asserts name a zsav_manager.cpp (beside
 * zsav_module.cpp), and this is the class it would hold. */

#include <list>

class CIO;
class CZAR;
class CSaveModule;

class CSaveManager : public std::list<CSaveModule *> {
public:
    CSaveManager(CIO *file);

    CZAR *m_pZar;               /* 0x0C */
    CIO *m_pFile;               /* 0x10 */
    int m_TempBufferSize;       /* 0x14 */
    char *m_TempBuffer;         /* 0x18 */
    unsigned char m_AbortLoad;  /* 0x1C */
    unsigned char m_AbortSave;  /* 0x1D */
    char *m_Filename;           /* 0x20 */
};

CSaveManager::CSaveManager(CIO *file)
    : m_pFile(file), m_TempBufferSize(0), m_TempBuffer(0)
{
}
