#include "GyroscopeRing.hpp"

void GyroscopeRing::WriteToRing(GyroscopeEntry entry)
{
    uint32_t index = m_updatedindex;
    if(index == 31)
        index = 0;
    else
        index++;
    ExclusiveWrite((u16*)&m_entries[index].x, entry.x);
    ExclusiveWrite((u16*)&m_entries[index].y, entry.y);
    ExclusiveWrite((u16*)&m_entries[index].z, entry.z);
    if(index == 0) // When index is 0 we need to update tickcount
    {
        m_oldtickcount = m_tickcount;
        m_tickcount = svcGetSystemTick();
    }
    ExclusiveWrite(&m_updatedindex, index);
}