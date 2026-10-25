/* ai.h -- the AI core (OVL2): the events and their callbacks, the path planners, the AI map. The
 * layouts are SOCOM 1's DWARF where it has them, moved where SOCOM II's code puts them; each
 * difference is said where it is. The directory is ours (SOCOM 1's DWARF names no AI file; its
 * unit names -- ai_events.cpp, ai_astar.cpp, ai_map.cpp, ai_mapio.cpp, ai_path.cpp, ai_maps.cpp,
 * ai_vismap.cpp -- come from its static initialisers and the press beta's asserts). */
#ifndef AI_H
#define AI_H

#include <algorithm>
#include <list>
#include <vector>

#include "../gamez/zmath/zmath.h"
#include "../gamez/zentity/zentity.h"

class CAiEvent;

/* the AI's small templates (SOCOM 1's namespace ai), declared only: zseal.h defines the same two,
 * so either header can come first; ai_map.cpp, whose tail holds the unsigned short instances
 * (0x519250, 0x519270), defines them as zseal.h does */
namespace ai {
template <class T>
const T &_max(const T &a, const T &b);
template <class T>
const T &_min(const T &a, const T &b);
}

/* the AI's free helpers (SOCOM 1's namespace ai; ai_util.cpp) */
namespace ai {
int get_clock_direction(const CPnt3D *dir);
bool quadratic(float a, float b, float c, float &r1, float &r2);
void elevate(CPnt3D &p, float y, float scale);
CPnt3D *AddNoiseXZ(CPnt3D *p, const PNT2D &range);
CPnt3D *AddNoiseXZ(const CPnt3D *p, CPnt3D *out, const PNT2D &range);
CPnt3D *AddNoise(CPnt3D *p, const PNT2D &range);
CPnt3D *AddNoise(const CPnt3D *p, CPnt3D *out, const PNT2D &range);
CPnt3D *MakeNoiseXZ(CPnt3D *out, const PNT2D &range);
CPnt3D *MakeNoise(CPnt3D *out, const PNT2D &range);
bool is_moving(const CPnt3D *v, float speedSq);
}

/* the AI's aiming helpers (SOCOM 1's CAi, only static functions; ai_util.cpp) */
class CAi {
public:
    static void AlignToPoint(const CMatrix *m, const CPnt3D *p, CMatrix *out, float f);
    static void Extrapolate(const CPnt3D *pos, const CPnt3D *vel, float t, CPnt3D *out);
    static bool LeadTargetG(const CPnt3D *a, const CPnt3D *b, float t, const CPnt3D *c,
                            const CPnt3D *d, CPnt3D *out);
};

/* every event of a kind, or the pool of free ones (SOCOM 1: a list of pointers and nothing more) */
class CAiEventList : public std::list<CAiEvent *> {
};

/* a function told of each event made (SOCOM 1's; the names are ours) */
class CAiEventCallback {
public:
    void (*m_func)(CAiEvent *e, unsigned int data);   // 0x0
    unsigned int m_data;   // 0x4
};

class CAiEventCallbackList : public std::list<CAiEventCallback> {
public:
    /* declared, not defined here: implicit, it calls ~__list_imp<CAiEventCallback> (0x50FFE0),
     * whose target copy calls begin() and end() out of line -- include/list's inlines them (0xB8
     * against 0x80), a list destructor height to settle with the list remodel. Until then the
     * generated draft is 0x5101C0 and its callee */
    ~CAiEventCallbackList();
};

/* an event a SEAL has heard of, by id: two bytes in SOCOM II (SOCOM 1's was eight, a word of
 * flags after the id) -- the list node is 12 bytes and copies it with lhu/sh */
class CAiEventHandle {
public:
    CAiEventHandle(unsigned int id) : m_id(id) {}

    unsigned short m_id;   // 0x0
};

/* the events a SEAL is handling (SOCOM 1's DWARF: a list of handles and a flag) */
class CAiEventQ : public std::list<CAiEventHandle> {
public:
    CAiEventHandle *Create(unsigned int id);

    unsigned char m_diRequested;   // 0xc
};

/* something the AI can notice: a noise, a body, a door, a shot (SOCOM 1's DWARF, 0x28 bytes, the
 * same in SOCOM II). The events live in three lists: the free pool, the ones running down their
 * duration and the ones that never expire */
class CAiEvent {
public:
    /* SOCOM II's thirteen kinds (GetEventString's cases; SOCOM 1 had fifteen in another order:
     * SURRENDER, GETDOWN and HELPATTACK went, HELPME came) -- a signed byte (-enum min, AIE_NONE) */
    enum EVENT {
        AIE_NONE = -1,
        AIE_UNKNOWN = 0,
        AIE_GENERIC = 1,
        AIE_POINT = 2,
        AIE_ENTITY = 3,
        AIE_VOICE = 4,
        AIE_CORPSE = 5,
        AIE_WEPHIT = 6,
        AIE_WEPFIRE = 7,
        AIE_EXPLOSION = 8,
        AIE_HELPME = 9,
        AIE_ALARM = 10,
        AIE_LIGHTSOFF = 11,
        AIE_DOOR = 12,
        AIE_MAX = 13
    };
    /* which list a walk reads (the names are ours) */
    enum EVENT_CLASS { EC_INUSE = 0, EC_PERSIST = 1 };

