#include <cmath>
#include "CirclePad.hpp"


void CirclePad::GetConfigSettings() {
    struct circlepadcalib {
        float scalex;
        float scaley;
        uint16_t centerx;
        uint16_t centery;
        uint8_t pad[0x10];
    };

    static_assert(sizeof(circlepadcalib) == 0x1C, "Size of circlepadcalib is not 0x1C bytes!");
    circlepadcalib calib;
    cfguInit();
    Result ret = CFG_GetConfigInfoBlk4(0x1C, 0x40001, &calib);
    cfguExit();

    if (R_FAILED(ret))
        *(u32*)ret = 0xCCC;

    m_scalex = calib.scalex;
    m_scaley = calib.scaley;
    m_center.x = calib.centerx;
    m_center.y = calib.centery;
}

void CirclePad::RawToCirclePadCoords(CirclePadEntry *result, CirclePadEntry raw) {
    result->x = raw.x;
    s16 diffx = raw.x - m_latestdata.x;

    if (diffx < 0)
        diffx = -diffx;
    else if (diffx > 5)
        m_latestdata.x = raw.x;
    else
        result->x = m_latestdata.x;

    result->y = raw.y;
    s16 diffy = (raw.y - m_latestdata.y);

    if (diffy < 0)
        diffy = -diffy;

    if (diffy <= 5)
        result->y = m_latestdata.y;
    else if (diffy > 5)
        m_latestdata.y = raw.y;

    s16 tmpx = ((result->x - m_center.x) * m_scalex);
    s16 tmpx2;

    if (tmpx >= 0)
        tmpx2 = tmpx;
    else
        tmpx2 = -tmpx;

    s16 finalx = ((tmpx2 & 7u) >= 3) + (tmpx2 >> 3);

    if (tmpx < 0)
        finalx = -finalx;

    result->x = finalx;

    s16 tmpy = ((result->y - m_center.y) * m_scaley);
    s16 tmpy2;

    if ((tmpy & 0x8000u) == 0)
        tmpy2 = tmpy;
    else
        tmpy2 = -tmpy;

    s16 finaly = ((tmpy2 & 7u) >= 3) + (tmpy2 >> 3);

    if ((tmpy & 0x8000u) != 0)
        finaly = -finaly;

    result->y = finaly;
}

void CirclePad::AdjustValues(int16_t *adjustedx, int16_t *adjustedy, int rawx, int rawy, int min, int max) {

    int length1, v2;
    int length = rawx * rawx + rawy * rawy;

    if (length > min * min) {
        int tmp = length << 14;

        if (length << 14 > 0) {
            length1 = floor(sqrt(tmp));
        } else {
            length1 = 0;
        }

        if (max * max <= length)
            tmp = max - min;

        if (max * max > length) {
            v2 = length1 - (min << 7);
            tmp = (length1 >> 7) - min;
        } else {
            v2 = tmp << 7;
        }

        *adjustedx = rawx * v2 / length1;
        *adjustedy = rawy * v2 / length1;
    } else {
        *adjustedy = 0;
        *adjustedx = 0;
    }
}

uint32_t CirclePad::ConvertToHidButtons(CirclePadEntry *circlepad, uint32_t buttons) {
    int32_t tanybyx = 0, tanxbyy = 0;
    CirclePadEntry adjusted;
    buttons = buttons & 0xFFFFFFF;
    u32 left = KEY_CPAD_LEFT, right = KEY_CPAD_RIGHT, up = KEY_CPAD_UP, down = KEY_CPAD_DOWN;
    AdjustValues(&adjusted.x, &adjusted.y, circlepad->x, circlepad->y, 40, 145);

    if (adjusted.x)
        tanybyx = (adjusted.y << 8) / adjusted.x;

    if (adjusted.y)
        tanxbyy = (adjusted.x << 8) / adjusted.y;

    if (0 < adjusted.x && -443 <= tanybyx && 443 >= tanybyx)
        buttons |= right;
    else if (0 > adjusted.x && -443 <= tanybyx && 443 >= tanybyx)
        buttons |= left;

    if (0 < adjusted.y && -443 <= tanxbyy && 443 >= tanxbyy)
        buttons |= up;
    else if (0 > adjusted.y && -443 <= tanxbyy && 443 >= tanxbyy)
        buttons |= down;

    return buttons;
}