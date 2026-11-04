/* seal_dipool.h -- the visibility tests' request pool, SOCOM 1's CDiPool (its seal_sensors.cpp kept it
 * to itself; SOCOM II's seal_sensors.cpp and ai_target.cpp share it: the name of this header is ours).
 * SOCOM 1's _diPool was 0x28 bytes (the DiIntersects at +0, their count at +4, the used and free
 * request lists at +8/+0x14). SOCOM II's is 0x3C bytes at OVL1 bss 0x44D5E0 (OVL1's
 * __sinit_fts_ovl2statics.cpp builds it: six DiIntersects of 76 bytes at +0, the count 6 at +4, 2 and
 * 1 at +8/+0xC, the lists at +0x10 and +0x1C, the frame's counts at +0x30-+0x38) -- only what a
 * written function touches.
 *
 * Its walks are inline members the callers inline ONE level: CloseDiPool, CleanupDiPool and the two
 * target tick tasks call the request list's begin(), end(), !=, *, the iterator's assignment and
 * postfix ++ out of line (seal_sensors.cpp's copies, 0x5CC770-0x5CC880), as the target does -- the
 * pieces' code and the walks' frames come out byte-exact so. */

#ifndef SEAL_DIPOOL_H
#define SEAL_DIPOOL_H

#include <list>
#include "../zentity/zentity.h"
#include "../znode/node_world.h"

class CDiPool {
public:
    /* a request (SOCOM 1's CDiPool::dir; its constructor zeroes 0x0, 0x20, 0x28, 0x2C and 0x30) */
    struct dir {
        unsigned char m_state;   // 0x0 (0 free, 1 dropped, 2 done, 3 new, 4 registered)
        CPnt3D m_tail;   // 0x4: the ray's
        CPnt3D m_tip;   // 0x10
        int m_index;   // 0x1C: its DiIntersect's, -1 for none
        int m_limited;   // 0x20: taken only while the frame's budget lasts
        float m_age;   // 0x24
        CEntity *m_ent;   // 0x28: whose test it is
        int m_2c;   // 0x2C
        int m_30;   // 0x30

        /* dropped: marked for the pool's next pass, its DiIntersect let go (inline; the name ours) */
        void Drop()
        {
            m_state = 1;
            m_index = -1;
        }
        /* it holds a DiIntersect (inline: out of line at OVL2 0x60BB80 behind the post-tick's cut; the
         * name ours) */
        bool HasIntersect() const { return m_index >= 0; }
    };
    /* the free list (a class over the pointer list: SOCOM 1's ~CDiPool tests the member before the
     * deletion walk its destructor inlines; the names ours) */
    class dirlist : public std::list<dir *> {
    public:
        /* a request reset and put back (OVL2 0x5CC800: CloseDiPool and ai_target.cpp's 0x60B8F0 call
         * it -- SOCOM 1's CloseDiPool did it inline, the record re-read from the frame for each store,
         * its address handed to the insert) */
        void Free(dir *d);
        /* every request deleted, the list emptied (OVL2 0x5CC6C0: CloseDiPool calls it on the free
         * list -- SOCOM 1's did it inline, as its ~CDiPool does) */
        void DeleteAll();
        /* a range unlinked, the wrapper's iterators handed to the base's erase (inline: out of line at
         * OVL2 0x60BB90 behind the post-tick's cut) */
        iterator erase(iterator first, iterator last)
        {
            return iterator(base_list::erase(first, last));
        }

    private:
        typedef std::__list_imp<const void *, std::allocator<const void *> > base_list;
    };

    /* the world the rays are registered with (inline: out of line at OVL2 0x60BF30 behind the
     * pre-tick's cut; the name ours) */
    static zdb::CWorld *World() { return zdb::CWorld::m_world; }

    /* every used request put back on the free list, then all of them deleted (inline: CloseDiPool's) */
    void Close()
    {
        dirlist::iterator i;

        for (i = m_used.begin(); i != m_used.end(); i++)
            m_free.Free(*i);
        m_used.clear();
        m_free.DeleteAll();
    }
    /* an entity's pending requests dropped (inline: CleanupDiPool's) */
    void Release(CEntity *e)
    {
        dirlist::iterator i;

        for (i = m_used.begin(); i != m_used.end(); i++) {
            dir *d = *i;

            if (d->m_ent == e && d->m_state) {
                d->m_state = 1;
                d->m_index = -1;
            }
        }
    }
    /* the frame's rays registered with the world, one DiIntersect each while they last; the limited
     * ones only within the frame's budget; the rest aged (inline: target_pretick's) */
    void PreTick(float dT)
    {
        int n = 0;
        int k = 0;
        zdb::CWorld *w = World();
        dirlist::iterator i;

        for (i = m_used.begin(); n < m_count && i != m_used.end(); i++) {
            dir *d = *i;

            if (d->m_state == 1 || d->m_state == 4)
                continue;
            if (d->m_limited) {
                if (m_limitedUsed >= m_limitedMax)
                    continue;
                m_limitedUsed++;
            }
            n++;
            if (d->m_state == 2)
                continue;
            d->m_index = k++;
            d->m_state = 4;
            d->m_age += dT;
            m_pool[d->m_index].SetCharacterHits(false);
            if (w->RegIntersectDelayed(&m_pool[d->m_index], &d->m_tail, &d->m_tip, -1) != -1) {
                if (d->m_ent)
                    w->RegIgnoreNodeDelayed(&m_pool[d->m_index], d->m_ent->GetNode());
                else
                    w->ClearIgnoreNodeDelayed(&m_pool[d->m_index]);
            } else
                d->m_index = -1;
        }
        m_registered = n;
        for (; i != m_used.end(); i++)
            (*i)->m_age += dT;
    }
    /* the frame's finished and dropped requests put back on the free list; the registered ones let go
     * of their DiIntersects, those without one counted down (inline: target_post_tick's). The list
     * the long-waiting ones are gathered in is never read, nor are the two sizes (debug code compiled
     * out: the calls stay) */
    void PostTick()
    {
        dirlist late;
        int before = m_used.size();
        dirlist::iterator i;

        for (i = m_used.begin(); i != m_used.end();) {
            dir *d = *i;

            if (d->m_state == 2 || d->m_state == 1) {
                dirlist::iterator next(i);

                m_free.Free(*i);
                i = m_used.erase(i, ++next);
            } else if (d->m_state == 4) {
                if (d->HasIntersect())
                    d->m_index = -1;
                else {
                    if (d->m_index < -1)
                        late.push_back(d);
                    d->m_index--;
                }
                i++;
            } else
                i++;
        }
        int after = m_used.size();
        m_limitedUsed = 0;
        m_34 = 0;
    }

    zdb::DiIntersect *m_pool;   // 0x0: the DiIntersects
    int m_count;   // 0x4
    int m_8;   // 0x8
    int m_limitedMax;   // 0xc
    dirlist m_used;   // 0x10
    dirlist m_free;   // 0x1c
    unsigned char _28[0x30 - 0x28];
    int m_registered;   // 0x30
    int m_34;   // 0x34
    int m_limitedUsed;   // 0x38
};   // 0x3c bytes

extern CDiPool _diPool;

#endif
