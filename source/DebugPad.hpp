#include <3ds.h>
#include "DebugPadRing.hpp"

class DebugPad
{
    public:
        void Initialize();
        void SetDebugPadRing(DebugPadRing *ring) { m_ring = ring; };
        void SetTimer();
        Handle *GetTimer() { return &m_timer; };
        Handle *GetEvent() { return &m_event; };
        bool ReadEntry(DebugPadEntry *entry);
        void Sampling();
    
    private:
        uint8_t m_isinitialized = 0;
        uint16_t m_latestkeys = 0;
        Handle m_timer;
        Handle m_event;
        DebugPadRing *m_ring = nullptr;
        DebugPadEntry m_oldentry;
        bool m_writetoring = false;
};