    /* a walk over one list, filtered (SOCOM 1's CAiEvent::iterator; the names are ours) */
    class iterator {
    public:
        iterator()
        {
            m_list = 0;
            m_type = AIE_MAX;
            m_mask = ~0;
            m_owner = -1;
            m_bit = ~0;
        }
        iterator(CAiEventList *list)
        {
            m_list = list;
            m_it = m_list->begin();
            m_mask = ~0;
            m_owner = -1;
            m_bit = ~0;
            m_type = AIE_MAX;
        }

        CAiEventList *m_list;   // 0x0
        CAiEventList::iterator m_it;   // 0x4
        unsigned int m_bit;   // 0x8: events that have it set are skipped (~0: none)
        int m_owner;   // 0xc: this owner's events are skipped
        EVENT m_type;   // 0x10: the kind wanted (AIE_MAX: any)
        unsigned int m_mask;   // 0x14: the kinds wanted, a bit each
        float m_distSq;   // 0x18: the last event found, its distance squared
    };

    CAiEvent();
    void Initalize();
    bool Tick(float dT);
    bool IsPersistent() const { return m_duration == NoExpire; }
    /* whether a walk has already seen this event: its bit, which an entity's event never has
     * (GetNextEvent's test; the name is ours) */
    bool Seen(unsigned int bit) const { return m_type == AIE_ENTITY ? false : m_bits.Test(bit); }

    static unsigned int MakeMaskEx(EVENT e, ...);
    static CAiEvent *GetNextEvent(const CPnt3D *pos, iterator &it, float scale);
    static iterator GetFirstEvent(EVENT_CLASS c);
    static bool Tick(float dT, void *data);
    static void Release(CAiEvent *e);
    static void Create(CAiEvent *e);
    static CAiEvent *Acquire();
    static void Close();
    static void Open(unsigned int bits);
    static void FreeEvents();
    static EVENT GetEventFromString(const char *s, EVENT def);
    static float GetEventRange(EVENT e);
    static float GetEventRate(EVENT e);
    static const char *GetEventString(EVENT e);

    CPnt3D m_pos;   // 0x0
    float m_rangeSq;   // 0xc: 0 = heard everywhere
    float m_duration;   // 0x10: NoExpire = forever
    int m_owner;   // 0x14
    unsigned int m_id : 16;   // 0x18
    unsigned int m_visible : 1;   // 0x1a
    unsigned int m_type : 15;
    unsigned int m_userdata;   // 0x1c
    CBits m_bits;   // 0x20: which walks have seen it

    static const float NoExpire;
    static unsigned int bit_count;
    static unsigned int nextId;
    static CAiEventList m_pool;
    static CAiEventList m_inuse;
    static CAiEventList m_persist;
    static CAiEventCallbackList m_callbacks;
};

class CAiPath;

/* a cell of an AI map (SOCOM 1's DWARF, 8 bytes; SOCOM II reads the same bits) */
class CAiCell {
public:
    /* whether a step may end in it: not on soils 0, 8, 9 or 10 (new in SOCOM II, the name ours:
     * SOCOM 1's planner tested the soil itself. Called from every AI unit, inlined in the
     * planners' is_traversable; ai_astar.cpp's out-of-line copy is 0x511F40) */
    bool IsTraversable() const
    {
        unsigned int soil = m_soil;

        return !(soil == 0 || soil == 10 || soil == 8 || soil == 9);
    }

    /* every field to its start (SOCOM 1's CAiCell() body, 0x185200; SOCOM II's constructor calls
     * it out of line: fifteen statements and the return are past -inline auto's fifteen) */
    CAiCell();
    void Reset();
    static CAiCell *Alloc(unsigned int n);
    /* the soil (the name ours: read through it, the field is loaded again for a following
     * store -- _reset_cell) */
    unsigned int GetSoil() const { return m_soil; }
    /* the fields' setters: their arguments masked at run time, `(0 & 1) << n` for a constant
     * (the names ours) */
    void SetOccupied(unsigned int b) { m_occupied = b; }
    void SetName(unsigned int b) { m_name = b; }
    void SetAction(unsigned int b) { m_action = b; }
    void SetLink(unsigned int b) { m_link = b; }
    void SetAltitude(unsigned int a) { m_altitude = a; }
    void SetBoundary(unsigned int b) { m_boundary = b; }
    void SetShadow(unsigned int s) { m_shadow = s; }
    void SetInside(unsigned int b) { m_inside = b; }
    void SetBadAlt(unsigned int b) { m_badalt = b; }
    void SetRtFlag(unsigned int b) { m_rtflag = b; }
    void SetBilinear(unsigned int b) { m_bilinear = b; }

    unsigned int m_soil : 5;   // 0x0
    unsigned int m_occupied : 1;
    unsigned int m_name : 1;
    unsigned int m_action : 1;
    unsigned int m_link : 1;
    unsigned int m_altitude : 10;
    unsigned int m_boundary : 2;
    unsigned int m_mes : 1;
    unsigned int m_shadow : 2;
    unsigned int m_inside : 1;
    unsigned int m_badalt : 1;
    unsigned int m_rtflag : 1;
    unsigned int m_bilinear : 1;
    unsigned int m_astar_dir : 4;
    short m_astar_f;   // 0x4
    short m_astar_g;   // 0x6
};

