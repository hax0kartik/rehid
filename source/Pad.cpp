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

void Pad::ReadFromIO(PadEntry *entry, uint32_t *raw)
{
    uint32_t latest = (u32)IOHIDPAD ^ 0xFFF;
    *raw = latest;
    latest = latest & ~(2 * (latest & 0x40) | ((latest & 0x20u) >> 1));
    entry->pressedpadstate = (latest ^ m_latestkeys) & ~m_latestkeys;
    entry->releasedpadstate = (latest ^ m_latestkeys) & m_latestkeys;
    entry->currpadstate = latest;
    m_latestkeys = latest;
}

void Pad::Sampling()
{
    PadEntry finalentry; uint32_t latest;
    svcSetTimer(m_timer, 4000000LL, 0LL);
    ReadFromIO(&finalentry, &latest);
    m_ring->SetCurrPadState(latest);
    m_ring->WriteToRing(&finalentry);
    svcSignalEvent(m_event);
}