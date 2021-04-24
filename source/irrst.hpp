#pragma once
#include <3ds.h>
#include "exclusive_rw.hpp"
#include "CirclePad.hpp"

Result iruInit_(uint8_t steal = 0);
void irrstExit_(void);
void iruScanInput_(void);
u32 iruKeysHeld_(void);
Result irrstInit_(uint8_t);
void irSampling();

struct IrrstEntry
{
    s32 currpadstate;
    s32 pressedpadstate;
    s32 releasedpadstate;
    s16 pad0; // circlepad x
    s16 pad1; // circlepad y
};
static_assert(sizeof(IrrstEntry) == 0x10, "Sizeof irrstentry is not 0x10 bytes!");

class IrrstRing
{
    public:
        IrrstRing(){
            Reset();
        }
        void Reset(){
            m_tickcount = 0;
            m_oldtickcount = 0;
            m_updatedindex = 0;
        }
        s32 GetLatest(u8 index) { return m_entries[index].currpadstate; };
        s32 GetIndex() { return m_updatedindex; };
        int64_t GetTickCount() { return m_tickcount; };
        int64_t GetOldTickCount() { return m_oldtickcount; };
        void WriteToRing(IrrstEntry *entry);
    private:
        int64_t m_tickcount = 0;
        int64_t m_oldtickcount = 0;
        int32_t m_updatedindex = 0;
        IrrstEntry m_entries[8];
};
static_assert(sizeof(IrrstRing) == 0x98, "Size of irrstring is not 0x98 bytes!");