#include <malloc.h>
#include <new>
#include "hid.hpp"
#include "PadRing.hpp"
#include <cstring>
#include "irrst.hpp"
#include "code_ips.h"
extern "C"
{
    #include "csvc.h"
    #include "gpio.h"
    #include "i2c.h"
}
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
    svcCreateEvent(&dummyhandles[2], RESET_STICKY);
    svcCreateEvent(&dummyhandles[3], RESET_ONESHOT);
    LightLock_Init(&m_sleeplock);
}

void Hid::CreateRingsOnSharedmemoryBlock()
{
    m_padring = new(m_addr)PadRing;
    m_touchring = new((void*)((u32)(m_addr) + 0xA8))TouchRing;
    m_accelring = new((void*)((u32)(m_addr) + 0x108))AccelerometerRing;
    m_gyroring = new((void*)((u32)(m_addr) + 0x158))GyroscopeRing;
    m_debugpadring = new((void*)((u32)(m_addr) + 0x238))DebugPadRing;
}

void Hid::InitializePad()
{
    if(R_FAILED(codecInit())) svcBreak(USERBREAK_ASSERT);

    m_pad.Initialize();
    m_pad.SetPadRing(m_padring);

    m_touch.Initialize();
    m_touch.SetTouchRing(m_touchring);
}

void Hid::InitializeAccelerometer()
{
    m_accel.Initialize();
    m_accel.SetAccelerometerRing(m_accelring);
}

void Hid::InitializeGyroscope()
{
    if(R_FAILED(i2cHidInit())) svcBreak(USERBREAK_ASSERT);
    if(R_FAILED(gpiohidInit())) svcBreak(USERBREAK_ASSERT);

    m_gyro.Initialize();
    m_gyro.SetGyroscopeRing(m_gyroring);
}

void Hid::InitializeDebugPad()
{
    // We should init i2c here, but that will done by InitlaizeGyroscope

    m_debugpad.Initialize();
    m_debugpad.SetDebugPadRing(m_debugpadring);
}

static inline bool isServiceUsable(const char *name)
{
    bool r;
    return R_SUCCEEDED(srvIsServiceRegistered(&r, name)) && r;
}

u8 irneeded = 0;
static void irInit()
{
    while(!isServiceUsable("ir:u")) svcSleepThread(1e+9); // Wait For service
    srvSetBlockingPolicy(true);
    Result ret = iruInit_();
    if(ret == 0)
        irneeded = 1;
    srvSetBlockingPolicy(false);
}

extern Handle irtimer;
static void SamplingFunction(void *argv)
{
    Hid *hid = (Hid*)argv;
    Result ret = 0;
    u32 touchscreendata, circlepaddata;
    Handle *padtimer = hid->GetPad()->GetTimer();
    Handle *accelintrevent = hid->GetAccelerometer()->GetIntrEvent();
    Handle *gyrointrevent = hid->GetGyroscope()->GetIntrEvent();
    Handle *debugpadtimer = hid->GetDebugPad()->GetTimer();
    LightLock *lock = hid->GetSleepLock();
    irInit();
    int32_t out;
    while(!*hid->ExitThread())
    {
        Handle handles[] = {irtimer, *padtimer, *debugpadtimer, *gyrointrevent, *accelintrevent};
        LightLock_Lock(lock);
        ret = svcWaitSynchronizationN(&out, handles, 5, false, -1LL);
        if(!hid->GetGyroscope()->m_issetupdone && osGetTime() - hid->GetGyroscope()->timeenable > 90 && hid->GetGyroscope()->GetRefCount() > 0)
        {
            hid->GetGyroscope()->SetupForSampling();
            hid->GetGyroscope()->m_issetupdone = true;
        }
        switch(out){

            case 0:
            {
                irSampling();
                break;
            }

            case 1:
            {
                ret = CDCHID_GetData(&touchscreendata, &circlepaddata);
                if(ret == 0)
                {
                    hid->GetPad()->Sampling(circlepaddata, hid->GetRemapperObject());
                    hid->GetTouch()->Sampling(touchscreendata, hid->GetRemapperObject());
                }
                break;
            }

            case 2:
            {
                hid->GetDebugPad()->Sampling();
                break;
            }

            case 3:
            {
                hid->GetGyroscope()->Sampling();
                break;
            }

            case 4:
            {
                hid->GetAccelerometer()->Sampling();
                break;
            }
        }
        if(ret > 0) svcBreak(USERBREAK_ASSERT);
        
        LightLock_Unlock(lock);
    }
}

