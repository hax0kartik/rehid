#include "Camera.hpp"
#include "../app.hpp"

Camera::Camera() {
    camInit();
    LightLock_Init(&m_messagelock);
    LightLock_Init(&m_imagelock);
    svcCreateEvent(&m_finishedevent, RESET_STICKY);
    m_tex = new C3D_Tex;
    C3D_TexInit(m_tex, 512, 256, GPU_RGB565);
    C3D_TexSetFilter(m_tex, GPU_LINEAR, GPU_LINEAR);
    static const Tex3DS_SubTexture subt3x = { 512, 256, 0.0f, 1.0f, 1.0f, 0.0f };
    m_img = (C2D_Image) {
        m_tex, &subt3x
    };

    m_context = quirc_new();
    quirc_resize(m_context, 400, 240);
}

Camera::~Camera() {
    quirc_destroy(m_context);
    C3D_TexDelete(m_tex);
    delete m_tex;
    svcClearEvent(m_finishedevent);
    svcCloseHandle(m_finishedevent);
    m_finishedevent = 0;
    camExit();
}

void Camera::OnStateEnter(App *app) {
    m_payloadfound = false;
    m_textbuf = C2D_TextBufNew(500);
    m_buffer = new uint16_t[400 * 240];
    svcClearEvent(m_finishedevent);
    svcGetThreadPriority(&m_mainthreadprio, CUR_THREAD_HANDLE);
    memset(m_buffer, 0, 400 * 240 * sizeof(uint16_t));

    std::string s = "Looking for QR code..";
    SetString(s);

    m_decodeworker.CreateThread([](Camera& camera, App *app) -> void{
        std::string s2 = "Decode Thread created";
        int w, h;
        bool done = false;
        svcSetThreadPriority(CUR_THREAD_HANDLE, camera.GetMainThreadPrio() + 1);

        while (!done) {
            svcSleepThread(10); // yield from this thread;
            uint8_t *img = (uint8_t*)quirc_begin(camera.GetQuircContext(), &w, &h);
            LightLock_Lock(camera.GetImageLock());

            for (int x = 0; x < w; x++) {
                for (int y = 0; y < h; y++) {
                    uint16_t px = camera.GetImageBuffer()[y * 400 + x];
                    img[y * w + x] = (uint8_t)(((((px >> 11) & 0x1F) << 3) + (((px >> 5) & 0x3F) << 2) + ((px & 0x1F) << 3)) / 3);
                }
            }

            LightLock_Unlock(camera.GetImageLock());
            quirc_end(camera.GetQuircContext());

            if (quirc_count(camera.GetQuircContext()) > 0) {
                struct quirc_code code;
                struct quirc_data scandata;
                quirc_extract(camera.GetQuircContext(), 0, &code);

                if (!quirc_decode(&code, &scandata)) {
                    s2 = "QR found. Press A to proceed.";
                    camera.SetString(s2);
                    camera.SetPayloadFound(true);
                    app->GetPayload().resize(scandata.payload_len);
                    memcpy(&app->GetPayload()[0], &scandata.payload[0], scandata.payload_len);
                    done = true;
                }
            }

            if (!svcWaitSynchronization(camera.GetFinishedEvent(), 0))
                done = true;
        }
    }, *this, app, 100 * 1024);

    m_worker.CreateThread([](Camera& camera, App *app) -> void{
        uint16_t *buffer = new uint16_t[400 * 240];
        memset(buffer, 0, 400 * 240 * 2);
        Handle events[3] = {0};
        events[2] = camera.GetFinishedEvent();
        uint32_t transferunit = 0;
        bool finished = false;
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
        svcSetThreadPriority(CUR_THREAD_HANDLE, camera.GetMainThreadPrio() - 2);
        CAMU_SetReceiving(&events[0], buffer, PORT_CAM1, 400 * 240 * sizeof(u16), (s16) transferunit);
        CAMU_StartCapture(PORT_CAM1);

        while (!finished) {
            int32_t index = 0;
            svcWaitSynchronizationN(&index, events, 3, false, -1LL);

            switch (index) {
                case 0: { // Recieve Event
                    LightLock_Lock(camera.GetImageLock());
                    svcCloseHandle(events[0]);
                    events[0] = 0;
                    memcpy(&camera.GetImageBuffer()[0], buffer, 400 * 240 * sizeof(uint16_t));
                    CAMU_SetReceiving(&events[0], buffer, PORT_CAM1, 400 * 240 * sizeof(uint16_t), (int16_t)transferunit);
                    LightLock_Unlock(camera.GetImageLock());
                    break;
                }

                case 1: { // Buffer Error event
                    svcCloseHandle(events[0]);
                    events[0] = 0;
                    CAMU_ClearBuffer(PORT_CAM1);
                    CAMU_SetReceiving(&events[0], buffer, PORT_CAM1, 400 * 240 * sizeof(uint16_t), (int16_t) transferunit);
                    CAMU_StartCapture(PORT_CAM1);
                    break;
                }

                case 2: { // Finished scanning
                    finished = true;
                    break;
                }
            }
        }
        CAMU_StopCapture(PORT_CAM1);
        bool busy = false;

        while (R_SUCCEEDED(CAMU_IsBusy(&busy, PORT_CAM1)) && busy)
            svcSleepThread(1e+9);
        CAMU_ClearBuffer(PORT_CAM1);
        CAMU_Activate(SELECT_NONE);
        delete[] buffer;
    }, *this, app, 1024 * 1024 * 2);
}

