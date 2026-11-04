/* hud_tcmtree.cpp -- the radio command menu's tree (OVL1 0x231A30-0x23624F; the file name ours, as
 * every name here: SOCOM II's own menu, no SOCOM 1 counterpart). hud_tcm.h has the cut into three
 * units: this one calls hud_tcmcmds.cpp's two eight-byte node setters (0x230550/0x230540) and
 * hud_tcm.cpp's mode setter (0x236250) out of line, and has no initialiser (its statics
 * 0x4130A0-0x4130BF).
 *
 * Written: the node's state update, search, children, captions, name, delay, and the sounds. Not
 * yet: the subtree's display (0x231BA0) and the delay's tick (0x232260) -- both recursive and
 * self-inlined, their vector accessors' out-of-line copies after each (0x231DD0-0x231E30,
 * 0x232460-0x232480) --, the node's constructor and destructor, the cache builders (0x2329F0,
 * 0x232BC0, 0x2344E0) and the menu's input (0x234F40-0x2360D0). Defined in reverse of their image
 * order (-inline deferred). */

#include <string.h>

#include "hud_tcm.h"
#include "../../gamez/zsys/zsys_public.h"

class CZSealBody;

CZSealBody *ftsGetPlayer();

/* hud_tcmcmds.cpp's: the node an item's test runs for */
void tcmSetCurNode(CTCMNode *node);
void tcmClearCurNode();

/* the menu's sounds (their virtual's slot 1: CSnd::PlaySound, as hud_weaponsel.cpp's) */
class CSnd {
public:
    virtual ~CSnd();
    virtual void *PlaySound(void *inst, unsigned int flags);

    static CSnd *GetSoundByName(const char *name);
};

CTCMSounds::CTCMSounds()
{
    m_loaded = false;
    m_back = 0;
    m_slide = 0;
    m_select = 0;
}

void CTCMSounds::Load()
{
    m_loaded = true;
    m_slide = CSnd::GetSoundByName(".TCM_SLIDE");
    m_select = CSnd::GetSoundByName(".TCM_SELECT");
    m_back = CSnd::GetSoundByName(".TCM_SELECT");
}

void CTCMSounds::Clear()
{
    m_loaded = false;
    m_slide = 0;
    m_select = 0;
    m_back = 0;
}

void CTCMSounds::Play(TCM_SOUND which)
{
    if (m_loaded) {
        switch (which) {
        case TCM_SND_SLIDE:
            if (m_slide)
                m_slide->PlaySound(0, 255);
            break;
        case TCM_SND_SELECT:
            if (m_select)
                m_select->PlaySound(0, 255);
            break;
        case TCM_SND_BACK:
            if (m_back)
                m_back->PlaySound(0, 255);
            break;
        }
    }
}

/* a string freed and cleared (ours: the name's, inlined in its setter and the destructor) */
static inline void FreeString(char *&s)
{
    if (s)
        _free(s, 0, 0);
    s = 0;
}

/* the node: hidden, no parent, entry or name; its backing, bar, both captions and icon listed
 * under its root; the backing untextured, translucent and black, the bar's right corners clear;
 * the captions at 0.9 scale; ready */
CTCMNode::CTCMNode()
{
    m_parent = 0;
    m_entry = 0;
    m_name = 0;
    m_734 = -1;
    m_state = TCM_HIDDEN;
    m_root.push_back(&m_back);
    m_root.push_back(&m_bar);
    m_root.push_back(&m_text);
    m_root.push_back(&m_text2);
    m_root.push_back(&m_icon);
    m_back.SetHasTexture(false);
    m_back.SetIsTrans(true);
    m_back.SetTrans(100.0f);
    m_back.SetColor(0.0f, 0.0f, 0.0f);
    m_bar.SetHasTexture(false);
    m_bar.SetIsTrans(true);
    m_bar.SetTrans(0, 100.0f);
    m_bar.SetTrans(1, 100.0f);
    m_bar.SetTrans(2, 0.0f);
    m_bar.SetTrans(3, 0.0f);
    m_bar.SetColor(0.0f, 0.0f, 0.0f);
    m_text.m_scale = 0.9f;
    m_text2.m_scale = 0.9f;
    m_delay = 0.0f;
    m_ready = true;
    m_741 = false;
    m_action = 0;
}

/* its children deleted, its name freed (a written destructor: its first test reads the parent, but
 * the target then takes the node out of its OWN children -- a0 is still `this` at the call; the -1
 * last stored is the flag the C2DString destructors take) */
CTCMNode::~CTCMNode()
{
    if (m_parent)
        RemoveChild(this);
    DeleteChildren();
    FreeString(m_name);
    m_entry = 0;
    m_734 = -1;
}

/* the delays counted down, the whole subtree's (self-inlined: each copy calls the accessors one
 * level shallower) */
