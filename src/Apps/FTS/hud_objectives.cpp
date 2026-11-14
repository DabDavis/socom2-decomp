/* hud_objectives.cpp -- the objectives list (OVL1 0x205C40-0x208ACB): SOCOM 1's unit (its DWARF names
 * it, Z:\dev\Apps\FTS\hud_objectives.cpp). Every CZObjectiveList member byte-exact (FreeList,
 * RefreshList and AddObjective since the HUD's round 47, with FreeList's FreeVector pieces); its
 * tail is vector<bool>'s out-of-line members (0x207F60-0x208A8B) and the two array destructors.
 * Defined in reverse of their image order (-inline deferred). */

#include <algorithm>
#include <list>

#include "../../gamez/ztwod/ztwod.h"
#include "../../gamez/zvid/zvid.h"
#include "hud_missioncams.h"
#include "../../gamez/zsys/zsys.h"

/* the HUD: its menu state and mission cameras placed (SOCOM II's offsets) */
class CHUD {
public:
    unsigned char _0[Z_CHUD_OFS(0x498)];
    C2DFont *m_myfont;   // 0x498
    unsigned char _49c[Z_CHUD_OFS(0x1a220) - Z_CHUD_OFS(0x49c)];
    MENU_STATE m_menustate;   // 0x1a220
    unsigned char _1a221[Z_CHUD_OFS(0x1a2dc) - Z_CHUD_OFS(0x1a221)];
    CZHudMissionCams m_missioncams;   // 0x1a2dc
};

extern CHUD *theHUD;

namespace zdb {
class CWorld {
public:
    static CTexHandle *GetTexHandle(const char *name);
};
}

/* the sound system's tick (its static PostTick, SOCOM 1's CSnd::PostTick(float, void *)) */
class CSnd {
public:
    static void PostTick(float dT, void *data);
};

/* how fast the selected name pulses (data 0x3DC460: a variable, read with lwc1) */
static float s_pulserate = 1000.0f;
/* an unselected name's alpha (data 0x3DC450) */
/* a check box's alpha (data 0x3DC458) */
static float s_checktrans = 96.0f;
static float s_unselectedtrans = 80.0f;
/* a completed objective's name's alpha (data 0x3DC448) */
static float s_completetrans = 128.0f;
/* the map frame's alpha (data 0x3DC440: a variable, read with lwc1) */
static float s_frametrans = 128.0f;

/* the unit's statics, OVL1 bss 0x4090F0-0x409ECF, in the order the unit's initialiser
 * (__sinit_hud_objectives.cpp, 0x3FDBC0) builds them, each after its destructor record: the
 * darkened screen, the map's frame, its box and four edges, the title bars, the strings and the
 * boxes RefreshStatics lays out, and the four colours the items' tick 0x207A00 picks from (12-byte
 * CPnt3Ds, 16 apart, stored by the initialiser) -- in SOCOM 1's anonymous namespace (its
 * g_bmpMapTitleBg and g_bmpLegendBg are these two bars' names there); the other names ours */
namespace {
C2DBitmapPoly g_polyScreen;
C2DBitmap g_bmpMapFrame;
C2DBitmap g_bmpMapBox;
C2DBitmap g_bmpMapEdges[4];
C2DBitmap g_bmpMapTitleBg;
C2DString g_strMapTitle;
C2DBitmap g_bmpObjectivesBg;
C2DBitmap g_bmpMovieBg;
C2DString g_strPlayMovie;
C2DBitmap g_bmpObjectivesTitleBg;
C2DString g_strObjectivesTitle;
C2DBitmap g_bmpLegendBg;
C2DString g_strControls[6];
CPnt3D g_colComplete(32.0f, 64.0f, 32.0f);
CPnt3D g_colNormal(128.0f, 128.0f, 128.0f);
CPnt3D g_colUnavailable(72.0f, 72.0f, 72.0f);
CPnt3D g_colSelected(36.0f, 64.0f, 86.0f);
}

/* the mission: only its objectives (theMission, fts_state.h's CMission at 0x1854) */
class CMissionCam {
public:
    char *m_identifier;   // 0x0
    char *m_name;   // 0x4
    unsigned char _8[0x1c - 0x8];
};

/* an objective: what the list reads (fts_objectivecam.cpp's, SOCOM 1's DWARF) */
class CMissionObjectiveCam : public CMissionCam {
public:
    bool IsInvalid() const;
    bool IsDisplayOn() const;
    bool IsComplete() const;

    unsigned char _1c[0x40 - 0x1c];
    unsigned int m_isSecondary : 1;   // 0x40
    unsigned int m_isSubObjective : 1;   // 0x40
    unsigned int m_isCompass : 1;   // 0x40
    unsigned int m_bExtraction : 1;   // 0x40
    unsigned int m_bit4 : 1;   // 0x40: SOCOM II's
    unsigned int m_unused : 27;   // 0x40
};