void Hid::StartThreadsForSampling()
{
    if(!m_samplingthreadstarted)
    {
        m_padring->Reset();
        m_touchring->Reset();
        m_accelring->Reset();
        m_gyroring->Reset();
        m_debugpadring->Reset();
        m_pad.SetTimer();
        m_debugpad.SetTimer();
        m_accel.EnableOrDisableInterrupt();
        m_gyro.EnableSampling();
        m_gyro.timeenable = osGetTime();
        m_gyro.m_issetupdone = false;
        MyThread_Create(&m_samplingthread, SamplingFunction, this, hidthreadstack, 0x1000, 0x15, -2);
        m_accel.EnableOrDisableInterrupt(0); // disable accelerometer interrupt
        m_accel.SetAccelerometerStatus(1); // enable accelerometer
        m_samplingthreadstarted = true;
    }
}

void Hid::EnteringSleepMode()
{
    LightLock_Lock(&m_sleeplock); // now that main thread accquired the lock, sampling thread will get stuck
    svcClearEvent(dummyhandles[2]);
    PTMSYSM_NotifySleepPreparationComplete(0);
}


void Hid::ExitingSleepMode()
{
    LightLock_Unlock(&m_sleeplock); // Unlock lock and then reset timer
    m_padring->Reset();
    m_touchring->Reset();
    m_accelring->Reset();
    m_gyroring->Reset();
    m_debugpadring->Reset();
    m_pad.SetTimer();
    m_debugpad.SetTimer();
    PTMSYSM_NotifySleepPreparationComplete(0);
}

void Hid::RemapGenFileLoc()
{
    m_remapper.GenerateFileLocation(); 
    Result ret = m_remapper.ReadConfigFile();
        if(ret == -1) return;
        else if(ret) *(u32*)ret = 0xF00FBABE;
    m_remapper.ParseConfigFile();
}

void Hid::CheckIfIRPatchExists()
{
    Handle fshandle;
    Result ret = FSUSER_OpenFileDirectly(&fshandle, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, NULL), fsMakePath(PATH_ASCII, "/luma/titles/0004013000003302/code.ips"), FS_OPEN_READ, 0);
    if(ret) // Does not exist
    {
        u64 archivesd;
        FSUSER_OpenArchive(&archivesd, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""));
        ret = FSUSER_CreateDirectory(archivesd, fsMakePath(PATH_ASCII, "/luma/titles/0004013000003302/"), FS_ATTRIBUTE_DIRECTORY);
        if(R_FAILED(ret)) *(u32*)0xF009F008 = ret; // Shouldn't have happened
        ret = FSUSER_OpenFileDirectly(&fshandle, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, NULL), fsMakePath(PATH_ASCII, "/luma/titles/0004013000003302/code.ips"), FS_OPEN_WRITE | FS_OPEN_CREATE, 0);
        if(R_FAILED(ret)) *(u32*)0xF009F009 = ret; // Shouldn't have happened
        ret = FSFILE_Write(fshandle, nullptr, 0, code_ips, code_ips_size, 0);
        if(R_FAILED(ret)) *(u32*)0xf009f010 = ret; // Neither this should happen
        FSFILE_Close(fshandle);
        PTMSYSM_RebootAsync(2e+9);
    }
   return ;
}