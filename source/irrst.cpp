#include <new>
#include <cstdlib>
#include <3ds.h>
#include "irrst.hpp"
#include "printf.h"
Handle iruHandle;
int iruRefCount;
u32 latestkeys = 0;
u32 *latestKeysPA;
u32 *statePA;
u16 counter;
IrrstRing ring;
Handle irrstHandle_;
Handle irrstMemHandle_;
Handle irrstEvent_;
#define PA_PTR(addr)            (void *)((u32)(addr) | 1 << 31);
vu32* irrstSharedMem_;
Handle irtimer;
static u32 kHeld;
int irrstRefCount;
u8 overridecpadpro = 0;

Result IRRST_GetHandles_(Handle* outMemHandle, Handle* outEventHandle)
{
    u32* cmdbuf=getThreadCommandBuffer();
    cmdbuf[0]=IPC_MakeHeader(0x1,0,0); // 0x10000

    Result ret=0;
    if(R_FAILED(ret=svcSendSyncRequest(irrstHandle_)))return ret;

    if(outMemHandle)*outMemHandle=cmdbuf[3];
    if(outEventHandle)*outEventHandle=cmdbuf[4];

    return cmdbuf[1];
}

Result IRRST_Initialize_(u32 unk1, u8 unk2)
{
    u32* cmdbuf=getThreadCommandBuffer();
    cmdbuf[0]=IPC_MakeHeader(0x2,2,0); // 0x20080
    cmdbuf[1]=unk1;
    cmdbuf[2]=unk2;

    Result ret=0;
    if(R_FAILED(ret=svcSendSyncRequest(irrstHandle_)))return ret;

    return cmdbuf[1];
}

Result IRRST_Shutdown_(void)
{
    u32* cmdbuf=getThreadCommandBuffer();
    cmdbuf[0]=IPC_MakeHeader(0x3,0,0); // 0x30000

    Result ret=0;
    if(R_FAILED(ret=svcSendSyncRequest(irrstHandle_)))return ret;

    return cmdbuf[1];
}

Result irrstInit_(uint8_t steal)
{
    if (AtomicPostIncrement(&irrstRefCount)) return 0;

    Result ret=0;

    // Request service.
    if(R_FAILED(ret=srvGetServiceHandle(&irrstHandle_, "ir:rst"))) 
            goto cleanup0;

    // Get sharedmem handle.
    if(R_FAILED(ret=IRRST_GetHandles_(&irrstMemHandle_, &irrstEvent_))) goto cleanup1;

    // Initialize ir:rst
    if(envGetHandle("ir:rst") == 0) ret = IRRST_Initialize_(10, 0);

    // Map ir:rst shared memory.
    irrstSharedMem_=(vu32*)mappableAlloc(0x98);
    if(!irrstSharedMem_)
    {
        ret = -1;
        goto cleanup1;
    }

    if(R_FAILED(ret = svcMapMemoryBlock(irrstMemHandle_, (u32)irrstSharedMem_, MEMPERM_READ, MEMPERM_DONTCARE))) goto cleanup2;
    return 0;

cleanup2:
    svcCloseHandle(irrstMemHandle_);
    if(irrstSharedMem_ != NULL)
    {
        mappableFree((void*) irrstSharedMem_);
        irrstSharedMem_ = NULL;
    }
cleanup1:
    svcCloseHandle(irrstHandle_);
cleanup0:
    AtomicDecrement(&irrstRefCount);
    return ret;
}

void irrstExit_(void)
{
    if (AtomicDecrement(&irrstRefCount)) return;

    // Reset internal state.
    kHeld = 0;

    svcCloseHandle(irrstEvent_);
    // Unmap ir:rst sharedmem and close handles.
    svcUnmapMemoryBlock(irrstMemHandle_, (u32)irrstSharedMem_);
    if(envGetHandle("ir:rst") == 0) IRRST_Shutdown_();
    svcCloseHandle(irrstMemHandle_);
    svcCloseHandle(irrstHandle_);

    if(irrstSharedMem_ != NULL)
    {
        mappableFree((void*) irrstSharedMem_);
        irrstSharedMem_ = NULL;
    }
}

void irrstWaitForEvent_(bool nextEvent)
{
    if(nextEvent)svcClearEvent(irrstEvent_);
    svcWaitSynchronization(irrstEvent_, U64_MAX);
    if(!nextEvent)svcClearEvent(irrstEvent_);
}

u32 irrstCheckSectionUpdateTime_(u32 id)
{
    s64 tick0=0, tick1=0;
    if(id==0)
    {
        tick0 = ring.GetTickCount();
        tick1 = ring.GetOldTickCount();
        if(tick0==tick1 || tick0<0 || tick1<0)return 1;
    }
    return 0;
}