class CRdrFile;
class CFileIO;
/* one layer of an AI map's cells (new in SOCOM II, the name ours: the map keeps the first at 0xA8
 * and the rest by pointer; 0x2C bytes, built by 0x53A9C0). Only the cells are laid out */
class CAiMapLayer {
public:
    unsigned char _0[0x28];
    CAiCell *m_cells;   // 0x28: CAiCell::Alloc's
};
int zrdr_free(CRdrFile *file);
/* MAIN's strncasecmp (0x1993C0, a placeholder still: SOCOM 1's CAiMap::GetLocation calls
 * strncasecmp where SOCOM II's calls it) */
extern "C" int func_001993C0(const char *a, const char *b, unsigned int n);

class CGNode;
class CGEdge;
class CGraph;

/* the AI maps' graph fixups (ai_mapio.cpp; SOCOM 1's, ai_maps.cpp's there) */
void _fixupEdge(CGraph *graph, CGEdge *edge, bool fix, unsigned int &n, void *data);
void _fixupNode(CGraph *graph, CGNode *node, bool fix, unsigned int &n, void *data);

/* the AI maps' graph (graph_tree.cpp's CGraph; only what the maps call) */
class CGraph {
public:
    /* a graph read from a map file, each node and edge handed to a fixup with the caller's data */
    static CGraph *Create(CFileIO &f, void (*node)(CGraph *, CGNode *, bool, unsigned int &, void *),
                          void (*edge)(CGraph *, CGEdge *, bool, unsigned int &, void *), void *data);
    static void Destroy(CGraph *graph);
};

class CAiMap;

/* a cell handed to a walk's callback (below CAiMap: it points at a CAiMap::region) */
struct MAP_FOREACH_STRUCT;

/* two cells are the same cell when x and y agree, the map aside: an inline bool, which the
 * walks materialise (x's test, then y's into the same register) before they branch -- SOCOM 1's
 * code alike. A free function here, CAiMapLoc being the entity lane's (zentity.h) */
inline bool operator==(const CAiMapLoc &a, const CAiMapLoc &b)
{
    return a.x == b.x && a.y == b.y;
}

/* a run of cells (SOCOM 1's DWARF: a vector of CAiMapLoc and nothing more) */
class CAiMapLocVec : public std::vector<CAiMapLoc> {
};

/* an AI map (SOCOM 1's DWARF: CAiMap : AIMAP_PARAMS, 0x12C bytes; SOCOM II's tables sit 0x34
 * further on -- the routes at 0x140, the scanlines at 0x14C, the graph at 0x15C). Only what the
 * code written so far reads */
class CAiMap {
public:
    enum DIR { N = 0, NE = 1, E = 2, SE = 3, S = 4, SW = 5, W = 6, NW = 7, DIR_COUNT = 8 };
    /* a cell's soil (the names ours, from GetSoilString's: SOCOM 1's DWARF has no enum for it) */
    enum SOIL {
        SOIL_NOGO = 0,
        SOIL_DEFAULT = 1,
        SOIL_WATER = 2,
        SOIL_ROAD = 3,
        SOIL_BRUSH = 4,
        SOIL_FOREST = 5,
        SOIL_CLIMB = 6,
        SOIL_STAIRS = 7,
        SOIL_CLUTTER = 8,
        SOIL_TRANS = 9,
        SOIL_NOALT = 10,
        SOIL_STEEP = 11,
        SOIL_GRASS = 12,
        SOIL_COLLISION = 13,
        SOIL_COUNT = 14
    };

    /* a named cell (SOCOM 1's DWARF, 0x18 bytes; SOCOM II's name follows the cell, sixteen bytes
     * with the last kept zero, and the four after it are not read yet) */
    class name {
    public:
        /* the name copied in, cut to fifteen characters (new in SOCOM II, the name ours) */
        void SetName(const char *s);
        /* the name ended at its last byte (SOCOM 1's) */
        void ValidateName() { m_name[15] = 0; }

        CAiMapLoc m_loc;   // 0x0
        char m_name[16];   // 0x4
        unsigned int m_unk14;   // 0x14
    };
    class names : public std::vector<name> {
    };

    /* a cell linked to another (SOCOM 1's DWARF, 8 bytes; SOCOM II's is 0xC, a word of bits after
     * the two cells whose low four a new link sets to 8 -- 0x5309D0) */
    class link {
    public:
        CAiMapLoc m_loc;   // 0x0
        CAiMapLoc m_link;   // 0x4
        unsigned int m_kind : 4;   // 0x8: the name ours
        unsigned int m_unk8 : 28;
    };
    class links : public std::vector<link> {
    };

    /* a rectangle of cells, named and typed (SOCOM 1's DWARF, 0x1C bytes) */
    class region {
    public:
        /* the kinds (seven; 7 asks for any -- GetRegionContaining's test; the names ours) */
        enum TYPE { ANY = 7 };

