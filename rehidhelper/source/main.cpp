#include <iostream>
#include <malloc.h>
#include <3ds.h>
#include "ui.hpp"
#include "app.hpp"
#define freeze while(aptMainLoop()) {};
uiFuncs_s ui;
int main()
{
    romfsInit();
    acInit();
    amInit();
    cfguInit();
    u32 *socbuf = (u32*)memalign(0x1000, 0x100000);
    socInit(socbuf, 0x100000);
    //ui.debug = true;
	APT_SetAppCpuTimeLimit(30);
	aptSetSleepAllowed(false);
	aptSetHomeAllowed(false);
	
    ui.done = false;
    s32 prio;
    svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
	threadCreate((ThreadFunc)&uiThread, nullptr, 0x1000, prio - 3, -2, true);
    App app;
    app.DoStuffBeforeMain();
    app.MainLoop();
    ui.done = true;
    svcSleepThread(1e+9);
    socExit();
    amExit();
    acExit();
    return 0;
}