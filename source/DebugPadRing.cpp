#include "DebugPadRing.hpp"

void DebugPadRing::WriteToRing(DebugPadEntry *entry)
{
    uint32_t index = m_updatedindex;
    if(index == 7)
        index = 0;
    else
        index++;
    ExclusiveWrite((u16*)&m_entries[index].currpadstate, entry->currpadstate);
    ExclusiveWrite((u16*)&m_entries[index].pressedpadstate, entry->pressedpadstate);
    ExclusiveWrite((u16*)&m_entries[index].releasedpadstate, entry->releasedpadstate);
    ExclusiveWrite((u8*)&m_entries[index].leftstickx, entry->leftstickx);
    ExclusiveWrite((u8*)&m_entries[index].leftsticky, entry->leftsticky);
    ExclusiveWrite((u8*)&m_entries[index].rightstickx, entry->rightstickx);
    ExclusiveWrite((u8*)&m_entries[index].rightsticky, entry->rightsticky);
    if(index == 0) // When index is 0 we need to update tickcount
    {
        m_oldtickcount = m_tickcount;
        m_tickcount = svcGetSystemTick();
    }
    ExclusiveWrite(&m_updatedindex, index);
}