        CAiMapLoc &GetCenter(CAiMapLoc &loc) const;
        void Union(const CAiMapLoc &loc);
        void InflateRegion(int left, int top, int right, int bottom, const CAiMap *map);
        /* SOCOM 1's Contains, out of line there (0x19E020) and inlined here: the same map, and
         * the cell inside the rectangle -- each test materialised */
        bool Contains(const CAiMapLoc &loc) const
        {
            bool in = false;

            if (loc.m_mapid == m_loc.m_mapid) {
                int dx = loc.x - m_loc.x;
                int dy = loc.y - m_loc.y;

                in = dx >= 0 && dx < (int)m_dx && dy >= 0 && dy < (int)m_dy;
            }
            return in;
        }
        /* every field zeroed but the cell, each bitfield read, masked and written back as a word
         * (the mem-initializer list's way) and the name emptied (new in SOCOM II: SOCOM 1's had no
         * constructor; out of line at 0x517790) */
        region() : m_dx(0), m_dy(0), m_type(0), m_nodisplay(0), m_flags(0), m_unused(0) { m_name[0] = 0; }
        /* the width and height (the names ours: out of line where the walk is inlined one level,
         * 0x5176D0 and 0x517770) */
        unsigned int GetDx() const { return m_dx; }
        unsigned int GetDy() const { return m_dy; }
        /* the width and height, both taken before either is stored (the name ours) */
        void SetSize(unsigned int dx, unsigned int dy)
        {
            m_dx = dx;
            m_dy = dy;
        }
        /* whether the line map shows it (the name ours; materialised where it is tested) */
        bool IsDisplayed() const { return !m_nodisplay; }

        CAiMapLoc m_loc;   // 0x0
        unsigned int m_dx : 16;   // 0x4
        unsigned int m_dy : 16;
        unsigned int m_type : 5;   // 0x8
        unsigned int m_nodisplay : 1;
        unsigned int m_flags : 16;
        unsigned int m_unused : 10;
        char m_name[16];   // 0xc
    };
    class regions : public std::vector<region> {
    };

    /* a cell an AI reads a script from (SOCOM 1's DWARF, 0x10 bytes) */
    class reader {
    public:
        /* the old script freed and the new one kept (SOCOM 1's; inlined into ~readers) */
        void SetReader(CRdrFile *r);

        CAiMapLoc m_loc;   // 0x0
        CRdrFile *m_reader;   // 0x4
        unsigned int m_flags;   // 0x8
        unsigned int m_bytes;   // 0xc
    };
    class readers : public std::vector<reader> {
    public:
        ~readers();
    };

    /* a way between two regions (SOCOM 1's DWARF, 0x24 bytes) */
    class route {
    public:
        unsigned int m_flags;   // 0x0
        CAiMapLoc m_region[2];   // 0x4
        CAiMapLocVec m_waypts;   // 0xc
        CAiMapLocVec m_path;   // 0x18
    };
    class routes : public std::vector<route> {
    };

    /* where a SEAL can appear (SOCOM 1's DWARF, 0xC bytes) */
    class spore {
    public:
        CAiMapLoc m_loc;   // 0x0
        unsigned int m_dir : 4;   // 0x4
        unsigned int m_id : 12;
        unsigned int m_unused : 16;
        float m_altitude;   // 0x8
    };
    class spores : public std::vector<spore> {
    public:
        /* the run of one id's spores in the table, kept sorted by id (new in SOCOM II, the names
         * ours) */
        class range {
        public:
            unsigned int m_id;   // 0x0
            int m_first;   // 0x4
            int m_last;   // 0x8
            int m_count;   // 0xc
        };

        void GetRange(unsigned int id, range &r);
    };

    /* a line drawn on the map: its cells and four bytes of data (SOCOM 1's DWARF, 0x10 bytes) */
    class line_data {
    public:
        unsigned int m_data;   // 0x0
    };
    class line : public CAiMapLocVec, public line_data {
    };
    class lines : public std::vector<line> {
    };

    /* one row's run of cells (SOCOM 1's DWARF, 0xC bytes; SOCOM II keeps the run's first cell as
     * an index where SOCOM 1 kept pointers to the cells and the extra data) */
    class scanline {
    public:
        /* the cell's index, or -1 off the run (the name ours) */
        int GetIndex(unsigned int x) const
        {
            int i = -1;

            if (x >= m_start && x < m_end)
                i = m_index + (int)(x - m_start);
            return i;
        }

        unsigned short m_start;   // 0x0
        unsigned short m_end;   // 0x2
        int m_index;   // 0x4
        unsigned int m_unk8;   // 0x8
    };
    class scanlines : public std::vector<scanline> {
    };

    /* a cell's action: a door, a ladder, a switch (SOCOM 1's DWARF, 0x28 bytes; SOCOM II reads
     * the same bits) */
    class action {
    public:
        /* the kind, none once it is marked invalid (the name ours) */
        enum TYPE { NONE = 0, DOOR = 1, TYPE_MAX = 31 };

        TYPE GetType() const { return m_type_invalid == 0 ? (TYPE)m_type : NONE; }
        /* the setters (the names ours: masked at run time) */
        void SetState(unsigned int s) { m_state = s; }
        void SetTypeInvalid(unsigned int b) { m_type_invalid = b; }

