#include "Touch.hpp"
#include <stdio.h>
#include "printf.h"
void _putchar(char character)
{
    svcOutputDebugString(&character, 1);
}

void Touch::Initialize()
{
    if(!m_initialized)
    {
        m_initialized = 1;
        svcCreateEvent(&m_event, RESET_ONESHOT);
        if(R_FAILED(codecInit())) svcBreak(USERBREAK_ASSERT);
    }
}

void Touch::RawToPixel(int *arr, TouchEntry *pixeldata, TouchEntry *rawdata)
{
    pixeldata->touch = rawdata->touch;
    if (rawdata->touch)
    {
        int64_t v3 = 4 * rawdata->x - arr[0];
        int64_t pxx = (((v3 * arr[2]) >> 22) | (((arr[2] * v3) >> 32) << 10));
        if(pxx >= 5 && pxx <= 314)
            pixeldata->x = pxx;
        else if(pxx <= 5)
            pixeldata->x = 5;
        else
            pixeldata->x = 314;   
        
        int64_t v5 = 4 * rawdata->y - arr[3];
        int64_t pxy = (((v5 * arr[5]) >> 22) | (((arr[5] * v5) >> 32) << 10));
        if(pxy >= 5 && pxy <= 234)
            pixeldata->y = pxy;
        else if(pxx <= 5)
            pixeldata->y = 5;
        else
            pixeldata->y = 234;   
        
    }
    else
    {
        pixeldata->x = 0;
        pixeldata->y = 0;
    }
    return;
}

void Touch::Sampling()
{
    Result ret = 0;
    u32 dummy, touchscreendata;
    TouchEntry rawdata, pixdata;
    int arr[] = {51, 3276, 81940, 68, 4369, 61440};
    ret = CDCHID_GetData(&touchscreendata, &dummy);
    //printf_("CDCHID_GetData ret: %08x, touchscreendata x: %d, y :%d dummy: %X\n", ret, touchscreendata & 0xFFF, (touchscreendata & 0xFFF000) >> 12, dummy);
    if(ret == 0)
    {
        if (((touchscreendata & 0x1000000) >> 16) >> 16 && ((touchscreendata & 0x6000000) >> 25) & 0xFF)
        {
            rawdata.x = m_latest.x;
            rawdata.y = m_latest.y;
            rawdata.touch = m_latest.touch;
        }
        else
        {
            rawdata.x = m_latest.x = touchscreendata & 0xFFF;
            rawdata.y = m_latest.y = (touchscreendata & 0xFFF000) >> 12;
            rawdata.touch = m_latest.touch = (touchscreendata & 0x1000000) >> 24;
        }

        if(!rawdata.touch)
            rawdata.x = rawdata.y = 0;

        RawToPixel(arr, &pixdata, &rawdata);
        m_ring->SetRaw(rawdata);
        m_ring->WriteToRing(pixdata);

        svcSignalEvent(m_event);
    }
}