void Camera::OnStateExit(App *app) {
    svcSignalEvent(m_finishedevent);

    while (!m_worker.IsDone() || !m_decodeworker.IsDone()) {
        svcSleepThread(0.05e+9);
    }

    delete[] m_buffer;
    C2D_TextBufDelete(m_textbuf);
}

std::optional<ui::States> Camera::HandleEvent() {
    if ((keysDown() & KEY_A) && m_payloadfound)
        return ui::States::GameSelection;

    if (keysDown() & KEY_B)
        return ui::States::MainMenu;

    return {};
}

void Camera::RenderLoop() {
    auto top = ui::g_RenderTarget.GetRenderTarget(ui::Screen::Top);
    auto bottom = ui::g_RenderTarget.GetRenderTarget(ui::Screen::Bottom);

    /* Top */
    C2D_SceneBegin(top);
    C2D_TargetClear(top, C2D_Color32(0xEC, 0xF0, 0xF1, 0xFF));
    LightLock_Lock(&m_imagelock);

    for (uint32_t x = 0; x < 400; x++) {
        for (uint32_t y = 0; y < 240; y++) {
            uint32_t dstPos = ((((y >> 3) * (512 >> 3) + (x >> 3)) << 6) + ((x & 1) | ((y & 1) << 1) | ((x & 2) << 1) | ((y & 2) << 2) | ((x & 4) << 2) | ((y & 4) << 3))) * 2;
            uint32_t srcPos = (y * 400 + x) * 2;
            memcpy(&((u8*)m_img.tex->data)[dstPos], &((u8*)m_buffer)[srcPos], 2);
        }
    }

    C2D_DrawImageAt(m_img, 0.0f, 0.0f, 0.5f, NULL, 1.0f, 1.0f);
    LightLock_Unlock(&m_imagelock);

    /* Bottom */
    C2D_SceneBegin(bottom);
    C2D_TargetClear(bottom, C2D_Color32(0xEC, 0xF0, 0xF1, 0xFF));
    ui::Elements::DrawBars(ui::Screen::Bottom);
    auto height = 0.0f, width = 0.0f;
    LightLock_Lock(&m_messagelock);
    C2D_TextGetDimensions(&m_text, 0.5f, 0.5f, &width, &height);
    auto y = (ui::Dimensions::GetHeight() - height) / 2;
    C2D_DrawText(&m_text, C2D_AlignCenter, 160.0f, y, 1.0f, 0.5f, 0.5f);
    LightLock_Unlock(&m_messagelock);
}

void Camera::SetString(const std::string &str) {
    LightLock_Lock(&m_messagelock);
    m_message = str;
    C2D_TextParse(&m_text, m_textbuf, m_message.c_str());
    C2D_TextOptimize(&m_text);
    LightLock_Unlock(&m_messagelock);
}