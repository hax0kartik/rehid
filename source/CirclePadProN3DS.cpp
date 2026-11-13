#include <3ds.h>
#include "services/iru.hpp"
#include "services/irrst.hpp"
#include "CirclePadProN3DS.hpp"

#define PA_PTR(addr)            (void *)((u32)(addr) | 1 << 31);

Result CPPN3DS::Initialize() {
    Result ret = 0;

    if (R_FAILED(ret = iruInit_())) {
        svcBreak(USERBREAK_ASSERT);
    }

    uint32_t keysdirectpa = IRU_GetLatestKeysPA_();
    uint32_t statedirectpa = IRU_GetStatePA_();

    m_latestkeyspa = (uint32_t*)PA_PTR(keysdirectpa);
    m_statepa = (uint32_t*)PA_PTR(statedirectpa);

    iruExit_();

    m_ring.Reset();

    m_initialized = 1;
    m_enteringsleep = 0;

    return ret;
}

void CPPN3DS::SetTimer() {
    svcCreateTimer(&m_timer, RESET_ONESHOT);

    if (R_FAILED(svcSetTimer(m_timer, 8000000LL, 0LL)))
        svcBreak(USERBREAK_ASSERT);
}

void CPPN3DS::Sampling() {
    CPPEntry entry;
    svcSetTimer(m_timer, 8000000LL, 0LL);

    if (m_enteringsleep || !m_initialized) {
        return;
    }

    volatile uint32_t latest = *m_latestkeyspa;

    if ((latest & 0x10000) == 0x10000) {
        latest |= 0x4000;
        latest &= ~0x10000;
    }

    if ((latest & 0x20000) == 0x20000) {
        latest |= 0x8000;
        latest &= ~0x20000;
    }

    entry.pressedpadstate = (latest ^ m_latestkeys) & ~m_latestkeys;
    entry.releasedpadstate = (latest ^ m_latestkeys) & m_latestkeys;
    entry.currpadstate = latest;
    m_ring.WriteToRing(&entry);
    m_latestkeys = latest;
}

void CPPN3DS::ScanInput(Remapper *remapper) {
    if (m_enteringsleep || !m_initialized) {
        m_cppstate = {};
        return;
    }
    uint8_t state = *m_statepa;

    int refcount = irrstGetRefCount_();

    if (state == 1 && refcount >= 1 && remapper->m_overridecpadpro == 0) { //iruser was initialized
        irrstExit_();
    } else if (state == 0 && refcount <= 0) {
        irrstInit_(0);
    }

    m_cppstate = GetLatestInputFromRing();
}

static uint32_t CheckSectionUpdateTime(uint32_t id, CPPRing *ring) {
    int64_t tick0 = 0, tick1 = 0;

    if (id == 0) {
        tick0 = ring->GetTickCount();
        tick1 = ring->GetOldTickCount();

        if (tick0 == tick1 || tick0 < 0 || tick1 < 0)
            return 1;
    }

    return 0;
}

CPPEntry CPPN3DS::GetLatestInputFromRing() {
    uint32_t id = 0;
    CPPEntry latest {};

    id = m_ring.GetIndex(); //PAD / circle-pad

    if (id > 7)
        id = 0;

    if (CheckSectionUpdateTime(id, &m_ring) == 0) {
        latest = m_ring.GetLatest(id);
    }

    return latest;
}
