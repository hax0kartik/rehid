#include "Gyroscope.hpp"
#include "mcuhid.hpp"

void Gyroscope::Initialize()
{
    if(!m_initialized)
    {
        for(int i = 0; i < 6; i++)
            if(R_FAILED(svcCreateEvent(&m_event[i], RESET_ONESHOT))) *(u32*)0x8 = 0x1234;
        m_initialized = 1;
    }
}

void Gyroscope::Sampling()
{
    GyroscopeEntry rawvals;
    rawvals.x = 0;
    rawvals.y = 0;
    rawvals.z = 0;
    m_ring->SetRaw(rawvals);
    m_ring->WriteToRing(rawvals);
    for(int i = 0; i < 6; i++)
        svcSignalEvent(m_event[i]);
    //svcClearEvent(m_qtmevent);
    //if(R_FAILED(svcSignalEvent(m_event))) *(u32*)0xF = 0x98765;
    //if(R_FAILED(svcSignalEvent(m_qtmevent))) *(u32*)0xF = 0x98764;
}