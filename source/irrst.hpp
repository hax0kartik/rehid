#pragma once

Result iruInit_(uint8_t steal = 0);
void irrstExit_(void);
void iruScanInput_(void);
u32 iruKeysHeld_(void);
Result irrstInit_(uint8_t);