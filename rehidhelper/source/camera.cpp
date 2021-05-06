#include "camera.hpp"
#include "draw.hpp"
#include "icon_t3x.h"

int __stacksize__ = 100 * 1024;

void Camera::Initialize(Titles *titles)
{
    m_sheet = C2D_SpriteSheetLoadFromMem(icon_t3x, icon_t3x_size);
    m_globalimg = C2D_SpriteSheetGetImage(m_sheet, 0);
    m_titles = titles;
    m_buffer = new uint16_t[400 * 240];
    memset(m_buffer, 0, 400 * 240 * 2);
    m_tex = new C3D_Tex;
    m_context = quirc_new();
    quirc_resize(m_context, 400, 240);
    static const Tex3DS_SubTexture subt3x = { 512, 256, 0.0f, 1.0f, 1.0f, 0.0f };
    m_img = (C2D_Image){ m_tex, &subt3x };
    C3D_TexInit(m_tex, 512, 256, GPU_RGB565);
    C3D_TexSetFilter(m_tex, GPU_LINEAR, GPU_LINEAR);
    LightLock_Init(&m_lock);
    LightLock_Init(&m_botlock);
    svcCreateEvent(&m_finishedevent, RESET_STICKY);
}

void Camera::Finalize()
{
    delete[] m_buffer;
    m_buffer = 0;
    C3D_TexDelete(m_tex);
    delete m_tex;
    quirc_destroy(m_context);
    svcClearEvent(m_finishedevent);
    svcCloseHandle(m_finishedevent);
    m_finishedevent = 0;
    C2D_SpriteSheetFree(m_sheet);
}

static void CameraThread(void *arg)
{
    Camera *camera = (Camera*)arg;
    uint16_t *buffer = new uint16_t[400 * 240];
    memset(buffer, 0, 400 * 240 * 2);
    Handle events[3] = {0};
    events[2] = camera->GetFinishedEvent();
    uint32_t transferunit = 0;
    bool finished = false;
    camInit();
    CAMU_SetSize(SELECT_OUT1, SIZE_CTR_TOP_LCD, CONTEXT_A);
    CAMU_SetOutputFormat(SELECT_OUT1, OUTPUT_RGB_565, CONTEXT_A);
    CAMU_SetFrameRate(SELECT_OUT1, FRAME_RATE_30);
    CAMU_SetNoiseFilter(SELECT_OUT1, true);
    CAMU_SetAutoExposure(SELECT_OUT1, true);
    CAMU_SetAutoWhiteBalance(SELECT_OUT1, true);
    CAMU_Activate(SELECT_OUT1);
    CAMU_GetBufferErrorInterruptEvent(&events[1], PORT_CAM1);
    CAMU_SetTrimming(PORT_CAM1, false);
    CAMU_GetMaxBytes(&transferunit, 400, 240);
    CAMU_SetTransferBytes(PORT_CAM1, transferunit, 400, 240);
    CAMU_ClearBuffer(PORT_CAM1);
    svcSetThreadPriority(camera->GetCameraThreadHandle(), camera->GetMainThreadPriority() - 2);
    CAMU_SetReceiving(&events[0], buffer, PORT_CAM1, 400 * 240 * sizeof(u16), (s16) transferunit);
    CAMU_StartCapture(PORT_CAM1);
    while(!finished)
    {
        int32_t index = 0;
        svcWaitSynchronizationN(&index, events, 3, false, -1LL);
        switch(index)
        {
            case 0: // Recieve Event
            {
                LightLock_Lock(camera->GetLock());
                svcCloseHandle(events[0]);
                events[0] = 0;
                memcpy(&camera->GetBuffer()[0], buffer, 400 * 240 * sizeof(uint16_t));
                CAMU_SetReceiving(&events[0], buffer, PORT_CAM1, 400 * 240 * sizeof(uint16_t), (int16_t) transferunit);
                LightLock_Unlock(camera->GetLock());
                break;
            }

            case 1: // Buffer Error event
            {
                svcCloseHandle(events[0]);
                events[0] = 0;
                CAMU_ClearBuffer(PORT_CAM1);
                CAMU_SetReceiving(&events[0], buffer, PORT_CAM1, 400 * 240 * sizeof(uint16_t), (int16_t) transferunit);
                CAMU_StartCapture(PORT_CAM1);
                break;
            }

            case 2: // Finished scanning
            {
                finished = true;
                break;
            }
        }
    }
    CAMU_StopCapture(PORT_CAM1);
    bool busy = false;
    while(R_SUCCEEDED(CAMU_IsBusy(&busy, PORT_CAM1)) && busy)
        svcSleepThread(1e+9);
    CAMU_ClearBuffer(PORT_CAM1);
    CAMU_Activate(SELECT_NONE);
    camExit();
    delete[] buffer;
    buffer = 0;
    threadExit(0);
}