class CMission {
public:
    unsigned char _0[0x1854];
    std::list<CMissionObjectiveCam *> m_objectives;   // 0x1854

    /* the mission's objectives ticked (SOCOM 1's CMission::TickObjectives(float)) */
    void TickObjectives(float dT);
};

extern CMission theMission;

/* MSL's vector<bool> as the list's code shows it: the implicit destructor deletes its words (one
 * level), and an element read calls test_bit, out of line in the unit's tail (0x207F60, as SOCOM
 * 1's reference does); its swap and insert there are not modelled here */
namespace std {
template <>
class vector<bool, allocator<bool> > {
public:
    /* an element as SOCOM 1's DWARF has it: the vector and the index, read through test_bit */
    class reference {
    public:
        reference(vector &v, unsigned int n) : vec_(&v), n_(n) {}
        operator bool() const { return vec_->test_bit(n_); }

        vector *vec_;
        unsigned int n_;
    };

    /* a position as SOCOM 1's DWARF has it: the vector and the index */
    class iterator {
    public:
        iterator(vector *v, unsigned int n) : vec_(v), n_(n) {}
        iterator &operator+=(int n)
        {
            n_ += n;
            return *this;
        }

        vector *vec_;
        unsigned int n_;
    };

    vector() : capacity_(0), size_(0), data_(0) {}
    iterator begin() { return iterator(this, 0); }
    iterator end() { return iterator(this, size_); }
    void push_back(const bool &x) { insert(end(), x); }
    iterator insert(iterator position, const bool &x);
    void insert(iterator position, unsigned int n, const bool &x);
    unsigned int size() const { return size_; }
    unsigned int capacity() const { return capacity_; }
    unsigned int max_size() const;
    static unsigned int num_words(unsigned int capacity) { return (capacity - 1) / 32 + 1; }
    void set(unsigned int pos, unsigned int n, bool x);
    void copy_bits(const unsigned int *frmdata, unsigned int first, unsigned int last, unsigned int to);
    void shift_out(iterator pos, unsigned int n);
    static unsigned int *allocate(unsigned int n)
    {
        unsigned int *p = (unsigned int *)operator new(n * sizeof(unsigned int));

        if (!p)
            __msl_error("Memory allocation failure");
        return p;
    }

    ~vector() { if (data_) operator delete(data_); }
    reference operator[](unsigned int n) { return reference(*this, n); }
    void clear() { size_ = 0; }
    void swap(vector &x);
    bool test_bit(unsigned int n);

