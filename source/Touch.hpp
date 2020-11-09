#pragma once
#include "TouchRing.hpp"
extern "C"
{
    #include "codec.h"
}

class Touch
{
    public:
        void Initialize();
        void Sampling();
        void SetTouchRing(TouchRing *ring) { m_ring = ring; };
        void RawToPixel(int *arr, TouchEntry *pixeldata, TouchEntry *rawdata);
        Handle *GetEvent() { return &m_event; };
    private:
        TouchRing *m_ring = nullptr;
        TouchEntry m_latest;
        Handle m_event;
        uint8_t m_initialized = 0;
};