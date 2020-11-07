#include "Pad.hpp"

void Pad::Initialize()
{
    if(!m_isinitialized)
    {
        m_isinitialized = true;
        m_latestkeys = IOHIDPAD ^ 0xFFF;
        svcCreateTimer(&m_timer, RESET_ONESHOT);
        svcCreateEvent(&m_event, RESET_ONESHOT);
    }
}

void Pad::SetTimer()
{
    if(R_FAILED(svcSetTimer(m_timer, 4000000LL, 0LL)))
        svcBreak(USERBREAK_ASSERT);
}

void Pad::ReadFromIO(PadEntry *entry)
{
    uint32_t latest = IOHIDPAD ^ 0xFFF;
    entry->pressedpadstate = (latest ^ m_latestkeys) & ~m_latestkeys;
    entry->releasedpadstate = m_latestkeys & ~latest;
    entry->currpadstate = latest;
    m_latestkeys = latest;
}

void Pad::Sampling()
{
    PadEntry finalentry;
    svcSetTimer(m_timer, 4000000LL, 0LL);
    ReadFromIO(&finalentry);
    m_ring->SetCurrPadState(m_latestkeys);
    m_ring->WriteToRing(&finalentry);
    svcSignalEvent(m_event);
}