        CAiMapLoc m_loc;   // 0x0
        unsigned int m_direction : 4;   // 0x4
        unsigned int m_actionId : 16;
        unsigned int m_state : 3;
        unsigned int m_type : 5;
        unsigned int m_locExValid : 1;
        unsigned int m_offset : 4;   // 0x8
        unsigned int m_type_invalid : 1;
        CAiMapLoc m_locEx[6];   // 0xc
        CAiMapLoc m_region;   // 0x24
    };
    class actions : public std::vector<action> {
    };

    /* whether the row is in the map (the name ours; materialised, then the column's test) */
    bool IsRow(unsigned short y) const { return y >= 0 && y < m_ny; }
    bool IsCell(const CAiMapLoc &loc) const { return IsRow(loc.y) && loc.x < m_nx; }
    /* the entry of a name (the name ours: GetLocation's inline walk, which tests the name again) */
    name *FindName(const char *s)
    {
        if (s) {
            for (names::iterator i = m_namesTable.begin(); i != m_namesTable.end(); ++i)
                if (!func_001993C0((*i).m_name, s, 15))
                    return &*i;
        }
        return 0;
    }

    static const char *GetSoilString(SOIL soil);
    static SOIL GetSoilByName(const char *name);
    static DIR GetClosestDir(const CPnt3D *v);
    static DIR GetClosestDir(const CAiMapLoc &from, const CAiMapLoc &to);
    static int GetDirDelta(DIR a, DIR b);
    /* the whole map as a region: its first cell, its width and height (SOCOM 1's) */
    void GetMapRegion(region &r) const;
    /* the map's id (new in SOCOM II's layout, SOCOM 1's m_mapid; the name ours: ai_astar.cpp's
     * copy 0x5120A0) */
    int GetMapId() const { return m_mapid; }
    /* a layer, none for an index it has not, and its cells (new in SOCOM II, the names ours:
     * GetLayerCells' copy 0x517700) */
    CAiMapLayer *GetLayer(int layer) { return layer >= 0 && layer < m_layers.size() ? m_layers[layer] : 0; }
    CAiCell *GetLayerCells(int layer)
    {
        CAiMapLayer *l = GetLayer(layer);

        return l ? l->m_cells : 0;
    }
    /* every cell of the map handed to a callback, row by row, until one returns false (SOCOM
     * 1's foreach, the name ours): too big to inline whole, so a caller inlines it one level and
     * calls its pieces out of line -- the region's constructor and size, the walk struct's
     * constructor, the scanlines' size and [], the layer's cells, _saturate, _max, _min */
    bool ForEachCell(bool (*f)(const MAP_FOREACH_STRUCT &s, void *data), void *data, int layer);
    bool ResetCells();
    static CPnt3D &GetDirVec(DIR dir, CPnt3D &v);
    static CQuat &GetDirQuat(DIR dir, CQuat &q, bool reverse);
    static float GetDirCos(DIR a, DIR b);
    static bool _reset_rtflag(const MAP_FOREACH_STRUCT &s, void *data);
    static bool _reset_cell(const MAP_FOREACH_STRUCT &s, void *data);
    /* bit 1 of the flags: the map changed since it was read (the names ours) */
    bool IsModified() const { return (m_flags & 2) != 0; }
    bool SetModified(bool modified);
    int GetRouteIndex(const route *r);
    route *GetRoute(unsigned int i);
    link *GetCellLink(const CAiMapLoc &loc);
    bool Read(CFileIO &f);
    bool read_scanlines(CFileIO &f);
    bool read_v19(CFileIO &f);
    bool read_v20(CFileIO &f);
    bool read_v21(CFileIO &f);
    bool read_v22(CFileIO &f);
    bool read_v23(CFileIO &f);
    bool read_lines(CFileIO &f);
    bool read_routes(CFileIO &f);
    bool read_readers(CFileIO &f);
    void DestroyGraph();
    void CreateGraph(bool force);
    void ValidateRoutes(bool force);
    void ValidateCells();
    bool FindActionLocation(unsigned int id, CAiMapLoc &loc, action **pa);
    action *GetCellAction(const CAiMapLoc &loc) const;
    bool GetLocation(const char *name, CAiMapLoc &loc);
    name *GetCellName(const CAiMapLoc &loc);
    region *GetRegion(const char *name, CAiMapLoc &loc);
    region *GetRegion(const CAiMapLoc &loc);
    region *GetRegionContaining(const CAiMapLoc &loc, region::TYPE type, bool displayed);
    action *GetCellAction(const CAiMapLoc &loc);
    char *GetMapEx(const CAiMapLoc &loc);
    void FreeEx();
    char *AllocEx(unsigned int bytes);

