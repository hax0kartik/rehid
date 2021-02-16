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