    unsigned int capacity_;
    unsigned int size_;
    unsigned int *data_;
};

#pragma auto_inline off
/* the bits from a position on moved n up, the words first, then the bits, the word the position
 * is in keeping the bits below it (0x208600, SOCOM 1's -- its DWARF's locals, its line table's
 * statements -- with 32-bit words; MW unrolls both loops 8x here). The first five declared at the
 * top, numbered there (floormask, the word shift, offset, the counter, e: the target's t8, t7, t6,
 * t5, t4), and computed in the order the target schedules them (the word shift, the mask, e, the
 * offset: defined where they are declared the schedule changes) */
void vector<bool, allocator<bool> >::shift_out(iterator pos, unsigned int n)
{
    unsigned int floormask;
    unsigned int wshift;
    unsigned int offset;
    unsigned int i;
    unsigned int e;

    wshift = n / 32;
    floormask = ~0u << pos.n_ % 32;
    e = (size_ + n - 1) / 32 + 1;
    offset = pos.n_ / 32 + wshift;
    if (wshift > 0) {
        for (i = e - 1; i >= offset; --i)
            data_[i] = data_[i - wshift];
    }
    unsigned int rshift, lshift;

    lshift = n % 32;
    rshift = 32 - lshift;

    for (unsigned int i = e - 1; i > offset; --i) {
        data_[i] <<= lshift;
        data_[i] |= data_[i - 1] >> rshift;
    }
    if (wshift > 0 || floormask == ~0u)
        data_[offset] <<= lshift;
    else {
        unsigned int tmp = data_[offset];

        data_[offset] <<= lshift;
        data_[offset] &= floormask;
        data_[offset] |= ~floormask & tmp;
    }
}

/* the bits [first, last) of other words copied in at to, a word's run at a time (0x208510,
 * SOCOM 1's -- its DWARF's locals in their order, its line table's statements -- with 32-bit
 * words) */
void vector<bool, allocator<bool> >::copy_bits(const unsigned int *frmdata, unsigned int first,
                                                unsigned int last, unsigned int to)
{
    while (first != last) {
        unsigned int len = last - first;
        unsigned int o1 = first % 32;
        unsigned int o2 = to % 32;
        unsigned int tmp = 32 - o1;

        if (tmp < len)
            len = tmp;
        tmp = 32 - o2;
        if (tmp < len)
            len = tmp;
        unsigned int lmask = ~0u << o1;
        unsigned int rmask = ~0u;

        tmp = 32 - (first + len) % 32;
        if (tmp < 32)
            rmask >>= tmp;
        unsigned int mask = lmask & rmask;
        unsigned int bits = frmdata[first / 32] & mask;

        if (o1 < o2) {
            bits <<= o2 - o1;
            mask <<= o2 - o1;
        } else if (o2 < o1) {
            bits >>= o1 - o2;
            mask >>= o1 - o2;
        }
        unsigned int &target = data_[to / 32];

        target &= ~mask;
        target |= bits;
        first += len;
        to += len;
    }
}

/* n bits from pos set or cleared: the first and last words masked, the words between filled
 * (0x208270, SOCOM 1's -- its DWARF's locals, its line table's statements -- with 32-bit words) */
void vector<bool, allocator<bool> >::set(unsigned int pos, unsigned int n, bool x)
{
    unsigned int w1 = pos / 32;
    unsigned int w2 = (pos + n - 1) / 32;
    unsigned int bitmask1 = ~0u << pos % 32;
    unsigned int bitmask2 = ~0u;
    unsigned int tmp = 32 - (pos + n) % 32;

    if (tmp < 32)
        bitmask2 >>= tmp;
    if (w1 == w2) {
        bitmask1 &= bitmask2;
        if (x) {
            data_[w1] |= bitmask1;
            return;
        }
        data_[w1] &= ~bitmask1;
        return;
    }
    if (x) {
        data_[w1] |= bitmask1;
        for (unsigned int i = w1 + 1; i < w2; ++i)
            data_[i] = ~0u;
        data_[w2] |= bitmask2;
    } else {
        data_[w1] &= ~bitmask1;
        for (unsigned int i = w1 + 1; i < w2; ++i)
            data_[i] = 0;
        data_[w2] &= ~bitmask2;
    }
}

/* every size a count can hold (0x208260, SOCOM 1's) */
unsigned int vector<bool, allocator<bool> >::max_size() const
{
    return (unsigned int)-1;
}

/* n values put in before a position: shifted up in place if they fit, else into new words grown
 * from 32 by doubling, the old bits copied round them (0x208040, SOCOM 1's with 32-bit words).
 * The five locals declared at the top in SOCOM 1's DWARF order (its block lists them reversed:
 * pos, newcap, newsize, olddata, ms) -- each defined in place they swap a dozen saved registers;
 * the old words read before the capacity and the position (the target's delay-slot load); the
 * size read in place, `size_ + n` (through size() MW puts n first) */
void vector<bool, allocator<bool> >::insert(iterator position, unsigned int n, const bool &x)
{
    unsigned int ms;
    unsigned int *olddata;
    unsigned int newsize;
    unsigned int newcap;
    unsigned int pos;

    if (n == 0)
        return;
    ms = max_size();
    if (n > ms || size() > ms - n)
        __msl_error("vector::insert length error\n");
    newsize = size_ + n;
    if (newsize <= capacity()) {
        pos = position.n_;
        if (size_ - pos != 0)
            shift_out(position, n);
        set(pos, n, x);
        size_ += n;
    } else {
        olddata = data_;
        newcap = capacity_;
        pos = position.n_;
        if (newcap == 0)
            newcap = 32;
        while (newsize > newcap) {
            if (newcap < ms / 2)
                newcap *= 2;
            else
                newcap = ms;
        }
        data_ = allocate(num_words(newcap));
        if (pos > 0)
            copy_bits(olddata, 0, pos, 0);
        set(pos, n, x);
        if (pos < size())
            copy_bits(olddata, pos, size(), pos + n);
        if (olddata)
            operator delete(olddata);
        size_ = newsize;
        capacity_ = newcap;
    }
}

/* one value put in before a position, the position given back (0x207FD0, SOCOM 1's) */
vector<bool, allocator<bool> >::iterator vector<bool, allocator<bool> >::insert(iterator position,
                                                                                const bool &x)
{
    insert(position, 1, x);
    return begin() += position.n_;
}

/* the three words traded (0x207F90, SOCOM 1's exact; FreeVector's swap, called) */
void vector<bool, allocator<bool> >::swap(vector &x)
{
    if (this != &x) {
        std::swap(capacity_, x.capacity_);
        std::swap(size_, x.size_);
        std::swap(data_, x.data_);
    }
}

bool vector<bool, allocator<bool> >::test_bit(unsigned int n)
{
    return (data_[n >> 5] & (1 << (n & 31))) != 0;
}
#pragma auto_inline reset
}

