#pragma once
#include <3ds.h>

extern Handle cdchandle;
Result codecInit();
Result CDCHID_GetData(u32 *touchscreendata, u32 *circlepaddata);