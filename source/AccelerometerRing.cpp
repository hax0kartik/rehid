#include "AccelerometerRing.hpp"

void AccelerometerRing::WriteToRing(AccelerometerEntry entry)
{
    uint32_t index = m_updatedindex;
    if(index == 7)
        index = 0;
    else
        index++;
    ExclusiveWrite16((u16*)&m_entries[index].x, entry.x);
    ExclusiveWrite16((u16*)&m_entries[index].y, entry.y);
    ExclusiveWrite16((u16*)&m_entries[index].z, entry.z);
    if(index == 0) // When index is 0 we need to update tickcount
    {
        m_oldtickcount = m_tickcount;
        m_tickcount = svcGetSystemTick();
    }
    ExclusiveWrite32(&m_updatedindex, index);
}