/* the list: SOCOM 1's members (SOCOM 1's DWARF names them) at SOCOM II's offsets, C2D 4 bytes
 * shorter and C2DString 0x98 bytes; of SOCOM 1's seven anchor strings one is left (the implicit
 * destructor destroys an array of one). The destructor is implicit: this unit, defining Draw and
 * Tick, emits it (first in the image, 0x205C40) */
class CZObjectiveList : public C2D {
public:
    CZObjectiveList();
    virtual void Draw(zdb::CCamera *cam);
    virtual void Tick(float dT);
    void Init();
    void RefreshStatics();
    void RefreshList();
    bool AddObjective(const CMissionObjectiveCam &objective, float ypos);
    void SetCurColor(int index);
    static bool IsPlayingAnimation();
    /* an objective's index, if it is one (inline in SOCOM II: no copy of its own). SOCOM 1's
     * line table puts the test and `return false` first: the true block follows the false one */
    bool ResolveIndex(int index, int *newindex) const
    {
        int size = m_objectivenames.size();

        if (index < 0 || index >= size)
            return false;
        *newindex = index;
        return true;
    }
    /* whether an index is an objective's (SOCOM 1's GetObjective tests it so; the name ours) */
    bool IsValidIndex(int index) const
    {
        int size = m_objectivenames.size();

        if (index < 0 || index >= size)
            return false;
        return true;
    }
    /* an objective's camera: the mission's list walked to it (inline in SOCOM II) */
    CMissionObjectiveCam *GetObjective(int index) const
    {
        if (!IsValidIndex(index))
            return 0;
        std::list<CMissionObjectiveCam *>::iterator it = theMission.m_objectives.begin();

        for (int i = 0; i < index; i++)
            ++it;
        return *it;
    }
    /* the same walk with the first node read through the list's sentinel, tail(): FreeList's,
     * whose copy calls the end pair's second() out of line (0x207560) one inline level down,
     * under RetainSelection. A STAND-IN for the original's deeper begin() (include/list's is fitted
     * to level 5: read through tail() in GetObjective itself, GetCurrentObjective keeps its walk
     * but SelectNext/PreviousObjective's self-inlining grows, 0xA0 -> 0x22C) */
    CMissionObjectiveCam *GetObjectiveByTail(int index) const
    {
        if (!IsValidIndex(index))
            return 0;
        std::list<CMissionObjectiveCam *>::iterator it(std::__list_deleter<const void *,
            std::allocator<const void *> >::iterator(theMission.m_objectives.tail().next_));

        for (int i = 0; i < index; i++)
            ++it;
        return *it;
    }
    CMissionObjectiveCam *GetCurrentObjective() const;
    /* the selection's camera kept before the list is freed (inline; the name ours) */
    void RetainSelection() { m_cur_selected_objective = GetObjectiveByTail(m_selected_objective); }
    CMissionObjectiveCam *GetCurrentObjectiveRetainer() const;
    /* the selection's camera held and the objectives ticked (inline; the name ours) */
    void UpdateSelection()
    {
        m_cur_selected_objective = GetCurrentObjective();
        theMission.TickObjectives(1000.0f);
    }
    void SelectPreviousObjective();
    void SelectNextObjective();
    /* whether any objective can be selected (SOCOM 1's; inline in SOCOM II) */
    bool IsAnySelectable()
    {
        int size = m_objectivenames.size();

        for (int i = 0; i < size; i++) {
            if (m_vecSelectable[i])
                return true;
        }
        return false;
    }
    /* an objective's name greyed (inline; the name ours) */
    void GreyObjective(int index)
    {
        int newindex;

        if (ResolveIndex(index, &newindex)) {
            C2DString *curstring = m_objectivenames[newindex];

            curstring->SetColor(g_colNormal.x, g_colNormal.y, g_colNormal.z);
            curstring->SetTrans(s_unselectedtrans);
        }
    }
    void SelectObjective();
    void FreeList(bool wipe_vector);
    void MissionEndCleanup();
    void PreviousObjective();
    void NextObjective();

    std::vector<C2DString *> m_objectivenames;   // 0x18
    std::vector<C2DBitmap *> m_objectivecompletedbitmap;   // 0x24
    std::vector<bool> m_vecSelectable;   // 0x30
    C2DString m_heading;   // 0x3c
    C2DString m_strMapHeading;   // 0xd4
    C2DBitmap m_textbackground;   // 0x170
    C2DBitmap m_mapbackground;   // 0x1f0
    C2DBitmap m_backgrounddivider;   // 0x270
    C2DString m_strAnalogL;   // 0x2f0
    C2DString m_strAnalogR;   // 0x388
    C2DString m_strCircleLabel;   // 0x420
    C2DString m_strSquareLabel;   // 0x4b8
    C2DString m_strTriLabel;   // 0x550
    C2DString m_strXLabel;   // 0x5e8
    C2DString m_arrAnchorStrings[1];   // 0x680
    int m_selected_objective;   // 0x718
    CMissionObjectiveCam *m_cur_selected_objective;   // 0x71c
    zdb::CTexHandle *m_backgroundtex;   // 0x720
    zdb::CTexHandle *m_checkboxtex;   // 0x724
    zdb::CTexHandle *m_checktex;   // 0x728
    unsigned char m_increment;   // 0x72c
};

