/* zsav_manager.cpp -- CSaveManager, the save file's driver (OVL1 0x33D370-0x33E7EF): its modules
 * registered and found by name, an .rdr saved under a key, a save's header read, the file loaded
 * and saved -- through a buffer, the file scrambled on the card -- the destructor and the
 * constructor (0x33E4D0, byte-identical to SOCOM 1's, written first over the same layout). The
 * unit name is SOCOM 1's (the press beta's asserts name a zsav_manager.cpp; SOCOM 1's demo holds
 * SaveRdr, GetHeader, Load, RemoveExtension, Save, RegisterModule, the destructor and the
 * constructor in this order). Not written yet: the list's template pieces after the constructor
 * (0x33E510-0x33E7EF: SOCOM 1's sort<less<const void *> > and merge, include/list has neither).
 * Defined in reverse of their image order (-inline deferred). */

#include "zsave.h"
#include <string>
#include <functional>
#include <string.h>
#include <stdio.h>

class CRdrFile {
public:
    static CRdrFile *Load(zar::CZAR *zar, zar::CKey *key, int flags);
    zar::CKey *Insert(zar::CZAR *zar, zar::CKey *key);
};

int zrdr_free(CRdrFile *file);

/* zar::CZAR's members zar.h does not declare (placeholders, the archive first): the archive
 * opened (SOCOM 1's Open(const char *, int, unsigned, unsigned), a word more in SOCOM II), a key
 * made under the open key, a key's data fetched into a buffer, data written as a new key, the
 * archive written out, its data buffer handed over */
extern "C" bool func_00258920(zar::CZAR *zar, const char *name, int flags, unsigned int mode, unsigned int align,
                              int a);
extern "C" zar::CKey *func_00258740(zar::CZAR *zar, const char *name);
extern "C" bool func_0025A000(zar::CZAR *zar, zar::CKey *key, void *buf, int size, unsigned int a);
extern "C" void func_0025A2F0(zar::CZAR *zar, const char *name, const void *data, int size);
extern "C" void func_00258BD0(zar::CZAR *zar);
extern "C" void *func_00257F90(zar::CZAR *zar);

/* the module list sorted (SOCOM 1's sort<less<const void *> > of the pointer list, this unit's
 * template tail: not written yet) */
extern "C" void func_0033E510(CSaveManager *list, std::less<const void *> comp);

/* a string that is never null (SOCOM 1's zutil.h: MSL's basic_string; a local's destructor is the
 * base's, called with 0) */
class CSaferStr : public std::string {
public:
    void operator=(const char *sz)
    {
        if (sz)
            assign(sz);
        else
            erase();
    }
};

/* a scope traced (SOCOM II's; the class and its names ours): the scope's name and a step count, a
 * line formatted into a buffer of its own at each step -- the output compiled out */
class CScopeTrace {
public:
    CScopeTrace(const char *name)
    {
        m_name = name;
        m_count = 0;
        Print();
    }
    ~CScopeTrace() { Print(); }
    void Print()
    {
        char buf[256];

        sprintf(buf, "%s - %d", m_name, m_count++);
    }
    void Print(const char *what)
    {
        char buf[256];

        sprintf(buf, "%s - %s %d", m_name, what, m_count++);
    }

    const char *m_name;
    int m_count;
};

/* the archive's tail word SOCOM II's Save sets before opening it (ours: zar.h keeps the archive's
 * middle opaque) */
/* MSL's list::sort(), the comparator its own local (as a local of Save's it takes the slot above
 * begin()'s temporary; the target's sits below it) */
static inline void SortModules(CSaveManager *m)
{
    std::less<const void *> comp;

    func_0033E510(m, comp);
}

static inline char *&zar_tag(zar::CZAR &archive)
{
    return *(char **)((char *)&archive + 0x2c);
}

/* SOCOM 1's, byte-identical (0x33E4D0, after the destructor in the image) */
CSaveManager::CSaveManager(CIO *file)
    : m_pFile(file), m_TempBufferSize(0), m_TempBuffer(0)
{
}

/* SOCOM 1's: the buffer freed, the module list emptied */
CSaveManager::~CSaveManager()
{
    if (m_TempBuffer) {
        delete m_TempBuffer;
        m_TempBuffer = 0;
    }
}

/* SOCOM 1's: a module joins unless one of its name is there */
void CSaveManager::RegisterModule(CSaveModule *module)
{
    if (!FindModule(module->m_ModuleName)) {
        push_back(module);
        module->m_Manager = this;
    }
}

