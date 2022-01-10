#include "i2c.h"

Handle i2chidHandle;
static int i2chidRefCount;

Result i2cHidInit()
{
    if(AtomicPostIncrement(&i2chidRefCount)) return 0;
    Result ret = 0;
    if(R_FAILED(ret = srvGetServiceHandle(&i2chidHandle, "i2c::HID")))
        AtomicDecrement(&i2chidRefCount);
    return ret;
}

void i2cHidFinalize()
{
    AtomicDecrement(&i2chidRefCount);
    svcCloseHandle(i2chidHandle);
}

Result I2C_EnableRegisterBits8(u8 devid, u8 regid, u8 enablemask)
{
    Result ret = 0;
    u32 *cmdbuf = getThreadCommandBuffer();
    cmdbuf[0] = IPC_MakeHeader(2, 3, 0);
    cmdbuf[1] = devid;
    cmdbuf[2] = regid;
    cmdbuf[3] = enablemask;
    if(R_FAILED(ret = svcSendSyncRequest(i2chidHandle))) return ret;
    return cmdbuf[1];
}

Result I2C_DisableRegisterBits8(u8 devid, u8 regid, u8 disablemask)
{
    Result ret = 0;
    u32 *cmdbuf = getThreadCommandBuffer();
    cmdbuf[0] = IPC_MakeHeader(3, 3, 0);
    cmdbuf[1] = devid;
    cmdbuf[2] = regid;
    cmdbuf[3] = disablemask;
    if(R_FAILED(ret = svcSendSyncRequest(i2chidHandle))) return ret;
    return cmdbuf[1];
}

Result I2C_WriteRegister8(u8 devid, u8 regid, u8 regdata)
{
    Result ret = 0;
    u32 *cmdbuf = getThreadCommandBuffer();
    cmdbuf[0] = IPC_MakeHeader(5, 3, 0);
    cmdbuf[1] = devid;
    cmdbuf[2] = regid;
    cmdbuf[3] = regdata;
    if(R_FAILED(ret = svcSendSyncRequest(i2chidHandle))) return ret;
    return cmdbuf[1];
}

Result I2C_ReadRegisterBuffer8(u8 devid, u8 regid, u8 *buffer, size_t buffersize)
{
    Result ret = 0;
    u32 savedbufs[2];
    u32 *cmdbuf = getThreadCommandBuffer();
    cmdbuf[0] = 0xD00C0;
    cmdbuf[1] = devid;
    cmdbuf[2] = regid;
    cmdbuf[3] = buffersize;
    u32 *staticbuf = getThreadStaticBuffers();
    savedbufs[0] = staticbuf[0];
    savedbufs[1] = staticbuf[1];
    staticbuf[0] = (buffersize << 14) | 2;
    staticbuf[1] = (u32)buffer;

    if(R_FAILED(ret = svcSendSyncRequest(i2chidHandle)))
        cmdbuf[1] = ret;

    staticbuf[0] = savedbufs[0];
    staticbuf[1] = savedbufs[1];
    return cmdbuf[1];
}

Result I2C_WriteRegisterBuffer(u8 devid, u8 regid, u8 *buffer, size_t buffersize)
{
    Result ret = 0;
    u32 *cmdbuf = getThreadCommandBuffer();
    cmdbuf[0] = 0xE00C2;
    cmdbuf[1] = devid;
    cmdbuf[2] = regid;
    cmdbuf[3] = buffersize;
    cmdbuf[4] = (buffersize << 14) | 0x402;
    cmdbuf[5] = (u32)buffer;

    if(R_FAILED(ret = svcSendSyncRequest(i2chidHandle)))
        return ret;

    return cmdbuf[1];
}

Result I2C_ReadRegisterBuffer(u8 devid, u8 regid, u8 *buffer, size_t buffersize)
{
    Result ret = 0;
    u32 savedbufs[2];
    u32 *cmdbuf = getThreadCommandBuffer();
    cmdbuf[0] = 0xF00C0;
    cmdbuf[1] = devid;
    cmdbuf[2] = regid;
    cmdbuf[3] = buffersize;
    u32 *staticbuf = getThreadStaticBuffers();
    savedbufs[0] = staticbuf[0];
    savedbufs[1] = staticbuf[1];
    staticbuf[0] = (buffersize << 14) | 2;
    staticbuf[1] = (u32)buffer;

    if(R_FAILED(ret = svcSendSyncRequest(i2chidHandle)))
        cmdbuf[1] = ret;

    staticbuf[0] = savedbufs[0];
    staticbuf[1] = savedbufs[1];
    return cmdbuf[1];
}