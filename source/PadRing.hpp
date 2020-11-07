#pragma once
#include <3ds.h>

struct CirclePad
{
    s16 x;
    s16 y;
};

struct PadEntry
{
    u32 currpadstate;
    u32 pressedpadstate;
    u32 releasedpadstate;
    CirclePad circlepadstate;
};

class PadRing
{
    public:
        PadRing(){
            Reset();
        }
        void Reset(){
            m_tickcount = -1;
            m_oldtickcount = -1;
            m_updatedindex = -1;
        }
        void SetCurrPadState(uint32_t state)
        {
            m_curpadstate = state;
        }
        void WriteToRing(PadEntry *entry);
    private:
        int64_t m_tickcount = 0;
        int64_t m_oldtickcount = 0;
        uint32_t m_updatedindex = 0;
        uint32_t padding;
        uint32_t m_unk;
        uint32_t m_curpadstate;
        CirclePad m_circlepadraw;
        PadEntry m_entries[8];
};