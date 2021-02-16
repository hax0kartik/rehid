#include "Pad.hpp"
#include "printf.h"
void Pad::Initialize()
{
    if(!m_isinitialized)
    {
        m_isinitialized = true;
        m_latestkeys = (vu32)(IOHIDPAD) ^ 0xFFF;
        svcCreateTimer(&m_timer, RESET_ONESHOT);
        svcCreateEvent(&m_event, RESET_ONESHOT);
    }
}

void Pad::SetTimer()
{
    if(R_FAILED(svcSetTimer(m_timer, 4000000LL, 0LL)))
        svcBreak(USERBREAK_ASSERT);
}

void Pad::ReadFromIO(PadEntry *entry, uint32_t *raw, CirclePadEntry circlepad)
{
    volatile uint32_t latest = (vu32)(IOHIDPAD) ^ 0xFFF; 
    *raw = latest;
    latest = latest & ~(2 * (latest & 0x40) | ((latest & 0x20u) >> 1));
    latest = m_circlepad.ConvertToHidButtons(circlepad, latest);
    latest = m_remapper.Remap(latest);
    entry->pressedpadstate = (latest ^ m_latestkeys) & ~m_latestkeys;
    entry->releasedpadstate = (latest ^ m_latestkeys) & m_latestkeys;
    entry->currpadstate = latest;
    m_latestkeys = latest;
}

void Pad::Sampling(u32 rcpr)
{
    PadEntry finalentry; uint32_t latest;
    CirclePadEntry rawcirclepad;
    rawcirclepad.x = rcpr & 0xFFF;
    rawcirclepad.y = (rcpr & 0xFFF000) >> 12;
    CirclePadEntry finalcirclepad;
    svcSetTimer(m_timer, 4000000LL, 0LL);
    m_circlepad.RawToCirclePadCoords(&finalcirclepad, rawcirclepad);
    if(!(m_counter % 3u))
    {
        m_slider.ReadValuesFromMCU();
        float sliderval = m_slider.Normalize();
        *(vu32*)0x1FF81080 = (u32)2.0f;
    }
    ++m_counter;
    ReadFromIO(&finalentry, &latest, finalcirclepad);
    m_ring->SetCurrPadState(latest, rawcirclepad);
    m_ring->WriteToRing(&finalentry, &finalcirclepad);
    svcSignalEvent(m_event);
}

void Pad::RemapGenFileLoc()
{
    m_remapper.GenerateFileLocation(); 
    Result ret = m_remapper.ReadConfigFile();
        if(ret == -1) return;
        else if(ret) *(u32*)ret = 0xF00FBABE;
    m_remapper.ParseConfigFile();
}