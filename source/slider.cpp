#include <3ds.h>
#include "slider.hpp"
#include "mcuhid.hpp"

void Slider::ReadValuesFromMCU()
{
    Result ret = mcuHidGetSliderState((u8*)&m_rawstate);
    if(R_FAILED(ret))
        m_rawstate = m_oldrawstate;
    else
        m_oldrawstate = m_rawstate;
}

void Slider::GetConfigSettings()
{
    struct bounds
    {
        int16_t lowerbound3d;
        int16_t upperbound3d;
        int16_t lowerboundsnd;
        int16_t upperboundsnd;
    };
    bounds bound;

    cfguInit();
    Result ret = CFG_GetConfigInfoBlk4(8, 0x120000u, &bound);
    cfguExit();
    if(ret != 0) *(u32*)0xFFFF1234 = 0x901;

    m_lowerbound = bound.lowerbound3d + 8;
    m_upperbound = bound.upperbound3d - 8;
}

// We try to keep this function as close as possible to the function in the hid module
float Slider::Normalize()
{
    static int32_t v1 = 0;
    static float fval1 = 0.0f;
    float fval2 = 0.0f;
    const int32_t SOMECONST = 2; 

    if (m_rawstate - v1 < -SOMECONST)
        v1 = m_rawstate + SOMECONST;
    else if(m_rawstate - v1 > SOMECONST)
        v1 = m_rawstate - SOMECONST;

    if(m_lowerbound + SOMECONST >= v1)
        fval2 = 0.0f;

    if(m_lowerbound + SOMECONST < v1)
    {
        if(v1 < m_upperbound - SOMECONST)
            fval2 = (float)(v1 - m_lowerbound - SOMECONST) / (m_upperbound - m_lowerbound - (SOMECONST * 2));
        else
            fval2 = 1.0f;
    }

    fval1 = fval1 + ((fval2 - fval1) * 0.5f);

    if(fval1 > 1.0f) fval1 = 1.0f;
    else if(fval1 < 0.00001f) fval1 = 0.0f;

    return fval1;
}