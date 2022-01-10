#include <3ds.h>

Result gpiohidInit(void);
void gpioHidExit();
Result GPIOHID_SetRegPart1(u32 value, u32 mask);
Result GPIOHID_SetRegPart2(u32 value, u32 mask);
Result GPIOHID_SetInterruptMask(u32 value, u32 mask);
Result GPIOHID_BindInterrupt(Handle *intr);
Result GPIOHID_UnbindInterrupt(Handle *intr);
Result GPIOHID_GetData(u32 mask, u32 *value);