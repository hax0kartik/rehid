#include <3ds.h>
#include "mcuhid.hpp"

Handle mcuHidHandle;
Result mcuHidInit()
{
    Result ret = 0;
    ret = srvGetServiceHandle(&mcuHidHandle, "mcu::HID");
    return ret;
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