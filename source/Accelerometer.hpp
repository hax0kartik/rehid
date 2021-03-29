#pragma once
#include "AccelerometerRing.hpp"

class Accelerometer
{
    public:
        void Initialize();
        void Sampling();
        void SetAccelerometerRing(AccelerometerRing *ring) { m_ring = ring; };
        Handle *GetEvent() { return &m_event; };
        Handle *GetIntrEvent() { return &m_irqevent; };
        void EnableOrDisableInterrupt(u8 explicitdisable = -1);
        void SetAccelerometerStatus(u8 enable);
    private:
        AccelerometerRing *m_ring = nullptr;
        Handle m_event;
        Handle m_irqevent;
        uint8_t m_initialized = 0;
        int32_t m_refcount = 0;
};