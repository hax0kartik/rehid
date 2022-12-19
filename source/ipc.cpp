#include "ipc.hpp"

void IPC::HandleCommands(Hid *hid)
{
    uint32_t *cmdbuf = getThreadCommandBuffer();
    uint16_t cmdid = cmdbuf[0] >> 16;
    switch(cmdid)
    {
        case 1: // TouchScreenCalibrateParam
        {
            cmdbuf[0] = IPC_MakeHeader(cmdid, 1, 0);
            cmdbuf[1] = 0;
            break;
        }

        case 2: // TouchScreenFlushParam
        {
            cmdbuf[0] = IPC_MakeHeader(cmdid, 1, 0);
            cmdbuf[1] = 0;
            break;
        }

        case 3:
        {
            cmdbuf[0] = IPC_MakeHeader(cmdid, 1, 0);
            cmdbuf[1] = 0;
            break;
        }

        case 4:
        {
            cmdbuf[0] = IPC_MakeHeader(cmdid, 1, 0);
            cmdbuf[1] = 0;
            break;
        }

        case 5:
        {
            cmdbuf[0] = IPC_MakeHeader(cmdid, 1, 0);
            cmdbuf[1] = 0;
            break;
        }

        case 6:
        {
            cmdbuf[0] = IPC_MakeHeader(cmdid, 1, 0);
            cmdbuf[1] = 0;
            break;
        }

        case 7:
        {
            cmdbuf[0] = IPC_MakeHeader(cmdid, 1, 0);
            cmdbuf[1] = 0;
            break;
        }

        case 8:
        {
            cmdbuf[0] = IPC_MakeHeader(cmdid, 1, 0);
            cmdbuf[1] = 0;
            break;
        }

        case 9:
        {
            cmdbuf[0] = IPC_MakeHeader(cmdid, 1, 0);
            cmdbuf[1] = 0;
            break;
        }

        case 0xA:
        {
            //hid->TakeOverIRRSTIfRequired();
            //svcBreak(USERBREAK_ASSERT);
            //irCheckAndActivateIfRequired(tid);
            hid->RemapGenFileLoc();
            cmdbuf[0] = 0xA0047;
            cmdbuf[1] = 0;
            cmdbuf[2] = 0x14000000;
            cmdbuf[3] = *hid->GetSharedMemHandle();
            cmdbuf[4] = *hid->GetPad()->GetEvent();
            cmdbuf[5] = *hid->GetTouch()->GetEvent();
            cmdbuf[6] = *hid->GetAccelerometer()->GetEvent();
            cmdbuf[7] = *hid->GetGyroscope()->GetEvent();
            cmdbuf[8] = *hid->GetDebugPad()->GetEvent();
            break;
        }

        case 0xB:
        {
            cmdbuf[0] = IPC_MakeHeader(cmdid, 1, 0);
            cmdbuf[1] = 0;
            break;
        }

        case 0xC:
        {
            cmdbuf[0] = IPC_MakeHeader(cmdid, 1, 0);
            cmdbuf[1] = 0;
            break;
        }

        case 0xD:
        {
            cmdbuf[0] = IPC_MakeHeader(cmdid, 1, 0);
            cmdbuf[1] = 0;
            break;
        }

        case 0xE:
        {
            cmdbuf[0] = 0xE0200;
            cmdbuf[1] = 0;
            break;
        }

        case 0xF:
        {
            cmdbuf[0] = IPC_MakeHeader(cmdid, 1, 0);
            cmdbuf[1] = 0;
            break;
        }

        case 0x10:
        {
            cmdbuf[0] = 0x100100;
            cmdbuf[1] = 0;
            break;
        }

        case 0x11: // EnableAccelerometer
        {
            hid->GetAccelerometer()->EnableAndIncreementRef();
            cmdbuf[0] = IPC_MakeHeader(cmdid, 1, 0);
            cmdbuf[1] = 0;
            break;
        }

        case 0x12: // DisableAccelerometer
        {
            hid->GetAccelerometer()->DisableAndDecreementRef();
            cmdbuf[0] = IPC_MakeHeader(cmdid, 1, 0);
            cmdbuf[1] = 0;
            break;
        }

        case 0x13:
        {
            hid->GetGyroscope()->IncreementHandleIndex();
            cmdbuf[0] = IPC_MakeHeader(cmdid, 1, 0);
            cmdbuf[1] = 0;
            break;
        }

        case 0x14:
        {
            hid->GetGyroscope()->DecreementhandleIndex();
            if(hid->GetGyroscope()->GetRefCount() == 0){
                hid->GetGyroscope()->m_issetupdone = false;
                hid->GetGyroscope()->DisableSampling();
            }
            cmdbuf[0] = IPC_MakeHeader(cmdid, 1, 0);
            cmdbuf[1] = 0;
            break;
        }

        case 0x15:
        {
            cmdbuf[0] = 0x150080;
            cmdbuf[1] = 0;
            cmdbuf[2] = 0x41660000;
            break;
        }

        case 0x16:
        {
            cmdbuf[0] = 0x160180;
            cmdbuf[1] = 0;
            hid->GetGyroscope()->GetCalibParam((GyroscopeCalibrateParam*)&cmdbuf[2]);
            break;
        }

        case 0x17:
        {
            cmdbuf[0] = 0x170080;
            cmdbuf[1] = 0;
            cmdbuf[2] = 0;
            break;
        }

        case 0x18:
        {
            cmdbuf[0] = 0x180040;
            cmdbuf[1] = 0;
            break;
        }

        case 0x19: // Special rehid IPC command
        {
            cmdbuf[0] = 0x190040;
            cmdbuf[1] = 0;
            break;
        }

        default:
        {
            cmdbuf[1] = 0xD900182F; 
            break;
        }
    }
}