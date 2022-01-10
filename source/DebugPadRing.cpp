#include "DebugPadRing.hpp"

void DebugPadRing::WriteToRing(DebugPadEntry *entry)
{
    uint32_t index = m_updatedindex;
    if(index == 7)
        index = 0;
    else
        index++;
    ExclusiveWrite16((u16*)&m_entries[index].currpadstate, entry->currpadstate);
    ExclusiveWrite16((u16*)&m_entries[index].pressedpadstate, entry->pressedpadstate);
    ExclusiveWrite16((u16*)&m_entries[index].releasedpadstate, entry->releasedpadstate);
    ExclusiveWrite8((u8*)&m_entries[index].leftstickx, entry->leftstickx);
    ExclusiveWrite8((u8*)&m_entries[index].leftsticky, entry->leftsticky);
    ExclusiveWrite8((u8*)&m_entries[index].rightstickx, entry->rightstickx);
    ExclusiveWrite8((u8*)&m_entries[index].rightsticky, entry->rightsticky);
    if(index == 0) // When index is 0 we need to update tickcount
    {
        m_oldtickcount = m_tickcount;
        m_tickcount = svcGetSystemTick();
    }
    ExclusiveWrite32(&m_updatedindex, index);
}