char data[100];

void irrstScanInput_(void)
{
    u32 id=0;
    kHeld = 0;
    id = ring.GetIndex(); //PAD / circle-pad
    if(id>7)id=0;
    if(irrstCheckSectionUpdateTime_(id)==0)
    {
        kHeld = ring.GetLatest(id);
    }
}

u32 irrstKeysHeld_(void)
{
    return kHeld;
}


Result IRU_Initialize_(void)
{
    Result ret = 0;
    u32 *cmdbuf = getThreadCommandBuffer();

    cmdbuf[0] = IPC_MakeHeader(0x1,0,0); // 0x10000

    if(R_FAILED(ret = svcSendSyncRequest(iruHandle)))return ret;
    ret = (Result)cmdbuf[1];

    return ret;
}

u32 IRU_GetLatestKeysPA_()
{
    Result ret = 0;
    u32 *cmdbuf = getThreadCommandBuffer();

    cmdbuf[0] = IPC_MakeHeader(0x7,0,0); // 0x70000

    if(R_FAILED(ret = svcSendSyncRequest(iruHandle)))return ret;
    return cmdbuf[1];
}

u32 IRU_GetStatePA_()
{
    Result ret = 0;
    u32 *cmdbuf = getThreadCommandBuffer();

    cmdbuf[0] = IPC_MakeHeader(0x8,0,0); // 0x80000

    if(R_FAILED(ret = svcSendSyncRequest(iruHandle)))return ret;
    return cmdbuf[1];
}

Result IRU_Shutdown_(void)
{
    Result ret = 0;
    u32 *cmdbuf = getThreadCommandBuffer();

    cmdbuf[0] = IPC_MakeHeader(0x2,0,0); // 0x20000

    if(R_FAILED(ret = svcSendSyncRequest(iruHandle)))return ret;
    ret = (Result)cmdbuf[1];

    return ret;
}


Result iruInit_(unsigned char c)
{
    Result ret = 0;
    u32 keysdirectPA;
    u32 statedirectPA;
    if(AtomicPostIncrement(&iruRefCount)) return 0;

    ret = srvGetServiceHandle(&iruHandle, "ir:u");
    if(R_FAILED(ret))goto cleanup0;

    svcCreateTimer(&irtimer, RESET_ONESHOT);
    if(R_FAILED(ret = svcSetTimer(irtimer, 8000000LL, 0LL)))
        *(u32*)0x8 = ret;

    ret = IRU_Initialize_();
    if(R_FAILED(ret))goto cleanup1;

    keysdirectPA = IRU_GetLatestKeysPA_();
    statedirectPA = IRU_GetStatePA_();
    latestKeysPA = (u32*)PA_PTR(keysdirectPA);
    statePA = (u32*)PA_PTR(statedirectPA);

    IRU_Shutdown_();

cleanup1:
    svcCloseHandle(iruHandle);
cleanup0:
    AtomicDecrement(&iruRefCount);
    return ret;
}

void iruExit_(void)
{
    if(AtomicDecrement(&iruRefCount)) return;
    IRU_Shutdown_();
    svcCloseHandle(iruHandle);
    iruHandle = 0;
}

void iruScanInput_()
{
    u8 state = *statePA;
    
    if(state == 1 && irrstRefCount >= 1 && overridecpadpro == 0) { //iruser was initialized
        irrstExit_();
    }
    else if(state == 0 && irrstRefCount <= 0) {
        irrstInit_(0);
    }
    irrstScanInput_();
}

u32 iruKeysHeld_()
{
    return kHeld;
}

void irSampling()
{
    IrrstEntry entry;
    svcSetTimer(irtimer, 8000000LL, 0LL);
    volatile uint32_t latest = *latestKeysPA;
    entry.pressedpadstate = (latest ^ latestkeys) & ~latestkeys;
    entry.releasedpadstate = (latest ^ latestkeys) & latestkeys;
    entry.currpadstate = latest;
    ring.WriteToRing(&entry);
    latestkeys = latest;
}

void IrrstRing::WriteToRing(IrrstEntry *entry)
{
    uint32_t index = m_updatedindex;
    if(index == 7)
        index = 0;
    else
        index++;
    ExclusiveWrite(&m_entries[index].currpadstate, entry->currpadstate);
    ExclusiveWrite(&m_entries[index].pressedpadstate, entry->pressedpadstate);
    ExclusiveWrite(&m_entries[index].releasedpadstate, entry->releasedpadstate);
    if(index == 0) // When index is 0 we need to update tickcount
    {
        m_oldtickcount = m_tickcount;
        m_tickcount = svcGetSystemTick();
    }
    ExclusiveWrite(&m_updatedindex, index);
}