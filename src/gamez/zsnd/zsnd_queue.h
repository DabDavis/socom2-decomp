/* zsnd_queue.h -- SOCOM II's sound queues: sounds played one at a time after a delay each.
 * CSndQueue and its entries are zsnd_queue.cpp's, the headset's CSndVocalQueue over it is
 * zsnd_vocalqueue.cpp's -- two units (README "The two queue units"). Not in SOCOM 1; the class
 * names are the RTTI's, the members' and the header's names ours. */
#ifndef ZSND_QUEUE_H
#define ZSND_QUEUE_H

#include "zsnd.h"
#include "../zsys/zsys.h"

/* one queued sound: played when its delay has run out, if it still can be (the RTTI's names) */
class CSndQueueEntry {
public:
    CSndQueueEntry();
    virtual CSndInstance *Play();
    virtual bool CanPlay();
    void SetUsed(bool used);
    void Set(CSnd *sound, float delay, float vol);

    float m_delay;      /* 0x04: counted down by the queue's tick */
    CSnd *m_sound;      /* 0x08 */
    float m_vol;        /* 0x0C */
    bool m_used;        /* 0x10: taken from the free entries */
};   /* 0x14 bytes */

/* a voice's entry: an id to remove it by, and a position to play it at */
class CSndVocalQueueEntry : public CSndQueueEntry {
public:
    CSndVocalQueueEntry() : m_id(-1), m_is3D(false) {}
    virtual CSndInstance *Play();
    virtual bool CanPlay();

    int m_id;           /* 0x14: passed as the play's entity id */
    PNT3D m_pos;        /* 0x18 */
    bool m_is3D;        /* 0x24 */
};   /* 0x28 bytes */

/* the queue: the entries waiting, in order, and the free ones they come from */
class CSndQueue {
public:
    CSndQueue();
    /* declared only: the image holds no destructor (the queue is a global never destroyed), and an
     * implicit one would be emitted with the vocal queue's constructor */
    ~CSndQueue();
    virtual void Stop();
    virtual void OnPlay(CSndQueueEntry *entry);
    virtual void Open();
    virtual void Close();
    void Tick(float dT);
    CSndQueueEntry *GetFreeEntry();
    void PlayNext();
    void DumpQueue();
    void DumpFree();

    std::vector<CSndQueueEntry *> m_queue;  /* 0x04 */
    std::vector<CSndQueueEntry *> m_free;   /* 0x10 */
    CSndInstance *m_instance;               /* 0x1C: the entry playing */
    CSnd *m_sound;                          /* 0x20: its sound */
};   /* 0x24 bytes */

class CSndVocalQueue : public CSndQueue {
public:
    CSndVocalQueue();
    virtual void Stop();
    virtual void OnPlay(CSndQueueEntry *entry);
    virtual void Open();
    void Add(CSnd *sound, int id, const CPnt3D *pos, float delay, float vol);
    void Remove(int id);

    int m_id;           /* 0x24: the playing entry's id, -1 for none */
};   /* 0x28 bytes */

#endif
