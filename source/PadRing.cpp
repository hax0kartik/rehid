#include "PadRing.hpp"

void PadRing::WriteToRing(PadEntry *entry, CirclePadEntry *circlepadentry)
{
    uint32_t index = m_updatedindex;
    if(index == 7)
        index = 0;
    else
        index++;
    ExclusiveWrite(&m_entries[index].currpadstate, entry->currpadstate);
    ExclusiveWrite(&m_entries[index].pressedpadstate, entry->pressedpadstate);
    ExclusiveWrite(&m_entries[index].releasedpadstate, entry->releasedpadstate);
    ExclusiveWrite((u16*)&m_entries[index].circlepadstate.x, circlepadentry->x);
    ExclusiveWrite((u16*)&m_entries[index].circlepadstate.y, circlepadentry->y);
    if(index == 0) // When index is 0 we need to update tickcount
    {
        m_oldtickcount = m_tickcount;
        m_tickcount = svcGetSystemTick();
    }
    ExclusiveWrite(&m_updatedindex, index);
}