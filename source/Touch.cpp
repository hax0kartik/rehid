#include "Touch.hpp"
#include <stdio.h>
#include "printf.h"
void _putchar(char character) {
    svcOutputDebugString(&character, 1);
}

void Touch::Initialize() {
    if (!m_initialized) {
        m_initialized = 1;
        cfguInit();
        Result ret = CFG_GetConfigInfoBlk4(0x10, 0x40000u, &m_touchcfg);
        cfguExit();

        if (ret != 0)
            *(u32*)0x123457 = ret;

        CalculateCalibrationStruct();
        svcCreateEvent(&m_event, RESET_ONESHOT);
    }
}

void Touch::CalculateCalibrationStruct() {
    touchcalib tmp;
    tmp.xdotsize = ((m_touchcfg.rawx0 - m_touchcfg.rawx1) << 8) / (int32_t)(m_touchcfg.pointx0 - m_touchcfg.pointx1);
    tmp.x = (((m_touchcfg.rawx0 + m_touchcfg.rawx1) << 8) - (m_touchcfg.pointx0 + m_touchcfg.pointx1) * tmp.xdotsize) >> 7;
    tmp.xdotsizeinv = 0x10000000 / (uint32_t)tmp.xdotsize;

    tmp.ydotsize = ((m_touchcfg.rawy0 - m_touchcfg.rawy1) << 8) / (int32_t)(m_touchcfg.pointy0 - m_touchcfg.pointy1);
    tmp.y = (((m_touchcfg.rawy0 + m_touchcfg.rawy1) << 8) - (m_touchcfg.pointy0 + m_touchcfg.pointy1) * tmp.ydotsize) >> 7;
    tmp.ydotsizeinv = 0x10000000 / (uint32_t)tmp.ydotsize;

    if (tmp.xdotsize == 0) {
        m_calib.x = 0;
        m_calib.xdotsize = 0;
        m_calib.xdotsizeinv = 0;
    } else {
        m_calib.x = tmp.x;
        m_calib.xdotsize = tmp.xdotsize;
        m_calib.xdotsizeinv = tmp.xdotsizeinv;
    }

    if (tmp.ydotsize == 0) {
        m_calib.y = 0;
        m_calib.ydotsize = 0;
        m_calib.ydotsizeinv = 0;
    } else {
        m_calib.y = tmp.y;
        m_calib.ydotsize = tmp.ydotsize;
        m_calib.ydotsizeinv = tmp.ydotsizeinv;
    }
}

void Touch::RawToPixel(int *arr, TouchEntry *pixeldata, TouchEntry *rawdata) {
    pixeldata->touch = rawdata->touch;

    if (rawdata->touch) {
        int64_t v3 = 4 * rawdata->x - m_calib.x;
        int64_t pxx = ((((v3 * m_calib.xdotsizeinv) >> 22) | (((m_calib.xdotsizeinv * v3) >> 32) << 10)));

        if (pxx >= 5 && pxx <= 314)
            pixeldata->x = pxx;
        else if (pxx <= 5)
            pixeldata->x = 5;
        else
            pixeldata->x = 314;

        int64_t v5 = 4 * rawdata->y - m_calib.y;
        int64_t pxy = (((v5 * m_calib.ydotsizeinv) >> 22) | (((m_calib.ydotsizeinv * v5) >> 32) << 10));

        if (pxy >= 5 && pxy <= 234)
            pixeldata->y = pxy;
        else if (pxy <= 5)
            pixeldata->y = 5;
        else
            pixeldata->y = 234;

    } else {
        pixeldata->x = 0;
        pixeldata->y = 0;
    }

    return;
}

void Touch::Sampling(u32 touchscreendata, Remapper *remapper) {
    TouchEntry rawdata, pixdata;
    int arr[] = {51, 3276, 81940, 68, 4369, 61440};
    remapper->m_remaptouchkeys = 0;

    //printf_("CDCHID_GetData ret: %08x, touchscreendata x: %d, y :%d dummy: %X\n", ret, touchscreendata & 0xFFF, (touchscreendata & 0xFFF000) >> 12, dummy);
    if (((touchscreendata & 0x1000000) >> 16) >> 16 && ((touchscreendata & 0x6000000) >> 25) & 0xFF) {
        rawdata.x = m_latest.x;
        rawdata.y = m_latest.y;
        rawdata.touch = m_latest.touch;
    } else {
        rawdata.x = m_latest.x = touchscreendata & 0xFFF;
        rawdata.y = m_latest.y = (touchscreendata & 0xFFF000) >> 12;
        rawdata.touch = m_latest.touch = (touchscreendata & 0x1000000) >> 24;
    }

    if (!rawdata.touch)
        rawdata.x = rawdata.y = 0;

    if (remapper->m_touchoveridex != 0 && remapper->m_touchoveridey != 0) {
        rawdata.touch = 1;
        pixdata.touch = 1;
        m_latest.touch = 1;
        pixdata.x = remapper->m_touchoveridex;
        pixdata.y = remapper->m_touchoveridey;
    } else
        RawToPixel(arr, &pixdata, &rawdata);

    for (int i = 0; i < remapper->m_touchtokeysentries; i++) {
        uint16_t x = remapper->m_remaptouchtokeysobjects[i].x;
        uint16_t y = remapper->m_remaptouchtokeysobjects[i].y;
        uint16_t h = remapper->m_remaptouchtokeysobjects[i].h;
        uint16_t w = remapper->m_remaptouchtokeysobjects[i].w;

        if (pixdata.x > x && pixdata.x < x + w && pixdata.y > y && pixdata.y < y + h) {
            m_latest.touch = pixdata.touch = 0;
            remapper->m_remaptouchkeys = remapper->m_remaptouchtokeysobjects[i].key;
        }
    }

    m_ring->SetRaw(rawdata);
    m_ring->WriteToRing(pixdata);

    svcSignalEvent(m_event);
}