#pragma once
#include <vector>
#include <string>
#include "../ui.hpp"

class GameSelection : public ui::State{
    public:
        GameSelection();
        ~GameSelection() override;
        std::optional<ui::States> HandleEvent() override;
        void OnStateEnter(App *app) override;
        void OnStateExit(App *app) override;
        void RenderLoop() override;

    private:
        int m_selected = 0;
        int m_page = 0;
        int m_iconsconverted = 0;
        int m_broken = false;
        std::vector<uint64_t> m_titles;
        std::string m_payload;
        std::vector<std::string> m_descriptions;
        bool m_showgeneratedtext = false;
        bool m_generatedforglobal = false;

        /* UI */
        C2D_TextBuf m_textbuf;
        std::vector<C2D_Image> m_images;
        std::vector<C3D_Tex*> m_texs;
        C2D_Text m_generatedtext;
        std::vector<C2D_Text> m_descriptiontexts;
        const int NO_OF_ICONS_PER_PAGE = 70;
};