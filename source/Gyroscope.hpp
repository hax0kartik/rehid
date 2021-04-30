#pragma once
#include "GyroscopeRing.hpp"

class Gyroscope
{
    public:
        void Initialize();
        void Sampling();
        void SetGyroscopeRing(GyroscopeRing *ring) { m_ring = ring; };
        Handle *GetEvent() { return &m_event[m_returnedhandle]; };
        void IncreementHandleIndex() { ++m_returnedhandle; };
        void DecreementhandleIndex() { --m_returnedhandle; };
    private:
        GyroscopeRing *m_ring = nullptr;
        Handle m_event[6];
        uint8_t m_returnedhandle = 0; 
        uint8_t m_initialized = 0;
};