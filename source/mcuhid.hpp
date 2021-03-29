#pragma once
#include <3ds.h>

Result mcuHidInit();
Result mcuHidGetSliderState(u8 *rawvalue);
Result mcuHidGetAccelerometerEventHandle(Handle *handle);
Result mcuHidEnableAccelerometerInterrupt(u8 enable);
Result mcuHidEnableAccelerometer(u8 enable);