#pragma once
#include <3ds.h>
#include "exclusive_rw.hpp"

struct AccelerometerEntry
{
    int16_t x;
    int16_t y;
    int16_t z;
};
static_assert(sizeof(AccelerometerEntry) == 6, "Size of AccelerometerEntry is invalid");

class AccelerometerRing
{
    public:
        AccelerometerRing(){
            Reset();
        };

        void Reset()
        {
            m_tickcount = -1;
            m_oldtickcount = -1;
            m_updatedindex = -1;
        };

        void SetRaw(AccelerometerEntry entry) {
            ExclusiveWrite((u16*)&m_rawentry.x, entry.x);
            ExclusiveWrite((u16*)&m_rawentry.y, entry.y);
            ExclusiveWrite((u16*)&m_rawentry.z, entry.z);
        }

        void WriteToRing(AccelerometerEntry entry);
    private:
        int64_t m_tickcount = 0;
        int64_t m_oldtickcount = 0;
        int32_t m_updatedindex = 0;
        uint32_t padding;
        AccelerometerEntry m_rawentry;
        uint16_t padding2;
        AccelerometerEntry m_entries[8];
};