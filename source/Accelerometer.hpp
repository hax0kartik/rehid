#pragma once
#include "AccelerometerRing.hpp"

struct AccelerometerCalibration
{
    int16_t scalex;
    int16_t offsetx;
    int16_t scaley;
    int16_t offsety;
    int16_t scalez;
    int16_t offsetz;
};

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
        void EnableAndIncreementRef();
        void DisableAndDecreementRef();
        void CalibrateVals(AccelerometerEntry *raw, AccelerometerEntry *final);
    private:
        AccelerometerRing *m_ring = nullptr;
        AccelerometerCalibration m_calib;
        Handle m_event;
        Handle m_irqevent;
        uint8_t m_initialized = 0;
        int32_t m_refcount = 0;
};