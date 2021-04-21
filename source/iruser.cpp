#include <new>
#include "iruser.hpp"

IrUser::IrUser()
{
    Result ret = 0;
    if((ret = svcCreateEvent(&m_connectionevent, RESET_ONESHOT)) != 0) *(u32*)ret = 0xf006;
    /*
    svcCreateEvent(&m_recvevent, RESET_ONESHOT);
    svcCreateEvent(&m_sendevent, RESET_ONESHOT);
    */
}

Result IrUser::IntializeIrNopShared(size_t bufsize, size_t recvbufsize, s32 maxrecvcount, size_t sendbufsize, s32 maxsendcount, u32 baudrate, Handle sharedmemhandle)
{
    Result ret = 0;
    m_addr = mappableAlloc(bufsize);
    if(m_addr)
    {
        ret = svcMapMemoryBlock(sharedmemhandle, (u32)m_addr, (MemPerm)3, (MemPerm)1);
        if(ret != 0) 
            *(u32*)ret = (u32)m_addr;
    }
    else svcBreak(USERBREAK_ASSERT);
    m_sharedmemoryheader = new(m_addr)SharedMemoryBlockHeader;
    m_sharedmemoryheader->info.initialized = 1;
    m_sharedmemoryheader->info.networkid = 6; // random number
    m_sharedmemoryheader->info.machineid = 0;
    m_sharedmemoryheader->info.connstatus = 0;
    m_sharedmemoryheader->info.role = 0;
    return 0;
}

Handle *IrUser::GetConnectionEvent()
{   
    return &m_connectionevent;
}

Handle *IrUser::GetRecieveEvent()
{
    return &m_recvevent;
}

Handle *IrUser::GetSendEvent()
{
    return &m_sendevent;
}
Result IrUser::RequireConnection()
{
    m_sharedmemoryheader->info.connstatus = 2; // Connected
    m_sharedmemoryheader->info.role = 1;
    m_sharedmemoryheader->info.targetid = 1;
    Result ret = 0;
    if((ret = svcSignalEvent(m_connectionevent)) != 0) *(u32*)ret = 0xf007;
    return 0;
}