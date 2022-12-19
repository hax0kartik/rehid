#pragma once
#include <3ds.h>
#include "exclusive_rw.hpp"

struct GyroscopeEntry
{
    int16_t x;
    int16_t y;
    int16_t z;
};
static_assert(sizeof(GyroscopeEntry) == 6, "Size of AccelerometerEntry is invalid");

class GyroscopeRing
{
    public:
        GyroscopeRing(){
            Reset();
        };

        void Reset(){
            m_tickcount = -1;
            m_oldtickcount = -1;
            m_updatedindex = -1;
        };

        void SetRaw(GyroscopeEntry entry){
            ExclusiveWrite((u16*)&m_rawentry.x, entry.x);
            ExclusiveWrite((u16*)&m_rawentry.y, entry.y);
            ExclusiveWrite((u16*)&m_rawentry.z, entry.z);
        }

        void WriteToRing(GyroscopeEntry entry);
    private:
        int64_t m_tickcount = 0;
        int64_t m_oldtickcount = 0;
        int32_t m_updatedindex = 0;
        uint32_t padding;
        GyroscopeEntry m_rawentry;
        uint16_t padding2;
        GyroscopeEntry m_entries[32];
};
static_assert(sizeof(GyroscopeRing) == 0xE0, "Sizeof Gyroring is not 0xE0 bytess!");