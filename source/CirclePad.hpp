#pragma once
#include <3ds.h>
#include "Remapper.hpp"
struct CirclePadEntry
{
    s16 x;
    s16 y;
};

class CirclePad
{
    public:
        void RawToCirclePadCoords(CirclePadEntry *result, CirclePadEntry raw);
        uint32_t ConvertToHidButtons(CirclePadEntry *circlepad, uint32_t buttons, Remapper *remapper);
        void AdjustValues(int16_t *adjustedx, int16_t *adjustedy, int rawx, int rawy, int min, int max);
    private:
        CirclePadEntry m_latestdata = {0x800, 0x800};
        CirclePadEntry m_center = {0x800, 0x800};
        float m_scalex = 1.0f;
        float m_scaley = 1.0f;
};