#pragma once
#include <3ds.h>
#include "exclusive_rw.hpp"
#include "CirclePad.hpp"

struct PadEntry
{
    s32 currpadstate;
    s32 pressedpadstate;
    s32 releasedpadstate;
    CirclePadEntry circlepadstate;
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
        void SetCurrPadState(uint32_t state, CirclePadEntry circlepadentry)
        {
            ExclusiveWrite((u16*)&m_circlepadraw.x, circlepadentry.x);
            ExclusiveWrite((u16*)&m_circlepadraw.y, circlepadentry.y);
            ExclusiveWrite((s32*)&m_curpadstate, state);
        }
        void Set3dSliderVal(float val){
            m_3dsliderval = val;
        }
        void WriteToRing(PadEntry *entry, CirclePadEntry *circlepadentry);
    private:
        int64_t m_tickcount = 0;
        int64_t m_oldtickcount = 0;
        int32_t m_updatedindex = 0;
        uint32_t padding;
        float m_3dsliderval = 0.0f;
        uint32_t m_curpadstate;
        CirclePadEntry m_circlepadraw;
        uint32_t padding2;
        PadEntry m_entries[8];
};