#include "DebugPad.hpp"
extern "C" {
#include "i2c.h"
}

bool DebugPad::ReadEntry(DebugPadEntry *entry)
{
    uint8_t buf[21];
    Result ret = 0;
    if(R_FAILED(ret = I2C_ReadRegisterBuffer(12, 0, buf, 21))){
        return false;
    }
    int i = 0;
    while(buf[i] != 0xFF) i++;
    if(i == 21) return false;

    entry->currpadstate = ~(buf[5] | (buf[4] << 8));
    entry->pressedpadstate = (entry->currpadstate ^ m_latestkeys) & ~m_latestkeys;
    entry->releasedpadstate = (entry->currpadstate ^ m_latestkeys) & m_latestkeys;
    m_latestkeys = entry->currpadstate;
    return true;
}

void DebugPad::SetTimer()
{
    if(R_FAILED(svcSetTimer(m_timer, 16000000LL, 16000000LL)))
        svcBreak(USERBREAK_ASSERT);
}

void DebugPad::Initialize()
{
    if(m_isinitialized) return;

    Result ret = 0;
    uint8_t buf = 0x55; // Disables encryption
    if(R_FAILED(ret = I2C_WriteRegisterBuffer(12, 0xF0, &buf, sizeof(buf)))){
        return;
    }

    svcSleepThread(5e+6);
    if(R_FAILED(ret = I2C_ReadRegisterBuffer(12, 0xFF, &buf, sizeof(buf)))){
        return;
    }
    if(buf != 0xF0) return;

    svcSleepThread(5e+6);

    buf = 0; // Disables vibration
    if(R_FAILED(ret = I2C_WriteRegisterBuffer(12, 0xF0, &buf, sizeof(buf)))){
        return;
    }

    /* Official hid code, now generates a key and then turns encryption on
       we simply skip all of that. */
    DebugPadEntry entry;
    if(!ReadEntry(&entry)){
        return;
    }

    svcCreateTimer(&m_timer, RESET_ONESHOT);
    svcCreateEvent(&m_event, RESET_ONESHOT);
    m_isinitialized = 1;
}

void DebugPad::Sampling()
{
    DebugPadEntry entry;
    if(!ReadEntry(&entry)){
        /* some logic */
    }

    if(m_writetoring){
        m_ring->WriteToRing(&m_oldentry);
        m_oldentry = entry;
    }
    else{
        m_oldentry = entry;
        m_writetoring = true;
    }
    svcSignalEvent(m_event);
}