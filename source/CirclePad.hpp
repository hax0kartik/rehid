#pragma once
#include <3ds.h>

struct CirclePadEntry
{
    s16 x;
    s16 y;
};

class CirclePad
{
    public:
        void RawToCirclePadCoords(CirclePadEntry *result, CirclePadEntry raw);
    private:
        CirclePadEntry m_latestdata = {0x800, 0x800};
        CirclePadEntry m_center = {0x800, 0x800};
        float m_scalex = 1.0f;
        float m_scaley = 1.0f;
};