void Camera::CreateCameraThread()
{
    svcGetThreadPriority(&m_mainthreadprio, CUR_THREAD_HANDLE);
    m_thread = threadCreate((ThreadFunc)CameraThread, this, 0x1000, m_mainthreadprio + 2, 1, true);
}

static void CameraUiTop(Camera *camera)
{
    LightLock_Lock(camera->GetLock());
    for (u32 x = 0; x < 400; x++)
    {
        for (u32 y = 0; y < 240; y++)
        {
            u32 dstPos = ((((y >> 3) * (512 >> 3) + (x >> 3)) << 6) + ((x & 1) | ((y & 1) << 1) | ((x & 2) << 1) | ((y & 2) << 2) | ((x & 4) << 2) | ((y & 4) << 3))) * 2;
            u32 srcPos = (y * 400 + x) * 2;
            memcpy(&((u8*)camera->GetImage()->tex->data)[dstPos], &((u8*)camera->GetBuffer())[srcPos], 2);
        }
    }
    C2D_DrawImageAt(*camera->GetImage(), 0.0f, 0.0f, 0.5f, NULL, 1.0f, 1.0f);
    LightLock_Unlock(camera->GetLock());
}

void Camera::HandOverUI()
{
    m_str = "Looking for QR code..";
    ui.bot_func = std::bind(Draw::DrawTextInCentre, false, &m_botlock, &m_str);
    ui.top_func = std::bind(CameraUiTop, this);
}

void Camera::Controls()
{
    int w, h;
    int selected = 0, page = 0;
    int size = 0;
    auto tids = m_titles->GetTitles();
    tids.push_back(0);
    m_state = 0;
    while(aptMainLoop())
    {
        hidScanInput();

        if(m_state == 1)
        {
            if(keysDown() & KEY_A)
            {
                auto descvec = m_titles->GetTitlesDescription();
                descvec.push_back("Global (settings will be applied to all titles.)");
                ui.top_func = std::bind(Draw::DrawTitleInfo, &m_lock, descvec, &selected);
                auto iconvec = m_titles->GetC2DSMDHImgs();
                iconvec.push_back(m_globalimg);
                size = iconvec.size();
                ui.bot_func = std::bind(Draw::DrawGameSelectionScreen, &m_botlock, iconvec, &selected, &page);
                m_state = 3;
            }
        }
        else if(m_state == 2)
        {
            if(keysDown() & KEY_B)
            {
                ui.bot_func = nullptr;
                ui.top_func = nullptr;
                break;
            }
        }
        else if(m_state == 3)
        {
            LightLock_Lock(&m_botlock);
            if(keysDown() & KEY_RIGHT)
                selected++;
            if(keysDown() & KEY_LEFT)
                selected--;
            if(keysDown() & KEY_DOWN)
                selected += 10;
            if(keysDown() & KEY_UP)
                selected -= 10;
            
            if(keysDown() & KEY_A)
            {
                ui.bot_func = nullptr;
                m_utils.CreateRehidJson(tids[selected], selected == size - 1, m_data);
                break;
            }

            if(keysDown() & KEY_B)
            {
                ui.bot_func = nullptr;
                m_state = 0;
            }

            if(selected < 0)
                selected = size - 1;
            
            else if(selected > size - 1)
                selected = 0;
            
            page = selected / 70;
            LightLock_Unlock(&m_botlock);
        }
        else
        {
            uint8_t *img = (uint8_t*)quirc_begin(m_context, &w, &h);
            LightLock_Lock(&m_lock);
            for(int x = 0; x < w; x++)
            {
                for(int y = 0; y < h; y++)
                {
                    uint16_t px = m_buffer[y * 400 + x];
                    img[y * w + x] = (uint8_t)(((((px >> 11) & 0x1F) << 3) + (((px >> 5) & 0x3F) << 2) + ((px & 0x1F) << 3)) / 3);
                }
            }
            LightLock_Unlock(&m_lock);
            quirc_end(m_context);
            if(quirc_count(m_context) > 0)
            {
                struct quirc_code code;
                struct quirc_data scandata;
                quirc_extract(m_context, 0, &code);
                if (!quirc_decode(&code, &scandata))
                {
                    LightLock_Lock(&m_botlock);
                    m_str = "QR found. Press A to proceed.";
                    LightLock_Unlock(&m_botlock);
                    svcSignalEvent(m_finishedevent);
                    m_data.resize(scandata.payload_len);
                    memcpy(&m_data[0], &scandata.payload[0], scandata.payload_len);
                    const std::string parsed = m_utils.ParseJson(m_data);
                    ui.top_func = std::bind(Draw::DrawConfigScreen, parsed);
                    m_state = 1;
                }
            }
        }
    }
}