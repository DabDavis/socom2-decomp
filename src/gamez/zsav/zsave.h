/* zsave.h -- the save system: CSaveManager writes and reads a save file as a ZAR, its modules
 * (CSaveModule) each serialising one subsystem into their own key. Layouts are SOCOM 1's DWARF
 * (SCUS_972.05: CSaveManager 0x24 bytes, a list of the modules as its base; CSaveModule 0x18,
 * its vtable pointer first). The header's name is ours; the units are SOCOM 1's (the press
 * beta's asserts name zsav_manager.cpp and zsav_module.cpp). This directory is zsave/ because
 * the UI-widgets/file lane held zsav/ this round, where zsav_manager.cpp already defines
 * CSaveManager's constructor over the same layout: the two want merging. */
#ifndef ZSAVE_H
#define ZSAVE_H

#include <list>
#include "../zar/zar.h"

class CRdrFile;
class CSaveModule;

class CSaveManager : public std::list<CSaveModule *> {
public:
    /* a save's header, its own key (SOCOM 1's nested class; no DWARF -- the layout SOCOM II's Save
     * writes: its size, the flags it was saved with, the version its modules read, a description) */
    class CSaveHeader {
    public:
        int m_size;   // 0x0: 268
        int m_flags;   // 0x4
        int m_version;   // 0x8
        char m_desc[256];   // 0xc
    };   // 0x10c bytes

    CSaveManager(CIO *file);
    ~CSaveManager();

    bool SaveRdr(zar::CKey *key, const char *name, CRdrFile *file);
    bool GetHeader(const char *name, int flags, CSaveHeader *header);
    bool Load(const char *name, int flags);
    bool Save(const char *name, int flags, int version, char *desc, char *tag);
    void RegisterModule(CSaveModule *module);
    CSaveModule *FindModule(const char *name);

    zar::CZAR *m_pZar;          /* 0x0C */
    CIO *m_pFile;               /* 0x10 */
    int m_TempBufferSize;       /* 0x14 */
    char *m_TempBuffer;         /* 0x18 */
    unsigned char m_AbortLoad;  /* 0x1C */
    unsigned char m_AbortSave;  /* 0x1D */
    char *m_Filename;           /* 0x20 */
};   // 0x24 bytes

/* a subsystem's part of a save: its name and key, its manager (SOCOM 1's slots, the vtable at
 * 0x406AA0: the destructor, Serialize (pure), the two UnSerializes, TestFlags) */
class CSaveModule {
public:
    CSaveModule(const char *name, CSaveManager *manager);
    virtual ~CSaveModule();
    virtual void Serialize(int flags) = 0;
    virtual void UnSerialize(const char *name, CRdrFile *file, int flags);
    virtual void UnSerialize(const char *name, zar::CZAR *zar, int flags);
    virtual bool TestFlags(int flags) const;

    bool SaveRdr(const char *name, CRdrFile *file);

    CSaveManager *m_Manager;    /* 0x04 */
    char *m_ModuleName;         /* 0x08 */
    int m_Priority;             /* 0x0C */
    int m_Flags;                /* 0x10 */
    zar::CKey *m_pKey;          /* 0x14 */
};   // 0x18 bytes

#endif
