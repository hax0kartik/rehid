#include <3ds.h>
#include "mcuhid.hpp"
#include <cstring>
Handle mcuHidHandle;
Result mcuHidInit()
{
    Result ret = 0;
    ret = srvGetServiceHandle(&mcuHidHandle, "mcu::HID");
    return ret;
}

Result mcuHidEnableAccelerometer(u8 enable)
{
    u32 *cmdbuf = getThreadCommandBuffer();
    cmdbuf[0] = 0x10040;
    cmdbuf[1] = enable;
    Result ret = svcSendSyncRequest(mcuHidHandle);
    if(R_FAILED(ret)) return ret;
    return cmdbuf[1];
}

Result mcuHidReadAccelerometerValues(AccelerometerEntry *entry)
{
    u32 *cmdbuf = getThreadCommandBuffer();
    cmdbuf[0] = 0x60000;
    Result ret = svcSendSyncRequest(mcuHidHandle);
    if(R_FAILED(ret)) return ret;
    memcpy(entry, &cmdbuf[2], 6);
    return cmdbuf[1];
}

Result mcuHidGetSliderState(u8 *rawvalue)
{
    u32 *cmdbuf = getThreadCommandBuffer();
    cmdbuf[0] = 0x70000;
    Result ret = svcSendSyncRequest(mcuHidHandle);
    if(R_FAILED(ret)) return ret;
    *rawvalue = cmdbuf[2] & 0xFF;
    return cmdbuf[1];
}

Result mcuHidGetAccelerometerEventHandle(Handle *handle)
{
    u32 *cmdbuf = getThreadCommandBuffer();
    cmdbuf[0] = 0xC0000;
    Result ret = svcSendSyncRequest(mcuHidHandle);
    if(R_FAILED(ret)) return ret;
    *handle = cmdbuf[3];
    return cmdbuf[1];
}

Result mcuHidGetEventReason(uint32_t *reason)
{
    u32 *cmdbuf = getThreadCommandBuffer();
    cmdbuf[0] = 0xD0000;
    Result ret = svcSendSyncRequest(mcuHidHandle);
    if(R_FAILED(ret)) return ret;
    *reason = cmdbuf[2];
    return cmdbuf[1];
}

Result mcuHidEnableAccelerometerInterrupt(u8 enable)
{
    u32 *cmdbuf = getThreadCommandBuffer();
    cmdbuf[0] = 0xF0040;
    cmdbuf[1] = enable;
    Result ret = svcSendSyncRequest(mcuHidHandle);
    if(R_FAILED(ret)) return ret;
    return cmdbuf[1];
}