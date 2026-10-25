/* ai.h -- the AI core (OVL2): the events and their callbacks, the path planners, the AI map. The
 * layouts are SOCOM 1's DWARF where it has them, moved where SOCOM II's code puts them; each
 * difference is said where it is. The directory is ours (SOCOM 1's DWARF names no AI file; its
 * unit names -- ai_events.cpp, ai_astar.cpp, ai_map.cpp, ai_mapio.cpp, ai_path.cpp, ai_maps.cpp,
 * ai_vismap.cpp -- come from its static initialisers and the press beta's asserts). */
#ifndef AI_H
#define AI_H

#include <list>
#include <vector>

#include "../gamez/zmath/zmath.h"
#include "../gamez/zentity/zentity.h"

class CAiEvent;

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
int zrdr_free(CRdrFile *file);

/* the AI maps' graph (graph_tree.cpp's CGraph; only what the maps call) */
class CGraph {
public:
    static void Destroy(CGraph *graph);
};

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

    /* a named cell (SOCOM 1's DWARF, 0x18 bytes) */
    class name {
    public:
        CAiMapLoc m_loc;   // 0x0
        float m_altitude;   // 0x4
        char m_name[15];   // 0x8
        unsigned char m_type;   // 0x17
    };
    class names : public std::vector<name> {
    };

    /* a cell linked to another (SOCOM 1's DWARF, 8 bytes) */
    class link {
    public:
        CAiMapLoc m_loc;   // 0x0
        CAiMapLoc m_link;   // 0x4
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

    /* a cell's action: a door, a ladder, a switch (SOCOM 1's DWARF, 0x28 bytes; SOCOM II reads
     * the same bits) */
    class action {
    public:
        /* the kind, none once it is marked invalid (the name ours) */
        enum TYPE { NONE = 0, DOOR = 1, TYPE_MAX = 31 };

        TYPE GetType() const { return m_type_invalid == 0 ? (TYPE)m_type : NONE; }

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

    void DestroyGraph();
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
    unsigned int m_reserved[10];   // 0x7c
    /* CAiMap's own (SOCOM II's: a layer object at 0xA8 that the constructor builds and pushes onto
     * the pointer vector at 0xD4, the cells in the layers; the extra data, the owner and the
     * altitude scale after it, then SOCOM 1's nine tables) */
    unsigned int m_version;   // 0xa4
    unsigned char m_layer0[0x2c];   // 0xa8: not laid out yet (built by 0x53A9C0)
    unsigned char m_layers[0xc];   // 0xd4: a vector of layer pointers
    char *m_mapex;   // 0xe0: SOCOM 1's 0xac
    void *m_maps;   // 0xe4: the constructor's argument
    float m_alt_quant;   // 0xe8: SOCOM 1's 0xb4
    names m_namesTable;   // 0xec
    actions m_actionTable;   // 0xf8
    links m_linksTable;   // 0x104
    regions m_regionTable;   // 0x110
    unsigned char m_sporeTable[0xc];   // 0x11c: spores, not laid out yet
    unsigned char m_lineTable[0xc];   // 0x128: lines, not laid out yet
    readers m_readerTable;   // 0x134
    routes m_routeTable;   // 0x140
    unsigned char m_scanlines[0xc];   // 0x14c: scanlines, not laid out yet
    void *m_overlay;   // 0x158
    CGraph *m_graph;   // 0x15c
};

/* a planned path (SOCOM 1's DWARF, 0x9C bytes; SOCOM II's members from the search limit on sit
 * 8 further). Only what the code written so far reads; the rest is a hole */
class CAiPath {
public:
    unsigned char m_unk0[0x8c];   // 0x0: not laid out yet
    unsigned int m_search_limit;   // 0x8c: SOCOM 1's 0x84
};

/* how the A* search weighs a step (SOCOM 1's; SOCOM II's vtable has a slot more after reset, and
 * its subclasses are CBasicPlanner, CStealthPlanner and CRestrainedPlanner where SOCOM 1's were
 * the *Travel classes) */
class CAiPlanner {
public:
    virtual void reset();
    /* whether two cells are the same cell, the map aside (new in SOCOM II; the name is ours) */
    virtual bool is_goal(CAiMap &map, const CAiMapLoc &a, const CAiMapLoc &b);
    virtual int dist(CAiMap &map, const CAiMapLoc &a, const CAiMapLoc &b);
    virtual bool is_traversable(CAiMap *map, CAiPath *path, const CAiMapLoc &from, CAiMap::DIR dir,
                                CAiMapLoc &to, CAiCell *&cell);
    virtual int fcost(CAiMap &map, const CAiMapLoc &from, CAiMap::DIR dir, const CAiMapLoc &to);
};

/* a planner that keeps off doors (SOCOM 1's CRestrainedTravel) */
class CRestrainedPlanner : public CAiPlanner {
public:
    virtual bool is_traversable(CAiMap *map, CAiPath *path, const CAiMapLoc &from, CAiMap::DIR dir,
                                CAiMapLoc &to, CAiCell *&cell);
};

#endif
