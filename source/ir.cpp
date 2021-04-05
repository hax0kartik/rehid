#include <3ds.h>
#include <cstring>
#include "ir.hpp"

Handle irrstHandle_;
Handle irrstMemHandle_;
Handle irrstEvent_;

vu32* irrstSharedMem_;

static u32 kHeld;
static circlePosition csPos;
static int irrstRefCount;

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

Result irrstInit_(void)
{
	if (AtomicPostIncrement(&irrstRefCount)) return 0;

	Result ret=0;

	// Request service.
	if(R_FAILED(ret=srvGetServiceHandle(&irrstHandle_, "ir:rst"))) goto cleanup0;

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
	if(envGetHandle("ir:rst") == 0) IRRST_Shutdown();
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
	memset(&csPos, 0, sizeof(circlePosition));

	Id = irrstSharedMem_[4]; //PAD / circle-pad
	if(Id>7)Id=7;
	if(irrstCheckSectionUpdateTime_(irrstSharedMem_, Id)==0)
	{
		kHeld = irrstSharedMem_[6 + Id*4];
		csPos = *(circlePosition*)&irrstSharedMem_[6 + Id*4 + 3];
	}
}

u32 irrstKeysHeld_(void)
{
	if(irrstRefCount>0)return kHeld;
	return 0;
}

void irrstCstickRead_(circlePosition* pos)
{
	if (pos) *pos = csPos;
}