#include "PadRing.hpp"

void PadRing::WriteToRing(PadEntry *entry)
{
    uint32_t index = m_updatedindex;
    if(index == 7)
        index = 0;
    else
        index++;
    m_entries[index].currpadstate = entry->currpadstate;
    m_entries[index].pressedpadstate = entry->pressedpadstate;
    m_entries[index].releasedpadstate = entry->releasedpadstate;
    if(index == 0) // When index is 0 we need to update tickcount
    {
        m_oldtickcount = m_tickcount;
        m_tickcount = svcGetSystemTick();
    }
    m_updatedindex = index;
}