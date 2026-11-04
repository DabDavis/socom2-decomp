/* zsys_overlay.cpp -- the overlays' loader (OVL1 0x3515D0-0x3518BF): which overlay each of the
 * three slots holds, and the load of FTSCORE, ZSEALETC or DNAS into its slot (0x3515D0, not
 * written yet: fts_patch.cpp's CSMDownloadPatch and fts_dnas.cpp's CSMDNASAuthenticate call it on
 * the object below). A unit of its own between zsys_vu.cpp and valve_list.cpp: its initialiser
 * 0x4034D0, between zsys_vu.cpp's bss (0x49E7E0) and valve_list.cpp's, clears the object's three
 * bytes (the integrator's initialiser research). No SOCOM 1 counterpart (SOCOM 1 had no
 * overlays); the unit's, the class's and the members' names are ours. */

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
