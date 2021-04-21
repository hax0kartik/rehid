#include <3ds.h>
#include "irrst.hpp"
#include "printf.h"
Handle iruHandle;
int iruRefCount;
u32 latestkeys = 0;
u32 *latestKeysPA;
u32 *statePA;
u16 statecounter;
#define PA_PTR(addr)            (void *)((u32)(addr) | 1 << 31)
Handle irrstHandle_;
Handle irrstMemHandle_;
Handle irrstEvent_;

vu32* irrstSharedMem_;

static u32 kHeld;
int irrstRefCount;


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

u32 irrstCheckSectionUpdateTime_(vu32 *sharedmem_section, u32 id)
{
	s64 tick0=0, tick1=0;

	if(id==0)
	{
		tick0 = *((u64*)&sharedmem_section[0]);
		tick1 = *((u64*)&sharedmem_section[2]);

		if(tick0==tick1 || tick0<0 || tick1<0)return 1;
	}

	return 0;
}

void irrstScanInput_(void)
{
	if(irrstRefCount==0)return;

	u32 Id=0;
	kHeld = 0;
	if(irrstSharedMem_ != NULL) return;
	Id = irrstSharedMem_[4]; //PAD / circle-pad
	if(Id>7)Id=0;
	if(irrstCheckSectionUpdateTime_(irrstSharedMem_, Id)==0)
	{
		kHeld = irrstSharedMem_[6 + Id*4];
	}
}

u32 irrstKeysHeld_(void)
{
	if(irrstRefCount>0)return kHeld;
	return -1;
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
	if(*statePA == 1 || *statePA == 2)
	{
		statecounter = 0; 
		latestkeys = 0;
		latestkeys = (*latestKeysPA) & ~KEY_CSTICK_DOWN;
		*latestKeysPA  = 0;
	}
	else
		statecounter++;
	
	if(statecounter > 8000){
		srvSetBlockingPolicy(false);
		irrstInit_(0);
		srvSetBlockingPolicy(true);
		irrstScanInput_();
		statecounter = 0;
	}
}

char data[100];
u32 iruKeysHeld_()
{
	u32 keys = irrstKeysHeld_();
	sprintf_(data, "keys %08X latestkeys %08X irrstrefcount %d\n", keys, latestkeys, irrstRefCount);
	svcOutputDebugString(data, 100);
	if(keys != -1)
		return keys & ~KEY_CSTICK_DOWN;
	else
		return latestkeys;
}