/* the members built, room for four names, nothing selected (SOCOM 1's constructor, its body's
 * stores in its order; CZNewHudMap's constructor calls it for its member at 0xE0) */
CZObjectiveList::CZObjectiveList()
{
    m_objectivenames.reserve(4);
    m_selected_objective = -1;
    m_checkboxtex = 0;
    m_checktex = 0;
    m_increment = 1;
    m_cur_selected_objective = 0;
}

/* the list's three textures looked up by name */
void CZObjectiveList::Init()
{
    m_checkboxtex = zdb::CWorld::GetTexHandle("hud_checkbox.tif");
    m_checktex = zdb::CWorld::GetTexHandle("hud_check.tif");
    m_backgroundtex = zdb::CWorld::GetTexHandle("team_background.tif");
}

/* unless a mission camera plays: the sound ticked, and the selected objective's name pulsed
 * between black and yellow (SOCOM 1's red and its rate of 600), the X label with it */
void CZObjectiveList::Tick(float dT)
{

    if (theHUD->m_missioncams.IsPlaying())
        return;
    CSnd::PostTick(0.0f, 0);
    int newindex;

    if (ResolveIndex(m_selected_objective, &newindex)) {
        C2DString *curstring = m_objectivenames[newindex];
        float fCurLevel = curstring->m_color[1];

        if (m_increment) {
            fCurLevel += s_pulserate * dT;
            if (fCurLevel > 255.0f) {
                fCurLevel = 255.0f;
                m_increment = false;
            }
        } else {
            fCurLevel -= s_pulserate * dT;
            if (fCurLevel < 0.0f) {
                fCurLevel = 0.0f;
                m_increment = true;
            }
        }
        curstring->m_color[0] = fCurLevel;
        curstring->m_color[1] = fCurLevel;
        curstring->m_color[2] = 0.0f;
        m_strXLabel.SetTrans(0.5f * fCurLevel);
    }
}

/* an objective's name and check box made and listed, at the given height: its name indented
 * further for a secondary objective, greyed if it cannot be done, green when done, blue when
 * marked; whether it can be selected (not done, possible and shown) recorded; nothing shown if
 * the objective is not displayed (SOCOM 1's; the mark and the display test SOCOM II's) */
/* (the two bools declared at the top are numbered there, ahead of bSecondary -- SOCOM 1's
 * locals; defined where they are computed, they swap three saved registers; the texture picked
 * before the corners are summed, as the target's branch comes first) */
bool CZObjectiveList::AddObjective(const CMissionObjectiveCam &objective, float ypos)
{
    bool bComplete;
    bool bValid;
    C2DString *string = new C2DString;
    C2DBitmap *bitmap = new C2DBitmap;

    m_objectivecompletedbitmap.push_back(bitmap);
    m_objectivenames.push_back(string);
    bool bSecondary = objective.m_isSecondary;

    string->Load(0.765f, objective.m_name, theHUD->m_myfont, 10.0f + (float)(bSecondary ? 45 : 30),
                 ypos, true);
    bComplete = objective.IsComplete();
    bValid = !objective.IsInvalid();
    bool bMarked = objective.m_bit4;
    bool bSelectable = !bComplete && bValid && objective.IsDisplayOn();

    m_vecSelectable.push_back(bSelectable);
    if (!objective.IsDisplayOn()) {
        string->Off();
        bitmap->Off();
        return false;
    }
    float fXOffset = (float)(bSecondary ? 25 : 10);
    zdb::CTexHandle *tex = bComplete ? m_checktex : m_checkboxtex;
    float fLeftX = 10.0f + fXOffset;
    float fTopY = 1.0f + (-13.0f + ypos);

    bitmap->Load(fLeftX, fTopY, 15.0f + fLeftX, 15.0f + fTopY, tex);
    if (bMarked) {
        string->SetColor(g_colSelected.x, g_colSelected.y, g_colSelected.z);
        string->SetTrans(s_unselectedtrans);
        bitmap->SetColor(g_colSelected);
        bitmap->SetTrans(s_checktrans);
    } else if (bComplete) {
        string->SetColor(g_colComplete.x, g_colComplete.y, g_colComplete.z);
        string->SetTrans(s_completetrans);
        bitmap->SetColor(g_colComplete);
        bitmap->SetTrans(s_checktrans);
    } else if (!bValid) {
        string->SetColor(g_colUnavailable.x, g_colUnavailable.y, g_colUnavailable.z);
        string->SetTrans(s_unselectedtrans);
        bitmap->SetColor(g_colUnavailable);
        bitmap->SetTrans(s_checktrans);
    } else {
        string->SetColor(g_colNormal.x, g_colNormal.y, g_colNormal.z);
        string->SetTrans(s_unselectedtrans);
        bitmap->SetColor(g_colNormal);
        bitmap->SetTrans(s_checktrans);
    }
    push_back(bitmap);
    push_back(string);
    return true;
}

