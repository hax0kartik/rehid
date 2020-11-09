#include "codec.h"

Handle cdchandle;
Result CDCHID_Initialize()
{
    u32 *cmdbuf = getThreadCommandBuffer();
    cmdbuf[0] = 0x20000;
    Result ret = 0;
    if(R_FAILED(svcSendSyncRequest(cdchandle)))
        return ret;
    ret = cmdbuf[1];
    return ret;
}

Result codecInit()
{
    Result ret = srvGetServiceHandle(&cdchandle, "cdc:HID");
    if(R_SUCCEEDED(ret))
    {
        ret = 0xC9403800;
        while(ret == 0xC9403800)
        {
            ret = CDCHID_Initialize();
            svcSleepThread(1 * 1000 * 1000);
        }
    }
    return ret;
}


Result CDCHID_GetData(u32 *touchscreendata, u32 *circlepaddata)
{
    u32 *cmdbuf = getThreadCommandBuffer();
    cmdbuf[0] = 0x10000;
    Result ret = 0;
    if(R_FAILED(svcSendSyncRequest(cdchandle)))
        return ret;
    ret = cmdbuf[1];
    *touchscreendata = cmdbuf[2];
    *circlepaddata = cmdbuf[3];
    return ret;
}