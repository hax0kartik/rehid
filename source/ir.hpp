#pragma once

Result irrstInit_(uint8_t steal = 0);
void irrstExit_(void);
void irrstScanInput_(void);
u32 irrstKeysHeld_(void);
