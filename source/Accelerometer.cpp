#include "Accelerometer.hpp"
#include "mcuhid.hpp"

void Accelerometer::Initialize()
{
    if(!m_initialized)
    {
        svcCreateEvent(&m_event, RESET_ONESHOT);
        if(R_FAILED(mcuHidGetAccelerometerEventHandle(&m_irqevent))) svcBreak(USERBREAK_ASSERT);
        m_initialized = 1;
    }
}

void Accelerometer::EnableOrDisableInterrupt(u8 explicitenable)
{
    if(explicitenable != -1)
    {
        mcuHidEnableAccelerometerInterrupt(explicitenable); 
        return;
    }
    
    else if(m_refcount <= 0)
        mcuHidEnableAccelerometerInterrupt(0); // disable
    else
        mcuHidEnableAccelerometerInterrupt(1); // enable
}

void Accelerometer::SetAccelerometerStatus(u8 enable)
{
    mcuHidEnableAccelerometer(enable);
}

void Accelerometer::Sampling()
{
    *(u32*)0xF00F4321 = 0xBABE;
}