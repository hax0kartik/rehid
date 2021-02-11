#include <3ds.h>
#include "hid.hpp"
#include "ipc.hpp"
extern "C" {
    #include "csvc.h"
    #include "services.h"
}
#define ONERRSVCBREAK(ret) if(R_FAILED(ret)) svcBreak(USERBREAK_ASSERT);

#define MAX_SESSIONS 10
#define SERVICE_ENDPOINTS 3

static Result HandleNotifications(Hid *hid, int *exit)
{
    uint32_t notid = 0;
    Result ret = srvReceiveNotification(&notid);
    if(R_FAILED(ret)) return ret;

    switch(notid)
    {
        case 0x100: // Exit
        {
            *exit = 1;
            break;
        }

        case 0x104: // Entering SleepMode
        {
            hid->EnteringSleepMode();
            break;
        }

        case 0x105: // Exiting SleepMode
        {
            hid->ExitingSleepMode();
            break;
        }

        case 0x213: // Sleep Opened
        {
            hid->IsShellOpened(true);
            break;
        }

        case 0x214: // Shell Closed
        {
            hid->IsShellOpened(false);
            break;
        }
    }
    return 0;
}

extern "C"
{
    extern u32 __ctru_heap, __ctru_heap_size, __ctru_linear_heap, __ctru_linear_heap_size;
    extern char *fake_heap_start;
    extern char *fake_heap_end;

    // this is called before main
    void __system_allocateHeaps(void)
    {
        u32 tmp=0;
	    __ctru_heap_size = 0x8000;
	    // Allocate the application heap
	    __ctru_heap = 0x08000000;
	    svcControlMemoryEx(&tmp, __ctru_heap, 0x0, __ctru_heap_size, MEMOP_ALLOC, (MemPerm)(MEMPERM_READWRITE | MEMREGION_BASE), false);
	    // Set up newlib heap
	    fake_heap_start = (char*)__ctru_heap;
	    fake_heap_end = fake_heap_start + __ctru_heap_size;
    }

    void __appInit() {
        srvSysInit();
        fsSysInit();
        //gdbHioDevInit();
        //gdbHioDevRedirectStdStreams(false, true, false);
        ptmSysmInit();
      //  logInit();
    }

    // this is called after main exits
    void __appExit() {
        ptmSysmExit();
        srvSysExit();
    }

    // stubs for non-needed pre-main functions
    void __sync_init();
    void __sync_fini();
    void __system_initSyscalls();
    void __libc_init_array(void);
    void __libc_fini_array(void);

    void initSystem(void (*retAddr)(void)) {
        __libc_init_array();
        __sync_init();
        __system_initSyscalls();
        __system_allocateHeaps();
        __appInit();
    }

    void __ctru_exit(int rc) {
        __appExit();
        __sync_fini();
        __libc_fini_array();
        svcExitProcess();
    }
}

int main()
{   
    Hid hid;
    IPC ipc;
    Handle handles[SERVICE_ENDPOINTS + MAX_SESSIONS];
    Result ret = 0;
    const char *srvnames[] = {"", "hid:SPVR", "hid:USER", "hid:QTM"};
    for(int i = 1; i <= 3; i++)
    {
        ret = srvRegisterService(&handles[i], srvnames[i], 6);
        ONERRSVCBREAK(ret);
    }
    
    //hid.TakeOverIRRSTIfRequired();
    hid.CreateAndMapMemoryBlock();
    hid.CreateRingsOnSharedmemoryBlock();
    hid.InitializePad();
    hid.StartThreadsForSampling();

    ONERRSVCBREAK(srvEnableNotification(&handles[0]));
    ONERRSVCBREAK(srvSubscribe(0x104));
    ONERRSVCBREAK(srvSubscribe(0x105));
    ONERRSVCBREAK(srvSubscribe(0x213));
    ONERRSVCBREAK(srvSubscribe(0x214));

    Handle replytarget = 0;
    int termrequest = 0;
    int activehandles = SERVICE_ENDPOINTS + 1;
    do {
        if (replytarget == 0) {
            u32 *cmdbuf = getThreadCommandBuffer();
            cmdbuf[0] = 0xFFFF0000;
        }
        s32 requestindex;
        //logPrintf("B SRAR %d %x\n", request_index, reply_target);
        ret = svcReplyAndReceive(&requestindex, handles, activehandles, replytarget);
        //logPrintf("A SRAR %d %x\n", request_index, reply_target);

        if (R_FAILED(ret)) {
            // check if any handle has been closed
            if (ret == 0xC920181A) {
                if (requestindex == -1) {
                    for (int i = SERVICE_ENDPOINTS; i < MAX_SESSIONS+SERVICE_ENDPOINTS; i++) {
                        if (handles[i] == replytarget) {
                            requestindex = i;
                            break;
                        }
                    }
                }
                svcCloseHandle(handles[requestindex]);
                handles[requestindex] = handles[activehandles-1];
                activehandles--;
                replytarget = 0;
            } else {
                svcBreak(USERBREAK_ASSERT);
            }
        } else {
            // process responses
            replytarget = 0;
            switch (requestindex) {
                case 0: { // notification
                    if (R_FAILED(HandleNotifications(&hid, &termrequest))) {
                        svcBreak(USERBREAK_ASSERT);
                    }
                    break;
                }
                case 1: // new session
                case 2: // new session
                case 3:{// new session
                    //logPrintf("New Session %d\n", request_index);
                    Handle handle;
                    if (R_FAILED(svcAcceptSession(&handle, handles[requestindex]))) {
                        svcBreak(USERBREAK_ASSERT);
                    }
                    //logPrintf("New Session accepted %x on index %d\n", handle, nmbActiveHandles);
                    if (activehandles < MAX_SESSIONS+SERVICE_ENDPOINTS) {
                        handles[activehandles] = handle;
                        activehandles++;
                    } else {
                        svcCloseHandle(handle);
                    }
                    break;
                }
                default: { // session
                    //logPrintf("cmd handle %x\n", request_index);
                //	__asm("bkpt #0");
                    ipc.HandleCommands(&hid);
                    replytarget = handles[requestindex];
                    break;
                }
            }
        }
    } while (!termrequest);
}