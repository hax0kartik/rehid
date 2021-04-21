#pragma once
#include <3ds.h>

struct StatusInfo
{
    uint32_t recverr;
    uint32_t senderr;
    uint8_t connstatus;
    uint8_t tryconnstatus;
    uint8_t role;
    uint8_t machineid;
    uint8_t targetid;
    uint8_t networkid;
    uint8_t initialized;
    uint8_t pad;
};

class SharedMemoryBlockHeader
{
    public:
        StatusInfo info;
};
static_assert(sizeof(SharedMemoryBlockHeader) == 0x10, "Invalied SharedmemoryBlockHeader Size!");

class IrUser{
    public:
        IrUser();
        /* Todo create constructor and init m_addr there */
        Result IntializeIrNopShared(size_t bufsize, size_t recvbufsize, s32 maxrecvcount, size_t sendbufsize, s32 maxsendcount, u32 baudrate, Handle sharedmemhandle);
        Handle *GetConnectionEvent();
        Result RequireConnection();
        Handle *GetRecieveEvent();
        Handle *GetSendEvent();
    private:
        void *m_addr = nullptr;
        SharedMemoryBlockHeader *m_sharedmemoryheader = nullptr;
        Handle m_connectionevent = 0;
        Handle m_recvevent = 0;
        Handle m_sendevent = 0;
};