/* the scramblers (SOCOM II's; the names ours): every byte xored with a running key that takes in
 * the clear byte -- from 150, front to back, then from the last byte, back to front. Called out of
 * line in the target */
void SaveEncrypt(char *buf, int size);
void SaveDecrypt(char *buf, int size);
CSaferStr RemoveExtension(const char *name);

/* SOCOM 1's (with SOCOM II's tag): every module whose flags take part serialised under a key of its
 * own in a ZAR built in memory, the header first; the buffer scrambled and written to the file. The
 * header is declared first (the frame's order), `ok` set ahead of the archive's write (its store in
 * the call's delay slot) */
bool CSaveManager::Save(const char *name, int flags, int version, char *desc, char *tag)
{
    CScopeTrace trace("CSaveManager::Save");
    CSaveHeader header;
    bool ok = false;
    CBufferIO buf;

    buf.Open(_malloc(26, 0, 0), 26);
    buf.m_size = 0;
    buf.m_mode = 26;
    buf.m_ptr = buf.m_buffer;
    zar::CZAR archive("root", &buf);
    zar_tag(archive) = tag;
    m_pZar = &archive;
    if (func_00258920(&archive, name, flags, 26, 16, 0)) {
        m_AbortSave = false;
        header.m_size = 268;
        header.m_flags = flags;
        sprintf(header.m_desc, "%s", desc ? desc : "");
        header.m_version = version;
        func_0025A2F0(&archive, "zSaveHeader", &header, header.m_size);
        zar::CKey *modules = func_00258740(&archive, "CSaveModuleList");
        if (modules) {
            iterator it;

            SortModules(this);
            it = begin();
            for (iterator e = end(); it != e; ++it) {
                CSaveModule *module = *it;

                if (module->TestFlags(version)) {
                    trace.Print(module->m_ModuleName);
                    if ((module->m_pKey = func_00258740(&archive, module->m_ModuleName)) != 0) {
                        module->Serialize(version);
                        archive.CloseKey(module->m_pKey);
                        module->m_pKey = 0;
                    }
                }
                if (m_AbortSave)
                    break;
            }
            archive.CloseKey(modules);
        }
        ok = true;
        func_00258BD0(&archive);
        if (m_pFile->Open(name, 26)) {
            char *data = buf.m_buffer;

            SaveEncrypt(data, buf.GetSize());
            m_pFile->fwrite(data, buf.GetSize());
            m_pFile->Close();
        }
        buf.Close();
        void *p = func_00257F90(&archive);
        if (p)
            delete p;
        archive.Close();
    }
    m_pZar = 0;
    trace.Print("zar.Close()");
    return ok;
}

/* (the two loops each with their own counter: a shared one re-colours the second loop) */
#pragma auto_inline off
void SaveDecrypt(char *buf, int size)
{
    unsigned char key = buf[size - 1];

    for (int i = size - 2; i >= 0; i--) {
        buf[i] ^= key;
        key = key ^ (key + buf[i]);
    }
    key = 150;
    for (int j = 0; j < size; j++) {
        buf[j] ^= key;
        key = key ^ (key + buf[j]);
    }
}

/* OPEN, 7 instructions: the target masks a copy of the key for the update first and sign-extends
 * the raw key register for the store (`andi t3,t1; dsll32 t1,t1`); with the old key in a local, as
 * here, the sign extension reads the masked copy (`dsll32 t1,t3`), and every form that xors with
 * the key itself (`buf[i] ^= key` before the update, a temporary for the new key, casts) orders the
 * sign extension first, which re-colours the unrolled loop (84 differing) */
void SaveEncrypt(char *buf, int size)
{
    unsigned char key = 150;

    for (int i = 0; i < size; i++) {
        unsigned char old = key;

        key = key ^ (key + buf[i]);
        buf[i] ^= old;
    }
    key = buf[size - 1];
    for (int j = size - 2; j >= 0; j--) {
        unsigned char old = key;

        key = key ^ (key + buf[j]);
        buf[j] ^= old;
    }
}

/* SOCOM 1's: a key's name without its ".rdr" */
CSaferStr RemoveExtension(const char *name)
{
    CSaferStr s;
    char buf[512];

    strncpy(buf, name, strlen(name) - 4);
    buf[strlen(name) - 4] = 0;
    s = buf;
    return s;
}
#pragma auto_inline reset