/* the list rebuilt: freed (its vectors kept), the statics laid out again, an entry added per
 * objective 15 apart from 104 down; the camera held before selected again if it is still
 * selectable, else the first objective that is not secondary and is selectable, else on from
 * the last (SOCOM 1's, the held camera SOCOM II's). The walks are const_iterators over a const
 * reference (SOCOM 1's const tail() and generic_iterator<1>): plain iterators lose the begin/end
 * temporaries. The held camera read through GetCurrentObjectiveRetainer(), auto-inlined, is
 * numbered after the named locals and takes s2 (read straight off the member it takes s4) */
void CZObjectiveList::RefreshList()
{
    CMissionObjectiveCam *retained = GetCurrentObjectiveRetainer();

    FreeList(false);
    RefreshStatics();
    float ypos = 104.0f;
    const std::list<CMissionObjectiveCam *> &objectives = theMission.m_objectives;

    for (std::list<CMissionObjectiveCam *>::const_iterator it = objectives.begin(); it != objectives.end();
         ++it) {
        if (AddObjective(**it, ypos))
            ypos += 15.0f;
    }
    if (retained) {
        int i = 0;

        for (std::list<CMissionObjectiveCam *>::const_iterator it = objectives.begin();
             it != objectives.end(); ++it, ++i) {
            if (*it == retained && m_vecSelectable[i]) {
                m_selected_objective = i;
                UpdateSelection();
                return;
            }
        }
    }
    m_selected_objective = 0;
    UpdateSelection();
    for (std::list<CMissionObjectiveCam *>::const_iterator it = objectives.begin(); it != objectives.end();
         ++it) {
        if (!(*it)->m_isSecondary && m_vecSelectable[m_selected_objective])
            return;
        m_selected_objective++;
        UpdateSelection();
    }
    m_selected_objective = m_objectivenames.size();
    UpdateSelection();
    NextObjective();
}

/* a list's entry deleted (std::for_each's function object; the name ours) */
template <class T>
struct CDeleteEntry {
    CDeleteEntry() {}
    void operator()(T *p) const { delete p; }
};

/* the list emptied: the selection's camera kept, the children dropped, the names and their
 * check boxes deleted and the three vectors cleared -- and, wiped, their storage traded away
 * (SOCOM 1's, each vector through FreeVector) */
void CZObjectiveList::FreeList(bool wipe_vector)
{
    RetainSelection();
    clear();
    std::for_each(m_objectivenames.begin(), m_objectivenames.end(), CDeleteEntry<C2DString>());
    m_objectivenames.clear();
    if (wipe_vector)
        FreeVector(m_objectivenames);
    std::for_each(m_objectivecompletedbitmap.begin(), m_objectivecompletedbitmap.end(),
                  CDeleteEntry<C2DBitmap>());
    m_objectivecompletedbitmap.clear();
    if (wipe_vector)
        FreeVector(m_objectivecompletedbitmap);
    m_vecSelectable.clear();
    if (wipe_vector)
        FreeVector(m_vecSelectable);
}

/* the selected objective's name given the colour of another's (SOCOM 1's). Never inlined in the
 * target (NextObjective and PreviousObjective call it): pinned */
#pragma auto_inline off
void CZObjectiveList::SetCurColor(int index)
{
    int newindex;

    if (!ResolveIndex(index, &newindex))
        return;
    C2DString *curstring = m_objectivenames[newindex];
    float r = curstring->m_color[0];
    float g = curstring->m_color[1];
    float b = curstring->m_color[2];

    if (ResolveIndex(m_selected_objective, &newindex)) {
        curstring = m_objectivenames[newindex];
        curstring->m_color[0] = r;
        curstring->m_color[1] = g;
        curstring->m_color[2] = b;
    }
}

#pragma auto_inline reset

/* on to the next selectable objective: its colour the old one's, the old one greyed (SOCOM 1's
 * 80 grey; SOCOM II's colour and alpha its own) */
void CZObjectiveList::NextObjective()
{
    int oldindex = m_selected_objective;

    if (!IsAnySelectable())
        return;
    SelectNextObjective();
    SetCurColor(oldindex);
    GreyObjective(oldindex);
}

