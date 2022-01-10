#include "DebugPad.hpp"
#include "CirclePad.hpp"
extern "C" {
#include "i2c.h"
}

static uint8_t DecryptData(uint8_t buf){
    return (buf ^ 0x17) + 0x17;
}

int16_t map(int16_t x, int16_t in_min, int16_t in_max, int16_t out_min, int16_t out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

static inline uint32_t RemapKey(int16_t keys){
    uint32_t nkeys = 0;

    if(keys & 0x1)
        nkeys |= KEY_DUP;

    if(keys & 0x2)
        nkeys |= KEY_DLEFT;

    if(keys & 0x4)
        nkeys |= KEY_ZR;

    if(keys & 0x8)
        nkeys |= KEY_X;

    if(keys & 0x10)
        nkeys |= KEY_A;

    if(keys & 0x20)
        nkeys |= KEY_Y;
    
    if(keys & 0x40)
        nkeys |= KEY_B;

    if(keys & 0x80)
        nkeys |= KEY_ZL;
    
    if(keys & 0x200)
        nkeys |= KEY_R;

    if(keys & 0x400)
        nkeys |= KEY_START;
    
    if(keys & 0x1000)
        nkeys |= KEY_SELECT;

    if(keys & 0x2000)
        nkeys |= KEY_L;
    
    if(keys & 0x4000)
        nkeys |= KEY_DDOWN;
    
    if(keys & 0x8000)
        nkeys |= KEY_DRIGHT;
    
    return nkeys;
}

bool DebugPad::ReadEntry(DebugPadEntry *entry)
{
    uint8_t buf[21];
    Result ret = 0;
    if(R_FAILED(ret = I2C_ReadRegisterBuffer(12, 0, buf, 21))){
        return false;
    }
    int i = 0;
    for(i = 0; i < 21; i++){
        if(buf[i] != 0xFF) break;
    }
    if(i == 21) return false; 
    entry->currpadstate = ~(DecryptData(buf[5]) | (DecryptData(buf[4]) << 8));
    entry->currpadstate &= ~((entry->currpadstate & 0x1) << 14) & ~((entry->currpadstate & 0x2) << 14);
    entry->pressedpadstate = (entry->currpadstate ^ m_latestkeys) & ~m_latestkeys;
    entry->releasedpadstate = (entry->currpadstate ^ m_latestkeys) & m_latestkeys;
    entry->leftstickx = DecryptData(buf[0]) & 0x3F;
    entry->leftsticky = DecryptData(buf[1]) & 0x3F;
    entry->rightstickx = ((DecryptData(buf[0]) & 0xC0) >> 0x3)|((DecryptData(buf[1]) & 0xC0) >> 0x5)|((DecryptData(buf[2]) & 0x80) >> 0x7);
    entry->rightsticky = DecryptData(buf[2]) & 0x1F;
    entry->leftstickx -= m_leftxorigin;
    entry->leftsticky -= m_leftyorigin;
    entry->rightstickx -= m_rightxorigin;
    entry->rightsticky -= m_rightyorigin;
    m_latestkeys = entry->currpadstate;
    return true;
}

void DebugPad::SetTimer()
{
    svcSetTimer(m_timer, 16000000LL, 16000000LL);
}

void DebugPad::Initialize()
{
    if(m_isinitialized) return;

    svcCreateTimer(&m_timer, RESET_ONESHOT);
    svcCreateEvent(&m_event, RESET_ONESHOT);

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

    buf = 0xaa; // Enables encryption
    if(R_FAILED(ret = I2C_WriteRegisterBuffer(12, 0xF0, &buf, sizeof(buf)))){
        return;
    }

    buf = 0x0; // Write Key
    if(R_FAILED(ret = I2C_WriteRegisterBuffer(12, 0x40, &buf, sizeof(buf)))){
        return;
    }
    
    DebugPadEntry entry;
    if(!ReadEntry(&entry)){
        return;
    }

    m_leftxorigin = entry.leftstickx;
    m_leftyorigin = entry.leftsticky;
    m_rightxorigin = entry.rightstickx;
    m_rightyorigin = entry.rightsticky;

    m_isinitialized = 1;
}

u32 debugpadkeys = 0;
CirclePadEntry debugpadstick;
int pushed = 0;
void DebugPad::Sampling()
{
    if(!m_isinitialized) return;
    
    DebugPadEntry entry;
    if(!ReadEntry(&entry)){
        /* some logic */
        return;
    }

    debugpadkeys = RemapKey(entry.currpadstate);
    debugpadstick.x = map(entry.leftstickx, -32, 31, -180, 180);
    debugpadstick.y = map(entry.leftsticky, -32, 31, -180, 180);

    if(m_writetoring){
        pushed = 1;
        m_ring->WriteToRing(&m_oldentry);
        m_oldentry = entry;
    }
    else{
        m_oldentry = entry;
        m_writetoring = true;
    }
    svcSignalEvent(m_event);
}