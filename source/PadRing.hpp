#pragma once
#include <3ds.h>
#include "exclusive_rw.hpp"
struct CirclePad
{
    s16 x;
    s16 y;
};

struct PadEntry
{
    s32 currpadstate;
    s32 pressedpadstate;
    s32 releasedpadstate;
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
            ExclusiveWrite16((u16*)&m_circlepadraw.x, 0);
            ExclusiveWrite16((u16*)&m_circlepadraw.y, 0);
            ExclusiveWrite32((s32*)&m_curpadstate, state);
        }
        void WriteToRing(PadEntry *entry);
    private:
        int64_t m_tickcount = 0;
        int64_t m_oldtickcount = 0;
        int32_t m_updatedindex = 0;
        uint32_t padding;
        uint32_t m_unk;
        uint32_t m_curpadstate;
        CirclePad m_circlepadraw;
        uint32_t padding2;
        PadEntry m_entries[8];
};