/* the next selectable objective, round to the first (SOCOM 1's) */
void CZObjectiveList::SelectNextObjective()
{
    m_selected_objective++;
    UpdateSelection();
    if (m_selected_objective >= m_objectivenames.size()) {
        m_selected_objective = 0;
        UpdateSelection();
    }
    if (!m_vecSelectable[m_selected_objective])
        SelectNextObjective();
}

/* back to the previous selectable objective, as NextObjective (SOCOM 1's) */
void CZObjectiveList::PreviousObjective()
{
    int oldindex = m_selected_objective;

    if (!IsAnySelectable())
        return;
    SelectPreviousObjective();
    SetCurColor(oldindex);
    GreyObjective(oldindex);
}

/* the previous selectable objective, round to the last (SOCOM 1's) */
void CZObjectiveList::SelectPreviousObjective()
{
    m_selected_objective--;
    UpdateSelection();
    if (m_selected_objective < 0) {
        m_selected_objective = m_objectivenames.size() - 1;
        UpdateSelection();
    }
    if (m_selected_objective < 0) {
        m_selected_objective = 0;
        UpdateSelection();
    }
    if (!m_vecSelectable[m_selected_objective])
        SelectPreviousObjective();
}

/* the selected objective's camera played, unless one plays or the orders menu is up (SOCOM 1's) */
void CZObjectiveList::SelectObjective()
{
    CMissionObjectiveCam *objective = GetObjective(m_selected_objective);

    if (!objective)
        return;
    if (theHUD->m_missioncams.IsPlaying() || theHUD->m_menustate == stateOrders)
        return;
    theHUD->m_missioncams.PlayCamera(objective, false);
}

/* the children drawn while the list is on and no mission camera plays. SOCOM 1 cast each child
 * to C2D first and dropped the result; SOCOM II has no cast. The iterator is assigned from
 * begin() (it lives in memory), and the end taken after it into a local: begun first, `this`
 * dies there and its saved register takes the end (taking the end first keeps both) */
void CZObjectiveList::Draw(zdb::CCamera *cam)
{
    if (IsOn() && !theHUD->m_missioncams.IsPlaying()) {
        std::list<C2D *>::iterator i;

        i = begin();
        std::list<C2D *>::iterator e = end();

        for (; i != e; ++i)
            (*i)->Draw(cam);
    }
}

/* the selected objective's camera (SOCOM 1's, GetObjective inlined: SOCOM II has no copy of it) */
CMissionObjectiveCam *CZObjectiveList::GetCurrentObjective() const
{
    return GetObjective(m_selected_objective);
}

/* the objective whose camera is held (SOCOM 1's) */
CMissionObjectiveCam *CZObjectiveList::GetCurrentObjectiveRetainer() const
{
    return m_cur_selected_objective;
}

/* at the mission's end: every string wiped (the list's own through their tables, the unit's
 * static ones directly), then the list freed with its vectors (SOCOM II's; SOCOM 1's freed the
 * vectors here itself) */
void CZObjectiveList::MissionEndCleanup()
{
    m_heading.ClearAndWipe();
    m_strMapHeading.ClearAndWipe();
    m_strAnalogL.ClearAndWipe();
    m_strAnalogR.ClearAndWipe();
    m_strCircleLabel.ClearAndWipe();
    m_strSquareLabel.ClearAndWipe();
    m_strTriLabel.ClearAndWipe();
    m_strXLabel.ClearAndWipe();
    g_strMapTitle.ClearAndWipe();
    g_strPlayMovie.ClearAndWipe();
    g_strObjectivesTitle.ClearAndWipe();
    for (unsigned int i = 0; i < 6; i++)
        g_strControls[i].ClearAndWipe();
    m_arrAnchorStrings[0].ClearAndWipe();
    FreeList(true);
}

bool CZObjectiveList::IsPlayingAnimation()
{
    return theHUD->m_missioncams.IsPlaying();
}

namespace zdb {
class CAssetList {
public:
    CTexHandle *GetTexHandle(const char *name);
};

class CAssetMgr {
public:
    static CAssetList m_assets;
};
}

/* the tactical map's statics laid out and listed: the darkened screen, the map's frame and edges,
 * the title bars and their titles, the legend's box and its six lines (SOCOM 1's; moved to the
 * unit's end in SOCOM II, so first in the image after the implicit destructor) */
