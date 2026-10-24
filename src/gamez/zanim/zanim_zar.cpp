/* zanim_zar.cpp -- CZAnimMain's ZARs: opening and closing one, and loading its anim sets (OVL1
 * 0x267AB0-0x2681C0). Not anim_main.cpp: LoadZAR calls GetAnimSet(int), which -inline auto
 * inlines everywhere in anim_main.cpp (0x48 bytes; GetAnimIndex's copy), and the file-name
 * LoadZAR at 0x267AB0 inlines OpenZAR and CloseZAR but calls GetAnimSet too. Nor
 * zanim_cmd_tick.cpp, whose text ends with its template copies (vector::at, a clamp) at
 * 0x2679F0. SOCOM 1 has these between the two; no static initialiser names the unit, so its
 * name is ours. Defined in reverse of their image order (-inline deferred). */

#include <stdio.h>
#include <string.h>

#include "zanim.h"

extern "C" void func_00258CE0(CZAnimZAR *zar);
/* zar::CZAR::Open with a fifth argument SOCOM 1's did not have: not named */
extern "C" int func_00258920(zar::CZAR *zar, const char *name, int mode, unsigned int flags, unsigned int a, int b);

/* a ZAR's anim sets, or the one named: none when the anims are not loaded from ZARs, nothing
 * when the file is closed or its version not SOCOM II's (72; SOCOM 1's 30) */
int CZAnimMain::LoadZAR(CZAnimZAR *archive, const char *set_name)
{
    _zanim_main_params params;
    int set_count;

    if (!m_LoadFromZAR)
        return 1;
    if (!archive->IsOpen())
        return 0;
    bool ok = archive->Fetch("Anim_Main_Params", &params, sizeof(params));

    if (ok && params.m_version != 72)
        ok = false;
    if (ok) {
        archive->Fetch("Anim_Main_Params", (_zanim_main_params *)this, sizeof(_zanim_main_params));
        m_name_table.Load(*archive);
        archive->Fetch("Anim_Set_Count", &set_count, 4);
        archive->GetOpenKey();
        zar::CKey *sets = archive->OpenKey("Anim_Sets");

        if (sets) {
            for (zar::CKeyRing::const_iterator it = sets->begin(); it != sets->end(); ++it) {
                if (archive->OpenKey(*it)) {
                    bool load = true;

                    if (set_name && strcasecmp((*it)->m_name, set_name))
                        load = false;
                    if (load) {
                        CZAnimSet *set = GetAnimSet(GetAnimSetIndex((*it)->m_name, true));

                        m_CurAnimSet = set;
                        m_CurAnim = 0;
                        m_CurSeq = 0;
                        set->Load(*archive);
                    }
                    archive->CloseKey(*it);
                }
            }
            archive->CloseKey(sets);
        }
    }
    return 1;
}

/* a ZAR by file or directory name (the directory's zanim.zar) */
CZAnimZAR *CZAnimMain::OpenZAR(const char *name)
{
    if (!m_LoadFromZAR)
        return 0;
    CZAnimZAR *zar = new CZAnimZAR;

    if (zar) {
        char path[128];

        if (!strstr(name, "zanim.zar"))
            sprintf(path, "%s/%s", name, "zanim.zar");
        else
            sprintf(path, "%s", name);
        if (func_00258920(zar, path, 0, 1, 16, 0))
            return zar;
        delete zar;
        return 0;
    }
    return 0;
}

int CZAnimMain::CloseZAR(CZAnimZAR *zar)
{
    func_00258CE0(zar);
    delete zar;
    return 1;
}

CZAnimZAR::~CZAnimZAR()
{
}
