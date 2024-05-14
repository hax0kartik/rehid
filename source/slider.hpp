#include <3ds.h>
class Slider {
public:
    void GetConfigSettings();
    void ReadValuesFromMCU();
    float Normalize();

private:
    int16_t m_lowerbound = 0x24 + 8;
    int16_t m_upperbound = 0xDB - 8;
    int32_t m_oldrawstate = 0;
    int32_t m_rawstate = 0;
};