#include "CirclePad.hpp"

void CirclePad::RawToCirclePadCoords(CirclePadEntry *result, CirclePadEntry raw)
{
    result->x = raw.x;
    s16 diffx = raw.x - m_latestdata.x;
    if ( diffx < 0 )
        diffx = -diffx;
    else if ( diffx > 5 )
        m_latestdata.x = raw.x;
    else
        result->x = m_latestdata.x;
  
    result->y = raw.y;
    s16 diffy = (raw.y - m_latestdata.y);
    if ( diffy < 0 )
        diffy = -diffy;
    if ( diffy <= 5 )
        result->y = m_latestdata.y;
    else if ( diffy > 5 )
        m_latestdata.y = raw.y;

    s16 tmpx = ((result->x - m_center.x) * m_scalex);
    s16 tmpx2;
    if ( tmpx >= 0 )
        tmpx2 = tmpx;
    else
        tmpx2 = -tmpx;
    
    s16 finalx = ((tmpx2 & 7u) >= 3) + (tmpx2 >> 3);
    if (tmpx < 0)
        finalx = -finalx;
    result->x = finalx;
    
    s16 tmpy = ((result->y - m_center.y) * m_scaley);
    s16 tmpy2;
    if ( (tmpy & 0x8000u) == 0 )
        tmpy2 = tmpy;
    else
        tmpy2 = -tmpy;
    s16 finaly = ((tmpy2 & 7u) >= 3) + (tmpy2 >> 3);
    if ( (tmpy & 0x8000u) != 0 )
        finaly = -finaly;
    result->y = finaly;
}