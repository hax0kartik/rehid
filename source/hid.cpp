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

u8 *memsearch(u8 *startPos, const void *pattern, u32 size, u32 patternSize)
{
    const u8 *patternc = (const u8 *)pattern;
    u32 table[256];

    //Preprocessing
    for(u32 i = 0; i < 256; i++)
        table[i] = patternSize;
    for(u32 i = 0; i < patternSize - 1; i++)
        table[patternc[i]] = patternSize - i - 1;

    //Searching
    u32 j = 0;
    while(j <= size - patternSize)
    {
        u8 c = startPos[j + patternSize - 1];
        if(patternc[patternSize - 1] == c && memcmp(pattern, startPos + j, patternSize - 1) == 0)
            return startPos + j;
        j += table[c];
    }

    return NULL;
}

static inline void *decodeArmBranch(const void *src)
{
    u32 instr = *(const u32 *)src;
    s32 off = (instr & 0xFFFFFF) << 2;
    off = (off << 6) >> 6; // sign extend

    return (void *)((const u8 *)src + 8 + off);
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
        res = svcMapProcessMemoryEx2(CUR_PROCESS_HANDLE, 0x00200000, processHandle, (u32) startAddress, neededMemory);
        if(R_FAILED(res))
        {
            *(u32*)res = 0x213;
            svcCloseHandle(processHandle);
            return res;
        }
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
    u32 totalSize = textsz + rosz + rwsz;
    static u32* hookLoc = NULL;
    static u32* syncLoc = NULL;
    static u32* cppFlagLoc = NULL;
    static u32  origIrSync = 0;
    static u32  origCppFlag = 0;
    static u32  irOrigReadingCode[5] = {
        0xE5940000, // ldr r0, [r4]
        0xE1A01005, // mov r1, r5
        0xE3A03005, // mov r3, #5
        0xE3A02011, // mov r2, #17
        0x00000000  // (bl i2c_read_raw goes here)
    };

    static u32  irHook[] = {
        0xE5940000, // ldr r0, [r4]
        0xE1A01005, // mov r1, r5
        0xE59FC000, // ldr r12, [pc] (actually +8)
        0xE12FFF3C, // blx r12
        0x00000000  // irCodePhys goes here
    };

    static u32  syncHookCode[] = {
        0xE5900000, // ldr r0, [r0]
        0xEF000024, // svc 0x24
        0xE3A00000, // mov r0, #0
        0xE51FF004, // ldr pc, [pc, #-4]
        0x00000000, // (return address goes here)
    };

    static const u32 irOrigWaitSyncCode[] = {
            0xEF000024, // svc 0x24 (WaitSynchronization)
            0xE1B01FA0, // movs r1, r0, lsr#31
            0xE1A0A000, // mov r10, r0
        }, irOrigWaitSyncCodeOld[] = {
            0xE0AC6000, // adc r6, r12, r0
            0xE5D70000, // ldrb r0, [r7]
        }; // pattern for 8.1

    static const u32 irOrigCppFlagCode[] = {
            0xE3550000, // cmp r5, #0
            0xE3A0B080, // mov r11, #0x80
    };

    u32 irDataPhys = (u32)PA_FROM_VA_PTR(latestkeys);
    u32 irCodePhys = (u32)PA_FROM_VA_PTR(&IRCodePatchFunc);

    u32 *off = (u32 *)memsearch((u8 *)0x00200000, &irOrigReadingCode, totalSize, sizeof(irOrigReadingCode) - 4);

    u32 *off2 = (u32 *)memsearch((u8 *)0x00200000, &irOrigWaitSyncCode, totalSize, sizeof(irOrigWaitSyncCode));
    if(off2 == NULL)
    {
        off2 = (u32 *)memsearch((u8 *)0x00200000, &irOrigWaitSyncCodeOld, totalSize, sizeof(irOrigWaitSyncCodeOld));
    }

    u32 *off3 = (u32 *)memsearch((u8 *)0x00200000, &irOrigCppFlagCode, totalSize, sizeof(irOrigCppFlagCode));

    origIrSync = *off2;
    origCppFlag = *off3;

    *(void **)(irCodePhys + 8) = decodeArmBranch(off + 4) - 0x00100000;
    *(void **)(irCodePhys + 12) = (void*)irDataPhys;

    irHook[4] = irCodePhys;
    irOrigReadingCode[4] = off[4]; // Copy the branch.
    syncHookCode[4] = (u32)off2 - 0x00100000 + 4; // Hook return address

    hookLoc = (u32*)PA_FROM_VA_PTR(off);
    syncLoc = (u32*)PA_FROM_VA_PTR(off2);
    cppFlagLoc = (u32*)PA_FROM_VA_PTR(off3);

    memcpy(hookLoc, &irHook, sizeof(irHook));

    // We keep the WaitSynchronization1 to avoid general slowdown because of the high cpu load
    if (*syncLoc == 0xEF000024) // svc 0x24 (WaitSynchronization)
    {
        syncLoc[-1] = 0xE51FF004;
        syncLoc[0] = (u32)PA_FROM_VA_PTR(&syncHookCode);
    }
    else
    {
        // This "NOP"s out a WaitSynchronisation1 (on the event bound to the 'IR' interrupt) or the check of a previous one
        *syncLoc = 0xE3A00000; // mov r0, #0
    }

    // This NOPs out a flag check in ir:user's CPP emulation
    *cppFlagLoc = 0xE3150000; // tst r5, #0
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