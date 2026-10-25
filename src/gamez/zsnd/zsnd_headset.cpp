/* zsnd_headset.cpp -- Headset, the USB headset's voice stream (OVL1 0x34A620-...). SOCOM 1 links
 * its functions after CSndSequence's; the unit name is ours. Defined in reverse of their image
 * order (-inline deferred). */

#include "zsnd.h"

Headset *Headset::theHeadset;

/* OPEN, 8 bytes long: the target keeps new's result in v0 across the constructor call and stores
 * v0; MW here moves it to a0 first and stores a0 (tried: new Headset(), a local) */
void Headset::Create()
{
    theHeadset = new Headset;
}

/* the stream request and the headset state reset (SOCOM II's; CSnd::Close calls it) */
void Headset::Reset()
{
    Headset *h = theHeadset;

    if (h) {
        h->m_34 = 0;
        h->m_stop = 0;
        h->m_b = 1;
        h->m_0 = 0;
        h->m_4 = 0;
        h->m_8 = 0;
        h->m_state = 3;
        h->m_c = 0;
        h->m_sound = 0;
        h->m_vol = 0;
    }
    theHeadsetState = 0;
}

void Headset::ResetStream()
{
    Headset *h = theHeadset;

    if (h) {
        h->m_instance = 0;
        h->m_3c = -1;
    }
}

void Headset::StaticSoundSysCallback(unsigned int handle, unsigned long user_data)
{
    if ((Headset *)user_data)
        ((Headset *)user_data)->SoundSysCallback(handle);
}

/* OPEN, 4 instructions: the case chain's constants 3 and 2 sit in a0 and v1 in the target (each
 * reused by a case's store), in v1 and a0 here. Case 1 has a block of its own (a return) */
void Headset::SoundSysCallback(unsigned int handle)
{
    switch (m_state) {
    case 0:
        if (handle == 0) {
            snd_DoExternCall(0x12C4E67A, 7, 0, 0, 0, 0, 0);
            theHeadsetState = 2;
            m_state = 4;
        } else
            m_state = 2;
        break;
    case 1:
        return;
    case 2:
        if (handle == 0) {
            m_state = 3;
            m_stop = 0;
        } else
            m_state = 1;
        break;
    case 3:
        break;
    }
}

void Headset::Tick(float dT)
{
    if (theHeadsetState == 2)
        DisconnectedTick(dT);
    else if (theHeadsetState == 1)
        ConnectedTick(dT);
}

void Headset::Pause()
{
    if (theHeadsetState == 1)
        snd_DoExternCall_A(0x12C4E67A, 4, 0, 0, 0, 0, 0);
}

void Headset::Resume()
{
    if (theHeadsetState == 1)
        snd_DoExternCall_A(0x12C4E67A, 5, 0, 0, 0, 0, 0);
}
