#include "GameSelection.hpp"
#include "../app.hpp"
#include "../Utils/Misc.hpp"

GameSelection::GameSelection() {
    m_selected = 0;
    m_page = 0;
    m_iconsconverted = 0;
}

GameSelection::~GameSelection() {

}

void GameSelection::OnStateEnter(App *app) {
    m_selected = 0;
    m_page = 0;
    m_showgeneratedtext = false;
    m_generatedforglobal = false;
    m_textbuf = C2D_TextBufNew(2000);
    m_titles = app->GetTitleManager().GetFilteredTitles();
    m_payload = app->GetPayload();

    auto descs = app->GetTitleManager().GetTitleDescription();
    descs.push_back("Global (settings will be applied to all titles.)");

    m_descriptiontexts.resize(descs.size());

    for (int i = 0; i < (int)descs.size(); i++) {
        descs[i] = "Current Title: " + descs[i];
        C2D_TextParse(&m_descriptiontexts[i], m_textbuf, descs[i].c_str());
        C2D_TextOptimize(&m_descriptiontexts[i]);
    }

    const char *text = "Remapping Generated. Press START if you want to exit.";
    C2D_TextParse(&m_generatedtext, m_textbuf, text);
    C2D_TextOptimize(&m_generatedtext);

    if (m_iconsconverted == 0) {
        app->GetTitleManager().ConvertIconsToC2DImage(m_images, m_texs);
        C2D_SpriteSheet sheet = C2D_SpriteSheetLoad("/3ds/rehid/images/icon.t3x");

        if (!sheet)
            sheet = C2D_SpriteSheetLoad("romfs:/icon.t3x");

        C2D_Image image = C2D_SpriteSheetGetImage(sheet, 1);
        m_images.push_back(image);
        m_iconsconverted = 1;
    }
}

void GameSelection::OnStateExit(App *app) {
    m_titles.clear();
    m_titles.shrink_to_fit();
    m_descriptiontexts.clear();
    m_descriptiontexts.shrink_to_fit();

    if (m_generatedforglobal)
        Utils::Misc::Reboot();

    C2D_TextBufDelete(m_textbuf);
}

std::optional<ui::States> GameSelection::HandleEvent() {
    uint32_t kDown = hidKeysDown();

    if (kDown & KEY_RIGHT)
        m_selected++;

    if (kDown & KEY_LEFT)
        m_selected--;

    if (kDown & KEY_DOWN)
        m_selected += 10;

    if (kDown & KEY_UP)
        m_selected -= 10;

    if (kDown & KEY_B)
        return ui::States::MainMenu;

    if (kDown & KEY_A) {
        auto isglobal = m_selected == m_descriptiontexts.size() - 1;
        auto tid = isglobal ? 0 : m_titles[m_selected];
        m_generatedforglobal = isglobal;
        Utils::Misc::GenerateRemapping(tid, isglobal, m_payload);
        m_showgeneratedtext = true;
    }

    if (m_selected < 0)
        m_selected = m_descriptiontexts.size() - 1;
    else if (m_selected > (int)m_descriptiontexts.size() - 1)
        m_selected = 0;

    m_page = m_selected / NO_OF_ICONS_PER_PAGE;
    return {};
}

void GameSelection::RenderLoop() {
    auto top = ui::g_RenderTarget.GetRenderTarget(ui::Screen::Top);
    auto bottom = ui::g_RenderTarget.GetRenderTarget(ui::Screen::Bottom);

    /* Top */
    C2D_SceneBegin(top);
    C2D_TargetClear(top, C2D_Color32(0xEC, 0xF0, 0xF1, 0xFF));
    ui::Elements::DrawBars(ui::Screen::Top);
    ui::Elements::IconAndText::GetInstance().DrawIconAndTextInMiddle(ui::Screen::Top);

    if (m_showgeneratedtext)
        C2D_DrawText(&m_generatedtext, 0, 10.0f, 190.0f, 1.0f, 0.5f, 0.5f);

    C2D_DrawText(&m_descriptiontexts[m_selected], 0, 10.0f, 210.0f, 1.0f, 0.5f, 0.5f);

    /* Bottom */
    C2D_SceneBegin(bottom);
    C2D_TargetClear(bottom, C2D_Color32(0xEC, 0xF0, 0xF1, 0xFF));
    ui::Elements::DrawBars(ui::Screen::Bottom);
    float x = 13.0f, y = 15.0f;

    for (int i = 0; i < NO_OF_ICONS_PER_PAGE && (m_page * NO_OF_ICONS_PER_PAGE) + i < (int)m_images.size(); i++) {
        if (x + m_images[(m_page * NO_OF_ICONS_PER_PAGE) + i].subtex->width > 320.0f) {
            y = y + m_images[(m_page * NO_OF_ICONS_PER_PAGE) + i].subtex->height + 6.0f;
            x = 13.0f;
        }

        C2D_ImageTint tint;

        if (((m_page * NO_OF_ICONS_PER_PAGE) + i) == m_selected)
            C2D_AlphaImageTint(&tint, 1.0f);
        else
            C2D_AlphaImageTint(&tint, 0.5f);

        C2D_DrawImageAt(m_images[(m_page * NO_OF_ICONS_PER_PAGE) + i], x, y, 1.0f, &tint);
        x += m_images[(m_page * NO_OF_ICONS_PER_PAGE) + i].subtex->width + 6.0f;
    }
}