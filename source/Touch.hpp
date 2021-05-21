#pragma once
#include "TouchRing.hpp"
#include "Remapper.hpp"

struct touchcfg
{
    int16_t rawx0;
    int16_t rawy0;
    int16_t pointx0;
    int16_t pointy0;
    int16_t rawx1;
    int16_t rawy1;
    int16_t pointx1;
    int16_t pointy1;
};

struct touchcalib
{
    int32_t x;
    int32_t xdotsize;
    int32_t xdotsizeinv;
    int32_t y;
    int32_t ydotsize;
    int32_t ydotsizeinv;
};

class Touch
{
    public:
        void Initialize();
        void Sampling(u32 touchscreendata, Remapper *remapper);
        void SetTouchRing(TouchRing *ring) { m_ring = ring; };
        void RawToPixel(int *arr, TouchEntry *pixeldata, TouchEntry *rawdata);
        Handle *GetEvent() { return &m_event; };
    private:
        void CalculateCalibrationStruct();
        TouchRing *m_ring = nullptr;
        TouchEntry m_latest;
        Handle m_event;
        uint8_t m_initialized = 0;
        touchcfg m_touchcfg;
        touchcalib m_calib;
};