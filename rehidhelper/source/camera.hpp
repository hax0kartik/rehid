#include <string>
#include <3ds.h>
#include <citro2d.h>
#include "ui.hpp"
#include "utils.hpp"
#include "titles.hpp"
extern "C"{
    #include "quirc/quirc.h"
}

class Camera
{
    public:
        void Initialize(Titles *titles);
        void CreateCameraThread();
        void HandOverUI();
        void Controls();
        void Finalize();
        int32_t GetMainThreadPriority() { return m_mainthreadprio; };
        Handle GetCameraThreadHandle() { return threadGetHandle(m_thread); };
        LightLock *GetLock() { return &m_lock; };
        uint16_t *GetBuffer() { return m_buffer; };
        C2D_Image *GetImage() { return &m_img; };
        Handle GetFinishedEvent() { return m_finishedevent; };

    private:
        uint16_t *m_buffer;
        C3D_Tex *m_tex;
        C2D_Image m_img;
        C2D_Image m_globalimg;
        Thread m_thread;
        LightLock m_lock;
        LightLock m_botlock;
        int32_t m_mainthreadprio;
        struct quirc* m_context;
        std::string m_data;
        std::string m_str;
        uint8_t m_state = 0;
        Handle m_finishedevent;
        Titles *m_titles;
        Utils m_utils;
        C2D_SpriteSheet m_sheet;
};