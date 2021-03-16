#include <malloc.h>
#include <new>
#include "hid.hpp"
#include "PadRing.hpp"
#include <cstring>
extern "C"
{
    #include "csvc.h"
}

#define PA_PTR(addr)            (void *)((u32)(addr) | 1 << 31)

#ifndef PA_FROM_VA_PTR
#define PA_FROM_VA_PTR(addr)    PA_PTR(svcConvertVAToPA((const void *)(addr), false))
#endif

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

static inline bool isServiceUsable(const char *name)
{
    bool r;
    return R_SUCCEEDED(srvIsServiceRegistered(&r, name)) && r;
}

typedef Result(* OperateOnProcessCb)(Handle processHandle, u32 textSz, u32 roSz, u32 rwSz, Remapper *remapper, uint32_t *keys);

Result OpenProcessByName(const char *name, Handle *h)
{
    u32 pidList[0x40];
    s32 processCount;
    svcGetProcessList(&processCount, pidList, 0x40);
    Handle dstProcessHandle = 0;

    for(s32 i = 0; i < processCount; i++)
    {
        Handle processHandle;
        Result res = svcOpenProcess(&processHandle, pidList[i]);
        if(R_FAILED(res))
            continue;

        char procName[8] = {0};
        svcGetProcessInfo((s64 *)procName, processHandle, 0x10000);
        if(strncmp(procName, name, 8) == 0)
            dstProcessHandle = processHandle;
        else
            svcCloseHandle(processHandle);
    }

    if(dstProcessHandle == 0)
        return -1;

    *h = dstProcessHandle;
    return 0;
}

Result OperateOnProcessByName(const char *name, OperateOnProcessCb func, Remapper *remapper, uint32_t *keys)
{
    Result res;
    Handle processHandle;
    res = OpenProcessByName(name, &processHandle);
    if (R_FAILED(res))
    {
        *(u32*)res = 0x212;
        return res;
    }

    // Find offsets for required patches
    s64     startAddress, textTotalRoundedSize, rodataTotalRoundedSize, dataTotalRoundedSize;
    u32     totalSize;

    res = svcGetProcessInfo(&textTotalRoundedSize, processHandle, 0x10002); // only patch .text + .data
    svcGetProcessInfo(&rodataTotalRoundedSize, processHandle, 0x10003);
    svcGetProcessInfo(&dataTotalRoundedSize, processHandle, 0x10004);

    totalSize = (u32)(textTotalRoundedSize + rodataTotalRoundedSize + dataTotalRoundedSize);

    svcGetProcessInfo(&startAddress, processHandle, 0x10005);

    u32 neededMemory =  (totalSize + 0xFFF) & ~0xFFF; 

    res = svcMapProcessMemoryEx(processHandle, 0x00200000, (u32) startAddress, neededMemory);
    if(R_FAILED(res))
    {
        *(u32*)res = 0x213;
        svcCloseHandle(processHandle);
        return res;
    }

    res = func(processHandle, (u32)textTotalRoundedSize, (u32)rodataTotalRoundedSize, (u32)dataTotalRoundedSize, remapper, keys);

    svcUnmapProcessMemoryEx(processHandle, 0x00200000, totalSize);
    return res;
}

extern "C" {
    extern void IRCodePatchFunc(void);
}

Result irpatch_cb(Handle phandle, u32 textsz, u32 rosz, u32 rwsz, Remapper *remapper, uint32_t *latestkeys)
{
    u32 *start = (u32*)0x00200000;
    u32 IRCodePatchFuncphys = (u32)PA_FROM_VA_PTR(&IRCodePatchFunc);
    u32 remapperphys = (u32)PA_FROM_VA_PTR(remapper);
    u32 latestkeysphys = (u32)PA_FROM_VA_PTR(latestkeys);
    // r2 = Remapper object
    u32 irHook[] = {
        0xE59F2008, // ldr r2,  [pc, #4]
        0xE59F3008, // ldr r3,  [pc, #4]
        0xE59FC008, // ldr r12, [pc, #4]
        0xE12FFF3C, // blx r12
        remapperphys,
        latestkeysphys,
        IRCodePatchFuncphys,
        0xE320F000, // NOP
        0xE320F000, // NOP
        0xE320F000, // NOP
    };
    
    for(; start < textsz + start; start++)
    {
        if(*start == 0xE5DD0021)
            break;
    }
    start += 1;
    u32 *patchoff = start;
    memcpy(patchoff, &irHook, sizeof(irHook));
    svcInvalidateEntireInstructionCache();
    return (Result)0;
}

static void irPatch(void *argv)
{
    Hid *hid = (Hid*)argv; 
    while(!isServiceUsable("ir:u")) svcSleepThread(1e+9); // Wait For service
    Result res = OperateOnProcessByName("ir", irpatch_cb, hid->GetPad()->GetRemapperObject(), hid->GetPad()->GetLatestRawKeys());
}

static void SamplingFunction(void *argv)
{
    Hid *hid = (Hid*)argv;
    Result ret = 0;
    u32 touchscreendata, circlepaddata;
    Handle *padtimer = hid->GetPad()->GetTimer();
    LightLock *lock = hid->GetSleepLock();
    irPatch(argv);
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


void Hid::ExitingSleepMode()
{
    LightLock_Unlock(&m_sleeplock); // Unlock lock and then reset timer
    m_padring->Reset();
    m_touchring->Reset();
    m_pad.SetTimer();
    PTMSYSM_NotifySleepPreparationComplete(0);
}