    /* AIMAP_PARAMS, the file's header (SOCOM 1's DWARF; the same in SOCOM II as far as read) */
    CBBox m_bbox;   // 0x0
    unsigned int m_nx;   // 0x18
    unsigned int m_ny;   // 0x1c
    float m_dx;   // 0x20
    float m_dy;   // 0x24
    float m_idx;   // 0x28
    float m_idy;   // 0x2c
    int m_floor;   // 0x30
    char m_name[32];   // 0x34
    float m_resolution;   // 0x54
    unsigned int m_flags;   // 0x58
    float m_slope_threshold;   // 0x5c
    unsigned int m_mapex_bytes;   // 0x60
    float m_light[2];   // 0x64
    unsigned int m_compressed;   // 0x6c
    unsigned int m_cellcount;   // 0x70
    unsigned int m_revision;   // 0x74
    unsigned int m_date[1];   // 0x78
    unsigned int m_reserved0;   // 0x7c
    unsigned int m_layercount;   // 0x80: the map's layers (new in SOCOM II, SOCOM 1's m_reserved[1])
    unsigned int m_reserved[8];   // 0x84
    /* CAiMap's own (SOCOM II's: a layer object at 0xA8 that the constructor builds and pushes onto
     * the pointer vector at 0xD4, the cells in the layers; the extra data, the owner and the
     * altitude scale after it, then SOCOM 1's nine tables) */
    unsigned int m_version;   // 0xa4
    unsigned char m_layer0[0x2c];   // 0xa8: not laid out yet (built by 0x53A9C0)
    std::vector<CAiMapLayer *> m_layers;   // 0xd4: the layers, m_layer0 first
    char *m_mapex;   // 0xe0: SOCOM 1's 0xac
    int m_mapid;   // 0xe4: SOCOM 1's 0xb0 (ValidateCells gives every name this map's id)
    float m_alt_quant;   // 0xe8: SOCOM 1's 0xb4
    names m_namesTable;   // 0xec
    actions m_actionTable;   // 0xf8
    links m_linksTable;   // 0x104
    regions m_regionTable;   // 0x110
    spores m_sporeTable;   // 0x11c
    lines m_lineTable;   // 0x128
    readers m_readerTable;   // 0x134
    routes m_routeTable;   // 0x140
    scanlines m_scanlines;   // 0x14c
    void *m_overlay;   // 0x158
    CGraph *m_graph;   // 0x15c
};

/* a cell handed to a walk's callback (SOCOM 1's MAP_FOREACH_STRUCT, which its DWARF does not lay
 * out: the map, the cell and its location as the callbacks read them) */
struct MAP_FOREACH_STRUCT {
    /* empty, and called out of line where the walk is inlined one level (0x517780) */
    MAP_FOREACH_STRUCT() {}

    CAiMap *m_map;   // 0x0
    CAiCell *m_cell;   // 0x4
    const CAiMap::region *m_region;   // 0x8: the region walked
    CAiMapLoc m_loc;   // 0xc
};

/* every cell of the map handed to a callback, row by row, until one returns false (SOCOM 1's
 * foreach, the name ours): too big to inline whole, so a caller inlines it one level and calls its
 * pieces out of line -- the region's constructor and size, GetMapRegion, the walk struct's
 * constructor, the scanlines' size and [], the layer's cells, _saturate, _max, _min. The region is
 * read through the walk struct, so after each call it is read again */
inline bool CAiMap::ForEachCell(bool (*f)(const MAP_FOREACH_STRUCT &s, void *data), void *data, int layer)
{
    region r;

    GetMapRegion(r);
    MAP_FOREACH_STRUCT s;
    const region *pr = &r;

    s.m_map = this;
    s.m_region = pr;
    s.m_loc.m_mapid = GetMapId();
    int y0 = pr->m_loc.y;
    int y1 = pr->m_loc.y + pr->GetDy();

    _saturate(&y0, 0, (int)m_scanlines.size());
    _saturate(&y1, 0, (int)m_scanlines.size());
    CAiCell *cells = GetLayerCells(layer);

    if (cells) {
        for (int y = y0; y < y1; y++) {
            scanline &sl = m_scanlines[y];

            s.m_loc.y = y;
            int x0 = ai::_max(sl.m_start, (unsigned short)pr->m_loc.x);
            int x1 = ai::_min(sl.m_end, (unsigned short)(pr->m_loc.x + pr->GetDx()));

            for (int x = x0; x < x1; x++) {
                s.m_loc.x = x;
                s.m_cell = &cells[sl.m_index + (x - sl.m_start)];
                if (!f(s, data))
                    return false;
            }
        }
    }
    return true;
}

class CAiStar;
class CAiOverlay;
class CAiPlanner;

/* the maps a path crosses (SOCOM 1's DWARF: a vector of CAiMap pointers and nothing more) */
class CAiMapVec : public std::vector<CAiMap *> {
};

/* a search's tallies (SOCOM 1's DWARF, 0x34 bytes; the same in SOCOM II) */
class CAiPathStats {
public:
    CAiPathStats() { reset(); }
    /* every count zeroed and the visited cells dropped (SOCOM 1's reset, out of line there
     * (0x1968E0); SOCOM II's plan_setup and Clear zero the nine words in place) */
    void reset()
    {
        nodes_searched = 0;
        nodes_added = 0;
        nodes_removed = 0;
        nodes_visited = 0;
        nodes_left = 0;
        path_length = 0;
        path_cost = 0;
        m_ticks = 0;
        m_time = 0.0f;
        m_visited.clear();
    }

