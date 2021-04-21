#pragma once

Result iruInit_(uint8_t steal = 0);
void irrstExit_(void);
void iruScanInput_(void);
u32 iruKeysHeld_(void);
Result IRRST_Initialize_(u32 unk1, u8 unk2);
Result IRRST_Shutdown_(void);