#pragma once
#include <3ds.h>

Result mcuHidInit();
Result mcuHidGetSliderState(u8 *rawvalue);