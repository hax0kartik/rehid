#pragma once
#include <3ds.h>
#include "AccelerometerRing.hpp" // For AccelerometerEntry

Result mcuHidInit();
void   mcuHidExit();
Result mcuHidGetSliderState(u8 *rawvalue);
Result mcuHidGetAccelerometerEventHandle(Handle *handle);
Result mcuHidEnableAccelerometerInterrupt(u8 enable);
Result mcuHidEnableAccelerometer(u8 enable);
Result mcuHidReadAccelerometerValues(AccelerometerEntry *entry);
Result mcuHidGetSoundVolume(uint8_t *volume);
Result mcuHidGetEventReason(uint32_t *reason);