void CTCMNode::Tick(float dt)
{
    if (IsWaiting()) {
        m_delay -= dt;
        if (m_delay < 0.0f) {
            m_delay = 0.0f;
            m_ready = true;
        }
    }
    if (!NoChildren()) {
        unsigned int n = GetNumChildren();

        for (unsigned int i = 0; i < n; i++)
            GetChild(i)->Tick(dt);
    }
}

/* the message's delay: only ever lengthened (the node waits until it runs out) */
void CTCMNode::SetDelay(float delay)
{
    if (!(delay <= m_delay)) {
        m_delay = delay;
        m_ready = false;
    }
}

void CTCMNode::SetEntry(CTCMEntry *entry)
{
    m_entry = entry;
}

/* the caption, cut to a length with a dash when it is longer (a length of 0 takes it whole) */
void CTCMNode::SetCaption(const char *caption, int len)
{
    char buf[128];

    if (len == 0) {
        m_text.SetString(caption, true);
        return;
    }
    if (caption && strlen(caption)) {
        strncpy(buf, caption, len);
        if (strlen(buf) > len - 1) {
            buf[len - 2] = '-';
            buf[len - 1] = 0;
        }
        m_text.SetString(buf, true);
    }
}

void CTCMNode::SetName(const char *name)
{
    if (m_name)
        FreeString(m_name);
    if (name)
        m_name = _strdup(name, 0, 0);
}

void CTCMNode::SetText2(const char *s)
{
    m_text2.SetString(s, true);
}

/* a child taken out of the list and returned (as the target does it: every later child copied
 * into its slot in turn, and the parent cleared through the found pointer before it is set --
 * never the child's; a signed counter, or MW unrolls the copy 8x) */
CTCMNode *CTCMNode::RemoveChild(CTCMNode *child)
{
    CTCMNode *found = 0;
    unsigned int n = m_children.size();

    for (unsigned int i = 0; i < n && !found; i++) {
        if (m_children[i] == child) {
            if (i != n - 1) {
                for (int j = i + 1; j < n; j++)
                    m_children[i] = m_children[j];
                found->m_parent = 0;
                found = child;
            }
            m_children.pop_back();
        }
    }
    return found;
}

/* (a top-tested loop: inlined into the destructor after a join, a `while` is entered at its
 * bottom test) */
void CTCMNode::DeleteChildren()
{
    if (m_children.size())
        do {
            delete m_children.back();
            m_children.pop_back();
        } while (m_children.size());
}

CTCMNode *CTCMNode::AddChild(CTCMNode *child)
{
    m_children.push_back(child);
    child->m_parent = this;
    return child;
}

/* the children captioned with a name (and, if asked, theirs): pushed onto a list, counted */
int CTCMNode::Find(const char *name, std::vector<CTCMNode *> &found, bool recurse)
{
    int count = 0;

    if (name) {
        unsigned int n = m_children.size();
        unsigned int i;

        for (i = 0; i < n; i++) {
            const char *s = m_children[i]->m_text.m_string + m_children[i]->m_text.m_firstCharPos;

            if (s && !strcasecmp(s, name)) {
                found.push_back(m_children[i]);
                count++;
            }
        }
        /* a counter of its own (the shared one re-colours this loop) */
        if (recurse)
            for (unsigned int i = 0; i < n; i++)
                count += m_children[i]->Find(name, found, recurse);
    }
    return count;
}

/* the whole subtree's display off (self-inlined four times: each copy calls the accessors one level
 * shallower, the fifth calls them all and itself) */
void CTCMNode::HideAll()
{
    GetRoot()->Off();
    unsigned int n = GetNumChildren();

    for (unsigned int i = 0; i < n; i++)
        GetChild(i)->HideAll();
}

/* the node's state from its item's test (shown when it has none), its children's if asked; a
 * node with children is hidden when none of them is shown */
TCM_STATE CTCMNode::UpdateState(bool recurse)
{
    if (m_entry && m_entry->m_update) {
        tcmSetCurNode(this);
        m_state = (TCM_STATE)m_entry->m_update(ftsGetPlayer(), m_entry->m_updateArg);
        tcmClearCurNode();
    } else {
        m_state = TCM_SHOWN;
    }
    if (m_state != TCM_HIDDEN && recurse) {
        unsigned int n = m_children.size();

        if (n)
            for (unsigned int i = 0; i < n; i++)
                m_children[i]->UpdateState(recurse);
    }
    if (m_state != TCM_HIDDEN) {
        unsigned int n = m_children.size();

        if (n) {
            bool shown = false;

            for (unsigned int i = 0; i < n && !shown; i++)
                if (m_children[i]->m_state == TCM_SHOWN)
                    shown = true;
            if (!shown)
                m_state = TCM_HIDDEN;
        }
    }
    return m_state;
}