void CZObjectiveList::RefreshStatics()
{
    unsigned int i;
    zdb::CTexHandle *tex = zdb::CAssetMgr::m_assets.GetTexHandle("newweapnbkrnd.tif");

    push_back(&g_polyScreen);
    g_polyScreen.SetPos(0.0f, 0.0f, zVid.renderWidth, zVid.renderHeight);
    g_polyScreen.SetHasTexture(false);
    g_polyScreen.SetColor(0.0f, 0.0f, 0.0f);
    g_polyScreen.SetTrans(100.0f);
    g_polyScreen.SetTrans(1, 0.0f);
    g_polyScreen.SetTrans(2, 0.0f);
    g_bmpMapFrame.SetHasTexture(false);
    g_bmpMapFrame.SetPos(230, 60, 630, 365);
    g_bmpMapFrame.SetIsTrans(true);
    g_bmpMapFrame.SetColor(0.0f, 0.0f, 0.0f);
    g_bmpMapFrame.SetTrans(s_frametrans);
    push_back(&g_bmpMapFrame);
    g_bmpMapBox.Load(230.0f, 60.0f, 630.0f, 365.0f, tex);
    push_back(&g_bmpMapBox);
    g_bmpMapBox.SetUV(0.3f, 0.3f, 0.4f, 0.4f);
    g_bmpMapBox.SetTrans(72.0f);
    for (i = 0; i < 4; i++) {
        g_bmpMapEdges[i].SetHasTexture(false);
        g_bmpMapEdges[i].SetColor(64.0f, 128.0f, 128.0f);
        g_bmpMapEdges[i].SetTrans(128.0f);
        push_back(&g_bmpMapEdges[i]);
    }
    g_bmpMapEdges[0].SetPos(230, 60, 232, 365);
    g_bmpMapEdges[1].SetPos(230, 60, 630, 62);
    g_bmpMapEdges[2].SetPos(630, 60, 628, 365);
    g_bmpMapEdges[3].SetPos(230, 363, 630, 365);
    g_bmpMapTitleBg.Load(230.0f, 15.0f, 630.0f, 55.0f, tex);
    push_back(&g_bmpMapTitleBg);
    g_bmpMapTitleBg.SetUV(0.3f, 0.3f, 0.4f, 0.4f);
    g_bmpMapTitleBg.SetTrans(128.0f);
    g_strMapTitle.Load(1.5f, "TACTICAL MAP", theHUD->m_myfont, 430.0f, 48.0f, true);
    push_back(&g_strMapTitle);
    g_strMapTitle.m_isCentered = 1;
    g_strMapTitle.UpdatePos();
    g_bmpObjectivesBg.Load(10.0f, 84.0f, 224.0f, 365.0f, tex);
    g_bmpObjectivesBg.SetUV(0.3f, 0.3f, 0.4f, 0.4f);
    g_bmpObjectivesBg.SetTrans(72.0f);
    push_back(&g_bmpObjectivesBg);
    g_bmpMovieBg.Load(10.0f, 60.0f, 224.0f, 84.0f, tex);
    g_bmpMovieBg.SetUV(0.3f, 0.3f, 0.4f, 0.4f);
    g_bmpMovieBg.SetTrans(128.0f);
    push_back(&g_bmpMovieBg);
    g_strPlayMovie.Load(0.8f, "\xa6  PLAY MOVIE", theHUD->m_myfont, 20.0f, 77.0f, true);
    push_back(&g_strPlayMovie);
    g_bmpObjectivesTitleBg.Load(10.0f, 15.0f, 224.0f, 55.0f, tex);
    push_back(&g_bmpObjectivesTitleBg);
    g_bmpObjectivesTitleBg.SetUV(0.3f, 0.3f, 0.4f, 0.4f);
    g_bmpObjectivesTitleBg.SetTrans(128.0f);
    g_strObjectivesTitle.Load(1.5f, "OBJECTIVES", theHUD->m_myfont, 117.0f, 48.0f, true);
    push_back(&g_strObjectivesTitle);
    g_strObjectivesTitle.m_isCentered = 1;
    g_strObjectivesTitle.UpdatePos();
    g_bmpLegendBg.Load(10.0f, 370.0f, 630.0f, 434.0f, tex);
    push_back(&g_bmpLegendBg);
    g_bmpLegendBg.SetUV(0.3f, 0.3f, 0.4f, 0.4f);
    g_bmpLegendBg.SetTrans(128.0f);
    for (unsigned int j = 0; j < 6; j++) {
        g_strControls[j].Load(0.8f, " ", theHUD->m_myfont, 0.0f, 0.0f, true);
        push_back(&g_strControls[j]);
    }
    g_strControls[0].SetString("\xb4  SNAP TO PLAYER", true);
    g_strControls[0].SetPos(20, 395);
    g_strControls[1].SetString("\xb7  SNAP TO OBJECTIVE", true);
    g_strControls[1].SetPos(20, 419);
    g_strControls[2].SetString("LEFT ANALOG STICK = MOVE MAP", true);
    g_strControls[2].SetPos(230, 395);
    g_strControls[3].SetString("RIGHT ANALOG STICK = ZOOM MAP", true);
    g_strControls[3].SetPos(230, 419);
    g_strControls[4].SetString("\xac  RESUME GAME", true);
    g_strControls[4].SetPos(500, 395);
    g_strControls[5].SetString("\xbe  LEGEND", true);
    g_strControls[5].SetPos(500, 419);
}