    int nodes_searched;   // 0x0
    int nodes_added;   // 0x4
    int nodes_removed;   // 0x8
    int nodes_visited;   // 0xc
    int nodes_left;   // 0x10
    int path_length;   // 0x14
    int path_cost;   // 0x18
    int m_ticks;   // 0x1c
    float m_time;   // 0x20
    CAiMapLoc m_current;   // 0x24: left unbuilt (CAiMapLoc() is empty in SOCOM II)
    /* a plain vector (SOCOM 1's DWARF: a CAiMapLocVec): CAiPath's constructor inlines the stats'
     * whole, one level shallower than through CAiMapLocVec's implicit constructor */
    std::vector<CAiMapLoc> m_visited;   // 0x28
};

/* a planned path (SOCOM 1's DWARF, 0x9C bytes; SOCOM II's is 0xA8: a float at 0x18 -- the
 * planner queue's priority -- and words at 0x24 and 0x90 are new, so the flags sit 4 further on,
 * the legs and everything to the search limit 8, the snake and the points 0xC -- SetTemplatePath
 * (0x527300) still stores 0x10, Clear and SetSnakeLength put the rest where said) */
class CAiPath {
public:
    /* the planner a path takes (SOCOM 1's DWARF, a byte) */
    enum PATH_TYPE { PATH_DEFAULT = 0, PATH_BASIC = 1, PATH_STEALTH = 2, PATH_RESTRAINED = 3 };
    /* where its plan stands (SOCOM 1's DWARF, a byte) */
    enum PATH_STATE { UNKNOWN = 0, INQUEUE = 1, PLANNING = 2, ABORT = 3, COMPLETE = 4, FAILED = 5 };
    /* how the search ended (SOCOM 1's DWARF, a signed byte) */
    enum ASTAR_STATUS {
        ASTAR_STATUS_UNKNOWN = -1,
        ASTAR_SUCCESS = 0,
        ASTAR_FAILURE_NOPATH = 1,
        ASTAR_FAILURE_MANYTICKS = 2,
        ASTAR_FAILURE_TOOLONG = 3,
        ASTAR_FAILURE_NONEOPEN = 4,
        ASTAR_FAILURE_OUTOFTIME = 5,
        ASTAR_FAILURE_UNKNOWN = 6
    };

    /* a cell of the smoothed path (SOCOM 1's DWARF, 0xC bytes) */
    class point {
    public:
        CAiMapLoc m_loc;   // 0x0
        unsigned int m_width : 8;   // 0x4
        unsigned int m_flags : 8;
        unsigned int m_unused : 16;
        float m_angle;   // 0x8
    };
    class points : public std::vector<point> {
    };

    CAiPath();

    /* queued, planning or aborting (SOCOM 1 tested the three states one by one; SOCOM II's code,
     * inlined everywhere, is one unsigned range test -- the name ours) */
    bool IsPlanning() const { return (unsigned int)(m_state - 1) < 3; }
    /* the queue's order (new in SOCOM II, the name ours; out of line at 0x510C20) */
    float GetPriority() const { return m_priority; }

    /* the planner object let go: its search always, the planner itself only when the path owns
     * it and `all` (SOCOM 1's took no flag) */
    void DeletePlanner(bool all);
    CAiStar *CreatePlanner();
    /* a planner of the caller's own, which the path then owns (new in SOCOM II, the name ours;
     * an AI state hands it a CAiHidePlanner) */
    void SetPlanner(CAiPlanner *planner);
    bool Clear();
    void SetTemplatePath(CAiPath *path);
    void PostProcess();
    void UpdatePathWidth(unsigned int width);
    /* the path drawn for debugging, under a name (SOCOM 1's took none) */
    void PrintPath(const char *name) const;
    void plan_setup();
    bool plan_addleg(CAiMap *map, const CAiMapLoc &start, const CAiMapLoc &goal);
    bool plan_legcomplete(PATH_STATE state);
    void plan_complete(PATH_STATE state);
    CAiMap *GetLegMap(int leg);
    bool GetLegPoints(CAiMapLoc &start, CAiMapLoc &goal, int leg) const;

    CAiStar *m_astar;   // 0x0
    CAiPlanner *m_planner;   // 0x4
    unsigned char m_custom;   // 0x8: the planner is the path's own
    PATH_TYPE m_type;   // 0x9
    float m_type_bias;   // 0xc
    CAiPath *m_template;   // 0x10
    CAiOverlay *m_overlay;   // 0x14
    float m_priority;   // 0x18: new in SOCOM II
    unsigned int m_flags;   // 0x1c: SOCOM 1's 0x18
    unsigned int m_app_flags;   // 0x20
    unsigned int m_unk24;   // 0x24: new in SOCOM II
    unsigned int m_legcount;   // 0x28: SOCOM 1's 0x20
    unsigned int m_planleg;   // 0x2c
    unsigned int m_curleg;   // 0x30
    CAiMapLocVec m_waypts;   // 0x34
    CAiMapVec m_maps;   // 0x40
    CAiPathStats m_stats;   // 0x4c
    PATH_STATE m_state;   // 0x80: SOCOM 1's 0x78
    ASTAR_STATUS m_status;   // 0x81
    float m_length;   // 0x84
    unsigned int m_tolerance;   // 0x88
    unsigned int m_search_limit;   // 0x8c: SOCOM 1's 0x84
    unsigned int m_unk90;   // 0x90: new in SOCOM II
    unsigned int m_snake_len;   // 0x94: SOCOM 1's 0x88
    points m_points;   // 0x98: SOCOM 1's 0x8c
    unsigned int m_pathId;   // 0xa4
};