/* SOCOM 1's: the file read whole and unscrambled, its header checked against the flags, every key
 * under the module list handed to its module -- an .rdr as a file, anything else as the archive.
 * The loop hoists the key-name temporary's two text addresses (the short form's sp+1465, the long
 * form's word at sp+1472) into s7/s8, the short form s7: include/string's c_str() written short form
 * first (`!is_long() ? s_.data_ : (const charT *)w_[2]`; the rdr lane's measurement, applied over
 * every hand-written unit with nothing lost) */
bool CSaveManager::Load(const char *name, int flags)
{
    CScopeTrace trace("CSaveManager::Load");
    CSaveHeader header;
    bool ok = false;
    CBufferIO buf;

    if (m_pFile->Open(name, 1)) {
        char *data = (char *)_malloc(m_pFile->GetSize(), 0, 0);

        m_pFile->fread(data, m_pFile->GetSize());
        SaveDecrypt(data, m_pFile->GetSize());
        buf.Open((void *)data, m_pFile->GetSize());
        m_pFile->Close();
    }
    zar::CZAR archive("root", &buf);

    m_Filename = (char *)name;
    if (GetHeader(name, flags, &header) && flags != -1 && header.m_flags != flags)
        return false;
    if (func_00258920(&archive, name, flags, 1, 16, 0)) {
        m_AbortLoad = false;
        zar::CKey *modules = archive.OpenKey("CSaveModuleList");
        if (modules) {
            for (zar::CKeyRing::iterator i = modules->begin(); i != modules->end(); ++i) {
                CSaveModule *module = FindModule((*i)->m_name);

                if (module) {
                    for (zar::CKeyRing::iterator j = (*i)->begin(); j != (*i)->end(); ++j) {
                        CRdrFile *file;
                        const char *keyname = (*j)->m_name;

                        if (strstr(keyname, ".rdr")) {
                            file = CRdrFile::Load(&archive, *j, 0);

                            if (file) {
                                module->UnSerialize(RemoveExtension(keyname).c_str(), file, header.m_version);
                                zrdr_free(file);
                            }
                        } else
                            module->UnSerialize(keyname, &archive, header.m_version);
                        if (m_AbortLoad)
                            break;
                    }
                    if (m_AbortLoad)
                        break;
                }
            }
            archive.CloseKey(modules);
        }
        ok = true;
        buf.Close();
        void *p = func_00257F90(&archive);
        if (p)
            _free(p, 0, 0);
        archive.Close();
    }
    m_Filename = 0;
    return ok;
}

/* SOCOM 1's (read through a buffer in SOCOM II): the file's header key fetched */
bool CSaveManager::GetHeader(const char *name, int flags, CSaveHeader *header)
{
    bool ok = false;
    CBufferIO buf;

    if (m_pFile->Open(name, 1)) {
        char *data = (char *)_malloc(m_pFile->GetSize(), 0, 0);

        m_pFile->fread(data, m_pFile->GetSize());
        SaveDecrypt(data, m_pFile->GetSize());
        buf.Open((void *)data, m_pFile->GetSize());
        m_pFile->Close();
    }
    zar::CZAR archive("root", &buf);
    if (func_00258920(&archive, name, flags, 1, 16, 0)) {
        zar::CKey *key = archive.OpenKey("zSaveHeader");

        if (key) {
            ok = func_0025A000(&archive, key, header, 268, 0);
            archive.CloseKey(key);
        }
        buf.Close();
        void *p = func_00257F90(&archive);
        if (p)
            _free(p, 0, 0);
        archive.Close();
    }
    return ok;
}

/* SOCOM 1's: `name`.rdr written under the key; SOCOM II's always answers false */
bool CSaveManager::SaveRdr(zar::CKey *key, const char *name, CRdrFile *file)
{
    char buf[80];

    sprintf(buf, "%s.rdr", name);
    if (key) {
        zar::CKey *k = func_00258740(m_pZar, buf);
        if (k) {
            file->Insert(m_pZar, k);
            m_pZar->CloseKey(k);
        }
    }
    return false;
}

/* the registered module of that name (SOCOM II's; the name ours). RegisterModule calls it out
 * of line: pinned */
#pragma auto_inline off
CSaveModule *CSaveManager::FindModule(const char *name)
{
    CSaveModule *found = 0;
    iterator i;

    i = begin();
    iterator e = end();

    /* the element read through the base iterator, cast out by value: the wrapper's reference
     * keeps the element's address in a saved register across strcmp, where the target reloads */
    for (; i != e; ++i) {
        if (!strcmp(((CSaveModule *)*i.i_)->m_ModuleName, name)) {
            found = (CSaveModule *)*i.i_;
            break;
        }
    }
    return found;
}
#pragma auto_inline reset
