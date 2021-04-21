#include "Gyroscope.hpp"
#include "mcuhid.hpp"

void Gyroscope::Initialize()
{
    if(!m_initialized)
    {
        svcCreateEvent(&m_event, RESET_ONESHOT);
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
    svcSignalEvent(m_event);
}