/* the A* search of one leg (SOCOM 1's was 0x2C bytes, its open and closed lists two vectors of
 * nodes; SOCOM II's is 0x14 -- the lists became the unit's statics -- and CreatePlanner inlines
 * its constructor: the planner, the leg's map, its two ends, the path's tallies. The ends are
 * CAiMapLocs left unbuilt: CreatePlanner is byte-exact only so) */
class CAiStar {
public:
    CAiStar(CAiPath *path) : m_planner(path->m_planner)
    {
        m_map = path->GetLegMap(-1);
        m_stats = &path->m_stats;
        path->GetLegPoints(m_start, m_goal, -1);
    }
    ~CAiStar();
    void find_path(CAiPath *path);

    CAiPlanner *m_planner;   // 0x0
    CAiMap *m_map;   // 0x4
    CAiMapLoc m_start;   // 0x8
    CAiMapLoc m_goal;   // 0xc
    CAiPathStats *m_stats;   // 0x10
};

/* the planner queue (SOCOM 1's ai::path_queue, 24 bytes: a list of paths and three counters; the
 * names ours) */
namespace ai {
class CAiPathQueue : public std::list<CAiPath *> {
public:
    /* whether the path is queued (the name ours: AbortPathPlan's third arm, which calls begin(),
     * end(), std::find and != out of line -- an inline too tall to inline whole) */
    bool Contains(CAiPath *path) { return std::find(begin(), end(), path) != end(); }

    unsigned int m_maxSize;   // 0xc: the longest the queue has been
    unsigned int m_completed;   // 0x10: paths planned to their end
    unsigned int m_ticks;   // 0x14: searches run
};

extern CAiPathQueue path_queue;

bool PathPlanImmediate(CAiMap *map, const CAiMapLoc &start, const CAiMapLoc &goal, CAiPath *path);
bool TickPathPlanner(float dT, void *data);
void ResetPathPlan();
bool AbortPathPlan(CAiPath *path);
void PathPlanExecute();
void PathPlanAbort();
bool PathPlanAddLeg(CAiMap *map, const CAiMapLoc &start, const CAiMapLoc &goal);
bool PathPlanBegin(CAiPath *path);
void FlushPlannerQueue();
}

/* how the A* search weighs a step (SOCOM 1's; SOCOM II's vtable has a slot more after reset, and
 * its subclasses are CBasicPlanner, CStealthPlanner and CRestrainedPlanner where SOCOM 1's were
 * the *Travel classes). No data: CreatePlanner's statics are built by their vtable store alone.
 * fcost returns a float ($f0: CBasicPlanner's adds to the base's result). The three fcosts, the
 * A* loop and ~CAiStar read cells through an inline (a CAiMap::GetCell) that the target inlines
 * ONE level: its callees are called out of line -- 0x512140 (the cell on the map: its row, then
 * the row's run), 0x512120 (a layer index below m_layers' size), 0x512100 (the scanlines' [],
 * 12-byte elements), 0x5120C0 (scanline::GetIndex), 0x5120B0 (m_layers' [], the layer's cells at
 * +0x28, 8 bytes each) -- the cell left 0 when either test fails */
class CAiPlanner {
public:
    virtual void reset();
    /* whether two cells are the same cell, the map aside (new in SOCOM II; the name is ours) */
    virtual bool is_goal(CAiMap &map, const CAiMapLoc &a, const CAiMapLoc &b);
    virtual int dist(CAiMap &map, const CAiMapLoc &a, const CAiMapLoc &b);
    virtual bool is_traversable(CAiMap *map, CAiPath *path, const CAiMapLoc &from, CAiMap::DIR dir,
                                CAiMapLoc &to, CAiCell *&cell);
    virtual float fcost(CAiMap &map, const CAiMapLoc &from, CAiMap::DIR dir, const CAiMapLoc &to);
};

/* a planner that keeps off doors (SOCOM 1's CRestrainedTravel) */
class CRestrainedPlanner : public CAiPlanner {
public:
    virtual bool is_traversable(CAiMap *map, CAiPath *path, const CAiMapLoc &from, CAiMap::DIR dir,
                                CAiMapLoc &to, CAiCell *&cell);
};

/* a planner that keeps to the shadows (SOCOM 1's CStealthTravel) */
class CStealthPlanner : public CAiPlanner {
public:
    virtual float fcost(CAiMap &map, const CAiMapLoc &from, CAiMap::DIR dir, const CAiMapLoc &to);
};

/* a planner that keeps to paths (SOCOM 1's CBasicTravel) */
class CBasicPlanner : public CAiPlanner {
public:
    virtual float fcost(CAiMap &map, const CAiMapLoc &from, CAiMap::DIR dir, const CAiMapLoc &to);
};

#endif
