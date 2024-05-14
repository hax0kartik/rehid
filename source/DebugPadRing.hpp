#pragma once
#include <3ds.h>
#include "exclusive_rw.hpp"

struct DebugPadEntry {
    int16_t currpadstate;
    int16_t pressedpadstate;
    int16_t releasedpadstate;
    int8_t leftstickx;
    int8_t leftsticky;
    int8_t rightstickx;
    int8_t rightsticky;
    uint8_t pad[2];
};

static_assert(sizeof(DebugPadEntry) == 0xC, "Size of DebugPadEntry is invalid");

class DebugPadRing {
public:
    DebugPadRing() {
        Reset();
    };

    void Reset() {
        m_tickcount = -1;
        m_oldtickcount = -1;
        m_updatedindex = -1;
    };

    void WriteToRing(DebugPadEntry *entry);
private:
    int64_t m_tickcount = 0;
    int64_t m_oldtickcount = 0;
    int32_t m_updatedindex = 0;
    uint32_t padding;
    DebugPadEntry m_entries[8];
};

static_assert(sizeof(DebugPadRing) == 0x78, "Sizeof DebugPadRing is not 0x78 bytess!");