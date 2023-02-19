#pragma once
#include <optional>
#include <3ds.h>
#include <citro2d.h>
/* forward declaration */
class App;
namespace ui
{
    enum Screen
    {
        Top,
        Bottom
    };

    enum States
    {
        Initial, MainMenu, 
        Download, 
        Camera, GameSelection,
        ToggleState
    };

    namespace Dimensions
    {
        constexpr inline float GetHeight()
        {
            return 240;
        }

        constexpr inline float GetWidth(const Screen screen)
        {
            if (screen == Screen::Top)
                return 400;
            else 
                return 320;
        }
    };

    class State
    {
        public:
            virtual std::optional<ui::States> HandleEvent() = 0;
            virtual void OnStateEnter(App *app) = 0;
            virtual void OnStateExit(App *app) = 0;
            virtual void RenderLoop() = 0;
            virtual ~State() = default;
    };

    namespace Elements
    {
        void DrawBars(const Screen s);
        class IconAndText{
            public:
                void Intialize();
                void DrawIconAndTextInMiddle(const Screen s, bool dofadeffect = false);
                static auto &GetInstance(){
                    static IconAndText m_instance;
                    return m_instance;
                }
                IconAndText(IconAndText &other) = delete;
                void operator=(const IconAndText &) = delete;
            protected:
                IconAndText() {}
            private:
                C2D_Image m_image;
                C2D_Text m_text;
                C2D_TextBuf m_textbuf;
                float m_alpha = 1.0f;
                uint8_t m_effect = 0;
        };
    }

    Result Intialize();

    class RenderTargets{
        public:
            void CreateRenderTargets(){
                m_top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
                m_bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
            };
            auto GetRenderTarget(Screen s) const {
                return Screen::Top == s ? m_top : m_bottom;
            };
        private:
            C3D_RenderTarget *m_top;
            C3D_RenderTarget *m_bottom;
    };


    extern RenderTargets g_RenderTarget;
};