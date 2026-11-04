/* zsys_overlay.cpp -- the overlays' loader (OVL1 0x3515D0-0x3518BF): which overlay each of the
 * three slots holds, and the load of FTSCORE, ZSEALETC or DNAS into its slot (0x3515D0:
 * fts_patch.cpp's CSMDownloadPatch and fts_dnas.cpp's CSMDNASAuthenticate call it on the object
 * below). A unit of its own between zsys_vu.cpp and valve_list.cpp: its initialiser
 * 0x4034D0, between zsys_vu.cpp's bss (0x49E7E0) and valve_list.cpp's, clears the object's three
 * bytes (the integrator's initialiser research). No SOCOM 1 counterpart (SOCOM 1 had no
 * overlays); the unit's, the class's and the members' names are ours. */

#include <stdio.h>

#include "zsys_public.h"
#include "../zutil/zutil.h"

/* the overlay slots' load addresses, by slot (MAIN's table, 0x1D52A0) */
extern void *data_001D52A0[];
/* the device the overlays load from, negative for none (main.cpp's word after main, 0x1E70F4:
 * -1 in the image) */
extern int data_001E70F4;
/* an overlay read from the device and inflated into its slot (MAIN's boot-loader client: the
 * device's twin of loader.c's func_001C5DA0, which reads it from the disc's ZDB) */
extern "C" int func_001C60B0(int device, const char *name, void *dest, void *buf, int second);
extern "C" int func_001C5DA0(const char *name, void *dest, void *buf, int second);
/* the overlay's code made current (MAIN's mwoverlay.c) */
extern "C" void func_00181D80(void *dest, int size);
/* DNAS.BIN's, called by their addresses (README "The map"): the library started, and shut down */
extern "C" int func_00534830();
extern "C" void func_004C73F0();

/* the A* planner's tables set up again (ai.h's) */
class CAiStar {
public:
    static void Init();
};

/* the overlay a slot holds (the loader's kind: 1 FTSCORE into slot 2, 2 ZSEALETC and 3 DNAS into
 * slot 1 -- 0x3515D0's switch; the names ours) */
enum OVERLAY_KIND {
    OVERLAY_NONE = 0,
    OVERLAY_FTSCORE = 1,
    OVERLAY_ZSEALETC = 2,
    OVERLAY_DNAS = 3
};

class COverlayLoader {
public:
    /* nothing loaded (inline: the initialiser stores the three bytes itself) */
    COverlayLoader()
    {
        m_loaded[0] = OVERLAY_NONE;
        m_loaded[1] = OVERLAY_NONE;
        m_loaded[2] = OVERLAY_NONE;
    }
    /* the kind loaded into its slot unless the slot holds it already; whether it loaded */
    bool Load(OVERLAY_KIND kind);

    OVERLAY_KIND m_loaded[3];   // 0x0: by slot
};   // 3 bytes

/* the loader (0x49E7E8; the name ours) */
COverlayLoader theOverlayLoader;

/* a slot's load address (an inline of ours: its result, numbered as an inline's variable after the
 * function's named locals, takes the saved register the target gives the address -- read into a
 * local, or the table read in the test, it trades registers with the narrowed kind) */
inline void *OverlaySlot(int slot)
{
    return data_001D52A0[slot];
}

/* the overlay's file named and loaded into its slot unless the slot holds it already: from the
 * device when there is one (DNAS from the disc as a plain file), the slot marked on success, DNAS.BIN
 * started and shut down around the inflation of the two others; ZSEALETC's load sets up the A*
 * planner again whatever happened */
bool COverlayLoader::Load(OVERLAY_KIND kind)
{
    bool ok = false;
    const char *name = 0;
    int slot = 0;

    switch (kind) {
    case OVERLAY_ZSEALETC:
        name = "ZSEALETC";
        slot = 1;
        break;
    case OVERLAY_DNAS:
        name = "DNAS";
        slot = 1;
        break;
    case OVERLAY_FTSCORE:
        slot = 2;
        name = "FTSCORE";
        break;
    }

    void *dest = OverlaySlot(slot);

    if (dest && m_loaded[slot] != kind) {
        char path[256] = {0};

        sprintf(path, "%s%s.BIN", "OVERLAY\\REL\\", name);
        if (data_001E70F4 >= 0 && kind != OVERLAY_DNAS) {
            void *buf = _malloc(0x180000, 0, 0);

            func_00534830();
            ok = func_001C60B0(data_001E70F4, name, dest, buf, kind == OVERLAY_ZSEALETC);
            if (ok)
                m_loaded[slot] = kind;
            if (kind != OVERLAY_ZSEALETC)
                func_004C73F0();
            _free(buf, 0, 0);
        } else if (kind == OVERLAY_DNAS) {
            int done = 0;
            int size = 0;
            int got = -1;

            {
                CFileIO f;

                if (f.Open(path, 1)) {
                    size = f.GetSize();
                    if (size)
                        got = f.fread(dest, size);
                    f.Close();
                }
                got = got == size ? got : 0;   /* the target's bne/b pair: an `if` gives beq */
            }
            if (got > 0) {
                func_00181D80(dest, got);
                done = 1;
            }
            ok = done;
            if (ok)
                m_loaded[slot] = kind;
        } else {
            void *buf = _malloc(0x180000, 0, 0);

            func_00534830();
            ok = func_001C5DA0(name, dest, buf, kind == OVERLAY_ZSEALETC);
            if (ok)
                m_loaded[slot] = kind;
            if (kind != OVERLAY_ZSEALETC)
                func_004C73F0();
            _free(buf, 0, 0);
        }
    }
    if (kind == OVERLAY_ZSEALETC)
        CAiStar::Init();
    return ok;
}
