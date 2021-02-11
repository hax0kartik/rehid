#include <malloc.h>
#include <new>
#include "hid.hpp"
#include "PadRing.hpp"

static uint8_t ALIGN(8) hidthreadstack[0x1000];
void Hid::CreateAndMapMemoryBlock()
{
    // 0x1000 is rounded off size for 0x2B0
    Result ret = svcCreateMemoryBlock(&m_sharedmemhandle, 0, 0x1000, (MemPerm)(MEMPERM_READ | MEMPERM_WRITE), MEMPERM_READ);
    if(R_SUCCEEDED(ret))
    {
        mappableInit(0x10000000, 0x14000000);
        m_addr = mappableAlloc(0x1000);
        if(m_addr)
        {
            ret = svcMapMemoryBlock(m_sharedmemhandle, (u32)m_addr, (MemPerm)(MEMPERM_READ | MEMPERM_WRITE), MEMPERM_DONTCARE);
            if(ret != 0) 
                *(u32*)ret = (u32)m_addr;
        }
        else svcBreak(USERBREAK_ASSERT);
        
    }
    else svcBreak(USERBREAK_ASSERT);
    for(int i = 0; i < 4; i++)
        svcCreateEvent(&dummyhandles[i], RESET_ONESHOT);
    LightLock_Init(&m_sleeplock);
}

void Hid::CreateRingsOnSharedmemoryBlock()
{
    m_padring = new(m_addr) PadRing;
    m_touchring = new((void*)((u32)(m_addr) + 0xA8))TouchRing;
}

void Hid::InitializePad()
{
    if(R_FAILED(codecInit())) svcBreak(USERBREAK_ASSERT);

    m_pad.Initialize();
    m_pad.SetPadRing(m_padring);

    m_touch.Initialize();
    m_touch.SetTouchRing(m_touchring);
}

static void SamplingFunction(void *argv)
{
    Hid *hid = (Hid*)argv;
    Result ret = 0;
    u32 touchscreendata, circlepaddata;
    Handle *padtimer = hid->GetPad()->GetTimer();
    LightLock *lock = hid->GetSleepLock();
    while(!*hid->ExitThread())
    {
        LightLock_Lock(lock);
        ret = svcWaitSynchronization(*padtimer, U64_MAX);
        if(ret > 0) svcBreak(USERBREAK_ASSERT);
        ret = CDCHID_GetData(&touchscreendata, &circlepaddata);
        if(ret == 0)
        {
            hid->GetPad()->Sampling(circlepaddata);
            hid->GetTouch()->Sampling(touchscreendata);
        }
        LightLock_Unlock(lock);
    }
}

void Hid::StartThreadsForSampling()
{
    if(!m_samplingthreadstarted)
    {
        m_padring->Reset();
        m_touchring->Reset();
        m_pad.SetTimer();
        MyThread_Create(&m_samplingthread, SamplingFunction, this, hidthreadstack, 0x1000, 0x15, -2);
        m_samplingthreadstarted = true;
    }
}

void Hid::EnteringSleepMode()
{
    LightLock_Lock(&m_sleeplock); // now that main thread accquired the lock, sampling thread will get stuck
    PTMSYSM_NotifySleepPreparationComplete(0);
}

void Hid::TakeOverIRRSTIfRequired()
{
    Result ret = 0;
    do
    {
        svcSleepThread(0.1e+9);
        ret = irrstInit();
    } while (ret != 0);
}

void Hid::ExitingSleepMode()
{
    LightLock_Unlock(&m_sleeplock); // Unlock lock and then reset timer
    m_padring->Reset();
    m_touchring->Reset();
    m_pad.SetTimer();
    PTMSYSM_NotifySleepPreparationComplete(0);
}