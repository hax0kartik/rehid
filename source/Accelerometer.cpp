#include "Accelerometer.hpp"
#include "mcuhid.hpp"

void Accelerometer::Initialize()
{
    if(!m_initialized)
    {
        m_calib.scalex = 1024;
        m_calib.scaley = 1024;
        m_calib.scalez = 1024;
        m_calib.offsetx = 0;
        m_calib.offsety = 0;
        m_calib.offsetz = 0;
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

void Accelerometer::EnableAndIncreementRef()
{
    if(!m_refcount)
        mcuHidEnableAccelerometerInterrupt(1); // enable
    ++m_refcount;
}

void Accelerometer::DisableAndDecreementRef()
{
    --m_refcount;
    if(!m_refcount)
        mcuHidEnableAccelerometerInterrupt(0); // disable
}

void Accelerometer::SetAccelerometerStatus(u8 enable)
{
    mcuHidEnableAccelerometer(enable);
}

void Accelerometer::CalibrateVals(AccelerometerEntry *raw, AccelerometerEntry *final)
{
    int16_t tmpx = ((raw->x - m_calib.offsetx) << 10) / (2 * m_calib.scalex);
    final->x = tmpx;
    if ( tmpx >= -930 )
    {
        if ( tmpx > 930 )
            tmpx = 930;
    }
    else
    {
        tmpx = -930;
    }
    final->x = tmpx;
  
    int16_t tmpy = ((raw->y - m_calib.offsety) << 10) / (2 * m_calib.scaley);
    final->y = tmpy;
    if ( tmpy >= -930 )
    {
        if ( tmpy > 930 )
            tmpy = 930;
    }
    else
    {
        tmpy = -930;
    }
    final->y = tmpy;

    int16_t tmpz = ((raw->z - m_calib.offsetz) << 10) / (2 * m_calib.scalez);
    final->z = tmpz;
    if ( tmpz >= -930 )
    {
        if ( tmpz > 930 )
            tmpz = 930;
    }
    else
    {
        tmpz = -930;
    }
    final->z = tmpz;

}
void Accelerometer::Sampling()
{
    AccelerometerEntry rawvals;
    if(m_refcount > 0)
    {
        uint32_t reason;
        Result ret = mcuHidGetEventReason(&reason);
        if(R_SUCCEEDED(ret) && (reason & 0x1000) != 0)
        {
            ret = mcuHidReadAccelerometerValues(&rawvals);
            if(R_SUCCEEDED(ret))
            {
                AccelerometerEntry calibvals;
                AccelerometerEntry finalvals;
                CalibrateVals(&rawvals, &calibvals);
                finalvals.x = -calibvals.x;
                finalvals.y = calibvals.z;
                finalvals.z = -calibvals.y;
                m_ring->SetRaw(rawvals);
                m_ring->WriteToRing(finalvals);
                svcSignalEvent(m_event);
            }
        }
    }
}