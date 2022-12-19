#include "TouchRing.hpp"

void TouchRing::WriteToRing(TouchEntry entry)
{
    uint32_t index = m_updatedindex;
    if(index == 7)
        index = 0;
    else
        index++;
    ExclusiveWrite((u16*)&m_entries[index].x, entry.x);
    ExclusiveWrite((u16*)&m_entries[index].y, entry.y);
    m_entries[index].touch = entry.touch;
    if(index == 0) // When index is 0 we need to update tickcount
    {
        m_oldtickcount = m_tickcount;
        m_tickcount = svcGetSystemTick();
    }
    ExclusiveWrite(&m_updatedindex, index);
}