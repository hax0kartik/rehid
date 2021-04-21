#pragma once
#include "GyroscopeRing.hpp"

class Gyroscope
{
    public:
        void Initialize();
        void Sampling();
        void SetGyroscopeRing(GyroscopeRing *ring) { m_ring = ring; };
        Handle *GetEvent() { return &m_event; };
    private:
        GyroscopeRing *m_ring = nullptr;
        Handle m_event;
        Handle m_irqevent;
        uint8_t m_initialized = 0;
};