#include "gpio.h"

Handle gpioHidHandle;
static int gpioHidRefCount;

Result gpiohidInit(void) {
    if (AtomicPostIncrement(&gpioHidRefCount))
        return 0;

    Result ret = 0;

    if (R_FAILED(ret = srvGetServiceHandle(&gpioHidHandle, "gpio:HID")))
        AtomicDecrement(&gpioHidRefCount);

    return ret;
}

void gpioHidExit() {
    AtomicDecrement(&gpioHidRefCount);
    svcCloseHandle(gpioHidHandle);
}

Result GPIOHID_SetRegPart1(u32 value, u32 mask) {
    Result ret = 0;
    u32 *cmdbuf = getThreadCommandBuffer();
    cmdbuf[0] = IPC_MakeHeader(0x2, 2, 0);
    cmdbuf[1] = value;
    cmdbuf[2] = mask;

    if (R_FAILED(ret = svcSendSyncRequest(gpioHidHandle)))
        return ret;

    return cmdbuf[1];
}

Result GPIOHID_SetRegPart2(u32 value, u32 mask) {
    Result ret = 0;
    u32 *cmdbuf = getThreadCommandBuffer();
    cmdbuf[0] = IPC_MakeHeader(0x4, 2, 0);
    cmdbuf[1] = value;
    cmdbuf[2] = mask;

    if (R_FAILED(ret = svcSendSyncRequest(gpioHidHandle)))
        return ret;

    return cmdbuf[1];
}

Result GPIOHID_SetInterruptMask(u32 value, u32 mask) {
    Result ret = 0;
    u32 *cmdbuf = getThreadCommandBuffer();
    cmdbuf[0] = IPC_MakeHeader(0x6, 2, 0);
    cmdbuf[1] = value;
    cmdbuf[2] = mask;

    if (R_FAILED(ret = svcSendSyncRequest(gpioHidHandle)))
        return ret;

    return cmdbuf[1];
}

Result GPIOHID_GetData(u32 mask, u32 *value) {
    Result ret = 0;
    u32 *cmdbuf = getThreadCommandBuffer();
    cmdbuf[0] = 0x70040;
    cmdbuf[1] = mask;

    if (R_FAILED(ret = svcSendSyncRequest(gpioHidHandle)))
        return ret;

    *value = cmdbuf[2];
    return cmdbuf[1];
}

Result GPIOHID_BindInterrupt(Handle *intr) {
    Result ret = 0;
    u32 *cmdbuf = getThreadCommandBuffer();
    cmdbuf[0] = IPC_MakeHeader(0x9, 2, 2);
    cmdbuf[1] = 0x100; // mask
    cmdbuf[2] = 8; // priority
    cmdbuf[3] = IPC_Desc_SharedHandles(1);
    cmdbuf[4] = *intr;

    if (R_FAILED(ret = svcSendSyncRequest(gpioHidHandle)))
        return ret;

    return cmdbuf[1];
}

Result GPIOHID_UnbindInterrupt(Handle *intr) {
    Result ret = 0;
    u32 *cmdbuf = getThreadCommandBuffer();
    cmdbuf[0] = IPC_MakeHeader(0xA, 1, 2);
    cmdbuf[1] = 0x100; // mask
    cmdbuf[2] = IPC_Desc_SharedHandles(1);
    cmdbuf[3] = *intr;

    if (R_FAILED(ret = svcSendSyncRequest(gpioHidHandle)))
        return ret;

    return cmdbuf[1];
}
