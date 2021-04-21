#include "iruseripc.hpp"
#include <cstdio>
void irUSERIPC::HandleCommands(IrUser *ir){

    uint32_t *cmdbuf = getThreadCommandBuffer();
    uint16_t cmdid = cmdbuf[0] >> 16;
    printf("Cmdid %08X recieved for IR:USER", cmdbuf[0]);
    
    switch(cmdid)
    {
        case 0x6: // RequireConnection
        {
            ir->RequireConnection();
            cmdbuf[0] = 0x60040;
            cmdbuf[1] = 0;
            break;
        }

        case 0x9: // Disconnect
        {
            
        }

        case 0xA: // GetRecieveEvent
        {
            cmdbuf[0] = 0xA0042;
            cmdbuf[1] = 0;
            cmdbuf[2] = 0;
            cmdbuf[3] = (u32)*ir->GetRecieveEvent();
            break;
        }
        
        case 0xB: // GetSendEvent()
        {
            cmdbuf[0] = 0xB0042;
            cmdbuf[1] = 0;
            cmdbuf[2] = 0;
            cmdbuf[3] =(u32)*ir->GetSendEvent();
            break;
        }

        case 0xC: // GetConnectionEvent
        {
            cmdbuf[0] = 0xC0042;
            cmdbuf[1] = 0;
            cmdbuf[2] = 0;
            cmdbuf[3] = (u32)*ir->GetConnectionEvent();
            break;
        }

        case 0x18: // IntializeIrNopShared
        {
            ir->IntializeIrNopShared((size_t)cmdbuf[1], (size_t)cmdbuf[2], (s32)cmdbuf[3], (size_t)cmdbuf[4], (s32)cmdbuf[5], (u32)cmdbuf[6], (Handle)cmdbuf[8]);
            cmdbuf[0] = 0x180040;
            cmdbuf[1] = 0;
            break;
        }
        default:
            *(u32*)cmdid = 0xf00d;
    }
    return;
}