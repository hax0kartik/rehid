#pragma once
#include <string>
#include "../ui.hpp"
#include "../workerthread.hpp"
extern "C" {
#include "../quirc/quirc.h"
}

class Camera : public ui::State {
public:
    Camera();
    ~Camera() override;
    std::optional<ui::States> HandleEvent() override;
    void OnStateEnter(App *app) override;
    void OnStateExit(App *app) override;
    void RenderLoop() override;

    void SetString(const std::string& s);
    Handle GetFinishedEvent() {
        return m_finishedevent;
    }

    LightLock *GetImageLock() {
        return &m_imagelock;
    }

    uint16_t *GetImageBuffer() {
        return m_buffer;
    }

    int32_t GetMainThreadPrio() {
        return m_mainthreadprio;
    }

    struct quirc *GetQuircContext() {
        return m_context;
    }

    std::string& GetPayload() {
        return m_payload;
    }

    void SetPayloadFound(bool val) {
        m_payloadfound = val;
    }

private:
    std::string m_message;
    std::string m_payload;
    LightLock m_messagelock;
    LightLock m_imagelock;
    Handle m_finishedevent;
    uint16_t *m_buffer;
    int32_t m_mainthreadprio;
    struct quirc *m_context;
    bool m_payloadfound;

    /* UI */
    C3D_Tex *m_tex;
    C2D_Image m_img;
    C2D_TextBuf m_textbuf;
    C2D_Text m_text;
    WorkerThread<Camera&, App*> m_worker;
    WorkerThread<Camera&, App*> m_decodeworker;
};