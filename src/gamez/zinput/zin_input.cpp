/* zin_input.cpp -- CInput: the pads made and deleted, ticked and flushed, the input brought up and
 * down and across an IOP reboot (OVL1 0x2D9CC0-0x2D9FE7). SOCOM 1's order (DeletePad first, the
 * reboot handler last); SOCOM II reads one pad (each loop over NUM_PADS keeps the array's address
 * form) and keeps the pad object, re-constructing it in place and destroying it without freeing.
 * A unit of its own: it calls CPad::IsOpen, CInput::ClosePadIO and CKeyboard::Tick (an empty
 * function) out of line, which -inline auto would inline within one unit. The file name is ours
 * (SOCOM 1's input directory has zin_cheats.cpp). Defined in reverse of their image order
 * (-inline deferred). */

#include <new>

#include "zinput.h"

CPad *CInput::m_pads[2];
bool CInput::m_init;
CKeyboard *CInput::m_keyboard;

/* the pads and the keyboard closed before the IOP reboots, opened again after (SOCOM 1's) */
bool CInput::IOPRebootHandler(CIOPManager::RebootPhase phase, CIOPManager::RebootMode mode, void *data)
{
    if (phase == CIOPManager::REBOOT_BEGIN) {
        UninitKeyreadStuff();
        ClosePadIO();
    } else {
        OpenPadIO();
        InitKeyreadStuff();
    }
    return true;
}

/* the pad IO opened, the reboot handler registered, the keyboard made (SOCOM 1's) */
int CInput::Init()
{
    m_init = OpenPadIO() == 0;
    theIOPManager.RegisterRebootCallback(IOPRebootHandler, 0);
    InitKeyreadStuff();
    m_keyboard = new CKeyboard;
    m_init = true;
    return 0;
}

/* the keyboard deleted, the pad IO closed, the pad deleted (SOCOM 1's) */
void CInput::Uninit()
{
    if (m_keyboard) {
        delete m_keyboard;
        m_keyboard = 0;
    }
    UninitKeyreadStuff();
    ClosePadIO();
    m_init = false;
    for (int i = 0; i < NUM_PADS; i++) {
        if (m_pads[i])
            delete m_pads[i];
        m_pads[i] = 0;
    }
}

/* the keyboard and the pads ticked (SOCOM 1's) */
void CInput::Tick(float dt)
{
    if (m_keyboard)
        m_keyboard->Tick(dt);
    for (int i = 0; i < NUM_PADS; i++)
        if (m_pads[i])
            m_pads[i]->Tick(dt);
}

/* every pad's state cleared (SOCOM 1's) */
void CInput::Flush()
{
    for (int i = 0; i < NUM_PADS; i++)
        if (m_pads[i])
            m_pads[i]->Flush(false);
}

/* the pad on a port, opened: the open one, or one made (or re-made in place) and kept only if it
 * opens (SOCOM 1's) */
CPad *CInput::CreatePad(unsigned int n)
{
    if (n >= NUM_PADS)
        return 0;
    CPad *pad = m_pads[n];

    if (pad && pad->IsOpen())
        return pad;
    if (!pad)
        pad = m_pads[n] = new CPad(n, 0);
    else
        new (pad) CPad(n, 0);
    if (!pad->IsOpen())
        if (m_pads[0])
            m_pads[0]->~CPad();
    return m_pads[0];
}

/* the pad on a port destroyed, its storage kept (SOCOM 1's freed it) */
void CInput::DeletePad(unsigned int n)
{
    if (n < NUM_PADS)
        if (m_pads[n])
            m_pads[n]->~CPad();
}
