/* zsav_lobby.cpp -- the lobby's save (OVL1 0x33E7F0-0x33F04F, after zsav_manager.cpp's list sort
 * and ahead of 989snd.c): CLobbySaveManager writes its modules' entries into one blob, sends it as
 * a Medius file with its checksum, and fetches and hands it back entry by entry; CLobbySaveModule
 * is a module's part (zsav_lobby.h). SOCOM II's; the unit's name and every name in it are ours.
 * UICharacterProfile drives Save, Pending, Fetch and Load; CMenuState::UnInit calls Close; the
 * account database's Save writes its blob through Write. Defined in reverse of their image order
 * (-inline deferred). */

#include "zsav_lobby.h"
#include <map>

/* the transfers under way (zNetMediusFile.cpp's map, 0x450070) */
extern std::map<std::string, CLobbyFileIO *> data_00450070;

/* the account database's checksum of the current profile, 0 none (zNetMediusUniverse.cpp's, its
 * server at 0x44FC60 with the database at 0) */
class CZUniversInformationServer {
public:
    class CAcctDB {
    public:
        const unsigned char *GetChecksum();
    };
    CAcctDB m_acctDB;
};
extern CZUniversInformationServer data_0044FC60;

/* the lobby's UI event (fts_remote.cpp's declaration) */
extern "C" void (*data_0044F3C8)(const char *event, const char *arg);

/* a checksum over a buffer (Medius's, OVL2): the digest written after the buffer and its size */
struct LobbyChecksum {
    const char *m_data;   // 0x0
    int m_size;   // 0x4
    unsigned char m_digest[16];   // 0x8
};
extern "C" void func_0064BB80(LobbyChecksum *sum);

/* a module named and registered with its manager (theAcctDB's, from the online unit's
 * initialiser 0x4022D0) */
CLobbySaveModule::CLobbySaveModule(const char *name, CLobbySaveManager *manager)
{
    m_name = name;
    m_manager = manager;
    m_manager->RegisterModule(this);
}

/* the module's blob added under its name */
void CLobbySaveModule::Write(const char *data, int size)
{
    m_manager->Add(m_name.c_str(), data, size);
}

/* no modules, no blob, its own transfer */
CLobbySaveManager::CLobbySaveManager() : m_buffer(0), m_header(0), m_entries(0)
{
    m_file = new CLobbyFileIO;
}

CLobbySaveManager::~CLobbySaveManager()
{
    if (m_file)
        delete m_file;
}

/* the transfer closed, the blob dropped (CMenuState::UnInit) */
void CLobbySaveManager::Close()
{
    if (m_file)
        m_file->Close();
    FreeBuffer();
}

/* the blob built -- its header, an entry per module, each module's data -- checksummed for the
 * modules, and sent. Each walk holds its end in a local (written `i != end()` MW rebuilds the end
 * each pass and keeps one saved register fewer), and the digest's address is a local of its own
 * (the target keeps it in s1 from the memset to the second walk) */
bool CLobbySaveManager::Save(const char *name, int version, int flags)
{
    Header hdr;

    FreeBuffer();
    hdr.m_version = version;
    hdr.m_count = 0;
    unsigned int n = size() * sizeof(Entry);
    m_used = n + sizeof(Header);
    m_buffer = (char *)_malloc(m_used, 0, 0);
    memcpy(m_buffer, &hdr, sizeof(Header));
    memset(m_buffer + sizeof(Header), 0, n);
    m_header = (Header *)m_buffer;
    m_entries = (Entry *)(m_buffer + sizeof(Header));
    {
        iterator e = end();

        for (iterator i = begin(); i != e; ++i)
            (*i)->Save(flags);
    }
    m_file->Open(name, 10);
    LobbyChecksum sum;
    unsigned char *digest = sum.m_digest;

    sum.m_data = m_buffer;
    sum.m_size = m_used;
    memset(digest, 0, 16);
    func_0064BB80(&sum);
    {
        iterator e = end();

        for (iterator i = begin(); i != e; ++i)
            (*i)->SetChecksum(digest);
    }
    m_file->fwriteAsync(m_buffer, m_used);
    return true;
}

/* the blob fetched once its transfer is idle: the file opened first, then read whole unless the
 * account's checksum says it is unchanged (the UI told the file does not exist) */
bool CLobbySaveManager::Fetch(const char *name, int a, int b)
{
    int result;

    if (!m_file->fsyncAsync(0, &result))
        return false;
    if (!m_file->IsOpen()) {
        m_file->Open(name, 1);
        return false;
    }
    if (memcmp(data_0044FC60.m_acctDB.GetChecksum(), m_file->GetChecksum(), 16)) {
        FreeBuffer();
        m_used = m_file->GetSize();
        m_buffer = (char *)_malloc(m_used, 0, 0);
        m_file->freadAsync(m_buffer, m_used);
        return true;
    }
    if (data_0044F3C8)
        data_0044F3C8("MP_MEDIUS_FILE_DOES_NOT_EXIST", 0);
    return false;
}

/* the fetched blob handed out: each entry to the modules of its name, with the file's checksum,
 * when the header's version is the one asked for; the blob dropped and the file closed.
 * OPEN, 0x1A4 of 0x1B4, both causes include/list's (measured, the world lane r6,
 * build/scratch/wld/v_load*.py): (1) the target hoists the walk's end (this + 4) out of both
 * loops into a preheader placed after the function's return -- the outer loop entered by a `b` to
 * it, its `0 < count` guard there, then a `b` to the bottom test. include/list's end(), built from
 * `&this->end_`, is never hoisted; built from SOCOM 1's tail() --
 * `iterator(__list_deleter<...>::iterator(tail()))` -- it is, and the whole loop shape becomes the
 * target's (0x1C8: the rest is (2)'s extra saved register); (2) the element is read three times,
 * each a load folded from the node (8(node)), where MW keeps node + 8 in a saved register across
 * the calls: the CAST in the iterator's `((node *)node_)->data_` does it -- `*i`, `*i.i_`, the node
 * read directly, through an inline by value and even a volatile read all keep it, a walk through an
 * uncast node pointer folds all three loads and if-converts the name's c_str() ternary as the
 * target does. SOCOM 1's DWARF has exactly that: list's node_base links are `node *prev_, *next_`
 * and generic_iterator holds `node *node_` (include/list holds node_base * and casts) -- an MSL
 * headers item. The name is read through CLobbySaveModule::GetName(), which folds the string's
 * offset into the module's as the target's c_str() does */
bool CLobbySaveManager::Load(const char *name, int version)
{
    int result;

    if (!m_file->fsyncAsync(0, &result))
        return false;
    if (m_buffer) {
        m_header = (Header *)m_buffer;
        if (m_header->m_version == version) {
            m_entries = (Entry *)(m_buffer + sizeof(Header));
            for (int k = 0; k < m_header->m_count; k++) {
                Entry *e = &m_entries[k];

                for (iterator i = begin(); i != end(); ++i) {
                    if (!strcmp((*i)->GetName(), e->m_name)) {
                        (*i)->Load(m_buffer + e->m_offset, e->m_size);
                        (*i)->SetChecksum(m_file->GetChecksum());
                    }
                }
            }
        }
        FreeBuffer();
    }
    m_file->Close();
    return true;
}

/* a transfer still under way; one that has finished drops the blob */
bool CLobbySaveManager::Pending()
{
    bool r = m_file != 0;

    /* one result assigned on both paths: returning from inside the test is 0x10 longer */
    if (m_file && data_00450070.empty()) {
        FreeBuffer();
        r = false;
    }
    return r;
}
