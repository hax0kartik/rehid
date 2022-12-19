#pragma once
#include <3ds.h>
#include "exclusive_rw.hpp"
struct TouchEntry
{
    int16_t x;
    int16_t y;
    uint8_t touch;
    uint8_t pad[3];
};

class TouchRing
{
    public:
        TouchRing(){
            Reset();
        };
        void Reset()
        {
            m_tickcount = -1;
            m_oldtickcount = -1;
            m_updatedindex = -1;
        };
        void SetRaw(TouchEntry entry) {
            ExclusiveWrite((u16*)&m_rawentry.x, entry.x);
            ExclusiveWrite((u16*)&m_rawentry.y, entry.y);
            m_rawentry.touch = entry.touch;
        }
        void WriteToRing(TouchEntry entry);
    private:
        int64_t m_tickcount = 0;
        int64_t m_oldtickcount = 0;
        int32_t m_updatedindex = 0;
        uint32_t padding;
        TouchEntry m_rawentry;
        TouchEntry m_entries[8];
};